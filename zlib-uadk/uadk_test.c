/* *********************************************************************
  Copyright (c) 2023 Huawei Technologies Co., Ltd.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Huawei Technologies Co. nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************* */
#include <stdlib.h>
#include <pthread.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <dlfcn.h>
#include "zlib.h"
#define MAX_PARALLEL 1024
#define MAX_FUNC_NUM 128
volatile int g_cstart = 0;
volatile int g_cfinish = 0;
volatile int g_dstart = 0;
volatile int g_dfinish = 0;
typedef struct {
    volatile size_t bytesHandle;
    volatile unsigned char ready;
    volatile unsigned char finished;
} comp_param;
typedef int (*compression_op_func) (char* src, size_t srcLen, char* dst, size_t dstLen);
typedef int (*compression_lib_init_func) ();
void* g_lib_funcs[MAX_FUNC_NUM];
void* g_lib_handle;

int zlib_comp(char* src, size_t srcLen, char* dst, size_t dstLen) {
    size_t ret = dstLen;
    int err = compress2(dst, &ret, src, srcLen, 6);
    if (err)
        return err > 0 ? -err : err;
    return ret;
}
int zlib_decomp(char* src, size_t srcLen, char* dst, size_t dstLen) {
    size_t ret = dstLen;
    int err = uncompress(dst, &ret, src, srcLen);
    if (err)
        return err > 0 ? -err : err;
    return ret;
}
void* create_copy(void* src, size_t size) {
    void* ret = malloc(size);
    memcpy(ret, src, size);
    return ret;
}
#define SUPPORTED_ALG_NUM 2
comp_param g_comp_params[MAX_PARALLEL];
unsigned int g_parallel;
size_t g_filesize;
char* g_inbuf;
char* g_compbuf;
size_t g_chunk_num;
unsigned int g_chunksize;
size_t* g_compressed_chunk_offset;
size_t* g_compressed_chunk_size;
size_t g_comp_data_size;
void reset_g_comp_params() {
    memset(g_comp_params, 0, g_parallel * sizeof(comp_param));
}
int all_thread_ready() {
    for (int i = 0;i < g_parallel;i++) {
        if (!g_comp_params[i].ready) return 0;
    }
    return 1;
}
int all_thread_finish() {
    for (int i = 0;i < g_parallel;i++) {
        if (!g_comp_params[i].finished) return 0;
    }
    return 1;
}
size_t getFileSize(const char* infilename)
{
    int r;
    struct stat statbuf;
    r = stat(infilename, &statbuf);
    if (r || !S_ISREG(statbuf.st_mode)) return 0;
    return (size_t)statbuf.st_size;
}
void* comp_func(void* arg) {
    comp_param* param = (comp_param*)arg;
    param->bytesHandle = 0;
    unsigned int bufSize = g_chunksize + g_chunksize / 10;
    char* indata = (char*)create_copy(g_inbuf, g_filesize);
    char* buf = (char*)malloc(bufSize);
    param->ready = 1;
    while(!g_cstart) {}
    while(1) {
        size_t currPos = 0;
	while (currPos < g_filesize) {
	    size_t compsize = g_filesize - currPos;
	    if (compsize > g_chunksize)  compsize = g_chunksize;
	    int ret = zlib_comp(indata + currPos, compsize, buf, bufSize);
            if (ret < 0) {
	        printf("compress error %d\n", ret);
	    }
	    param->bytesHandle += compsize;
	    currPos += compsize;
	    if (g_cfinish) {
	        goto comp_finish;
	    }
	}
    }
comp_finish:
    free(buf);
    free(indata);
    param->finished = 1;
    return NULL;
}
void* decomp_func(void* arg) {
    comp_param* param = (comp_param*)arg;
    param->bytesHandle = 0;
    unsigned int bufSize = g_chunksize + g_chunksize / 10;
    char* buf = (char*)malloc(bufSize);
    char* comp_data = (char*)create_copy(g_compbuf, g_comp_data_size);
    size_t* compressed_chunk_offset = (size_t*)create_copy(g_compressed_chunk_offset, sizeof(size_t) * g_chunk_num);
    size_t* compressed_chunk_size = (size_t*)create_copy(g_compressed_chunk_size, sizeof(size_t) * g_chunk_num);
    param->ready = 1;
    while(!g_dstart) {}
    while(1) {
        for (int i = 0;i < g_chunk_num;i++) {
	    unsigned int expectedSize = g_filesize - g_chunksize * i;
	    if (expectedSize > g_chunksize) expectedSize = g_chunksize;
	    unsigned int uncompressedSize = zlib_decomp(comp_data + compressed_chunk_offset[i], compressed_chunk_size[i], buf, expectedSize);
	    if (uncompressedSize == expectedSize) {
	        param->bytesHandle += expectedSize;
	    } else {
	        printf("expected decompressed size %d but actually %d\n", expectedSize, uncompressedSize);
	    }
	    if (*(size_t*)(g_inbuf + g_chunksize * i) != *(size_t*)(buf)) {
	        printf("decompress data error!\n");
	    }
	    if (g_dfinish) {
	        goto decomp_finish;
	    }
	}
    }
decomp_finish:
    free(buf);
    free(comp_data);
    free(compressed_chunk_offset);
    free(compressed_chunk_size);
    param->finished = 1;
    return NULL;
}
void printUsage(char* program_name) {
   printf("usage: %s (parallel) (chunksize) (compress_secs) (decompress_secs) (infile)\n", program_name); 
}
int main(int argc, char* argv[]) {
    if (argc < 6) {
        printUsage(argv[0]);
	return 1;
    }
    int arg_idx = 1;
    g_parallel = atoi(argv[arg_idx++]);
    g_chunksize = atoi(argv[arg_idx++]);
    int csec = atoi(argv[arg_idx++]);
    int dsec = atoi(argv[arg_idx++]);
    char* infile = argv[arg_idx++];
    if (g_parallel > MAX_PARALLEL) {
        printf("expected max parallel %d but actually %d.\n", MAX_PARALLEL, g_parallel);
        printUsage(argv[0]);
	return 1;
    }
    printf("parameters: parallel = %d, chunksize = %d, compress secs = %d, decompress secs = %d, input file = %s\n", g_parallel, g_chunksize, csec, dsec, infile);
    g_filesize = getFileSize(infile);
    FILE* in = fopen(infile, "rb");
    if (!in) {
        printf("open file %s error!\n", infile);
	return 1;
    }
    g_inbuf = (char*)malloc(g_filesize);
    size_t readsize = fread(g_inbuf, 1, g_filesize, in);
    fclose(in);
    if (readsize != g_filesize) {
        printf("expected read size=%lld but actually %lld!\n", g_filesize, readsize);
	free(g_inbuf);
	return 1;
    }
    reset_g_comp_params();
    pthread_t tmp;
    for (int i = 0;i < g_parallel;i++) {
        if (pthread_create(&tmp, NULL, comp_func, g_comp_params + i)) {
	    g_cfinish = 1;
	    g_cstart = 1;
	    printf("comp thread %d create failed.\n", i);
	    return 1;
	}
    }
    while (!all_thread_ready()) {}
    g_cstart = 1;
    sleep(csec);
    g_cfinish = 1;
    size_t comp_sum_handle = 0;
    for (int i = 0;i < g_parallel;i++) {
        comp_sum_handle += g_comp_params[i].bytesHandle;
    }
    while (!all_thread_finish()) {}
    size_t compbuf_size = g_filesize + g_filesize / 10;
    g_compbuf = (char*)malloc(compbuf_size);
    size_t uncompPos = 0, compPos = 0;
    g_chunk_num = ((g_filesize - 1) / g_chunksize) + 1;
    g_compressed_chunk_offset = (size_t*)malloc(g_chunk_num * sizeof(size_t*));
    g_compressed_chunk_size = (size_t*)malloc(g_chunk_num * sizeof(size_t));
    while (uncompPos < g_filesize) {
        size_t compsize = g_filesize - uncompPos;
        if (compsize > g_chunksize) compsize = g_chunksize;
        int compdatasize = zlib_comp(g_inbuf + uncompPos, compsize, g_compbuf + compPos, compsize + compsize / 10);
        if (compdatasize < 0) {
	    printf("compress data error!\n");
            return 1;
	}
	g_compressed_chunk_offset[uncompPos / g_chunksize] = compPos;
	g_compressed_chunk_size[uncompPos / g_chunksize] = compdatasize;
        uncompPos += compsize;
	compPos += compdatasize;
    }
    g_comp_data_size = compPos;
    reset_g_comp_params();
    for (int i = 0;i < g_parallel;i++) {
        if (pthread_create(&tmp, NULL, decomp_func, g_comp_params + i)) {
            g_dfinish = 1;
            g_dstart = 1;
            printf("decomp thread %d create failed.\n", i);
            return 1;
        }
    }
    while (!all_thread_ready()) {}
    free(g_compbuf);
    g_dstart = 1;
    sleep(dsec);
    g_dfinish = 1;
    size_t decomp_sum_handle = 0;
    for (int i = 0;i < g_parallel;i++) {
        decomp_sum_handle += g_comp_params[i].bytesHandle;
    }
    printf("comp speed\tdecomp speed\toriginal size\tcompressed size\tratio\n", decomp_sum_handle);
    printf("%.2f MB/s\t%.2f MB/s\t%lld\t%lld\t%.2f\n", comp_sum_handle / 1024.0 / 1024.0 / csec, decomp_sum_handle / 1024.0 / 1024.0 / dsec, 
        g_filesize, g_comp_data_size, g_comp_data_size * 100.0 / g_filesize);
    while (!all_thread_finish()) {}
    free(g_inbuf);
    return 0;
}