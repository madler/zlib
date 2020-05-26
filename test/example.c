/* example.c -- usage example of the zlib compression library
 * Copyright (C) 1995-2006, 2011, 2016 Jean-loup Gailly
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

#define SUCCESSFUL                 1
#define FAILED_WITH_ERROR_CODE     0
#define FAILED_WITHOUT_ERROR_CODE -1

typedef struct test_result_s {
    int         result; /* One of: SUCCESSFUL,
                           FAILED_WITH_ERROR_CODE, or
                           FAILED_WITHOUT_ERROR_CODE*/
    int         error_code; /* error code if success is FAILED_WITH_ERROR_CODE */
    int         line_number;
    const char* message;
    const char* extended_message;
} test_result;

#define STRING_BUFFER_SIZE 100
char string_buffer[STRING_BUFFER_SIZE];
int g_fail_fast;
char g_test_class[STRING_BUFFER_SIZE] = "default";

#define CHECK_ERR(_error_code, _message) { \
    if (_error_code != Z_OK) { \
        test_result result; \
        result.result = FAILED_WITH_ERROR_CODE; \
        result.error_code = _error_code; \
        result.line_number = __LINE__; \
        result.message = _message; \
        result.extended_message = NULL; \
        return result; \
    } \
}

#define RETURN_FAILURE(_message, _extended_message) { \
    test_result result; \
    result.result = FAILED_WITHOUT_ERROR_CODE; \
    result.error_code = Z_OK; \
    result.line_number = __LINE__; \
    result.message = _message; \
    result.extended_message = _extended_message; \
    return result; \
}

#define RETURN_SUCCESS(_message, _extended_message) { \
    test_result result; \
    result.result = SUCCESSFUL; \
    result.error_code = Z_OK; \
    result.message = _message; \
    result.extended_message = _extended_message; \
    return result; \
}

void handle_stdout_test_results OF((test_result result, const char* testcase_name));
void handle_junit_test_results OF((FILE* output, test_result result, const char* testcase_name));
void handle_test_results OF((FILE* output, test_result result, const char* testcase_name, int is_junit_output, int* failed_test_count));

void handle_stdout_test_results(result, testcase_name)
    test_result result;
    const char* testcase_name;
{
    if (result.result == FAILED_WITH_ERROR_CODE) {
        fprintf(stderr, "%s error: %d\n", result.message, result.error_code);
    } else if (result.result == FAILED_WITHOUT_ERROR_CODE) {
        fprintf(stderr, "%s", result.message);
        if (result.extended_message != NULL) {
            fprintf(stderr, "%s", result.extended_message);
        }
        fprintf(stderr, "\n");
    } else {
        if (result.message != NULL) {
            if (result.extended_message != NULL) {
                fprintf(stderr, "%s%s\n", result.message, result.extended_message);
            } else {
                fprintf(stderr, "%s", result.message);
            }
        }
    }
}

void handle_junit_test_results(output, result, testcase_name)
    FILE* output;
    test_result result;
    const char* testcase_name;
{
    /* github.com/cirruslabs/cirrus-ci-annotations/blob/master/testdata/junit/PythonXMLRunner.xml
     * suggests how to encode classname, file, and line to get
     * annotations to appear properly on github.
     */
    if (result.result == FAILED_WITH_ERROR_CODE) {
        fprintf(output,
            "\t\t<testcase classname=\"%s\" name=\"%s\" file=\"%s\" line=\"%d\">\n"
            "\t\t\t<failure>%s: error %d</failure>\n"
            "\t\t</testcase>\n",
            g_test_class, testcase_name, __FILE__, result.line_number, result.message,
            result.error_code);
    } else if (result.result == FAILED_WITHOUT_ERROR_CODE) {
        fprintf(output,
            "\t\t<testcase classname=\"%s\" name=\"%s\" file=\"%s\" line=\"%d\">\n"
            "\t\t\t<failure>%s%s</failure>\n"
            "\t\t</testcase>\n",
            g_test_class, testcase_name, __FILE__, result.line_number, result.message,
            (result.extended_message ? result.extended_message : ""));
    } else {
        fprintf(output, "\t\t<testcase name=\"%s\"></testcase>\n", testcase_name);
    }
}

void handle_test_results(output, result, testcase_name, is_junit_output, failed_test_count)
    FILE* output;
    test_result result;
    const char* testcase_name;
    int is_junit_output;
    int* failed_test_count;
{
    if (is_junit_output) {
        handle_junit_test_results(output, result, testcase_name);
    }
    handle_stdout_test_results(result, testcase_name);
    if (result.result == FAILED_WITH_ERROR_CODE || result.result == FAILED_WITHOUT_ERROR_CODE) {
        (*failed_test_count)++;
        if (g_fail_fast)
            exit(1);
    }
}

static z_const char hello[] = "hello, hello!";
/* "hello world" would be more standard, but the repeated "hello"
 * stresses the compression code better, sorry...
 */

static const char dictionary[] = "hello";
static uLong dictId;    /* Adler32 value of the dictionary */

test_result test_deflate       OF((Byte *compr, uLong comprLen));
test_result test_inflate       OF((Byte *compr, uLong comprLen,
                                   Byte *uncompr, uLong uncomprLen));
test_result test_large_deflate OF((Byte *compr, uLong comprLen,
                                   Byte *uncompr, uLong uncomprLen));
test_result test_large_inflate OF((Byte *compr, uLong comprLen,
                                   Byte *uncompr, uLong uncomprLen));
test_result test_flush         OF((Byte *compr, uLong *comprLen));
test_result test_sync          OF((Byte *compr, uLong comprLen,
                                   Byte *uncompr, uLong uncomprLen));
test_result test_dict_deflate  OF((Byte *compr, uLong comprLen));
test_result test_dict_inflate  OF((Byte *compr, uLong comprLen,
                                   Byte *uncompr, uLong uncomprLen));
int  main                      OF((int argc, char *argv[]));


#ifdef Z_SOLO

void *myalloc OF((void *, unsigned, unsigned));
void myfree OF((void *, void *));

void *myalloc(q, n, m)
    void *q;
    unsigned n, m;
{
    (void)q;
    return calloc(n, m);
}

void myfree(void *q, void *p)
{
    (void)q;
    free(p);
}

static alloc_func zalloc = myalloc;
static free_func zfree = myfree;

#else /* !Z_SOLO */

static alloc_func zalloc = (alloc_func)0;
static free_func zfree = (free_func)0;

test_result test_compress      OF((Byte *compr, uLong comprLen,
                                   Byte *uncompr, uLong uncomprLen));
test_result test_gzio          OF((const char *fname,
                                   Byte *uncompr, uLong uncomprLen));
test_result test_force_fail    OF((int force_fail));

/* ===========================================================================
 * Test compress() and uncompress()
 */
test_result test_compress(compr, comprLen, uncompr, uncomprLen)
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
        RETURN_FAILURE("bad uncompress\n", NULL);
    } else {
        RETURN_SUCCESS("uncompress(): ", (char*)uncompr);
    }
}

/* ===========================================================================
 * Test read/write of .gz files
 */
test_result test_gzio(fname, uncompr, uncomprLen)
    const char *fname; /* compressed file name */
    Byte *uncompr;
    uLong uncomprLen;
{
#ifdef NO_GZCOMPRESS
    RETURN_FAILURE("NO_GZCOMPRESS -- gz* functions cannot compress", NULL);
#else
    int err;
    int len = (int)strlen(hello)+1;
    gzFile file;
    z_off_t pos;

    file = gzopen(fname, "wb");
    if (file == NULL) {
        RETURN_FAILURE("gzopen error", NULL);
    }
    gzputc(file, 'h');
    if (gzputs(file, "ello") != 4) {
        RETURN_FAILURE("gzputs err: ", gzerror(file, &err));
    }
    if (gzprintf(file, ", %s!", "hello") != 8) {
        RETURN_FAILURE("gzprintf err: ", gzerror(file, &err));
    }
    gzseek(file, 1L, SEEK_CUR); /* add one zero byte */
    gzclose(file);

    file = gzopen(fname, "rb");
    if (file == NULL) {
        RETURN_FAILURE("gzopen error", NULL);
    }
    strcpy((char*)uncompr, "garbage");

    if (gzread(file, uncompr, (unsigned)uncomprLen) != len) {
        RETURN_FAILURE("gzread err: ", gzerror(file, &err));
    }
    if (strcmp((char*)uncompr, hello)) {
        RETURN_FAILURE("bad gzread: ", (char*)uncompr);
    } else {
        printf("gzread(): %s\n", (char*)uncompr);
    }

    pos = gzseek(file, -8L, SEEK_CUR);
    if (pos != 6 || gztell(file) != pos) {
        test_result result;
        sprintf(string_buffer, "gzseek error, pos=%ld, gztell=%ld\n",
                (long)pos, (long)gztell(file));
        result.result = FAILED_WITHOUT_ERROR_CODE;
        result.error_code = Z_OK;
        result.line_number = __LINE__;
        result.message = string_buffer;
        result.extended_message = NULL;
        return result;
    }

    if (gzgetc(file) != ' ') {
        RETURN_FAILURE("gzgetc error", NULL);
    }

    if (gzungetc(' ', file) != ' ') {
        RETURN_FAILURE("gzungetc error", NULL);
    }

    gzgets(file, (char*)uncompr, (int)uncomprLen);
    if (strlen((char*)uncompr) != 7) { /* " hello!" */
        RETURN_FAILURE("gzgets err after gzseek: ", gzerror(file, &err));
    }
    if (strcmp((char*)uncompr, hello + 6)) {
        RETURN_FAILURE("bad gzgets after gzseek", NULL);
    } else {
        printf("gzgets() after gzseek: %s\n", (char*)uncompr);
    }

    gzclose(file);

    RETURN_SUCCESS(NULL, NULL);
#endif
}

/* ===========================================================================
 * Optionally test fault injection.
 */
test_result test_force_fail(force_fail)
    int force_fail;
{
    int err = Z_OK;
    if (force_fail)
        err = 54321;
    CHECK_ERR(err, "forced");

    RETURN_SUCCESS(NULL, NULL);
}

#endif /* Z_SOLO */

/* ===========================================================================
 * Test deflate() with small buffers
 */
test_result test_deflate(compr, comprLen)
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

    RETURN_SUCCESS(NULL, NULL);
}

/* ===========================================================================
 * Test inflate() with small buffers
 */
test_result test_inflate(compr, comprLen, uncompr, uncomprLen)
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
        RETURN_FAILURE("bad inflate\n", NULL);
    } else {
        RETURN_SUCCESS("inflate(): ", (char*)uncompr);
    }
}

/* ===========================================================================
 * Test deflate() with large buffers and dynamic change of compression level
 */
test_result test_large_deflate(compr, comprLen, uncompr, uncomprLen)
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
        RETURN_FAILURE("deflate not greedy\n", NULL);
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
        RETURN_FAILURE("deflate should report Z_STREAM_END\n", NULL);
    }
    err = deflateEnd(&c_stream);
    CHECK_ERR(err, "deflateEnd");

    RETURN_SUCCESS(NULL, NULL);
}

/* ===========================================================================
 * Test inflate() with large buffers
 */
test_result test_large_inflate(compr, comprLen, uncompr, uncomprLen)
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
        test_result result;
        sprintf(string_buffer, "bad large inflate: %ld\n", d_stream.total_out);
        result.result = FAILED_WITHOUT_ERROR_CODE;
        result.error_code = Z_OK;
        result.line_number = __LINE__;
        result.message = string_buffer;
        result.extended_message = NULL;
        return result;
    } else {
        RETURN_SUCCESS("large_inflate(): OK\n", NULL);
    }
}

/* ===========================================================================
 * Test deflate() with full flush
 */
test_result test_flush(compr, comprLen)
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

    RETURN_SUCCESS(NULL, NULL);
}

/* ===========================================================================
 * Test inflateSync()
 */
test_result test_sync(compr, comprLen, uncompr, uncomprLen)
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

    err = inflate(&d_stream, Z_NO_FLUSH);
    CHECK_ERR(err, "inflate");

    d_stream.avail_in = (uInt)comprLen-2;   /* read all compressed data */
    err = inflateSync(&d_stream);           /* but skip the damaged part */
    CHECK_ERR(err, "inflateSync");

    err = inflate(&d_stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        RETURN_FAILURE("inflate should report Z_STREAM_END\n", NULL);
    }
    err = inflateEnd(&d_stream);
    CHECK_ERR(err, "inflateEnd");

    RETURN_SUCCESS("after inflateSync(): hel", (char*)uncompr);
}

/* ===========================================================================
 * Test deflate() with preset dictionary
 */
test_result test_dict_deflate(compr, comprLen)
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
        RETURN_FAILURE("deflate should report Z_STREAM_END\n", NULL);
    }
    err = deflateEnd(&c_stream);
    CHECK_ERR(err, "deflateEnd");

    RETURN_SUCCESS(NULL, NULL);
}

/* ===========================================================================
 * Test inflate() with a preset dictionary
 */
test_result test_dict_inflate(compr, comprLen, uncompr, uncomprLen)
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
        RETURN_FAILURE("bad inflate with dict\n", NULL);
    } else {
        RETURN_SUCCESS("inflate with dictionary: ", (char*)uncompr);
    }
}

/* ===========================================================================
 * Usage:  example [--fail_fast][--force_fail][--junit results.xml] [output.gz [input.gz]]
 */

int main(argc, argv)
    int argc;
    char *argv[];
{
    Byte *compr, *uncompr;
    uLong comprLen = 10000*sizeof(int); /* don't overflow on MSDOS */
    uLong uncomprLen = comprLen;
    static const char* myVersion = ZLIB_VERSION;
    test_result result;
    int is_junit_output = 0;
    const char* output_file_path = NULL;
    FILE* output = stdout;
    int next_argv_index = 1;
    int failed_test_count = 0;
    int force_fail = 0;

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
    (void)argc;
    (void)argv;
#else
    output_file_path = getenv("ZLIB_JUNIT_OUTPUT_FILE");
    force_fail = getenv("ZLIB_FORCE_FAIL") && atoi(getenv("ZLIB_FORCE_FAIL"));
    g_fail_fast = getenv("ZLIB_FAIL_FAST") && atoi(getenv("ZLIB_FAIL_FAST"));
    while (next_argv_index < argc && !strncmp(argv[next_argv_index], "--", 2)) {
        if (strcmp(argv[next_argv_index], "--junit") == 0) {
            next_argv_index++;
            if (argc <= next_argv_index) {
                fprintf(stderr, "--junit flag requires an output file parameter, like --junit output.xml");
                exit(1);
            }
            output_file_path = argv[next_argv_index];
            next_argv_index++;
        } else if (strcmp(argv[next_argv_index], "--fail_fast") == 0) {
            g_fail_fast = 1;
            next_argv_index++;
        } else if (strcmp(argv[next_argv_index], "--force_fail") == 0) {
            force_fail = 1;
            next_argv_index++;
        } else {
            fprintf(stderr, "Unrecognized option %s\n", argv[next_argv_index]);
            exit(1);
        }
    }
    if (output_file_path) {
        fprintf(stdout, "output path is %s", output_file_path);
        is_junit_output = 1;
        output = fopen(output_file_path, "w+");
        if (!output) {
            fprintf(stderr, "Could not open junit file %s\n", output_file_path);
            exit(1);
        } else {
            /* If run by CMakeLists.txt's JUNIT option, path is FOO/$exe-junit.xml.
             * Extract $exe and pass to handle_junit_test_results so github error
             * annotations show the name of the executable causing each error.
             */
            const char *p = strrchr(output_file_path, '/');
            if (p) {
                p++;   /* skip past slash */
                const char *q = strrchr(p, '-');
                int len = q - p;
                if (!strcmp(q, "-junit.xml") && (len < sizeof(g_test_class))) {
                    strncpy(g_test_class, p, len);
                    g_test_class[len] = 0;
                }
            }
        }
        fprintf(output, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
        fprintf(output, "<testsuites>\n");
        fprintf(output, "\t<testsuite name=\"zlib example suite\">\n");
    }

    result = test_compress(compr, comprLen, uncompr, uncomprLen);
    handle_test_results(output, result, "compress", is_junit_output, &failed_test_count);
	
    result = test_gzio((argc > next_argv_index ? argv[next_argv_index++] : TESTFILE),
                       uncompr, uncomprLen);
    handle_test_results(output, result, "gzio", is_junit_output, &failed_test_count);

    result = test_force_fail(force_fail);
    handle_test_results(output, result, "force fail", is_junit_output, &failed_test_count);
#endif

    result = test_deflate(compr, comprLen);
    handle_test_results(output, result, "deflate", is_junit_output, &failed_test_count);
    result = test_inflate(compr, comprLen, uncompr, uncomprLen);
    handle_test_results(output, result, "inflate", is_junit_output, &failed_test_count);

    result = test_large_deflate(compr, comprLen, uncompr, uncomprLen);
    handle_test_results(output, result, "large deflate", is_junit_output, &failed_test_count);
    result = test_large_inflate(compr, comprLen, uncompr, uncomprLen);
    handle_test_results(output, result, "large inflate", is_junit_output, &failed_test_count);

    result = test_flush(compr, &comprLen);
    handle_test_results(output, result, "flush", is_junit_output, &failed_test_count);
    result = test_sync(compr, comprLen, uncompr, uncomprLen);
    handle_test_results(output, result, "sync", is_junit_output, &failed_test_count);
    comprLen = uncomprLen;

    result = test_dict_deflate(compr, comprLen);
    handle_test_results(output, result, "dict deflate", is_junit_output, &failed_test_count);
    result = test_dict_inflate(compr, comprLen, uncompr, uncomprLen);
    handle_test_results(output, result, "dict inflate", is_junit_output, &failed_test_count);

    if (is_junit_output) {
        fprintf(output, "\t</testsuite>\n");
        fprintf(output, "</testsuites>");
        fclose(output);
    }
    free(compr);
    free(uncompr);

    if (failed_test_count) {
        return 1;
    }
    return 0;
}
