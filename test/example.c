/* example.c -- usage example of the zlib compression library
 * Copyright (C) 1995-2006, 2011 Jean-loup Gailly.
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* @(#) $Id$ */

#include "zlib.h"
#include <stdio.h>

#ifdef STDC
#  include <string.h>
#  include <stdlib.h>
#endif

#if defined(VMS) || defined(RISCOS)
#  define TESTFILE "foo-gz"
#else
#  define TESTFILE "foo.gz"
#endif

#define CHECK_ERR(err, msg) { \
    if (err != Z_OK) { \
        fprintf(stderr, "%s error: %d\n", msg, err); \
        exit(1); \
    } \
}

z_const char hello[] = "hello, hello!";
/* "hello world" would be more standard, but the repeated "hello"
 * stresses the compression code better, sorry...
 */

const char dictionary[] = "hello";
uLong dictId; /* Adler32 value of the dictionary */

void test_deflate       OF((Byte *compr, uLong comprLen));
void test_inflate       OF((Byte *compr, uLong comprLen,
                            Byte *uncompr, uLong uncomprLen));
void test_large_deflate OF((Byte *compr, uLong comprLen,
                            Byte *uncompr, uLong uncomprLen));
void test_large_inflate OF((Byte *compr, uLong comprLen,
                            Byte *uncompr, uLong uncomprLen));
void test_flush         OF((Byte *compr, uLong *comprLen));
void test_sync          OF((Byte *compr, uLong comprLen,
                            Byte *uncompr, uLong uncomprLen));
void test_dict_deflate  OF((Byte *compr, uLong comprLen));
void test_dict_inflate  OF((Byte *compr, uLong comprLen,
                            Byte *uncompr, uLong uncomprLen));
int  main               OF((int argc, char *argv[]));


#ifdef Z_SOLO

void *myalloc OF((void *, unsigned, unsigned));
void myfree OF((void *, void *));

void *myalloc(q, n, m)
    void *q;
    unsigned n, m;
{
    q = Z_NULL;
    return calloc(n, m);
}

void myfree(void *q, void *p)
{
    q = Z_NULL;
    free(p);
}

static alloc_func zalloc = myalloc;
static free_func zfree = myfree;

#else /* !Z_SOLO */

static alloc_func zalloc = (alloc_func)0;
static free_func zfree = (free_func)0;

void test_compress      OF((Byte *compr, uLong comprLen,
                            Byte *uncompr, uLong uncomprLen));
void test_gzio          OF((const char *fname,
                            Byte *uncompr, uLong uncomprLen,
/* START MODIFICATION BY INTELLIMAGIC, info@intellimagic.com */
/* added use_fp; added test_gzio_in_middle */
                            Byte use_fp));
void test_gzio_in_middle OF((const char *fname,
                            Byte *uncompr, uLong uncomprLen,
                            Byte use_fp));
void test_gzio_uncompressed_in_middle OF((const char *fname,
                            Byte *uncompr, uLong uncomprLen,
                            Byte use_fp, Byte use_compr));
/* END MODIFICATION BY INTELLIMAGIC, info@intellimagic.com */

/* ===========================================================================
 * Test compress() and uncompress()
 */
void test_compress(compr, comprLen, uncompr, uncomprLen)
    Byte *compr, *uncompr;
    uLong comprLen, uncomprLen;
{
    int err;
    uLong len = (uLong)strlen(hello)+1;

    err = compress(compr, &comprLen, (const Bytef*)hello, len);
    CHECK_ERR(err, "compress");

    strcpy((char*)uncompr, "garbage");

    err = uncompress(uncompr, &uncomprLen, compr, comprLen);
    CHECK_ERR(err, "uncompress");

    if (strcmp((char*)uncompr, hello)) {
        fprintf(stderr, "bad uncompress\n");
        exit(1);
    } else {
        printf("uncompress(): %s\n", (char *)uncompr);
    }
}

/* ===========================================================================
 * Test read/write of .gz files
 */
/* START MODIFICATION BY INTELLIMAGIC, info@intellimagic.com */
/* added use_fp */
void test_gzio(fname, uncompr, uncomprLen, use_fp)
    const char *fname; /* compressed file name */
    Byte *uncompr;
    uLong uncomprLen;
    Byte use_fp;
/* END MODIFICATION BY INTELLIMAGIC, info@intellimagic.com */
{
#ifdef NO_GZCOMPRESS
    fprintf(stderr, "NO_GZCOMPRESS -- gz* functions cannot compress\n");
#else
    int err;
    int len = (int)strlen(hello)+1;
    gzFile file;
    z_off_t pos;

/* START MODIFICATION BY INTELLIMAGIC, info@intellimagic.com */
    if (use_fp)
      file = gzopen_fp(fname, "wb");
    else
      file = gzopen(fname, "wb");
/* END MODIFICATION BY INTELLIMAGIC, info@intellimagic.com */
    if (file == NULL) {
        fprintf(stderr, "gzopen error\n");
        exit(1);
    }
    gzputc(file, 'h');
    if (gzputs(file, "ello") != 4) {
        fprintf(stderr, "gzputs err: %s\n", gzerror(file, &err));
        exit(1);
    }
    if (gzprintf(file, ", %s!", "hello") != 8) {
        fprintf(stderr, "gzprintf err: %s\n", gzerror(file, &err));
        exit(1);
    }
    gzseek(file, 1L, SEEK_CUR); /* add one zero byte */
    gzclose(file);

/* START MODIFICATION BY INTELLIMAGIC, info@intellimagic.com */
    if (use_fp)
      file = gzopen_fp(fname, "rb");
    else
      file = gzopen(fname, "rb");
/* END MODIFICATION BY INTELLIMAGIC, info@intellimagic.com */
    if (file == NULL) {
        fprintf(stderr, "gzopen error\n");
        exit(1);
    }
    strcpy((char*)uncompr, "garbage");

    if (gzread(file, uncompr, (unsigned)uncomprLen) != len) {
        fprintf(stderr, "gzread err: %s\n", gzerror(file, &err));
        exit(1);
    }
    if (strcmp((char*)uncompr, hello)) {
        fprintf(stderr, "bad gzread: %s\n", (char*)uncompr);
        exit(1);
    } else {
        printf("gzread(): %s\n", (char*)uncompr);
    }

    pos = gzseek(file, -8L, SEEK_CUR);
    if (pos != 6 || gztell(file) != pos) {
        fprintf(stderr, "gzseek error, pos=%ld, gztell=%ld\n",
                (long)pos, (long)gztell(file));
        exit(1);
    }

    if (gzgetc(file) != ' ') {
        fprintf(stderr, "gzgetc error\n");
        exit(1);
    }

    if (gzungetc(' ', file) != ' ') {
        fprintf(stderr, "gzungetc error\n");
        exit(1);
    }

    gzgets(file, (char*)uncompr, (int)uncomprLen);
    if (strlen((char*)uncompr) != 7) { /* " hello!" */
        fprintf(stderr, "gzgets err after gzseek: %s\n", gzerror(file, &err));
        exit(1);
    }
    if (strcmp((char*)uncompr, hello + 6)) {
        fprintf(stderr, "bad gzgets after gzseek\n");
        exit(1);
    } else {
        printf("gzgets() after gzseek: %s\n", (char*)uncompr);
    }

    gzclose(file);
#endif
}

/* START MODIFICATION BY INTELLIMAGIC, info@intellimagic.com */
#include <fcntl.h>

/* ===========================================================================
 * Test read/write of gzipped data stream after uncompressed prefix
 */
void test_gzio_in_middle(fname, uncompr, uncomprLen, use_fp)
    const char *fname; /* compressed file name */
    Byte *uncompr;
    uLong uncomprLen;
    Byte use_fp;
{
#ifdef NO_GZCOMPRESS
    fprintf(stderr, "NO_GZCOMPRESS -- gz* functions cannot compress\n");
#else
    int err;
    int len = (int)strlen(hello)+1;
    gzFile file;
    z_off_t pos;
    int fd;
    FILE *fp;

    if (use_fp) {
      fp = fopen(fname, "w");
      if (!fp) {
        fprintf(stderr, "open w error\n");
        exit(1);
      }
      fwrite(hello, 1, len, fp);

      file = gzfopen(fp, "wb");
    } else {
      fd = open(fname, (O_CREAT | O_WRONLY | O_TRUNC));
      if (fd == -1) {
        fprintf(stderr, "open w error\n");
        exit(1);
      }
      write(fd, hello, len);

      file = gzdopen(fd, "wb");
    }
    if (file == NULL) {
        fprintf(stderr, "gz[fd]open w error\n");
        exit(1);
    }

    gzputc(file, 'h');
    if (gzputs(file, "ello") != 4) {
        fprintf(stderr, "gzputs err: %s\n", gzerror(file, &err));
        exit(1);
    }
    if (gzprintf(file, ", %s!", "hello") != 8) {
        fprintf(stderr, "gzprintf err: %s\n", gzerror(file, &err));
        exit(1);
    }
    gzseek(file, 1L, SEEK_CUR); /* add one zero byte */
    gzclose(file);              /* also closes file descriptor fd */

    if (use_fp) {
      fp = fopen(fname, "r");
      if (!fp) {
        fprintf(stderr, "open r error\n");
        exit(1);
      }
    } else {
      fd = open(fname, O_RDONLY);
      if (fd == -1) {
        fprintf(stderr, "open r error\n");
        exit(1);
      }
    }

    strcpy((char*)uncompr, "garbage");

    if (use_fp) {
      if (fread(uncompr, 1, len, fp) < len) {
        fprintf(stderr, "read error\n");
        exit(1);
      }
    } else {
      if (read(fd, uncompr, len) < len) {
        fprintf(stderr, "read error\n");
        exit(1);
      }
    }
    if (strcmp((char*)uncompr, hello)) {
        fprintf(stderr, "bad read: %s\n", (char*)uncompr);
        exit(1);
    } else {
        printf("read(): %s\n", (char*)uncompr);
    }

    if (use_fp)
      file = gzfopen(fp, "rb");
    else
      file = gzdopen(fd, "rb");
    if (file == NULL) {
        fprintf(stderr, "gzdopen r error\n");
        exit(1);
    }
    strcpy((char*)uncompr, "garbage");

    if (gzread(file, uncompr, (unsigned)uncomprLen) != len) {
        fprintf(stderr, "gzread err: %s\n", gzerror(file, &err));
        exit(1);
    }
    if (strcmp((char*)uncompr, hello)) {
        fprintf(stderr, "bad gzread: %s\n", (char*)uncompr);
        exit(1);
    } else {
        printf("gzread(): %s\n", (char*)uncompr);
    }

    pos = gzseek(file, -8L, SEEK_CUR);
    if (pos != 6 || gztell(file) != pos) {
        fprintf(stderr, "gzseek error, pos=%ld, gztell=%ld\n",
                (long)pos, (long)gztell(file));
        exit(1);
    }

    if (gzgetc(file) != ' ') {
        fprintf(stderr, "gzgetc error\n");
        exit(1);
    }

    if (gzungetc(' ', file) != ' ') {
        fprintf(stderr, "gzungetc error\n");
        exit(1);
    }

    gzgets(file, (char*)uncompr, (int)uncomprLen);
    if (strlen((char*)uncompr) != 7) { /* " hello!" */
        fprintf(stderr, "gzgets err after gzseek: %s\n", gzerror(file, &err));
        exit(1);
    }
    if (strcmp((char*)uncompr, hello + 6)) {
        fprintf(stderr, "bad gzgets after gzseek\n");
        exit(1);
    } else {
        printf("gzgets() after gzseek: %s\n", (char*)uncompr);
    }

    gzclose(file);
#endif
}

/* ===========================================================================
 * Test reading of (un)compressed data through gzio after an
 * uncompressed prefix
 */
void test_gzio_uncompressed_in_middle(fname, uncompr, uncomprLen, use_fp, use_compr)
    const char *fname; /* compressed file name */
    Byte *uncompr;
    uLong uncomprLen;
    Byte use_fp;
    Byte use_compr;
{
#ifdef NO_GZCOMPRESS
    fprintf(stderr, "NO_GZCOMPRESS -- gz* functions cannot compress\n");
#else
    int err;
    int len = (int)strlen(hello)+1;
    gzFile file;
    z_off_t pos;
    z_off_t datastart;
    int fd;
    FILE *fp;

    if (use_fp) {               /* use file pointers */
      fp = fopen(fname, "w");
      if (!fp) {
        fprintf(stderr, "open uim w error [%d%d]\n",
                (int) use_fp, (int) use_compr);
        exit(1);
      }
      fwrite(hello, 1, len, fp);
      if (use_compr) {          /* compress the rest of the data */
        file = gzfopen(fp, "wb");
      } else {                  /* don't compress any */
        datastart = ftell(fp);
        fwrite("RECORD1", 1, sizeof("RECORD1"), fp);
        pos = ftell(fp) - datastart;
        fwrite("RECORD2", 1, sizeof("RECORD2"), fp);
        fclose(fp);
      }
    } else {                    /* use file descriptors */
      fd = open(fname, (O_CREAT | O_WRONLY | O_TRUNC));
      if (fd == -1) {
        fprintf(stderr, "open uim w error [%d%d]\n",
                (int) use_fp, (int) use_compr);
        exit(1);
      }
      write(fd, hello, len);
      if (use_compr) {          /* compress the rest of the data */
        file = gzdopen(fd, "wb");
      } else {                  /* don't compress any */
        datastart = lseek(fd, 0, SEEK_CUR);
        write(fd, "RECORD1", sizeof("RECORD1"));
        pos = lseek(fd, 0, SEEK_CUR) - datastart;
        write(fd, "RECORD2", sizeof("RECORD2"));
        close(fd);
      }
    }

    if (use_compr) {
      if (file == NULL) {
        fprintf(stderr, "gz[fd]open uim w error [%d%d]\n",
                (int) use_fp, (int) use_compr);
        exit(1);
      }

      if (gzwrite(file, "RECORD1", sizeof("RECORD1")) != sizeof("RECORD1")) {
        fprintf(stderr, "gzwrite uim err [%d%d]: %s\n",
                (int) use_fp, (int) use_compr,
                gzerror(file, &err));
        exit(1);
      }

      pos = gztell(file);

      if (gzwrite(file, "RECORD2", sizeof("RECORD2")) != sizeof("RECORD2")) {
        fprintf(stderr, "gzputs uim err [%d%d]: %s\n",
                (int) use_fp, (int) use_compr,
                gzerror(file, &err));
        exit(1);
      }

      gzclose(file);          /* also closes file pointer */
    }

    if (use_fp) {
      fp = fopen(fname, "r");
      if (!fp) {
        fprintf(stderr, "open uim r error [%d%d]\n",
                (int) use_fp, (int) use_compr);
        exit(1);
      }
    } else {
      fd = open(fname, O_RDONLY);
      if (fd == -1) {
        fprintf(stderr, "open uim r error [%d%d]\n",
                (int) use_fp, (int) use_compr);
        exit(1);
      }
    }

    strcpy((char*)uncompr, "garbage");

    if (use_fp) {
      if (fread(uncompr, 1, len, fp) < len) {
        fprintf(stderr, "read uim error [%d%d]\n",
                (int) use_fp, (int) use_compr);
        exit(1);
      }
    } else {
      if (read(fd, uncompr, len) < len) {
        fprintf(stderr, "read uim error [%d%d]\n",
                (int) use_fp, (int) use_compr);
        exit(1);
      }
    }
    if (strcmp((char*)uncompr, hello)) {
      fprintf(stderr, "bad uim read [%d%d]: %s\n",
              (int) use_fp, (int) use_compr,
              (char*)uncompr);
        exit(1);
    } else {
        printf("read() uim [%d%d]: %s\n",
               (int) use_fp, (int) use_compr,
               (char*)uncompr);
    }

    if (use_fp)
      file = gzfopen(fp, "rb");
    else
      file = gzdopen(fd, "rb");
    if (file == NULL) {
      fprintf(stderr, "gzdopen uim r error [%d%d]\n",
              (int) use_fp, (int) use_compr);
        exit(1);
    }

    /* gztell counts from where gz[fd]open was called */
    if (gztell(file) != 0) {
      fprintf(stderr, "gztell uim should be 0 [%d%d] but is %u\n",
              (int) use_fp, (int) use_compr,
              gztell(file));
      exit(1);
    }

    /* gzoffset estimates the offset into the source file without
       regards to compression */
    if (gzoffset(file) == 0) {
      fprintf(stderr, "gzoffset uim should not be 0 [%d%d] but is\n",
              (int) use_fp, (int) use_compr);
      exit(1);
    }

    strcpy((char*)uncompr, "garbage");

    gzseek(file, pos, SEEK_SET);

    if (gzread(file, uncompr, sizeof("RECORD2")) != sizeof("RECORD2")) {
        fprintf(stderr, "gzread uim err [%d%d]: %s\n",
                (int) use_fp, (int) use_compr,
                gzerror(file, &err));
        exit(1);
    }
    if (strcmp((char*)uncompr, "RECORD2")) {
        fprintf(stderr, "bad gzread uim [%d%d]: %s\n",
                (int) use_fp, (int) use_compr,
                (char*)uncompr);
        exit(1);
    } else {
        printf("gzread() uim [%d%d]: %s\n",
               (int) use_fp, (int) use_compr,
               (char*)uncompr);
    }

    gzclose(file);
#endif
}
/* END MODIFICATION BY INTELLIMAGIC, info@intellimagic.com */

#endif /* Z_SOLO */

/* ===========================================================================
 * Test deflate() with small buffers
 */
void test_deflate(compr, comprLen)
    Byte *compr;
    uLong comprLen;
{
    z_stream c_stream; /* compression stream */
    int err;
    uLong len = (uLong)strlen(hello)+1;

    c_stream.zalloc = zalloc;
    c_stream.zfree = zfree;
    c_stream.opaque = (voidpf)0;

    err = deflateInit(&c_stream, Z_DEFAULT_COMPRESSION);
    CHECK_ERR(err, "deflateInit");

    c_stream.next_in  = (z_const unsigned char *)hello;
    c_stream.next_out = compr;

    while (c_stream.total_in != len && c_stream.total_out < comprLen) {
        c_stream.avail_in = c_stream.avail_out = 1; /* force small buffers */
        err = deflate(&c_stream, Z_NO_FLUSH);
        CHECK_ERR(err, "deflate");
    }
    /* Finish the stream, still forcing small buffers: */
    for (;;) {
        c_stream.avail_out = 1;
        err = deflate(&c_stream, Z_FINISH);
        if (err == Z_STREAM_END) break;
        CHECK_ERR(err, "deflate");
    }

    err = deflateEnd(&c_stream);
    CHECK_ERR(err, "deflateEnd");
}

/* ===========================================================================
 * Test inflate() with small buffers
 */
void test_inflate(compr, comprLen, uncompr, uncomprLen)
    Byte *compr, *uncompr;
    uLong comprLen, uncomprLen;
{
    int err;
    z_stream d_stream; /* decompression stream */

    strcpy((char*)uncompr, "garbage");

    d_stream.zalloc = zalloc;
    d_stream.zfree = zfree;
    d_stream.opaque = (voidpf)0;

    d_stream.next_in  = compr;
    d_stream.avail_in = 0;
    d_stream.next_out = uncompr;

    err = inflateInit(&d_stream);
    CHECK_ERR(err, "inflateInit");

    while (d_stream.total_out < uncomprLen && d_stream.total_in < comprLen) {
        d_stream.avail_in = d_stream.avail_out = 1; /* force small buffers */
        err = inflate(&d_stream, Z_NO_FLUSH);
        if (err == Z_STREAM_END) break;
        CHECK_ERR(err, "inflate");
    }

    err = inflateEnd(&d_stream);
    CHECK_ERR(err, "inflateEnd");

    if (strcmp((char*)uncompr, hello)) {
        fprintf(stderr, "bad inflate\n");
        exit(1);
    } else {
        printf("inflate(): %s\n", (char *)uncompr);
    }
}

/* ===========================================================================
 * Test deflate() with large buffers and dynamic change of compression level
 */
void test_large_deflate(compr, comprLen, uncompr, uncomprLen)
    Byte *compr, *uncompr;
    uLong comprLen, uncomprLen;
{
    z_stream c_stream; /* compression stream */
    int err;

    c_stream.zalloc = zalloc;
    c_stream.zfree = zfree;
    c_stream.opaque = (voidpf)0;

    err = deflateInit(&c_stream, Z_BEST_SPEED);
    CHECK_ERR(err, "deflateInit");

    c_stream.next_out = compr;
    c_stream.avail_out = (uInt)comprLen;

    /* At this point, uncompr is still mostly zeroes, so it should compress
     * very well:
     */
    c_stream.next_in = uncompr;
    c_stream.avail_in = (uInt)uncomprLen;
    err = deflate(&c_stream, Z_NO_FLUSH);
    CHECK_ERR(err, "deflate");
    if (c_stream.avail_in != 0) {
        fprintf(stderr, "deflate not greedy\n");
        exit(1);
    }

    /* Feed in already compressed data and switch to no compression: */
    deflateParams(&c_stream, Z_NO_COMPRESSION, Z_DEFAULT_STRATEGY);
    c_stream.next_in = compr;
    c_stream.avail_in = (uInt)comprLen/2;
    err = deflate(&c_stream, Z_NO_FLUSH);
    CHECK_ERR(err, "deflate");

    /* Switch back to compressing mode: */
    deflateParams(&c_stream, Z_BEST_COMPRESSION, Z_FILTERED);
    c_stream.next_in = uncompr;
    c_stream.avail_in = (uInt)uncomprLen;
    err = deflate(&c_stream, Z_NO_FLUSH);
    CHECK_ERR(err, "deflate");

    err = deflate(&c_stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        fprintf(stderr, "deflate should report Z_STREAM_END\n");
        exit(1);
    }
    err = deflateEnd(&c_stream);
    CHECK_ERR(err, "deflateEnd");
}

/* ===========================================================================
 * Test inflate() with large buffers
 */
void test_large_inflate(compr, comprLen, uncompr, uncomprLen)
    Byte *compr, *uncompr;
    uLong comprLen, uncomprLen;
{
    int err;
    z_stream d_stream; /* decompression stream */

    strcpy((char*)uncompr, "garbage");

    d_stream.zalloc = zalloc;
    d_stream.zfree = zfree;
    d_stream.opaque = (voidpf)0;

    d_stream.next_in  = compr;
    d_stream.avail_in = (uInt)comprLen;

    err = inflateInit(&d_stream);
    CHECK_ERR(err, "inflateInit");

    for (;;) {
        d_stream.next_out = uncompr;            /* discard the output */
        d_stream.avail_out = (uInt)uncomprLen;
        err = inflate(&d_stream, Z_NO_FLUSH);
        if (err == Z_STREAM_END) break;
        CHECK_ERR(err, "large inflate");
    }

    err = inflateEnd(&d_stream);
    CHECK_ERR(err, "inflateEnd");

    if (d_stream.total_out != 2*uncomprLen + comprLen/2) {
        fprintf(stderr, "bad large inflate: %ld\n", d_stream.total_out);
        exit(1);
    } else {
        printf("large_inflate(): OK\n");
    }
}

/* ===========================================================================
 * Test deflate() with full flush
 */
void test_flush(compr, comprLen)
    Byte *compr;
    uLong *comprLen;
{
    z_stream c_stream; /* compression stream */
    int err;
    uInt len = (uInt)strlen(hello)+1;

    c_stream.zalloc = zalloc;
    c_stream.zfree = zfree;
    c_stream.opaque = (voidpf)0;

    err = deflateInit(&c_stream, Z_DEFAULT_COMPRESSION);
    CHECK_ERR(err, "deflateInit");

    c_stream.next_in  = (z_const unsigned char *)hello;
    c_stream.next_out = compr;
    c_stream.avail_in = 3;
    c_stream.avail_out = (uInt)*comprLen;
    err = deflate(&c_stream, Z_FULL_FLUSH);
    CHECK_ERR(err, "deflate");

    compr[3]++; /* force an error in first compressed block */
    c_stream.avail_in = len - 3;

    err = deflate(&c_stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        CHECK_ERR(err, "deflate");
    }
    err = deflateEnd(&c_stream);
    CHECK_ERR(err, "deflateEnd");

    *comprLen = c_stream.total_out;
}

/* ===========================================================================
 * Test inflateSync()
 */
void test_sync(compr, comprLen, uncompr, uncomprLen)
    Byte *compr, *uncompr;
    uLong comprLen, uncomprLen;
{
    int err;
    z_stream d_stream; /* decompression stream */

    strcpy((char*)uncompr, "garbage");

    d_stream.zalloc = zalloc;
    d_stream.zfree = zfree;
    d_stream.opaque = (voidpf)0;

    d_stream.next_in  = compr;
    d_stream.avail_in = 2; /* just read the zlib header */

    err = inflateInit(&d_stream);
    CHECK_ERR(err, "inflateInit");

    d_stream.next_out = uncompr;
    d_stream.avail_out = (uInt)uncomprLen;

    inflate(&d_stream, Z_NO_FLUSH);
    CHECK_ERR(err, "inflate");

    d_stream.avail_in = (uInt)comprLen-2;   /* read all compressed data */
    err = inflateSync(&d_stream);           /* but skip the damaged part */
    CHECK_ERR(err, "inflateSync");

    err = inflate(&d_stream, Z_FINISH);
    if (err != Z_DATA_ERROR) {
        fprintf(stderr, "inflate should report DATA_ERROR\n");
        /* Because of incorrect adler32 */
        exit(1);
    }
    err = inflateEnd(&d_stream);
    CHECK_ERR(err, "inflateEnd");

    printf("after inflateSync(): hel%s\n", (char *)uncompr);
}

/* ===========================================================================
 * Test deflate() with preset dictionary
 */
void test_dict_deflate(compr, comprLen)
    Byte *compr;
    uLong comprLen;
{
    z_stream c_stream; /* compression stream */
    int err;

    c_stream.zalloc = zalloc;
    c_stream.zfree = zfree;
    c_stream.opaque = (voidpf)0;

    err = deflateInit(&c_stream, Z_BEST_COMPRESSION);
    CHECK_ERR(err, "deflateInit");

    err = deflateSetDictionary(&c_stream,
                (const Bytef*)dictionary, (int)sizeof(dictionary));
    CHECK_ERR(err, "deflateSetDictionary");

    dictId = c_stream.adler;
    c_stream.next_out = compr;
    c_stream.avail_out = (uInt)comprLen;

    c_stream.next_in = (z_const unsigned char *)hello;
    c_stream.avail_in = (uInt)strlen(hello)+1;

    err = deflate(&c_stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        fprintf(stderr, "deflate should report Z_STREAM_END\n");
        exit(1);
    }
    err = deflateEnd(&c_stream);
    CHECK_ERR(err, "deflateEnd");
}

/* ===========================================================================
 * Test inflate() with a preset dictionary
 */
void test_dict_inflate(compr, comprLen, uncompr, uncomprLen)
    Byte *compr, *uncompr;
    uLong comprLen, uncomprLen;
{
    int err;
    z_stream d_stream; /* decompression stream */

    strcpy((char*)uncompr, "garbage");

    d_stream.zalloc = zalloc;
    d_stream.zfree = zfree;
    d_stream.opaque = (voidpf)0;

    d_stream.next_in  = compr;
    d_stream.avail_in = (uInt)comprLen;

    err = inflateInit(&d_stream);
    CHECK_ERR(err, "inflateInit");

    d_stream.next_out = uncompr;
    d_stream.avail_out = (uInt)uncomprLen;

    for (;;) {
        err = inflate(&d_stream, Z_NO_FLUSH);
        if (err == Z_STREAM_END) break;
        if (err == Z_NEED_DICT) {
            if (d_stream.adler != dictId) {
                fprintf(stderr, "unexpected dictionary");
                exit(1);
            }
            err = inflateSetDictionary(&d_stream, (const Bytef*)dictionary,
                                       (int)sizeof(dictionary));
        }
        CHECK_ERR(err, "inflate with dict");
    }

    err = inflateEnd(&d_stream);
    CHECK_ERR(err, "inflateEnd");

    if (strcmp((char*)uncompr, hello)) {
        fprintf(stderr, "bad inflate with dict\n");
        exit(1);
    } else {
        printf("inflate with dictionary: %s\n", (char *)uncompr);
    }
}

/* ===========================================================================
 * Usage:  example [output.gz  [input.gz]]
 */

int main(argc, argv)
    int argc;
    char *argv[];
{
    Byte *compr, *uncompr;
    uLong comprLen = 10000*sizeof(int); /* don't overflow on MSDOS */
    uLong uncomprLen = comprLen;
    static const char* myVersion = ZLIB_VERSION;

    if (zlibVersion()[0] != myVersion[0]) {
        fprintf(stderr, "incompatible zlib version\n");
        exit(1);

    } else if (strcmp(zlibVersion(), ZLIB_VERSION) != 0) {
        fprintf(stderr, "warning: different zlib version\n");
    }

    printf("zlib version %s = 0x%04x, compile flags = 0x%lx\n",
            ZLIB_VERSION, ZLIB_VERNUM, zlibCompileFlags());

    compr    = (Byte*)calloc((uInt)comprLen, 1);
    uncompr  = (Byte*)calloc((uInt)uncomprLen, 1);
    /* compr and uncompr are cleared to avoid reading uninitialized
     * data and to ensure that uncompr compresses well.
     */
    if (compr == Z_NULL || uncompr == Z_NULL) {
        printf("out of memory\n");
        exit(1);
    }

#ifdef Z_SOLO
    argc = strlen(argv[0]);
#else
    test_compress(compr, comprLen, uncompr, uncomprLen);

/* START MODIFICATION BY INTELLIMAGIC, info@intellimagic.com */
    puts(" Using file pointers:");
    test_gzio((argc > 1 ? argv[1] : TESTFILE),
              uncompr, uncomprLen, 1);
    test_gzio_in_middle((argc > 1 ? argv[1] : TESTFILE),
                        uncompr, uncomprLen, 1);
    test_gzio_uncompressed_in_middle((argc > 1 ? argv[1] : TESTFILE),
                                     uncompr, uncomprLen, 1, 0);
    test_gzio_uncompressed_in_middle((argc > 1 ? argv[1] : TESTFILE),
                                     uncompr, uncomprLen, 1, 1);

    puts(" Using file descriptors:");
    test_gzio((argc > 1 ? argv[1] : TESTFILE),
              uncompr, uncomprLen, 0);
    test_gzio_in_middle((argc > 1 ? argv[1] : TESTFILE),
                        uncompr, uncomprLen, 0);
    test_gzio_uncompressed_in_middle((argc > 1 ? argv[1] : TESTFILE),
                                     uncompr, uncomprLen, 0, 0);
    test_gzio_uncompressed_in_middle((argc > 1 ? argv[1] : TESTFILE),
                                     uncompr, uncomprLen, 0, 1);

    puts(" Without file I/O:");
/* END MODIFICATION BY INTELLIMAGIC, info@intellimagic.com */
#endif

    test_deflate(compr, comprLen);
    test_inflate(compr, comprLen, uncompr, uncomprLen);

    test_large_deflate(compr, comprLen, uncompr, uncomprLen);
    test_large_inflate(compr, comprLen, uncompr, uncomprLen);

    test_flush(compr, &comprLen);
    test_sync(compr, comprLen, uncompr, uncomprLen);
    comprLen = uncomprLen;

    test_dict_deflate(compr, comprLen);
    test_dict_inflate(compr, comprLen, uncompr, uncomprLen);

    free(compr);
    free(uncompr);

    return 0;
}
