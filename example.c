/* example.c -- usage example of the zlib compression library
 * Copyright (C) 1995 Jean-loup Gailly.
 * For conditions of distribution and use, see copyright notice in zlib.h 
 */

/* $Id: example.c,v 1.5 1995/04/14 20:35:56 jloup Exp $ */

#include <stdio.h>
#include "zlib.h"

#define BUFLEN 4096

#define local static
/* For MSDOS and other systems with limitation on stack size. For Unix,
    #define local
   works also.
 */

#define CHECK_ERR(err, msg) { \
    if (err != Z_OK) { \
        fprintf(stderr, "%s error: %d\n", msg, err); \
	exit(1); \
    } \
}

char *hello = "hello world";

/* ===========================================================================
 * Test compress() and uncompress()
 */
void test_compress()
{
    local Byte compr[BUFLEN];
    uLong comprLen = sizeof(compr);
    local Byte uncompr[BUFLEN];
    uLong uncomprLen = sizeof(uncompr);
    int err;
    uLong len = strlen(hello)+1;

    err = compress(compr, &comprLen, hello, len);
    CHECK_ERR(err, "compress");

    strcpy(uncompr, "garbage");

    err = uncompress(uncompr, &uncomprLen, compr, comprLen);
    CHECK_ERR(err, "uncompress");

    if (strcmp(uncompr, hello)) {
	fprintf(stderr, "bad uncompress\n");
    } else {
	printf("uncompress(): %s\n", uncompr);
    }
}

/* ===========================================================================
 * Test read/write of .gz files
 */
void test_gzio(out, in)
    char *out; /* output file */
    char *in;  /* input file */
{
    local Byte uncompr[BUFLEN];
    int uncomprLen = sizeof(uncompr);
    int err;
    int len = strlen(hello)+1;
    gzFile file;

    file = gzopen(out, "wb");
    if (file == NULL) {
	fprintf(stderr, "gzopen error\n");
	exit(1);
    }

    if (gzwrite(file, hello, len) != len) {
	fprintf(stderr, "gzwrite err: %s\n", gzerror(file, &err));
    }
    gzclose(file);

    file = gzopen(in, "rb");
    if (file == NULL) {
	fprintf(stderr, "gzopen error\n");
    }
    strcpy(uncompr, "garbage");

    uncomprLen = gzread(file, uncompr, uncomprLen);
    if (uncomprLen != len) {
	fprintf(stderr, "gzread err: %s\n", gzerror(file, &err));
    }
    gzclose(file);

    if (strcmp(uncompr, hello)) {
	fprintf(stderr, "bad gzread\n");
    } else {
	printf("gzread(): %s\n", uncompr);
    }
}

/* ===========================================================================
 * Test deflate() with small buffers, return the compressed length.
 */
uLong test_deflate(compr)
    Byte compr[];
{
    z_stream c_stream; /* compression stream */
    int err;
    int len = strlen(hello)+1;

    c_stream.zalloc = (alloc_func)0;
    c_stream.zfree = (free_func)0;

    err = deflateInit(&c_stream, Z_DEFAULT_COMPRESSION);
    CHECK_ERR(err, "deflateInit");

    c_stream.next_in  = (Byte*)hello;
    c_stream.next_out = compr;

    while (c_stream.total_in != (uLong)len) {
	c_stream.avail_in = c_stream.avail_out = 1; /* force small buffers */
	err = deflate(&c_stream, Z_NO_FLUSH);
	CHECK_ERR(err, "deflate");
    }
    /* Finish the stream, still forcing small buffers: */
    do {
	c_stream.avail_out = 1;
	err = deflate(&c_stream, Z_FINISH);
	CHECK_ERR(err, "deflate");
    } while (c_stream.avail_out == 0);

    err = deflateEnd(&c_stream);
    CHECK_ERR(err, "deflateEnd");

    return c_stream.total_out;
}

/* ===========================================================================
 * Test inflate() with small buffers
 */
void test_inflate(compr)
    Byte compr[];
{
    local Byte uncompr[BUFLEN];
    int err;
    z_stream d_stream; /* decompression stream */

    strcpy(uncompr, "garbage");

    d_stream.zalloc = (alloc_func)0;
    d_stream.zfree = (free_func)0;

    err = inflateInit(&d_stream);
    CHECK_ERR(err, "inflateInit");

    d_stream.next_in  = compr;
    d_stream.next_out = uncompr;

    for (;;) {
	d_stream.avail_in = d_stream.avail_out = 1; /* force small buffers */
	err = inflate(&d_stream, Z_NO_FLUSH);
	if (err == Z_STREAM_END) break;
	CHECK_ERR(err, "inflate");
    }

    err = inflateEnd(&d_stream);
    CHECK_ERR(err, "inflateEnd");

    if (strcmp(uncompr, hello)) {
	fprintf(stderr, "bad inflate\n");
    } else {
	printf("inflate(): %s\n", uncompr);
    }
}

/* ===========================================================================
 * Usage:  example [output.gz  [input.gz]]
 */

void main(argc, argv)
    int argc;
    char *argv[];
{
    local Byte compr[BUFLEN];
    uLong comprLen;

    if (zlib_version[0] != ZLIB_VERSION[0]) {
	fprintf(stderr, "incompatible zlib version\n");
	exit(1);

    } else if (strcmp(zlib_version, ZLIB_VERSION) != 0) {
	fprintf(stderr, "warning: different zlib version\n");
    }
    test_compress();

    test_gzio((argc > 1 ? argv[1] : "foo.gz"),
	      (argc > 2 ? argv[2] : "foo.gz"));

    comprLen = test_deflate(compr);

    test_inflate(compr);

    exit(0);
}
