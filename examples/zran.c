/* zran.c -- example of deflate stream indexing and random access
 * Copyright (C) 2005, 2012, 2018, 2023 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 * Version 1.3  18 Feb 2023  Mark Adler */

/* Version History:
 1.0  29 May 2005  First version
 1.1  29 Sep 2012  Fix memory reallocation error
 1.2  14 Oct 2018  Handle gzip streams with multiple members
                   Add a header file to facilitate usage in applications
 1.3  18 Feb 2023  Permit raw deflate streams as well as zlib and gzip
                   Permit crossing gzip member boundaries when extracting
                   Support a size_t size when extracting (was an int)
                   Do a binary search over the index for an access point
                   Expose the access point type to enable save and load
 */

// Illustrate the use of Z_BLOCK, inflatePrime(), and inflateSetDictionary()
// for random access of a compressed file. A file containing a raw deflate
// stream is provided on the command line. The compressed stream is decoded in
// its entirety, and an index built with access points about every SPAN bytes
// in the uncompressed output. The compressed file is left open, and can then
// be read randomly, having to decompress on the average SPAN/2 uncompressed
// bytes before getting to the desired block of data.
//
// An access point can be created at the start of any deflate block, by saving
// the starting file offset and bit of that block, and the 32K bytes of
// uncompressed data that precede that block. Also the uncompressed offset of
// that block is saved to provide a reference for locating a desired starting
// point in the uncompressed stream. deflate_index_build() decompresses the
// input raw deflate stream a block at a time, and at the end of each block
// decides if enough uncompressed data has gone by to justify the creation of a
// new access point. If so, that point is saved in a data structure that grows
// as needed to accommodate the points.
//
// To use the index, an offset in the uncompressed data is provided, for which
// the latest access point at or preceding that offset is located in the index.
// The input file is positioned to the specified location in the index, and if
// necessary the first few bits of the compressed data is read from the file.
// inflate is initialized with those bits and the 32K of uncompressed data, and
// decompression then proceeds until the desired offset in the file is reached.
// Then decompression continues to read the requested uncompressed data from
// the file.
//
// There is some fair bit of overhead to starting inflation for the random
// access, mainly copying the 32K byte dictionary. If small pieces of the file
// are being accessed, it would make sense to implement a cache to hold some
// lookahead to avoid many calls to deflate_index_extract() for small lengths.
//
// Another way to build an index would be to use inflateCopy(). That would not
// be constrained to have access points at block boundaries, but would require
// more memory per access point, and could not be saved to a file due to the
// use of pointers in the state. The approach here allows for storage of the
// index in a file.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "zlib.h"
#include "zran.h"

#define WINSIZE 32768U      // sliding window size
#define CHUNK 16384         // file input buffer size

// See comments in zran.h.
void deflate_index_free(struct deflate_index *index) {
    if (index != NULL) {
        free(index->list);
        free(index);
    }
}

// Add an access point to the list. If out of memory, deallocate the existing
// list and return NULL. index->mode is temporarily the allocated number of
// access points, until it is time for deflate_index_build() to return. Then
// index->mode is set to the mode of inflation.
static struct deflate_index *add_point(struct deflate_index *index,// int bits,
                                       off_t in, off_t out, unsigned left,
                                       unsigned char *window) {
    if (index == NULL) {
        // The list is empty. Create it, starting with eight access points.
        index = malloc(sizeof(struct deflate_index));
        if (index == NULL)
            return NULL;
        index->have = 0;
        index->mode = 8;
        index->list = malloc(sizeof(point_t) * index->mode);
        if (index->list == NULL) {
            free(index);
            return NULL;
        }
    }

    else if (index->have == index->mode) {
        // The list is full. Make it bigger.
        index->mode <<= 1;
        point_t *next = realloc(index->list, sizeof(point_t) * index->mode);
        if (next == NULL) {
            deflate_index_free(index);
            return NULL;
        }
        index->list = next;
    }

    // Fill in the access point and increment how many we have.
    point_t *next = (point_t *)(index->list) + index->have++;
    if (index->have < 0) {
        // Overflowed the int!
        deflate_index_free(index);
        return NULL;
    }
    next->out = out;
    next->in = in;
//    next->bits = bits;
    if (left)
        memcpy(next->window, window + WINSIZE - left, left);
    if (left < WINSIZE)
        memcpy(next->window + left, window, WINSIZE - left);

    // Return the index, which may have been newly allocated or destroyed.
    return index;
}

// Decompression modes. These are the inflateInit2() windowBits parameter.
#define RAW -15
#define ZLIB 15
#define GZIP 31

// See comments in zran.h.
int deflate_index_build(FILE *in, off_t span, struct deflate_index **built) {
    // Set up inflation state.
    z_stream strm = {0};        // inflate engine (gets fired up later)
    unsigned char buf[CHUNK];   // input buffer
    unsigned char win[WINSIZE] = {0};   // output sliding window
    off_t totin = 0;            // total bytes read from input
    off_t totout = 0;           // total bytes uncompressed
    int mode = 0;               // mode: RAW, ZLIB, or GZIP (0 => not set yet)

    // Decompress from in, generating access points along the way.
    int ret;                    // the return value from zlib, or Z_ERRNO
    off_t last;                 // last access point uncompressed offset
    struct deflate_index *index = NULL;     // list of access points
    const size_t stride0 = sizeof(buf);           // stride for reading input
    size_t stride = stride0;           // stride temp

    do {
        // Assure available input, at least until reaching EOF.
        if (strm.avail_in == 0) {
            strm.avail_in = fread(buf, 1, stride, in);
            totin += strm.avail_in;
            strm.next_in = buf;
            if (strm.avail_in < stride && ferror(in)) {
                ret = Z_ERRNO;
                break;
            }

            if (mode == 0) {
                // At the start of the input -- determine the type. Assume raw
                // if it is neither zlib nor gzip. This could in theory result
                // in a false positive for zlib, but in practice the fill bits
                // after a stored block are always zeros, so a raw stream won't
                // start with an 8 in the low nybble.
                mode = strm.avail_in == 0 ? RAW :       // empty -- will fail
                       (strm.next_in[0] & 0xf) == 8 ? ZLIB :
                       strm.next_in[0] == 0x1f ? GZIP :
                       /* else */ RAW;
                ret = inflateInit2(&strm, mode);
                if (ret != Z_OK)
                    break;
            }
        }

        // Assure available output. This rotates the output through, for use as
        // a sliding window on the uncompressed data.
        if (strm.avail_out == 0) {
            strm.avail_out = sizeof(win);
            strm.next_out = win;
        }

        if (mode == RAW && index == NULL)
            // We skip the inflate() call at the start of raw deflate data in
            // order generate an access point there. Set data_type to imitate
            // the end of a header.
            strm.data_type = 0x80;
        else {
            // Inflate and update the number of uncompressed bytes.
            unsigned before = strm.avail_out;
            ret = inflate(&strm, Z_BLOCK);
            totout += before - strm.avail_out;
        }

        if ((strm.data_type & 0xc0) == 0x80 &&
            (index == NULL || totout - last >= span)) {
            // We are at the end of a header or a non-last deflate block, so we
            // can add an access point here. Furthermore, we are either at the
            // very start for the first access point, or there has been span or
            // more uncompressed bytes since the last access point, so we want
            // to add an access point here.
            int bits = strm.data_type & 7;
            if (bits) {
                stride = 1;
                continue;
            }else stride=stride0;
            index = add_point(index, /*bits,*/ totin - strm.avail_in,
                              totout, strm.avail_out, win);
            if (index == NULL) {
                ret = Z_MEM_ERROR;
                break;
            }
            last = totout;
        }

        if (ret == Z_STREAM_END && mode == GZIP &&
            (strm.avail_in || ungetc(getc(in), in) != EOF))
            // There is more input after the end of a gzip member. Reset the
            // inflate state to read another gzip member. On success, this will
            // set ret to Z_OK to continue decompressing.
            ret = inflateReset2(&strm, GZIP);

        // Keep going until Z_STREAM_END or error. If the compressed data ends
        // prematurely without a file read error, Z_BUF_ERROR is returned.
    } while (ret == Z_OK);
    inflateEnd(&strm);

    if (ret != Z_STREAM_END) {
        // An error was encountered. Discard the index and return a negative
        // error code.
        deflate_index_free(index);
        return ret == Z_NEED_DICT ? Z_DATA_ERROR : ret;
    }

    // Shrink the index to only the occupied access points and return it.
    index->mode = mode;
    index->length = totout;
    point_t *list = realloc(index->list, sizeof(point_t) * index->have);
    if (list == NULL) {
        // Seems like a realloc() to make something smaller should always work,
        // but just in case.
        deflate_index_free(index);
        return Z_MEM_ERROR;
    }
    index->list = list;
    *built = index;
    return index->have;
}

// See comments in zran.h.
ptrdiff_t deflate_index_extract(FILE *in, struct deflate_index *index,
                                off_t offset, unsigned char *buf, size_t len) {
    // Do a quick sanity check on the index.
    if (index == NULL || index->have < 1 || index->list[0].out != 0)
        return Z_STREAM_ERROR;

    // If nothing to extract, return zero bytes extracted.
    if (len == 0 || offset < 0 || offset >= index->length)
        return 0;

    // Find the access point closest to but not after offset.
    int lo = -1, hi = index->have;
    point_t *point = index->list;
    while (hi - lo > 1) {
        int mid = (lo + hi) >> 1;
        if (offset < point[mid].out)
            hi = mid;
        else
            lo = mid;
    }
    point += lo;

    // Initialize the input file and prime the inflate engine to start there.
    int ret = fseeko(in, point->in /*- (point->bits ? 1 : 0)*/, SEEK_SET);
    if (ret == -1)
        return Z_ERRNO;
    int ch = 0;
    /*if (point->bits && (ch = getc(in)) == EOF)
        return ferror(in) ? Z_ERRNO : Z_BUF_ERROR;*/
    z_stream strm = {0};
    ret = inflateInit2(&strm, RAW);
    if (ret != Z_OK)
        return ret;
    /*if (point->bits)
        inflatePrime(&strm, point->bits, ch >> (8 - point->bits));*/
    inflateSetDictionary(&strm, point->window, WINSIZE);

    // Skip uncompressed bytes until offset reached, then satisfy request.
    unsigned char input[CHUNK];
    unsigned char discard[WINSIZE];
    offset -= point->out;       // number of bytes to skip to get to offset
    size_t left = len;          // number of bytes left to read after offset
    do {
        if (offset) {
            // Discard up to offset uncompressed bytes.
            strm.avail_out = offset < WINSIZE ? (unsigned)offset : WINSIZE;
            strm.next_out = discard;
        }
        else {
            // Uncompress up to left bytes into buf.
            strm.avail_out = left < UINT_MAX ? (unsigned)left : UINT_MAX;
            strm.next_out = buf + len - left;
        }

        // Uncompress, setting got to the number of bytes uncompressed.
        if (strm.avail_in == 0) {
            // Assure available input.
            strm.avail_in = fread(input, 1, CHUNK, in);
            if (strm.avail_in < CHUNK && ferror(in)) {
                ret = Z_ERRNO;
                break;
            }
            strm.next_in = input;
        }
        unsigned got = strm.avail_out;
        ret = inflate(&strm, Z_NO_FLUSH);
        got -= strm.avail_out;

        // Update the appropriate count.
        if (offset)
            offset -= got;
        else
            left -= got;

        // If we're at the end of a gzip member and there's more to read,
        // continue to the next gzip member.
        if (ret == Z_STREAM_END && index->mode == GZIP) {
            // Discard the gzip trailer.
            unsigned drop = 8;              // length of gzip trailer
            if (strm.avail_in >= drop) {
                strm.avail_in -= drop;
                strm.next_in += drop;
            }
            else {
                // Read and discard the remainder of the gzip trailer.
                drop -= strm.avail_in;
                strm.avail_in = 0;
                do {
                    if (getc(in) == EOF)
                        // The input does not have a complete trailer.
                        return ferror(in) ? Z_ERRNO : Z_BUF_ERROR;
                } while (--drop);
            }

            if (strm.avail_in || ungetc(getc(in), in) != EOF) {
                // There's more after the gzip trailer. Use inflate to skip the
                // gzip header and resume the raw inflate there.
                inflateReset2(&strm, GZIP);
                do {
                    if (strm.avail_in == 0) {
                        strm.avail_in = fread(input, 1, CHUNK, in);
                        if (strm.avail_in < CHUNK && ferror(in)) {
                            ret = Z_ERRNO;
                            break;
                        }
                        strm.next_in = input;
                    }
                    strm.avail_out = WINSIZE;
                    strm.next_out = discard;
                    ret = inflate(&strm, Z_BLOCK);  // stop at end of header
                } while (ret == Z_OK && (strm.data_type & 0x80) == 0);
                if (ret != Z_OK)
                    break;
                inflateReset2(&strm, RAW);
            }
        }

        // Continue until we have the requested data, the deflate data has
        // ended, or an error is encountered.
    } while (ret == Z_OK && left);
    inflateEnd(&strm);

    // Return the number of uncompressed bytes read into buf, or the error.
    return ret == Z_OK || ret == Z_STREAM_END ? len - left : ret;
}

#ifdef TEST

#define SPAN 1048576L       // desired distance between access points
#define LEN 16384           // number of bytes to extract

// Demonstrate the use of deflate_index_build() and deflate_index_extract() by
// processing the file provided on the command line, and extracting LEN bytes
// from 2/3rds of the way through the uncompressed output, writing that to
// stdout. An offset can be provided as the second argument, in which case the
// data is extracted from there instead.
int main(int argc, char **argv) {
    // Open the input file.
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: zran file.raw [offset]\n");
        return 1;
    }
    FILE *in = fopen(argv[1], "rb");
    if (in == NULL) {
        fprintf(stderr, "zran: could not open %s for reading\n", argv[1]);
        return 1;
    }

    // Get optional offset.
    off_t offset = -1;
    if (argc == 3) {
        char *end;
        offset = strtoll(argv[2], &end, 10);
        if (*end || offset < 0) {
            fprintf(stderr, "zran: %s is not a valid offset\n", argv[2]);
            return 1;
        }
    }

    // Build index.
    struct deflate_index *index = NULL;
    int len = deflate_index_build(in, SPAN, &index);
    if (len < 0) {
        fclose(in);
        switch (len) {
        case Z_MEM_ERROR:
            fprintf(stderr, "zran: out of memory\n");
            break;
        case Z_BUF_ERROR:
            fprintf(stderr, "zran: %s ended prematurely\n", argv[1]);
            break;
        case Z_DATA_ERROR:
            fprintf(stderr, "zran: compressed data error in %s\n", argv[1]);
            break;
        case Z_ERRNO:
            fprintf(stderr, "zran: read error on %s\n", argv[1]);
            break;
        default:
            fprintf(stderr, "zran: error %d while building index\n", len);
        }
        return 1;
    }
    fprintf(stderr, "zran: built index with %d access points\n", len);

    // Use index by reading some bytes from an arbitrary offset.
    unsigned char buf[LEN];
    if (offset == -1)
        offset = ((index->length + 1) << 1) / 3;
    ptrdiff_t got = deflate_index_extract(in, index, offset, buf, LEN);
    if (got < 0)
        fprintf(stderr, "zran: extraction failed: %s error\n",
                got == Z_MEM_ERROR ? "out of memory" : "input corrupted");
    else {
        fwrite(buf, 1, got, stdout);
        fprintf(stderr, "zran: extracted %ld bytes at %lld\n", got, offset);
    }

    // Clean up and exit.
    deflate_index_free(index);
    fclose(in);
    return 0;
}

#endif
