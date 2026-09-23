#ifndef DFLTCC_H
#define DFLTCC_H

#include "../../deflate.h"

#include <stdint.h>

/*
   Sizes of deflate block parts.
 */
#define DFLTCC_BLOCK_HEADER_BITS 3
#define DFLTCC_HLITS_COUNT_BITS 5
#define DFLTCC_HDISTS_COUNT_BITS 5
#define DFLTCC_HCLENS_COUNT_BITS 4
#define DFLTCC_MAX_HCLENS 19
#define DFLTCC_HCLEN_BITS 3
#define DFLTCC_MAX_HLITS 286
#define DFLTCC_MAX_HDISTS 30
#define DFLTCC_MAX_HLIT_HDIST_BITS 7
#define DFLTCC_MAX_SYMBOL_BITS 16
#define DFLTCC_MAX_EOBS_BITS 15
#define DFLTCC_MAX_PADDING_BITS 7

#define DEFLATE_BOUND_COMPLEN(source_len) \
    ((DFLTCC_BLOCK_HEADER_BITS + \
      DFLTCC_HLITS_COUNT_BITS + \
      DFLTCC_HDISTS_COUNT_BITS + \
      DFLTCC_HCLENS_COUNT_BITS + \
      DFLTCC_MAX_HCLENS * DFLTCC_HCLEN_BITS + \
      (DFLTCC_MAX_HLITS + DFLTCC_MAX_HDISTS) * DFLTCC_MAX_HLIT_HDIST_BITS + \
      (source_len) * DFLTCC_MAX_SYMBOL_BITS + \
      DFLTCC_MAX_EOBS_BITS + \
      DFLTCC_MAX_PADDING_BITS) >> 3)

voidpf ZLIB_INTERNAL dfltcc_alloc_state(z_streamp strm, uInt items, uInt size);
void ZLIB_INTERNAL dfltcc_copy_state(voidpf dst, const voidpf src, uInt size);
void ZLIB_INTERNAL dfltcc_reset(z_streamp strm, uInt size);
voidpf ZLIB_INTERNAL dfltcc_alloc_window(z_streamp strm, uInt items,
                                         uInt size);
void ZLIB_INTERNAL dfltcc_copy_window(void *dest, const void *src, size_t n);
void ZLIB_INTERNAL dfltcc_free_window(z_streamp strm, voidpf w);
#define DFLTCC_BLOCK_HEADER_BITS 3
#define DFLTCC_HLITS_COUNT_BITS 5
#define DFLTCC_HDISTS_COUNT_BITS 5
#define DFLTCC_HCLENS_COUNT_BITS 4
#define DFLTCC_MAX_HCLENS 19
#define DFLTCC_HCLEN_BITS 3
#define DFLTCC_MAX_HLITS 286
#define DFLTCC_MAX_HDISTS 30
#define DFLTCC_MAX_HLIT_HDIST_BITS 7
#define DFLTCC_MAX_SYMBOL_BITS 16
#define DFLTCC_MAX_EOBS_BITS 15
#define DFLTCC_MAX_PADDING_BITS 7

int ZLIB_INTERNAL dfltcc_can_inflate(z_streamp strm);
typedef enum {
    DFLTCC_INFLATE_CONTINUE,
    DFLTCC_INFLATE_BREAK,
    DFLTCC_INFLATE_SOFTWARE,
} dfltcc_inflate_action;
dfltcc_inflate_action ZLIB_INTERNAL dfltcc_inflate(z_streamp strm,
                                                   int flush, int *ret);
int ZLIB_INTERNAL dfltcc_was_inflate_used(z_streamp strm);
int ZLIB_INTERNAL dfltcc_inflate_disable(z_streamp strm);
int ZLIB_INTERNAL dfltcc_inflate_set_dictionary(z_streamp strm,
                                                const Bytef *dictionary,
                                                uInt dict_length);
int ZLIB_INTERNAL dfltcc_inflate_get_dictionary(z_streamp strm,
                                                Bytef *dictionary,
                                                uInt* dict_length);

int ZLIB_INTERNAL dfltcc_can_deflate(z_streamp strm);
int ZLIB_INTERNAL dfltcc_deflate(z_streamp strm,
                                 int flush,
                                 block_state *result);
int ZLIB_INTERNAL dfltcc_deflate_params(z_streamp strm, int level,
					int strategy, int *flush);
int ZLIB_INTERNAL dfltcc_deflate_done(z_streamp strm, int flush);
int ZLIB_INTERNAL dfltcc_deflate_set_dictionary(z_streamp strm,
                                                const Bytef *dictionary,
                                                uInt dict_length);
int ZLIB_INTERNAL dfltcc_deflate_get_dictionary(z_streamp strm,
                                                Bytef *dictionary,
                                                uInt* dict_length);

#endif
