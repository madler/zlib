#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include "zlib.h"

void test1(unsigned char* in, unsigned inlen, int strategy, int level, int windowBits)
{
    z_streamp strmp = Z_NULL;
    int ret = deflateInit(strmp, Z_DEFAULT_COMPRESSION);
    assert(ret == Z_STREAM_ERROR);

    z_stream strm;
    strm.zalloc = 0;
    strm.zfree = 0;
    ret = deflateInit2(&strm, level, Z_DEFLATED, windowBits, MAX_MEM_LEVEL, strategy);
    assert(ret == Z_OK);

    gz_header head;
    int dictlength = 0;
    char* dictionary = "zlib deflate infalte";
    dictlength = strlen(dictionary);
    if (windowBits > 15) {
        head.extra = "extra";
        head.extra_len = 5;
        head.name = "name";
        head.comment = "comment";
        ret = deflateSetHeader(&strm, &head);
        assert(ret == Z_OK);

        ret = deflateSetDictionary(&strm, dictionary, dictlength);
        assert(ret == Z_STREAM_ERROR);
    } else {
        ret = deflateSetHeader(&strm, &head);
        assert(ret == Z_STREAM_ERROR);

        ret = deflateSetDictionary(&strm, dictionary, dictlength);
        assert(ret == Z_OK);
    }

    ret = deflateTune(&strm, 100, 300, 300, 5000);
    assert(ret == Z_OK);

    z_stream strm2;
    ret = deflateCopy(&strm2, &strm);
    assert(ret == Z_OK);

    ret = deflateEnd(&strm2);
    assert(ret == Z_OK);

    unsigned outlen = deflateBound(&strm, inlen);
    unsigned char* out = malloc(outlen);
    assert(out != Z_NULL);

    strm.next_in = in;
    strm.avail_in = inlen;
    strm.next_out = out;
    strm.avail_out = outlen;

    ret = deflateParams(&strm, 10, strategy);
    assert(ret == Z_STREAM_ERROR);

    strategy = (strategy + 1) > Z_FIXED ? Z_DEFAULT_STRATEGY : (strategy + 1);
    ret = deflateParams(&strm, level, strategy);
    assert(ret == Z_OK);

    ret = deflate(&strm, Z_NO_FLUSH);
    assert(ret == Z_OK || ret == Z_BUF_ERROR);

    unsigned int* pending = (unsigned int*)malloc(outlen);
    assert(pending != Z_NULL);

    int bits;
    ret = deflatePending(&strm, pending, &bits);
    assert(ret == Z_OK);

    // incorrect usage, this is just for test
    ret = deflatePrime(&strm, 16, 0xFF);
    assert(ret == Z_OK);

    while (1) {
        ret = deflate(&strm, Z_FINISH);
        assert(ret == Z_STREAM_END || ret == Z_OK);

        if (ret == Z_STREAM_END) {
            break;
        }
    }

    unsigned char* dict = malloc(outlen);
    assert(dict != Z_NULL);

    ret = deflateGetDictionary(&strm, dict, &outlen);
    assert(ret == Z_OK);

    ret = deflateEnd(&strm);
    assert(ret == Z_OK);

    free(dict);
    free(out);
    free(pending);
}

void test2(const char* filename, int strategy, int level, int windowBits, int flush)
{
    FILE* fs = fopen(filename, "r");
    if (!fs) {
        printf("fopen file %s failed!!!\n", filename);
        return;
    }

    int inlen = 100;
    char in[100] = {0};
    int outlen = 50;
    char out[50] = {0};

    z_stream strm;
    strm.next_in = in;
    strm.avail_in = 0;
    strm.next_out = out;
    strm.avail_out = outlen;
    strm.zalloc = (alloc_func)0;
    strm.zfree = (free_func)0;

    int ret = deflateInit2(&strm, level, Z_DEFLATED, windowBits, MAX_MEM_LEVEL, strategy);
    assert(ret == Z_OK);

    while (1) {
        strm.next_in = in;
        strm.avail_in = fread(strm.next_in, 1, inlen, fs);
        if (feof(fs)) {
            break;
        }
        while (strm.avail_out == 0) {
            ret = deflate(&strm, flush);
            assert(ret == Z_OK);

            strm.avail_out = outlen;
        }
    }

    while (1) {
        ret = deflate(&strm, Z_FINISH);
        assert(ret == Z_OK || ret == Z_STREAM_END);

        if (ret == Z_STREAM_END) {
            break;
        }
        strm.avail_out = outlen;
    }

    ret = deflateEnd(&strm);
    fclose(fs);
}

unsigned int malloc_input(const char* filename, char** input)
{
    unsigned int fsize = 0;

    // open file
    FILE* fs = fopen(filename, "r");
    if(!fs) {
        printf("fopen file %s failed!!!\n", filename);
        return 0;
    }

    // get file size
    if (fs) {
        fpos_t pos;
        fgetpos(fs, &pos);
        if (fseek(fs, 0, SEEK_END) == 0) {
            long ret = ftell(fs);
            if (ret < 0) {
                printf("ftell error: %d !!!\n", errno);
                fclose(fs);
                return 0;
            }
            if (ret == 0) {
                printf("file may be empty!!!\n");
                fclose(fs);
                return 0;
            }
            fsize = (unsigned int)ret;
        }
        fsetpos(fs, &pos);
    }

    // malloc
    *input = (char*) malloc(fsize);
    if (!*input) {
        printf("malloc failed!!!\n");
        fclose(fs);
        return 0;
    }

    // read data
    if (fs) {
        size_t ret = fread(*input, 1, fsize, fs);
        if (ret != fsize) {
            printf("fread error: %d !!!\n", errno);
            fclose(fs);
            return 0;
        }
    }

    return fsize;    
}

void free_input(char* input)
{
    free(input);
}

int main(void)
{
    const char* filepath = "./zlib.h";
    char* input = NULL;
    long len = malloc_input(filepath, &input);
    assert(len > 0);

    test1(input, len, Z_FILTERED, Z_NO_COMPRESSION, MAX_WBITS);
    test1(input, len, Z_HUFFMAN_ONLY, Z_BEST_COMPRESSION, MAX_WBITS);
    test1(input, len, Z_RLE, Z_BEST_COMPRESSION, MAX_WBITS);
    test1(input, len, Z_FIXED, Z_BEST_SPEED, MAX_WBITS);

    test1(input, len, Z_DEFAULT_STRATEGY, Z_BEST_COMPRESSION, MAX_WBITS + 16);             // gzip head
    test1(input, len, Z_DEFAULT_STRATEGY, Z_DEFAULT_COMPRESSION, -MAX_WBITS);              // raw deflate

    free_input(input);

    test2(filepath, Z_DEFAULT_STRATEGY, Z_BEST_COMPRESSION, MAX_WBITS + 16, Z_NO_FLUSH);   // gzip head
    test2(filepath, Z_DEFAULT_STRATEGY, Z_BEST_COMPRESSION, MAX_WBITS, Z_FULL_FLUSH);
    test2(filepath, Z_DEFAULT_STRATEGY, Z_BEST_COMPRESSION, MAX_WBITS, Z_SYNC_FLUSH);
    test2(filepath, Z_DEFAULT_STRATEGY, Z_BEST_COMPRESSION, MAX_WBITS, Z_PARTIAL_FLUSH);
    
    return 0;
}