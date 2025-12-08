/* example.c -- usage example of the zlib compression library
 * Copyright (C) 1995-2006, 2011, 2016 Jean-loup Gailly
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* @(#) $Id$ */

#if defined(_WIN32) && !defined(_CRT_SECURE_NO_WARNINGS)
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include "zlib.h"
#include <stdio.h>
/*test를 위한 삽입*/
#include <errno.h>

#ifdef STDC
#  include <string.h>
#  include <stdlib.h>
#endif

#if defined(VMS)
#  define TESTFILE "foo-gz"
#elif defined(__riscos) && !defined(__TARGET_UNIXLIB__)
#  define TESTFILE "foo/gz"
#else
#  define TESTFILE "foo.gz"
#endif

/*코드의 에러 처리는 대부분 CHECK_ERR 매크로에 의존하고 있으며, 이 매크로를 중심으로 에러 메시지를 분석*/
/*기존에 존재하던 CHECK_ERR 수정*/
#define CHECK_ERR(err, msg) { \
    if (err != Z_OK) { \
        fprintf(stderr, \
            "[Zlib error occurred] \nError message: %s \nDetails: (%s).\n", msg, zError(err)); \
        fprintf(stderr, \
            "-> Error code: %d\n",err); \
            exit(1); \
    } \
}



/* 새로 만든 err 함수 */
void check_zlib_error(int err, z_stream *strm, const char *msg)
{
    if (err != Z_OK) {
        /* 새로운 오류 처리 함수를 사용하여 상세 메시지를 가져옴. */
        const char *full_msg = zlib_get_full_error(err, strm);
        
        /* 기존 메시지와 함께 상세 메시지를 표준 오류 스트림에 출력 */
        fprintf(stderr, "FATAL ZLIB ERROR: %s\n", msg);
        fprintf(stderr, "Zlib Detailed Status: %s\n", full_msg);

        exit(1);
    }   
}



static z_const char hello[] = "hello, hello!";
/* "hello world" would be more standard, but the repeated "hello"
 * stresses the compression code better, sorry...
 */

static const char dictionary[] = "hello";
static uLong dictId;    /* Adler32 value of the dictionary */

#ifdef Z_SOLO

static void *myalloc(void *q, unsigned n, unsigned m) {
    (void)q;
    return calloc(n, m);
}

static void myfree(void *q, void *p) {
    (void)q;
    free(p);
}

static alloc_func zalloc = myalloc;
static free_func zfree = myfree;

#else /* !Z_SOLO */

static alloc_func zalloc = (alloc_func)0;
static free_func zfree = (free_func)0;

/* ===========================================================================
 * Test compress() and uncompress()
 */
static void test_compress(Byte *compr, uLong comprLen, Byte *uncompr,
                   uLong uncomprLen) {
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
 * 여기는 gzerror()를 사용하기에 관련 에러출력 에러 코드 까지 나오게 통일시킴
 */
static void test_gzio(const char *fname, Byte *uncompr, uLong uncomprLen) {
#ifdef NO_GZCOMPRESS
    fprintf(stderr, "NO_GZCOMPRESS -- gz* functions cannot compress\n");
#else
    int err;
    int len = (int)strlen(hello)+1;
    gzFile file;
    z_off_t pos;

    file = gzopen(fname, "wb");
    if (file == NULL) {
        fprintf(stderr, "gzopen error\n");
        exit(1);
    }
    gzputc(file, 'h');
    if (gzputs(file, "ello") != 4) {
        const char* err_msg = gzerror(file, &err);
        fprintf(stderr, "[gzputs err] \nError message: %s\n-> Error code: %d", err_msg, err);  /*각각의 fprintf 수정함*/
        exit(1);
    }
    if (gzprintf(file, ", %s!", "hello") != 8) {
        const char* err_msg = gzerror(file, &err);
        fprintf(stderr, "[gzprintf err] \nError message: %s\n-> Error code: %d", err_msg, err);
        exit(1);
    }
    gzseek(file, 1L, SEEK_CUR); /* add one zero byte */
    gzclose(file);

    file = gzopen(fname, "rb");
    if (file == NULL) {
        fprintf(stderr, "gzopen error\n");
        exit(1);
    }
    strcpy((char*)uncompr, "garbage");

    if (gzread(file, uncompr, (unsigned)uncomprLen) != len) {
        const char* err_msg = gzerror(file, &err);
        fprintf(stderr, "[gzread err] \nError message: %s\n-> Error code: %d", err_msg, err);
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
        const char* err_msg = gzerror(file, &err);
        fprintf(stderr, "[gzgets err after gzseek] \nError message: %s\n-> Error code: %d", err_msg, err);
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

#endif /* Z_SOLO */

/* ===========================================================================
 * Test deflate() with small buffers
 */
static void test_deflate(Byte *compr, uLong comprLen) {
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
static void test_inflate(Byte *compr, uLong comprLen, Byte *uncompr,
                  uLong uncomprLen) {
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
static void test_large_deflate(Byte *compr, uLong comprLen, Byte *uncompr,
                        uLong uncomprLen) {
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
    c_stream.avail_in = (uInt)uncomprLen/2;
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
static void test_large_inflate(Byte *compr, uLong comprLen, Byte *uncompr,
                        uLong uncomprLen) {
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

    if (d_stream.total_out != 2*uncomprLen + uncomprLen/2) {
        fprintf(stderr, "bad large inflate: %ld\n", d_stream.total_out);
        exit(1);
    } else {
        printf("large_inflate(): OK\n");
    }
}

/* ===========================================================================
 * Test deflate() with full flush
 */
static void test_flush(Byte *compr, uLong *comprLen) {
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
static void test_sync(Byte *compr, uLong comprLen, Byte *uncompr,
                      uLong uncomprLen) {
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

    err = inflate(&d_stream, Z_NO_FLUSH);
    CHECK_ERR(err, "inflate");

    d_stream.avail_in = (uInt)comprLen-2;   /* read all compressed data */
    err = inflateSync(&d_stream);           /* but skip the damaged part */
    CHECK_ERR(err, "inflateSync");

    err = inflate(&d_stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        fprintf(stderr, "inflate should report Z_STREAM_END\n");
        exit(1);
    }
    err = inflateEnd(&d_stream);
    CHECK_ERR(err, "inflateEnd");

    printf("after inflateSync(): hel%s\n", (char *)uncompr);
}

/* ===========================================================================
 * Test deflate() with preset dictionary
 */
static void test_dict_deflate(Byte *compr, uLong comprLen) {
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
static void test_dict_inflate(Byte *compr, uLong comprLen, Byte *uncompr,
                       uLong uncomprLen) {
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
 * Test error conditions - 의도적으로 에러 발생시키기
 */
static void test_error_conditions(void) {
    z_stream stream;
    int err;
    Byte buffer[100];
    
    printf("\n=== Testing Error Conditions ===\n");
    
    /* 테스트 1: 초기화 없이 deflate 호출 */
    printf("Test 1: Using uninitialized stream\n");
    memset(&stream, 0, sizeof(stream));
    stream.next_in = (Bytef*)"test";
    stream.avail_in = 4;
    stream.next_out = buffer;
    stream.avail_out = 100;
    err = deflate(&stream, Z_FINISH);
    CHECK_ERR(err, "Z_STREAM_ERROR"); /*CHECK_ERR가 나는 순간 프로그램 정지*/
    /*printf("  Result: %d (Expected: Z_STREAM_ERROR = %d)\n\n", err, Z_STREAM_ERROR);*/
    
    /* 테스트 2: 잘못된 압축 레벨 */
    printf("Test 2: Invalid compression level\n");
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    err = deflateInit(&stream, 99);  // 유효하지 않은 레벨
    CHECK_ERR(err, "Z_STREAM_ERROR");
    
    /* 테스트 3: NULL 포인터 */
    printf("Test 3: NULL pointer\n");
    err = deflateInit(NULL, Z_DEFAULT_COMPRESSION);
    CHECK_ERR(err, "Z_STREAM_ERROR");
    
    /* 테스트 4: 손상된 데이터 압축 해제 */
    printf("\nTest 4: Corrupted compressed data\n");
    Byte corrupted[] = {0x78, 0x9c, 0xff, 0xff, 0xff, 0xff};  // 손상된 데이터
    memset(&stream, 0, sizeof(stream));
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    err = inflateInit(&stream);
    if (err == Z_OK) {
        stream.next_in = corrupted;
        stream.avail_in = sizeof(corrupted);
        stream.next_out = buffer;
        stream.avail_out = 100;
        err = inflate(&stream, Z_FINISH);
        CHECK_ERR(err, "Z_DATA_ERROR");
        inflateEnd(&stream);
    }
    
    printf("\n=== Error Condition Tests Complete ===\n\n");
}


/* ===========================================================================
 * Test error conditions_v2 - 의도적으로 에러 발생시키기
 */
static void test_error_conditions_v2() {
    z_stream stream;
    int err;
    Byte buffer[100];
    
    printf("\n=== Testing Error Conditions_V2 ===\n");
    
        /* 테스트 1: 초기화 없이 deflate 호출 */
    printf("Test 1: Using uninitialized stream\n");
    memset(&stream, 0, sizeof(stream));
    stream.next_in = (Bytef*)"test";
    stream.avail_in = 4;
    stream.next_out = buffer;
    stream.avail_out = 100;
    err = deflate(&stream, Z_FINISH);
    check_zlib_error(err, &stream, "Deflate on uninitialized stream");
    /*printf("  Result: %d (Expected: Z_STREAM_ERROR = %d)\n\n", err, Z_STREAM_ERROR);*/
    
    /* 테스트 2: 잘못된 압축 레벨 */
    printf("Test 2: Invalid compression level\n");
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    err = deflateInit(&stream, 99);  // 유효하지 않은 레벨
    check_zlib_error(err, &stream, "Invalid compression level");
    
    /* 테스트 3: NULL 포인터 */
    printf("Test 3: NULL pointer\n");
    err = deflateInit(NULL, Z_DEFAULT_COMPRESSION);
    check_zlib_error(err, &stream, "NULL pointer");
    
    
    
    /* 테스트 4: 손상된 데이터 압축 해제 */
    printf("\nTest 4: Corrupted compressed data\n");
    Byte corrupted[] = {0x78, 0x9c, 0xff, 0xff, 0xff, 0xff};  // 손상된 데이터
    memset(&stream, 0, sizeof(stream));
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    err = inflateInit(&stream);
    check_zlib_error(err, &stream, "Corrupted compressed data");
    if (err == Z_OK) {
        stream.next_in = corrupted;
        stream.avail_in = sizeof(corrupted);
        stream.next_out = buffer;
        stream.avail_out = 100;
        err = inflate(&stream, Z_FINISH);
        check_zlib_error(err, &stream, "Corrupted compressed data");
        /*printf("  Result: %d (Expected: Z_DATA_ERROR = %d)\n", err, Z_DATA_ERROR);*/
        inflateEnd(&stream);
    }
 


    /* 테스트 5: 오류 메시지 확인 (Z_ERRNO Simulation OS) */
    printf("Test 5: Z_ERRNO Simulation (Expected EXIT)\n");
    
    // 존재하지 않는 파일에 접근하는 상황을 시뮬레이션하기 위해 errno를 ENOENT로 설정
    // Zlib 함수는 Z_ERRNO를 반환하면서 errno를 설정했을 것이라고 가정합니다.
    errno = ENOENT;
    err = Z_ERRNO; // Zlib에서 Z_ERRNO (-1)를 반환했다고 가정
    
    // Z_ERRNO 발생 및 상세 보고 후 종료 예상. OS 오류 메시지가 출력되어야 합니다.
    check_zlib_error(err, NULL, "Simulated File I/O Error (Z_ERRNO Test)");


    printf("\n=== Error Condition Tests Complete ===\n\n");
}






/* ===========================================================================
 * Usage:  example [output.gz  [input.gz]]
 */

int main(int argc, char *argv[]) {
    Byte *compr, *uncompr;
    uLong uncomprLen = 20000;
    uLong comprLen = 3 * uncomprLen;
    static const char* myVersion = ZLIB_VERSION;

    if (zlibVersion()[0] != myVersion[0]) {
        fprintf(stderr, "incompatible zlib version\n");
        exit(1);

    } else if (strcmp(zlibVersion(), ZLIB_VERSION) != 0) {
        fprintf(stderr, "warning: different zlib version linked: %s\n",
                zlibVersion());
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


    /*강제 오류 발생하여 test*/
    /*test_error_conditions();*/
    test_error_conditions_v2();


#ifdef Z_SOLO
    (void)argc;
    (void)argv;
#else
    test_compress(compr, comprLen, uncompr, uncomprLen);

    test_gzio((argc > 1 ? argv[1] : TESTFILE),
              uncompr, uncomprLen);
#endif

    test_deflate(compr, comprLen);
    test_inflate(compr, comprLen, uncompr, uncomprLen);

    test_large_deflate(compr, comprLen, uncompr, uncomprLen);
    test_large_inflate(compr, comprLen, uncompr, uncomprLen);

    test_flush(compr, &comprLen);
    test_sync(compr, comprLen, uncompr, uncomprLen);
    comprLen = 3 * uncomprLen;

    test_dict_deflate(compr, comprLen);
    test_dict_inflate(compr, comprLen, uncompr, uncomprLen);

    free(compr);
    free(uncompr);



    return 0;
}