/* dfltcc.c - SystemZ DEFLATE CONVERSION CALL support. */

/*
   Use the following commands to build zlib with DFLTCC support:

        $ ./configure --dfltcc
        $ make
*/

#define _GNU_SOURCE
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "../../zutil.h"
#include "../../deflate.h"
#include "../../inftrees.h"
#include "../../inflate.h"
#include "dfltcc.h"
#include "dfltcc_deflate.h"
#ifdef HAVE_SYS_SDT_H
#include <sys/sdt.h>
#endif

/*
   C wrapper for the DEFLATE CONVERSION CALL instruction.
 */
typedef enum {
    DFLTCC_CC_OK = 0,
    DFLTCC_CC_OP1_TOO_SHORT = 1,
    DFLTCC_CC_OP2_TOO_SHORT = 2,
    DFLTCC_CC_OP2_CORRUPT = 2,
    DFLTCC_CC_AGAIN = 3,
} dfltcc_cc;

#define DFLTCC_QAF 0
#define DFLTCC_GDHT 1
#define DFLTCC_CMPR 2
#define DFLTCC_XPND 4
#define HBT_CIRCULAR (1 << 7)
#define HB_BITS 15
#define HB_SIZE (1 << HB_BITS)
#define DFLTCC_FACILITY 151

local inline dfltcc_cc dfltcc OF((int fn, void *param,
                                  Bytef **op1, size_t *len1,
                                  z_const Bytef **op2, size_t *len2,
                                  void *hist));
local inline dfltcc_cc dfltcc(fn, param, op1, len1, op2, len2, hist)
    int fn;
    void *param;
    Bytef **op1;
    size_t *len1;
    z_const Bytef **op2;
    size_t *len2;
    void *hist;
{
    Bytef *t2 = op1 ? *op1 : NULL;
    size_t t3 = len1 ? *len1 : 0;
    z_const Bytef *t4 = op2 ? *op2 : NULL;
    size_t t5 = len2 ? *len2 : 0;
    register int r0 __asm__("r0") = fn;
    register void *r1 __asm__("r1") = param;
    register Bytef *r2 __asm__("r2") = t2;
    register size_t r3 __asm__("r3") = t3;
    register z_const Bytef *r4 __asm__("r4") = t4;
    register size_t r5 __asm__("r5") = t5;
    int cc;

    __asm__ volatile(
#ifdef HAVE_SYS_SDT_H
                     STAP_PROBE_ASM(zlib, dfltcc_entry,
                                    STAP_PROBE_ASM_TEMPLATE(5))
#endif
                     ".insn rrf,0xb9390000,%[r2],%[r4],%[hist],0\n"
#ifdef HAVE_SYS_SDT_H
                     STAP_PROBE_ASM(zlib, dfltcc_exit,
                                    STAP_PROBE_ASM_TEMPLATE(5))
#endif
                     "ipm %[cc]\n"
                     : [r2] "+r" (r2)
                     , [r3] "+r" (r3)
                     , [r4] "+r" (r4)
                     , [r5] "+r" (r5)
                     , [cc] "=r" (cc)
                     : [r0] "r" (r0)
                     , [r1] "r" (r1)
                     , [hist] "r" (hist)
#ifdef HAVE_SYS_SDT_H
                     , STAP_PROBE_ASM_OPERANDS(5, r2, r3, r4, r5, hist)
#endif
                     : "cc", "memory");
    t2 = r2; t3 = r3; t4 = r4; t5 = r5;

    if (op1)
        *op1 = t2;
    if (len1)
        *len1 = t3;
    if (op2)
        *op2 = t4;
    if (len2)
        *len2 = t5;
    return (cc >> 28) & 3;
}

/*
   Parameter Block for Query Available Functions.
 */
#define static_assert(c, msg) \
        __attribute__((unused)) \
        static char static_assert_failed_ ## msg[c ? 1 : -1]

struct dfltcc_qaf_param {
    char fns[16];
    char reserved1[8];
    char fmts[2];
    char reserved2[6];
};

static_assert(sizeof(struct dfltcc_qaf_param) == 32,
              sizeof_struct_dfltcc_qaf_param_is_32);

local inline int is_bit_set OF((const char *bits, int n));
local inline int is_bit_set(bits, n)
    const char *bits;
    int n;
{
    return bits[n / 8] & (1 << (7 - (n % 8)));
}

local inline void clear_bit OF((char *bits, int n));
local inline void clear_bit(bits, n)
    char *bits;
    int n;
{
    bits[n / 8] &= ~(1 << (7 - (n % 8)));
}

#define DFLTCC_FMT0 0

/*
   Parameter Block for Generate Dynamic-Huffman Table, Compress and Expand.
 */
#define CVT_CRC32 0
#define CVT_ADLER32 1
#define HTT_FIXED 0
#define HTT_DYNAMIC 1

struct dfltcc_param_v0 {
    uint16_t pbvn;                     /* Parameter-Block-Version Number */
    uint8_t mvn;                       /* Model-Version Number */
    uint8_t ribm;                      /* Reserved for IBM use */
    unsigned reserved32 : 31;
    unsigned cf : 1;                   /* Continuation Flag */
    uint8_t reserved64[8];
    unsigned nt : 1;                   /* New Task */
    unsigned reserved129 : 1;
    unsigned cvt : 1;                  /* Check Value Type */
    unsigned reserved131 : 1;
    unsigned htt : 1;                  /* Huffman-Table Type */
    unsigned bcf : 1;                  /* Block-Continuation Flag */
    unsigned bcc : 1;                  /* Block Closing Control */
    unsigned bhf : 1;                  /* Block Header Final */
    unsigned reserved136 : 1;
    unsigned reserved137 : 1;
    unsigned dhtgc : 1;                /* DHT Generation Control */
    unsigned reserved139 : 5;
    unsigned reserved144 : 5;
    unsigned sbb : 3;                  /* Sub-Byte Boundary */
    uint8_t oesc;                      /* Operation-Ending-Supplemental Code */
    unsigned reserved160 : 12;
    unsigned ifs : 4;                  /* Incomplete-Function Status */
    uint16_t ifl;                      /* Incomplete-Function Length */
    uint8_t reserved192[8];
    uint8_t reserved256[8];
    uint8_t reserved320[4];
    uint16_t hl;                       /* History Length */
    unsigned reserved368 : 1;
    uint16_t ho : 15;                  /* History Offset */
    uint32_t cv;                       /* Check Value */
    unsigned eobs : 15;                /* End-of-block Symbol */
    unsigned reserved431: 1;
    uint8_t eobl : 4;                  /* End-of-block Length */
    unsigned reserved436 : 12;
    unsigned reserved448 : 4;
    uint16_t cdhtl : 12;               /* Compressed-Dynamic-Huffman Table
                                          Length */
    uint8_t reserved464[6];
    uint8_t cdht[288];
    uint8_t reserved[32];
    uint8_t csb[1152];
};

static_assert(sizeof(struct dfltcc_param_v0) == 1536,
              sizeof_struct_dfltcc_param_v0_is_1536);

local z_const char *oesc_msg OF((char *buf, int oesc));
local z_const char *oesc_msg(buf, oesc)
    char *buf;
    int oesc;
{
    if (oesc == 0x00)
        return NULL; /* Successful completion */
    else {
        sprintf(buf, "Operation-Ending-Supplemental Code is 0x%.2X", oesc);
        return buf;
    }
}

/*
   Extension of inflate_state and deflate_state. Must be doubleword-aligned.
*/
struct dfltcc_state {
    struct dfltcc_param_v0 param;      /* Parameter block. */
    struct dfltcc_qaf_param af;        /* Available functions. */
    uLong level_mask;                  /* Levels on which to use DFLTCC */
    uLong block_size;                  /* New block each X bytes */
    uLong block_threshold;             /* New block after total_in > X */
    uLong dht_threshold;               /* New block only if avail_in >= X */
    char msg[64];                      /* Buffer for strm->msg */
};

#define ALIGN_UP(p, size) \
        (__typeof__(p))(((uintptr_t)(p) + ((size) - 1)) & ~((size) - 1))

#define GET_DFLTCC_STATE(state) ((struct dfltcc_state FAR *)( \
        (char FAR *)(state) + ALIGN_UP(sizeof(*state), 8)))

/*
   Compress.
 */
local inline int dfltcc_can_deflate_with_params(z_streamp strm,
                                                int level,
                                                uInt window_bits,
                                                int strategy);
local inline int dfltcc_can_deflate_with_params(strm,
                                                level,
                                                window_bits,
                                                strategy)
    z_streamp strm;
    int level;
    uInt window_bits;
    int strategy;
{
    deflate_state FAR *state = (deflate_state FAR *)strm->state;
    struct dfltcc_state FAR *dfltcc_state = GET_DFLTCC_STATE(state);

    /* Unsupported compression settings */
    if ((dfltcc_state->level_mask & (1 << level)) == 0)
        return 0;
    if (window_bits != HB_BITS)
        return 0;
    if (strategy != Z_FIXED && strategy != Z_DEFAULT_STRATEGY)
        return 0;

    /* Unsupported hardware */
    if (!is_bit_set(dfltcc_state->af.fns, DFLTCC_GDHT) ||
            !is_bit_set(dfltcc_state->af.fns, DFLTCC_CMPR) ||
            !is_bit_set(dfltcc_state->af.fmts, DFLTCC_FMT0))
        return 0;

    return 1;
}

int ZLIB_INTERNAL dfltcc_can_deflate(strm)
    z_streamp strm;
{
    deflate_state FAR *state = (deflate_state FAR *)strm->state;

    return dfltcc_can_deflate_with_params(strm,
                                          state->level,
                                          state->w_bits,
                                          state->strategy);
}

local void dfltcc_gdht OF((z_streamp strm));
local void dfltcc_gdht(strm)
    z_streamp strm;
{
    deflate_state FAR *state = (deflate_state FAR *)strm->state;
    struct dfltcc_param_v0 FAR *param = &GET_DFLTCC_STATE(state)->param;
    size_t avail_in = avail_in = strm->avail_in;

    dfltcc(DFLTCC_GDHT,
           param, NULL, NULL,
           &strm->next_in, &avail_in, NULL);
}

local dfltcc_cc dfltcc_cmpr OF((z_streamp strm));
local dfltcc_cc dfltcc_cmpr(strm)
    z_streamp strm;
{
    deflate_state FAR *state = (deflate_state FAR *)strm->state;
    struct dfltcc_param_v0 FAR *param = &GET_DFLTCC_STATE(state)->param;
    size_t avail_in = strm->avail_in;
    size_t avail_out = strm->avail_out;
    dfltcc_cc cc;

    cc = dfltcc(DFLTCC_CMPR | HBT_CIRCULAR,
                param, &strm->next_out, &avail_out,
                &strm->next_in, &avail_in, state->window);
    strm->total_in += (strm->avail_in - avail_in);
    strm->total_out += (strm->avail_out - avail_out);
    strm->avail_in = avail_in;
    strm->avail_out = avail_out;
    return cc;
}

local void send_eobs OF((z_streamp strm,
                         z_const struct dfltcc_param_v0 FAR *param));
local void send_eobs(strm, param)
    z_streamp strm;
    z_const struct dfltcc_param_v0 FAR *param;
{
    deflate_state FAR *state = (deflate_state FAR *)strm->state;

    _tr_send_bits(
          state,
          bi_reverse(param->eobs >> (15 - param->eobl), param->eobl),
          param->eobl);
    flush_pending(strm);
    if (state->pending != 0) {
        /* The remaining data is located in pending_out[0:pending]. If someone
         * calls put_byte() - this might happen in deflate() - the byte will be
         * placed into pending_buf[pending], which is incorrect. Move the
         * remaining data to the beginning of pending_buf so that put_byte() is
         * usable again.
         */
        memmove(state->pending_buf, state->pending_out, state->pending);
        state->pending_out = state->pending_buf;
    }
#ifdef ZLIB_DEBUG
    state->compressed_len += param->eobl;
#endif
}

int ZLIB_INTERNAL dfltcc_deflate(strm, flush, result)
    z_streamp strm;
    int flush;
    block_state *result;
{
    deflate_state FAR *state = (deflate_state FAR *)strm->state;
    struct dfltcc_state FAR *dfltcc_state = GET_DFLTCC_STATE(state);
    struct dfltcc_param_v0 FAR *param = &dfltcc_state->param;
    uInt masked_avail_in;
    dfltcc_cc cc;
    int need_empty_block;
    int soft_bcc;
    int no_flush;

    if (!dfltcc_can_deflate(strm)) {
        /* Clear history. */
        if (flush == Z_FULL_FLUSH)
            param->hl = 0;
        return 0;
    }

again:
    masked_avail_in = 0;
    soft_bcc = 0;
    no_flush = flush == Z_NO_FLUSH;

    /* No input data. Return, except when Continuation Flag is set, which means
     * that DFLTCC has buffered some output in the parameter block and needs to
     * be called again in order to flush it.
     */
    if (strm->avail_in == 0 && !param->cf) {
        /* A block is still open, and the hardware does not support closing
         * blocks without adding data. Thus, close it manually.
         */
        if (!no_flush && param->bcf) {
            send_eobs(strm, param);
            param->bcf = 0;
        }
        /* Let one of deflate_* functions write a trailing empty block. */
        if (flush == Z_FINISH)
            return 0;
        /* Clear history. */
        if (flush == Z_FULL_FLUSH)
            param->hl = 0;
        /* Trigger block post-processing if necessary. */
        *result = no_flush ? need_more : block_done;
        return 1;
    }

    /* There is an open non-BFINAL block, we are not going to close it just
     * yet, we have compressed more than DFLTCC_BLOCK_SIZE bytes and we see
     * more than DFLTCC_DHT_MIN_SAMPLE_SIZE bytes. Open a new block with a new
     * DHT in order to adapt to a possibly changed input data distribution.
     */
    if (param->bcf && no_flush &&
            strm->total_in > dfltcc_state->block_threshold &&
            strm->avail_in >= dfltcc_state->dht_threshold) {
        if (param->cf) {
            /* We need to flush the DFLTCC buffer before writing the
             * End-of-block Symbol. Mask the input data and proceed as usual.
             */
            masked_avail_in += strm->avail_in;
            strm->avail_in = 0;
            no_flush = 0;
        } else {
            /* DFLTCC buffer is empty, so we can manually write the
             * End-of-block Symbol right away.
             */
            send_eobs(strm, param);
            param->bcf = 0;
            dfltcc_state->block_threshold =
                strm->total_in + dfltcc_state->block_size;
        }
    }

    /* No space for compressed data. If we proceed, dfltcc_cmpr() will return
     * DFLTCC_CC_OP1_TOO_SHORT without buffering header bits, but we will still
     * set BCF=1, which is wrong. Avoid complications and return early.
     */
    if (strm->avail_out == 0) {
        *result = need_more;
        return 1;
    }

    /* The caller gave us too much data. Pass only one block worth of
     * uncompressed data to DFLTCC and mask the rest, so that on the next
     * iteration we start a new block.
     */
    if (no_flush && strm->avail_in > dfltcc_state->block_size) {
        masked_avail_in += (strm->avail_in - dfltcc_state->block_size);
        strm->avail_in = dfltcc_state->block_size;
    }

    /* When we have an open non-BFINAL deflate block and caller indicates that
     * the stream is ending, we need to close an open deflate block and open a
     * BFINAL one.
     */
    need_empty_block = flush == Z_FINISH && param->bcf && !param->bhf;

    /* Translate stream to parameter block */
    param->cvt = state->wrap == 2 ? CVT_CRC32 : CVT_ADLER32;
    if (!no_flush)
        /* We need to close a block. Always do this in software - when there is
         * no input data, the hardware will not honor BCC. */
        soft_bcc = 1;
    if (flush == Z_FINISH && !param->bcf)
        /* We are about to open a BFINAL block, set Block Header Final bit
         * until the stream ends.
         */
        param->bhf = 1;
    /* DFLTCC-CMPR will write to next_out, so make sure that buffers with
     * higher precedence are empty.
     */
    Assert(state->pending == 0, "There must be no pending bytes");
    Assert(state->bi_valid < 8, "There must be less than 8 pending bits");
    param->sbb = (unsigned int)state->bi_valid;
    if (param->sbb > 0)
        *strm->next_out = (Bytef)state->bi_buf;
    /* Honor history and check value */
    param->nt = 0;
    param->cv = state->wrap == 2 ? ZSWAP32(strm->adler) : strm->adler;

    /* When opening a block, choose a Huffman-Table Type */
    if (!param->bcf) {
        if (state->strategy == Z_FIXED ||
                (strm->total_in == 0 && dfltcc_state->block_threshold > 0))
            param->htt = HTT_FIXED;
        else {
            param->htt = HTT_DYNAMIC;
            dfltcc_gdht(strm);
        }
    }

    /* Deflate */
    do {
        cc = dfltcc_cmpr(strm);
        if (strm->avail_in < 4096 && masked_avail_in > 0)
            /* We are about to call DFLTCC with a small input buffer, which is
             * inefficient. Since there is masked data, there will be at least
             * one more DFLTCC call, so skip the current one and make the next
             * one handle more data.
             */
            break;
    } while (cc == DFLTCC_CC_AGAIN);

    /* Translate parameter block to stream */
    strm->msg = oesc_msg(dfltcc_state->msg, param->oesc);
    state->bi_valid = param->sbb;
    if (state->bi_valid == 0)
        state->bi_buf = 0; /* Avoid accessing next_out */
    else
        state->bi_buf = *strm->next_out & ((1 << state->bi_valid) - 1);
    strm->adler = state->wrap == 2 ? ZSWAP32(param->cv) : param->cv;

    /* Unmask the input data */
    strm->avail_in += masked_avail_in;
    masked_avail_in = 0;

    /* If we encounter an error, it means there is a bug in DFLTCC call */
    Assert(cc != DFLTCC_CC_OP2_CORRUPT || param->oesc == 0, "BUG");

    /* Update Block-Continuation Flag. It will be used to check whether to call
     * GDHT the next time.
     */
    if (cc == DFLTCC_CC_OK) {
        if (soft_bcc) {
            send_eobs(strm, param);
            param->bcf = 0;
            dfltcc_state->block_threshold =
                strm->total_in + dfltcc_state->block_size;
        } else
            param->bcf = 1;
        if (flush == Z_FINISH) {
            if (need_empty_block)
                /* Make the current deflate() call also close the stream */
                return 0;
            else {
                bi_windup(state);
                *result = finish_done;
            }
        } else {
            if (flush == Z_FULL_FLUSH)
                param->hl = 0; /* Clear history */
            *result = flush == Z_NO_FLUSH ? need_more : block_done;
        }
    } else {
        param->bcf = 1;
        *result = need_more;
    }
    if (strm->avail_in != 0 && strm->avail_out != 0)
        goto again; /* deflate() must use all input or all output */
    return 1;
}

/*
   Expand.
 */
int ZLIB_INTERNAL dfltcc_can_inflate(strm)
    z_streamp strm;
{
    struct inflate_state FAR *state = (struct inflate_state FAR *)strm->state;
    struct dfltcc_state FAR *dfltcc_state = GET_DFLTCC_STATE(state);

    /* Unsupported compression settings */
    if (state->wbits != HB_BITS)
        return 0;

    /* Unsupported hardware */
    return is_bit_set(dfltcc_state->af.fns, DFLTCC_XPND) &&
               is_bit_set(dfltcc_state->af.fmts, DFLTCC_FMT0);
}

local dfltcc_cc dfltcc_xpnd OF((z_streamp strm));
local dfltcc_cc dfltcc_xpnd(strm)
    z_streamp strm;
{
    struct inflate_state FAR *state = (struct inflate_state FAR *)strm->state;
    struct dfltcc_param_v0 FAR *param = &GET_DFLTCC_STATE(state)->param;
    size_t avail_in = strm->avail_in;
    size_t avail_out = strm->avail_out;
    dfltcc_cc cc;

    cc = dfltcc(DFLTCC_XPND | HBT_CIRCULAR,
                param, &strm->next_out, &avail_out,
                &strm->next_in, &avail_in, state->window);
    strm->avail_in = avail_in;
    strm->avail_out = avail_out;
    return cc;
}

dfltcc_inflate_action ZLIB_INTERNAL dfltcc_inflate(strm, flush, ret)
    z_streamp strm;
    int flush;
    int *ret;
{
    struct inflate_state FAR *state = (struct inflate_state FAR *)strm->state;
    struct dfltcc_state FAR *dfltcc_state = GET_DFLTCC_STATE(state);
    struct dfltcc_param_v0 FAR *param = &dfltcc_state->param;
    dfltcc_cc cc;

    if (flush == Z_BLOCK || flush == Z_TREES) {
        /* DFLTCC does not support stopping on block boundaries */
        if (dfltcc_inflate_disable(strm)) {
            *ret = Z_STREAM_ERROR;
            return DFLTCC_INFLATE_BREAK;
        } else
            return DFLTCC_INFLATE_SOFTWARE;
    }

    if (state->last) {
        if (state->bits != 0) {
            strm->next_in++;
            strm->avail_in--;
            state->bits = 0;
        }
        state->mode = CHECK;
        return DFLTCC_INFLATE_CONTINUE;
    }

    if (strm->avail_in == 0 && !param->cf)
        return DFLTCC_INFLATE_BREAK;

    if (inflate_ensure_window(state)) {
        state->mode = MEM;
        return DFLTCC_INFLATE_CONTINUE;
    }

    /* Translate stream to parameter block */
    param->cvt = state->flags ? CVT_CRC32 : CVT_ADLER32;
    param->sbb = state->bits;
    param->hl = state->whave; /* Software and hardware history formats match */
    param->ho = (state->wnext - state->whave) & ((1 << HB_BITS) - 1);
    if (param->hl)
        param->nt = 0; /* Honor history for the first block */
    param->cv = state->flags ? ZSWAP32(state->check) : state->check;

    /* Inflate */
    do {
        cc = dfltcc_xpnd(strm);
    } while (cc == DFLTCC_CC_AGAIN);

    /* Translate parameter block to stream */
    strm->msg = oesc_msg(dfltcc_state->msg, param->oesc);
    state->last = cc == DFLTCC_CC_OK;
    state->bits = param->sbb;
    state->whave = param->hl;
    state->wnext = (param->ho + param->hl) & ((1 << HB_BITS) - 1);
    state->check = state->flags ? ZSWAP32(param->cv) : param->cv;
    if (cc == DFLTCC_CC_OP2_CORRUPT && param->oesc != 0) {
        /* Report an error if stream is corrupted */
        state->mode = BAD;
        return DFLTCC_INFLATE_CONTINUE;
    }
    state->mode = TYPEDO;
    /* Break if operands are exhausted, otherwise continue looping */
    return (cc == DFLTCC_CC_OP1_TOO_SHORT || cc == DFLTCC_CC_OP2_TOO_SHORT) ?
        DFLTCC_INFLATE_BREAK : DFLTCC_INFLATE_CONTINUE;
}

int ZLIB_INTERNAL dfltcc_was_inflate_used(strm)
    z_streamp strm;
{
    struct inflate_state FAR *state = (struct inflate_state FAR *)strm->state;
    struct dfltcc_param_v0 FAR *param = &GET_DFLTCC_STATE(state)->param;

    return !param->nt;
}

int ZLIB_INTERNAL dfltcc_inflate_disable(strm)
    z_streamp strm;
{
    struct inflate_state FAR *state = (struct inflate_state FAR *)strm->state;
    struct dfltcc_state FAR *dfltcc_state = GET_DFLTCC_STATE(state);

    if (!dfltcc_can_inflate(strm))
        return 0;
    if (dfltcc_was_inflate_used(strm))
        /* DFLTCC has already decompressed some data. Since there is not
         * enough information to resume decompression in software, the call
         * must fail.
         */
        return 1;
    /* DFLTCC was not used yet - decompress in software */
    memset(&dfltcc_state->af, 0, sizeof(dfltcc_state->af));
    return 0;
}

local int env_dfltcc_disabled;
local int env_source_date_epoch;
local unsigned long env_level_mask;
local unsigned long env_block_size;
local unsigned long env_block_threshold;
local unsigned long env_dht_threshold;
local unsigned long env_ribm;
local uint64_t cpu_facilities[(DFLTCC_FACILITY / 64) + 1];
local struct dfltcc_qaf_param cpu_af __attribute__((aligned(8)));

local inline int is_dfltcc_enabled OF((void));
local inline int is_dfltcc_enabled(void)
{
    if (env_dfltcc_disabled)
      /* User has explicitly disabled DFLTCC. */
      return 0;

    return is_bit_set((const char *)cpu_facilities, DFLTCC_FACILITY);
}

local unsigned long xstrtoul OF((const char *s, unsigned long _default));
local unsigned long xstrtoul(s, _default)
    const char *s;
    unsigned long _default;
{
    char *endptr;
    unsigned long result;

    if (!(s && *s))
        return _default;
    errno = 0;
    result = strtoul(s, &endptr, 0);
    return (errno || *endptr) ? _default : result;
}

__attribute__((constructor)) local void init_globals OF((void));
__attribute__((constructor)) local void init_globals(void)
{
    const char *endptr;
    const char *env;
    register char r0 __asm__("r0");

    env = secure_getenv("DFLTCC");
    env_dfltcc_disabled = env && !strcmp(env, "0");

    env = secure_getenv("SOURCE_DATE_EPOCH");
    env_source_date_epoch = !!env;

#ifndef DFLTCC_LEVEL_MASK
#define DFLTCC_LEVEL_MASK 0x2
#endif
    env_level_mask = xstrtoul(secure_getenv("DFLTCC_LEVEL_MASK"),
                              DFLTCC_LEVEL_MASK);

#ifndef DFLTCC_BLOCK_SIZE
#define DFLTCC_BLOCK_SIZE 1048576
#endif
    env_block_size = xstrtoul(secure_getenv("DFLTCC_BLOCK_SIZE"),
                              DFLTCC_BLOCK_SIZE);

#ifndef DFLTCC_FIRST_FHT_BLOCK_SIZE
#define DFLTCC_FIRST_FHT_BLOCK_SIZE 4096
#endif
    env_block_threshold = xstrtoul(secure_getenv("DFLTCC_FIRST_FHT_BLOCK_SIZE"),
                                   DFLTCC_FIRST_FHT_BLOCK_SIZE);

#ifndef DFLTCC_DHT_MIN_SAMPLE_SIZE
#define DFLTCC_DHT_MIN_SAMPLE_SIZE 4096
#endif
    env_dht_threshold = xstrtoul(secure_getenv("DFLTCC_DHT_MIN_SAMPLE_SIZE"),
                                 DFLTCC_DHT_MIN_SAMPLE_SIZE);

#ifndef DFLTCC_RIBM
#define DFLTCC_RIBM 0
#endif
    env_ribm = xstrtoul(secure_getenv("DFLTCC_RIBM"), DFLTCC_RIBM);

    memset(cpu_facilities, 0, sizeof(cpu_facilities));
    r0 = sizeof(cpu_facilities) / sizeof(cpu_facilities[0]) - 1;
    /* STFLE is supported since z9-109 and only in z/Architecture mode. When
     * compiling with -m31, gcc defaults to ESA mode, however, since the kernel
     * is 64-bit, it's always z/Architecture mode at runtime.
     */
    __asm__ volatile(
#ifndef __clang__
                     ".machinemode push\n"
                     ".machinemode zarch\n"
#endif
                     "stfle %[facilities]\n"
#ifndef __clang__
                     ".machinemode pop\n"
#endif
                     : [facilities] "=Q" (cpu_facilities)
                     , [r0] "+r" (r0)
                     :
                     : "cc");

    /* Initialize available functions */
    if (is_dfltcc_enabled())
        dfltcc(DFLTCC_QAF, &cpu_af, NULL, NULL, NULL, NULL, NULL);
    else
        memset(&cpu_af, 0, sizeof(cpu_af));
}

/*
   Memory management.

   DFLTCC requires parameter blocks and window to be aligned. zlib allows
   users to specify their own allocation functions, so using e.g.
   `posix_memalign' is not an option. Thus, we overallocate and take the
   aligned portion of the buffer.
*/
void ZLIB_INTERNAL dfltcc_reset(strm, size)
    z_streamp strm;
    uInt size;
{
    struct dfltcc_state *dfltcc_state =
        (struct dfltcc_state *)((char FAR *)strm->state + ALIGN_UP(size, 8));

    memcpy(&dfltcc_state->af, &cpu_af, sizeof(dfltcc_state->af));

    if (env_source_date_epoch)
        /* User needs reproducible results, but the output of DFLTCC_CMPR
         * depends on buffers' page offsets.
         */
        clear_bit(dfltcc_state->af.fns, DFLTCC_CMPR);

    /* Initialize parameter block */
    memset(&dfltcc_state->param, 0, sizeof(dfltcc_state->param));
    dfltcc_state->param.nt = 1;

    /* Initialize tuning parameters */
    dfltcc_state->level_mask = env_level_mask;
    dfltcc_state->block_size = env_block_size;
    dfltcc_state->block_threshold = env_block_threshold;
    dfltcc_state->dht_threshold = env_dht_threshold;
    dfltcc_state->param.ribm = env_ribm;
}

voidpf ZLIB_INTERNAL dfltcc_alloc_state(strm, items, size)
    z_streamp strm;
    uInt items;
    uInt size;
{
    return ZALLOC(strm,
                  ALIGN_UP(items * size, 8) + sizeof(struct dfltcc_state),
                  sizeof(unsigned char));
}

void ZLIB_INTERNAL dfltcc_copy_state(dst, src, size)
    voidpf dst;
    const voidpf src;
    uInt size;
{
    zmemcpy(dst, src, ALIGN_UP(size, 8) + sizeof(struct dfltcc_state));
}

static const int PAGE_ALIGN = 0x1000;

voidpf ZLIB_INTERNAL dfltcc_alloc_window(strm, items, size)
    z_streamp strm;
    uInt items;
    uInt size;
{
    voidpf p, w;

    /* To simplify freeing, we store the pointer to the allocated buffer right
     * before the window.
     */
    p = ZALLOC(strm, sizeof(voidpf) + items * size + PAGE_ALIGN,
               sizeof(unsigned char));
    if (p == NULL)
        return NULL;
    w = ALIGN_UP((char FAR *)p + sizeof(voidpf), PAGE_ALIGN);
    *(voidpf *)((char FAR *)w - sizeof(voidpf)) = p;
    return w;
}

void ZLIB_INTERNAL dfltcc_free_window(strm, w)
    z_streamp strm;
    voidpf w;
{
    if (w)
        ZFREE(strm, *(voidpf *)((unsigned char FAR *)w - sizeof(voidpf)));
}

/*
   Switching between hardware and software compression.

   DFLTCC does not support all zlib settings, e.g. generation of non-compressed
   blocks or alternative window sizes. When such settings are applied on the
   fly with deflateParams, we need to convert between hardware and software
   window formats.
*/
int ZLIB_INTERNAL dfltcc_deflate_params(strm, level, strategy, flush)
    z_streamp strm;
    int level;
    int strategy;
    int *flush;
{
    deflate_state FAR *state = (deflate_state FAR *)strm->state;
    struct dfltcc_state FAR *dfltcc_state = GET_DFLTCC_STATE(state);
    struct dfltcc_param_v0 FAR *param = &dfltcc_state->param;
    int could_deflate = dfltcc_can_deflate(strm);
    int can_deflate = dfltcc_can_deflate_with_params(strm,
                                                     level,
                                                     state->w_bits,
                                                     strategy);

    if (can_deflate == could_deflate)
        /* We continue to work in the same mode - no changes needed */
        return Z_OK;

    if (strm->total_in == 0 && param->nt == 1 && param->hl == 0)
        /* DFLTCC was not used yet - no changes needed */
        return Z_OK;

    /* For now, do not convert between window formats - simply get rid of the
     * old data instead.
     */
    *flush = Z_FULL_FLUSH;
    return Z_OK;
}

int ZLIB_INTERNAL dfltcc_deflate_done(strm, flush)
    z_streamp strm;
    int flush;
{
    deflate_state FAR *state = (deflate_state FAR *)strm->state;
    struct dfltcc_state FAR *dfltcc_state = GET_DFLTCC_STATE(state);
    struct dfltcc_param_v0 FAR *param = &dfltcc_state->param;

    /* When deflate(Z_FULL_FLUSH) is called with small avail_out, it might
     * close the block without resetting the compression state. Detect this
     * situation and return that deflation is not done.
     */
    if (flush == Z_FULL_FLUSH && strm->avail_out == 0)
        return 0;

    /* Return that deflation is not done if DFLTCC is used and either it
     * buffered some data (Continuation Flag is set), or has not written EOBS
     * yet (Block-Continuation Flag is set).
     */
    return !dfltcc_can_deflate(strm) || (!param->cf && !param->bcf);
}

/*
   Preloading history.
*/
local void append_history OF((struct dfltcc_param_v0 FAR *param,
                              Bytef *history,
                              const Bytef *buf,
                              uInt count));
local void append_history(param, history, buf, count)
    struct dfltcc_param_v0 FAR *param;
    Bytef *history;
    const Bytef *buf;
    uInt count;
{
    size_t offset;
    size_t n;

    /* Do not use more than 32K */
    if (count > HB_SIZE) {
        buf += count - HB_SIZE;
        count = HB_SIZE;
    }
    offset = (param->ho + param->hl) % HB_SIZE;
    if (offset + count <= HB_SIZE)
        /* Circular history buffer does not wrap - copy one chunk */
        zmemcpy(history + offset, buf, count);
    else {
        /* Circular history buffer wraps - copy two chunks */
        n = HB_SIZE - offset;
        zmemcpy(history + offset, buf, n);
        zmemcpy(history, buf + n, count - n);
    }
    n = param->hl + count;
    if (n <= HB_SIZE)
        /* All history fits into buffer - no need to discard anything */
        param->hl = n;
    else {
        /* History does not fit into buffer - discard extra bytes */
        param->ho = (param->ho + (n - HB_SIZE)) % HB_SIZE;
        param->hl = HB_SIZE;
    }
}

int ZLIB_INTERNAL dfltcc_deflate_set_dictionary(strm, dictionary, dict_length)
    z_streamp strm;
    const Bytef *dictionary;
    uInt dict_length;
{
    deflate_state FAR *state = (deflate_state FAR *)strm->state;
    struct dfltcc_state FAR *dfltcc_state = GET_DFLTCC_STATE(state);
    struct dfltcc_param_v0 FAR *param = &dfltcc_state->param;

    append_history(param, state->window, dictionary, dict_length);
    state->strstart = 1; /* Add FDICT to zlib header */
    state->block_start = state->strstart; /* Make deflate_stored happy */
    return Z_OK;
}

int ZLIB_INTERNAL dfltcc_deflate_get_dictionary(strm, dictionary, dict_length)
    z_streamp strm;
    Bytef *dictionary;
    uInt *dict_length;
{
    deflate_state FAR *state = (deflate_state FAR *)strm->state;
    struct dfltcc_state FAR *dfltcc_state = GET_DFLTCC_STATE(state);
    struct dfltcc_param_v0 FAR *param = &dfltcc_state->param;

    if (dictionary) {
        if (param->ho + param->hl <= HB_SIZE)
            /* Circular history buffer does not wrap - copy one chunk */
            zmemcpy(dictionary, state->window + param->ho, param->hl);
        else {
            /* Circular history buffer wraps - copy two chunks */
            zmemcpy(dictionary,
                    state->window + param->ho,
                    HB_SIZE - param->ho);
            zmemcpy(dictionary + HB_SIZE - param->ho,
                    state->window,
                    param->ho + param->hl - HB_SIZE);
        }
    }
    if (dict_length)
        *dict_length = param->hl;
    return Z_OK;
}
