/* example.c -- usage example of the zlib compression library
 * Copyright (C) 1995 Jean-loup Gailly.
 * For conditions of distribution and use, see copyright notice in zlib.h 
 */

/* $Id: example.c,v 1.9 1995/05/03 17:27:09 jloup Exp $ */

#include <stdio.h>
#include "zlib.h"

#ifdef STDC
#  include <string.h>
#endif

#ifndef __GO32__
extern void exit  OF((int));
#endif

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

char *hello = "hello, hello!";
/* "hello world" would be more standard, but the repeated "hello"
 * stresses the compression code better, sorry...
 */

void test_compress OF((void));
void test_gzio     OF((char *out, char *in));
void test_deflate  OF((Byte compr[]));
void test_inflate  OF((Byte compr[]));
void test_flush    OF((Byte compr[]));
void test_sync     OF((Byte compr[]));
int  main          OF((int argc, char *argv[]));

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

    err = compress(compr, &comprLen, (Byte*)hello, len);
    CHECK_ERR(err, "compress");

    strcpy((char*)uncompr, "garbage");

    err = uncompress(uncompr, &uncomprLen, compr, comprLen);
    CHECK_ERR(err, "uncompress");

    if (strcmp((char*)uncompr, hello)) {
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
    strcpy((char*)uncompr, "garbage");

    uncomprLen = gzread(file, uncompr, uncomprLen);
    if (uncomprLen != len) {
        fprintf(stderr, "gzread err: %s\n", gzerror(file, &err));
    }
    gzclose(file);

    if (strcmp((char*)uncompr, hello)) {
        fprintf(stderr, "bad gzread\n");
    } else {
        printf("gzread(): %s\n", uncompr);
    }
}

/* ===========================================================================
 * Test deflate() with small buffers
 */
void test_deflate(compr)
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
void test_inflate(compr)
    Byte compr[];
{
    local Byte uncompr[BUFLEN];
    int err;
    z_stream d_stream; /* decompression stream */

    strcpy((char*)uncompr, "garbage");

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

    if (strcmp((char*)uncompr, hello)) {
        fprintf(stderr, "bad inflate\n");
    } else {
        printf("inflate(): %s\n", uncompr);
    }
}

/* ===========================================================================
 * Test deflate() with full flush
 */
void test_flush(compr)
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
    c_stream.avail_in = 3;
    c_stream.avail_out = BUFLEN;
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
}

/* ===========================================================================
 * Test inflateSync()
 */
void test_sync(compr)
    Byte compr[];
{
    local Byte uncompr[BUFLEN];
    int err;
    z_stream d_stream; /* decompression stream */

    strcpy((char*)uncompr, "garbage");

    d_stream.zalloc = (alloc_func)0;
    d_stream.zfree = (free_func)0;

    err = inflateInit(&d_stream);
    CHECK_ERR(err, "inflateInit");

    d_stream.next_in  = compr;
    d_stream.next_out = uncompr;
    d_stream.avail_in = 2; /* just read the zlib header */
    d_stream.avail_out = sizeof(uncompr);

    inflate(&d_stream, Z_NO_FLUSH);
    CHECK_ERR(err, "inflate");

    d_stream.avail_in = BUFLEN-2; /* let inflate read all compressed data */
    err = inflateSync(&d_stream); /* skip the damaged part */
    CHECK_ERR(err, "inflateSync");

    err = inflate(&d_stream, Z_FINISH);
    if (err != Z_DATA_ERROR) {
        fprintf(stderr, "inflate should report DATA_ERROR\n");
        /* Because of incorrect adler32 */
    }
    err = inflateEnd(&d_stream);
    CHECK_ERR(err, "inflateEnd");

    printf("after inflateSync(): hel%s\n", uncompr);
}

/* ===========================================================================
 * Usage:  example [output.gz  [input.gz]]
 */

int main(argc, argv)
    int argc;
    char *argv[];
{
    local Byte compr[BUFLEN];

    if (zlib_version[0] != ZLIB_VERSION[0]) {
        fprintf(stderr, "incompatible zlib version\n");
        exit(1);

    } else if (strcmp(zlib_version, ZLIB_VERSION) != 0) {
        fprintf(stderr, "warning: different zlib version\n");
    }
    test_compress();

    test_gzio((argc > 1 ? argv[1] : "foo.gz"),
              (argc > 2 ? argv[2] : "foo.gz"));

    test_deflate(compr);
    test_inflate(compr);

    test_flush(compr);
    test_sync(compr);

    exit(0);
    return 0; /* to avoid warning */
}
