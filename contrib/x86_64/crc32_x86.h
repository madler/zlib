/*
 * Compute the CRC32 using a parallelized folding approach with the PCLMULQDQ
 * instruction.
 *
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
 * Copyright (C) 2016 Marian Beermann (support for initial value)
 * Copyright Wangyang Guo (wangyang.guo@intel.com)
 *
 * Modifications Copyright (C) 2024-2025, Advanced Micro Devices. All rights reserved.
 *
 * For conditions of distribution and use, see copyright notice in zlib.h
 */
#ifndef _CRC32_X86
#define _CRC32_X86
#include "../../zutil.h"
#include <stdint.h>

#ifdef CRC_32_Z_VARIANT
#include "../../crc32.h" // crc tables
#include <immintrin.h>

#define ALIGNED_(x) __attribute__ ((aligned(x)))

#define CRC32_FOLD_BUFFER_SIZE (16 * 4) /* sizeof(__m128i) * (4 folds) */

typedef struct crc32_fold_s {
    uint8_t fold[CRC32_FOLD_BUFFER_SIZE];
    uint32_t value;
} crc32_fold;

#define DO1 c = crc_table[(c ^ *buf++) & 0xff] ^ (c >> 8)

static inline uint32_t CRC_32_Z_X86(uint32_t crc32, const uint8_t* buf, size_t len);

#if CRC_32_Z_VARIANT == CRC_32_Z_X86_PCLMUL

#define CRC_32_Z_TARGET_ISA "avx,pclmul"
unsigned long
crc32_z_impl_x86_avx(unsigned long crc, const unsigned char FAR* buf,
    z_size_t len) {
    return CRC_32_Z_X86(crc, buf, len);
}

#elif CRC_32_Z_VARIANT == CRC_32_Z_X86_VPCLMULQDQ

#define CRC_32_Z_TARGET_ISA "avx,pclmul,avx512f,vpclmulqdq"
unsigned long
crc32_z_impl_x86_avx512(unsigned long crc, const unsigned char FAR* buf,
    z_size_t len) {
    return CRC_32_Z_X86(crc, buf, len);
}
#define X86_VPCLMULQDQ

#endif /* CRC_32_Z_VARIANT == */

#define ONCE(op)                 if (first) { first = 0; op; }
#define XOR_INITIAL128(where)    ONCE(where = _mm_xor_si128(where, xmm_initial))
#ifdef X86_VPCLMULQDQ
#  define XOR_INITIAL512(where)  ONCE(where = _mm512_xor_si512(where, zmm_initial))
#endif

#ifdef X86_VPCLMULQDQ
__attribute__((__target__(CRC_32_Z_TARGET_ISA)))
static size_t fold_16_vpclmulqdq(__m128i * xmm_crc0, __m128i * xmm_crc1,
    __m128i * xmm_crc2, __m128i * xmm_crc3, const uint8_t * src, size_t len,
    __m128i init_crc, int32_t first) {
#if (defined(__clang__) || (defined(__GNUC__) && (__GNUC__ > 11)))
    __m512i zmm_initial = _mm512_zextsi128_si512(init_crc);
#else
    __m512i zmm_initial = _mm512_setzero_si512();
    zmm_initial = _mm512_castsi128_si512(init_crc);
#endif
    __m512i zmm_t0, zmm_t1, zmm_t2, zmm_t3;
    __m512i zmm_crc0, zmm_crc1, zmm_crc2, zmm_crc3;
    __m512i z0, z1, z2, z3;
    size_t len_tmp = len;
    const __m512i zmm_fold4 = _mm512_set4_epi32(
        0x00000001, 0x54442bd4, 0x00000001, 0xc6e41596);
    const __m512i zmm_fold16 = _mm512_set4_epi32(
        0x00000001, 0x1542778a, 0x00000001, 0x322d1430);

    // zmm register init
    zmm_crc0 = _mm512_setzero_si512();
    zmm_t0 = _mm512_loadu_si512((__m512i*)src);
    XOR_INITIAL512(zmm_t0);
    zmm_crc1 = _mm512_loadu_si512((__m512i*)src + 1);
    zmm_crc2 = _mm512_loadu_si512((__m512i*)src + 2);
    zmm_crc3 = _mm512_loadu_si512((__m512i*)src + 3);

    /* already have intermediate CRC in xmm registers
        * fold4 with 4 xmm_crc to get zmm_crc0
    */
    zmm_crc0 = _mm512_inserti32x4(zmm_crc0, *xmm_crc0, 0);
    zmm_crc0 = _mm512_inserti32x4(zmm_crc0, *xmm_crc1, 1);
    zmm_crc0 = _mm512_inserti32x4(zmm_crc0, *xmm_crc2, 2);
    zmm_crc0 = _mm512_inserti32x4(zmm_crc0, *xmm_crc3, 3);
    z0 = _mm512_clmulepi64_epi128(zmm_crc0, zmm_fold4, 0x01);
    zmm_crc0 = _mm512_clmulepi64_epi128(zmm_crc0, zmm_fold4, 0x10);
    zmm_crc0 = _mm512_ternarylogic_epi32(zmm_crc0, z0, zmm_t0, 0x96);

    len -= 256;
    src += 256;

    // fold-16 loops
    while (len >= 256) {
        zmm_t0 = _mm512_loadu_si512((__m512i*)src);
        zmm_t1 = _mm512_loadu_si512((__m512i*)src + 1);
        zmm_t2 = _mm512_loadu_si512((__m512i*)src + 2);
        zmm_t3 = _mm512_loadu_si512((__m512i*)src + 3);

        z0 = _mm512_clmulepi64_epi128(zmm_crc0, zmm_fold16, 0x01);
        z1 = _mm512_clmulepi64_epi128(zmm_crc1, zmm_fold16, 0x01);
        z2 = _mm512_clmulepi64_epi128(zmm_crc2, zmm_fold16, 0x01);
        z3 = _mm512_clmulepi64_epi128(zmm_crc3, zmm_fold16, 0x01);

        zmm_crc0 = _mm512_clmulepi64_epi128(zmm_crc0, zmm_fold16, 0x10);
        zmm_crc1 = _mm512_clmulepi64_epi128(zmm_crc1, zmm_fold16, 0x10);
        zmm_crc2 = _mm512_clmulepi64_epi128(zmm_crc2, zmm_fold16, 0x10);
        zmm_crc3 = _mm512_clmulepi64_epi128(zmm_crc3, zmm_fold16, 0x10);

        zmm_crc0 = _mm512_ternarylogic_epi32(zmm_crc0, z0, zmm_t0, 0x96);
        zmm_crc1 = _mm512_ternarylogic_epi32(zmm_crc1, z1, zmm_t1, 0x96);
        zmm_crc2 = _mm512_ternarylogic_epi32(zmm_crc2, z2, zmm_t2, 0x96);
        zmm_crc3 = _mm512_ternarylogic_epi32(zmm_crc3, z3, zmm_t3, 0x96);

        len -= 256;
        src += 256;
    }
    // zmm_crc[0,1,2,3] -> zmm_crc0
    z0 = _mm512_clmulepi64_epi128(zmm_crc0, zmm_fold4, 0x01);
    zmm_crc0 = _mm512_clmulepi64_epi128(zmm_crc0, zmm_fold4, 0x10);
    zmm_crc0 = _mm512_ternarylogic_epi32(zmm_crc0, z0, zmm_crc1, 0x96);

    z0 = _mm512_clmulepi64_epi128(zmm_crc0, zmm_fold4, 0x01);
    zmm_crc0 = _mm512_clmulepi64_epi128(zmm_crc0, zmm_fold4, 0x10);
    zmm_crc0 = _mm512_ternarylogic_epi32(zmm_crc0, z0, zmm_crc2, 0x96);

    z0 = _mm512_clmulepi64_epi128(zmm_crc0, zmm_fold4, 0x01);
    zmm_crc0 = _mm512_clmulepi64_epi128(zmm_crc0, zmm_fold4, 0x10);
    zmm_crc0 = _mm512_ternarylogic_epi32(zmm_crc0, z0, zmm_crc3, 0x96);

    // zmm_crc0 -> xmm_crc[0, 1, 2, 3]
    *xmm_crc0 = _mm512_extracti32x4_epi32(zmm_crc0, 0);
    *xmm_crc1 = _mm512_extracti32x4_epi32(zmm_crc0, 1);
    *xmm_crc2 = _mm512_extracti32x4_epi32(zmm_crc0, 2);
    *xmm_crc3 = _mm512_extracti32x4_epi32(zmm_crc0, 3);

    return (len_tmp - len);  // return n bytes processed
}
#endif /* X86_VPCLMULQDQ */

__attribute__((__target__(CRC_32_Z_TARGET_ISA)))
static void fold_1(__m128i * xmm_crc0, __m128i * xmm_crc1, __m128i * xmm_crc2, __m128i * xmm_crc3) {
    const __m128i xmm_fold4 = _mm_set_epi32(0x00000001, 0x54442bd4,
        0x00000001, 0xc6e41596);
    __m128i x_tmp3;
    __m128 ps_crc0, ps_crc3, ps_res;

    x_tmp3 = *xmm_crc3;

    *xmm_crc3 = *xmm_crc0;
    *xmm_crc0 = _mm_clmulepi64_si128(*xmm_crc0, xmm_fold4, 0x01);
    *xmm_crc3 = _mm_clmulepi64_si128(*xmm_crc3, xmm_fold4, 0x10);
    ps_crc0 = _mm_castsi128_ps(*xmm_crc0);
    ps_crc3 = _mm_castsi128_ps(*xmm_crc3);
    ps_res = _mm_xor_ps(ps_crc0, ps_crc3);

    *xmm_crc0 = *xmm_crc1;
    *xmm_crc1 = *xmm_crc2;
    *xmm_crc2 = x_tmp3;
    *xmm_crc3 = _mm_castps_si128(ps_res);
}

__attribute__((__target__(CRC_32_Z_TARGET_ISA)))
static void fold_2(__m128i * xmm_crc0, __m128i * xmm_crc1, __m128i * xmm_crc2, __m128i * xmm_crc3) {
    const __m128i xmm_fold4 = _mm_set_epi32(0x00000001, 0x54442bd4,
        0x00000001, 0xc6e41596);
    __m128i x_tmp3, x_tmp2;
    __m128 ps_crc0, ps_crc1, ps_crc2, ps_crc3, ps_res31, ps_res20;

    x_tmp3 = *xmm_crc3;
    x_tmp2 = *xmm_crc2;

    *xmm_crc3 = *xmm_crc1;
    *xmm_crc1 = _mm_clmulepi64_si128(*xmm_crc1, xmm_fold4, 0x01);
    *xmm_crc3 = _mm_clmulepi64_si128(*xmm_crc3, xmm_fold4, 0x10);
    ps_crc3 = _mm_castsi128_ps(*xmm_crc3);
    ps_crc1 = _mm_castsi128_ps(*xmm_crc1);
    ps_res31 = _mm_xor_ps(ps_crc3, ps_crc1);

    *xmm_crc2 = *xmm_crc0;
    *xmm_crc0 = _mm_clmulepi64_si128(*xmm_crc0, xmm_fold4, 0x01);
    *xmm_crc2 = _mm_clmulepi64_si128(*xmm_crc2, xmm_fold4, 0x10);
    ps_crc0 = _mm_castsi128_ps(*xmm_crc0);
    ps_crc2 = _mm_castsi128_ps(*xmm_crc2);
    ps_res20 = _mm_xor_ps(ps_crc0, ps_crc2);

    *xmm_crc0 = x_tmp2;
    *xmm_crc1 = x_tmp3;
    *xmm_crc2 = _mm_castps_si128(ps_res20);
    *xmm_crc3 = _mm_castps_si128(ps_res31);
}

__attribute__((__target__(CRC_32_Z_TARGET_ISA)))
static void fold_3(__m128i * xmm_crc0, __m128i * xmm_crc1, __m128i * xmm_crc2, __m128i * xmm_crc3) {
    const __m128i xmm_fold4 = _mm_set_epi32(0x00000001, 0x54442bd4,
        0x00000001, 0xc6e41596);
    __m128i x_tmp3;
    __m128 ps_crc0, ps_crc1, ps_crc2, ps_crc3, ps_res32, ps_res21, ps_res10;

    x_tmp3 = *xmm_crc3;

    *xmm_crc3 = *xmm_crc2;
    *xmm_crc2 = _mm_clmulepi64_si128(*xmm_crc2, xmm_fold4, 0x01);
    *xmm_crc3 = _mm_clmulepi64_si128(*xmm_crc3, xmm_fold4, 0x10);
    ps_crc2 = _mm_castsi128_ps(*xmm_crc2);
    ps_crc3 = _mm_castsi128_ps(*xmm_crc3);
    ps_res32 = _mm_xor_ps(ps_crc2, ps_crc3);

    *xmm_crc2 = *xmm_crc1;
    *xmm_crc1 = _mm_clmulepi64_si128(*xmm_crc1, xmm_fold4, 0x01);
    *xmm_crc2 = _mm_clmulepi64_si128(*xmm_crc2, xmm_fold4, 0x10);
    ps_crc1 = _mm_castsi128_ps(*xmm_crc1);
    ps_crc2 = _mm_castsi128_ps(*xmm_crc2);
    ps_res21 = _mm_xor_ps(ps_crc1, ps_crc2);

    *xmm_crc1 = *xmm_crc0;
    *xmm_crc0 = _mm_clmulepi64_si128(*xmm_crc0, xmm_fold4, 0x01);
    *xmm_crc1 = _mm_clmulepi64_si128(*xmm_crc1, xmm_fold4, 0x10);
    ps_crc0 = _mm_castsi128_ps(*xmm_crc0);
    ps_crc1 = _mm_castsi128_ps(*xmm_crc1);
    ps_res10 = _mm_xor_ps(ps_crc0, ps_crc1);

    *xmm_crc0 = x_tmp3;
    *xmm_crc1 = _mm_castps_si128(ps_res10);
    *xmm_crc2 = _mm_castps_si128(ps_res21);
    *xmm_crc3 = _mm_castps_si128(ps_res32);
}

__attribute__((__target__(CRC_32_Z_TARGET_ISA)))
static void fold_4(__m128i * xmm_crc0, __m128i * xmm_crc1, __m128i * xmm_crc2, __m128i * xmm_crc3) {
    const __m128i xmm_fold4 = _mm_set_epi32(0x00000001, 0x54442bd4,
        0x00000001, 0xc6e41596);
    __m128i x_tmp0, x_tmp1, x_tmp2, x_tmp3;
    __m128 ps_crc0, ps_crc1, ps_crc2, ps_crc3;
    __m128 ps_t0, ps_t1, ps_t2, ps_t3;
    __m128 ps_res0, ps_res1, ps_res2, ps_res3;

    x_tmp0 = *xmm_crc0;
    x_tmp1 = *xmm_crc1;
    x_tmp2 = *xmm_crc2;
    x_tmp3 = *xmm_crc3;

    *xmm_crc0 = _mm_clmulepi64_si128(*xmm_crc0, xmm_fold4, 0x01);
    x_tmp0 = _mm_clmulepi64_si128(x_tmp0, xmm_fold4, 0x10);
    ps_crc0 = _mm_castsi128_ps(*xmm_crc0);
    ps_t0 = _mm_castsi128_ps(x_tmp0);
    ps_res0 = _mm_xor_ps(ps_crc0, ps_t0);

    *xmm_crc1 = _mm_clmulepi64_si128(*xmm_crc1, xmm_fold4, 0x01);
    x_tmp1 = _mm_clmulepi64_si128(x_tmp1, xmm_fold4, 0x10);
    ps_crc1 = _mm_castsi128_ps(*xmm_crc1);
    ps_t1 = _mm_castsi128_ps(x_tmp1);
    ps_res1 = _mm_xor_ps(ps_crc1, ps_t1);

    *xmm_crc2 = _mm_clmulepi64_si128(*xmm_crc2, xmm_fold4, 0x01);
    x_tmp2 = _mm_clmulepi64_si128(x_tmp2, xmm_fold4, 0x10);
    ps_crc2 = _mm_castsi128_ps(*xmm_crc2);
    ps_t2 = _mm_castsi128_ps(x_tmp2);
    ps_res2 = _mm_xor_ps(ps_crc2, ps_t2);

    *xmm_crc3 = _mm_clmulepi64_si128(*xmm_crc3, xmm_fold4, 0x01);
    x_tmp3 = _mm_clmulepi64_si128(x_tmp3, xmm_fold4, 0x10);
    ps_crc3 = _mm_castsi128_ps(*xmm_crc3);
    ps_t3 = _mm_castsi128_ps(x_tmp3);
    ps_res3 = _mm_xor_ps(ps_crc3, ps_t3);

    *xmm_crc0 = _mm_castps_si128(ps_res0);
    *xmm_crc1 = _mm_castps_si128(ps_res1);
    *xmm_crc2 = _mm_castps_si128(ps_res2);
    *xmm_crc3 = _mm_castps_si128(ps_res3);
}

static const unsigned ALIGNED_(32) pshufb_shf_table[60] = {
    0x84838281, 0x88878685, 0x8c8b8a89, 0x008f8e8d, /* shl 15 (16 - 1)/shr1 */
    0x85848382, 0x89888786, 0x8d8c8b8a, 0x01008f8e, /* shl 14 (16 - 3)/shr2 */
    0x86858483, 0x8a898887, 0x8e8d8c8b, 0x0201008f, /* shl 13 (16 - 4)/shr3 */
    0x87868584, 0x8b8a8988, 0x8f8e8d8c, 0x03020100, /* shl 12 (16 - 4)/shr4 */
    0x88878685, 0x8c8b8a89, 0x008f8e8d, 0x04030201, /* shl 11 (16 - 5)/shr5 */
    0x89888786, 0x8d8c8b8a, 0x01008f8e, 0x05040302, /* shl 10 (16 - 6)/shr6 */
    0x8a898887, 0x8e8d8c8b, 0x0201008f, 0x06050403, /* shl  9 (16 - 7)/shr7 */
    0x8b8a8988, 0x8f8e8d8c, 0x03020100, 0x07060504, /* shl  8 (16 - 8)/shr8 */
    0x8c8b8a89, 0x008f8e8d, 0x04030201, 0x08070605, /* shl  7 (16 - 9)/shr9 */
    0x8d8c8b8a, 0x01008f8e, 0x05040302, 0x09080706, /* shl  6 (16 -10)/shr10*/
    0x8e8d8c8b, 0x0201008f, 0x06050403, 0x0a090807, /* shl  5 (16 -11)/shr11*/
    0x8f8e8d8c, 0x03020100, 0x07060504, 0x0b0a0908, /* shl  4 (16 -12)/shr12*/
    0x008f8e8d, 0x04030201, 0x08070605, 0x0c0b0a09, /* shl  3 (16 -13)/shr13*/
    0x01008f8e, 0x05040302, 0x09080706, 0x0d0c0b0a, /* shl  2 (16 -14)/shr14*/
    0x0201008f, 0x06050403, 0x0a090807, 0x0e0d0c0b  /* shl  1 (16 -15)/shr15*/
};

__attribute__((__target__(CRC_32_Z_TARGET_ISA)))
static void partial_fold(const size_t len, __m128i * xmm_crc0, __m128i * xmm_crc1, __m128i * xmm_crc2,
    __m128i * xmm_crc3, __m128i * xmm_crc_part) {
    const __m128i xmm_fold4 = _mm_set_epi32(0x00000001, 0x54442bd4,
        0x00000001, 0xc6e41596);
    const __m128i xmm_mask3 = _mm_set1_epi32((int32_t)0x80808080);

    __m128i xmm_shl, xmm_shr, xmm_tmp1, xmm_tmp2, xmm_tmp3;
    __m128i xmm_a0_0, xmm_a0_1;
    __m128 ps_crc3, psa0_0, psa0_1, ps_res;

    xmm_shl = _mm_load_si128((__m128i*)(pshufb_shf_table + (4 * (len - 1))));
    xmm_shr = xmm_shl;
    xmm_shr = _mm_xor_si128(xmm_shr, xmm_mask3);

    xmm_a0_0 = _mm_shuffle_epi8(*xmm_crc0, xmm_shl);

    *xmm_crc0 = _mm_shuffle_epi8(*xmm_crc0, xmm_shr);
    xmm_tmp1 = _mm_shuffle_epi8(*xmm_crc1, xmm_shl);
    *xmm_crc0 = _mm_or_si128(*xmm_crc0, xmm_tmp1);

    *xmm_crc1 = _mm_shuffle_epi8(*xmm_crc1, xmm_shr);
    xmm_tmp2 = _mm_shuffle_epi8(*xmm_crc2, xmm_shl);
    *xmm_crc1 = _mm_or_si128(*xmm_crc1, xmm_tmp2);

    *xmm_crc2 = _mm_shuffle_epi8(*xmm_crc2, xmm_shr);
    xmm_tmp3 = _mm_shuffle_epi8(*xmm_crc3, xmm_shl);
    *xmm_crc2 = _mm_or_si128(*xmm_crc2, xmm_tmp3);

    *xmm_crc3 = _mm_shuffle_epi8(*xmm_crc3, xmm_shr);
    *xmm_crc_part = _mm_shuffle_epi8(*xmm_crc_part, xmm_shl);
    *xmm_crc3 = _mm_or_si128(*xmm_crc3, *xmm_crc_part);

    xmm_a0_1 = _mm_clmulepi64_si128(xmm_a0_0, xmm_fold4, 0x10);
    xmm_a0_0 = _mm_clmulepi64_si128(xmm_a0_0, xmm_fold4, 0x01);

    ps_crc3 = _mm_castsi128_ps(*xmm_crc3);
    psa0_0 = _mm_castsi128_ps(xmm_a0_0);
    psa0_1 = _mm_castsi128_ps(xmm_a0_1);

    ps_res = _mm_xor_ps(ps_crc3, psa0_0);
    ps_res = _mm_xor_ps(ps_res, psa0_1);

    *xmm_crc3 = _mm_castps_si128(ps_res);
}

__attribute__((__target__(CRC_32_Z_TARGET_ISA)))
static inline void crc32_fold_load(__m128i * fold, __m128i * fold0, __m128i * fold1, __m128i * fold2, __m128i * fold3) {
    *fold0 = _mm_load_si128(fold + 0);
    *fold1 = _mm_load_si128(fold + 1);
    *fold2 = _mm_load_si128(fold + 2);
    *fold3 = _mm_load_si128(fold + 3);
}

__attribute__((__target__(CRC_32_Z_TARGET_ISA)))
static inline void crc32_fold_save(__m128i * fold, const __m128i * fold0, const __m128i * fold1,
    const __m128i * fold2, const __m128i * fold3) {
    _mm_storeu_si128(fold + 0, *fold0);
    _mm_storeu_si128(fold + 1, *fold1);
    _mm_storeu_si128(fold + 2, *fold2);
    _mm_storeu_si128(fold + 3, *fold3);
}

__attribute__((__target__(CRC_32_Z_TARGET_ISA)))
static inline  uint32_t CRC32_FOLD_RESET(crc32_fold * crc) {
    __m128i xmm_crc0 = _mm_cvtsi32_si128(0x9db42487);
    __m128i xmm_zero = _mm_setzero_si128();
    crc32_fold_save((__m128i*)crc->fold, &xmm_crc0, &xmm_zero, &xmm_zero, &xmm_zero);
    return 0;
}

static const unsigned ALIGNED_(16) crc_k[] = {
    0xccaa009e, 0x00000000, /* rk1 */
    0x751997d0, 0x00000001, /* rk2 */
    0xccaa009e, 0x00000000, /* rk5 */
    0x63cd6124, 0x00000001, /* rk6 */
    0xf7011640, 0x00000001, /* rk7 */
    0xdb710640, 0x00000001  /* rk8 */
};

static const unsigned ALIGNED_(16) crc_mask[4] = {
    0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000
};

static const unsigned ALIGNED_(16) crc_mask2[4] = {
    0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF
};

__attribute__((__target__(CRC_32_Z_TARGET_ISA)))
static inline uint32_t CRC32_FOLD_FINAL(crc32_fold * crc) {
    const __m128i xmm_mask = _mm_load_si128((__m128i*)crc_mask);
    const __m128i xmm_mask2 = _mm_load_si128((__m128i*)crc_mask2);
    __m128i xmm_crc0, xmm_crc1, xmm_crc2, xmm_crc3;
    __m128i x_tmp0, x_tmp1, x_tmp2, crc_fold;

    crc32_fold_load((__m128i*)crc->fold, &xmm_crc0, &xmm_crc1, &xmm_crc2, &xmm_crc3);

    /*
     * k1
     */
    crc_fold = _mm_load_si128((__m128i*)crc_k);

    x_tmp0 = _mm_clmulepi64_si128(xmm_crc0, crc_fold, 0x10);
    xmm_crc0 = _mm_clmulepi64_si128(xmm_crc0, crc_fold, 0x01);
    xmm_crc1 = _mm_xor_si128(xmm_crc1, x_tmp0);
    xmm_crc1 = _mm_xor_si128(xmm_crc1, xmm_crc0);

    x_tmp1 = _mm_clmulepi64_si128(xmm_crc1, crc_fold, 0x10);
    xmm_crc1 = _mm_clmulepi64_si128(xmm_crc1, crc_fold, 0x01);
    xmm_crc2 = _mm_xor_si128(xmm_crc2, x_tmp1);
    xmm_crc2 = _mm_xor_si128(xmm_crc2, xmm_crc1);

    x_tmp2 = _mm_clmulepi64_si128(xmm_crc2, crc_fold, 0x10);
    xmm_crc2 = _mm_clmulepi64_si128(xmm_crc2, crc_fold, 0x01);
    xmm_crc3 = _mm_xor_si128(xmm_crc3, x_tmp2);
    xmm_crc3 = _mm_xor_si128(xmm_crc3, xmm_crc2);

    /*
     * k5
     */
    crc_fold = _mm_load_si128((__m128i*)(crc_k + 4));

    xmm_crc0 = xmm_crc3;
    xmm_crc3 = _mm_clmulepi64_si128(xmm_crc3, crc_fold, 0);
    xmm_crc0 = _mm_srli_si128(xmm_crc0, 8);
    xmm_crc3 = _mm_xor_si128(xmm_crc3, xmm_crc0);

    xmm_crc0 = xmm_crc3;
    xmm_crc3 = _mm_slli_si128(xmm_crc3, 4);
    xmm_crc3 = _mm_clmulepi64_si128(xmm_crc3, crc_fold, 0x10);
    xmm_crc3 = _mm_xor_si128(xmm_crc3, xmm_crc0);
    xmm_crc3 = _mm_and_si128(xmm_crc3, xmm_mask2);

    /*
     * k7
     */
    xmm_crc1 = xmm_crc3;
    xmm_crc2 = xmm_crc3;
    crc_fold = _mm_load_si128((__m128i*)(crc_k + 8));

    xmm_crc3 = _mm_clmulepi64_si128(xmm_crc3, crc_fold, 0);
    xmm_crc3 = _mm_xor_si128(xmm_crc3, xmm_crc2);
    xmm_crc3 = _mm_and_si128(xmm_crc3, xmm_mask);

    xmm_crc2 = xmm_crc3;
    xmm_crc3 = _mm_clmulepi64_si128(xmm_crc3, crc_fold, 0x10);
    xmm_crc3 = _mm_xor_si128(xmm_crc3, xmm_crc2);
    xmm_crc3 = _mm_xor_si128(xmm_crc3, xmm_crc1);

    crc->value = ~((uint32_t)_mm_extract_epi32(xmm_crc3, 2));

    return crc->value;
}

static inline uint32_t crc32_small(uint32_t crc, const uint8_t * buf, size_t len) {
    uint32_t c = (~crc) & 0xffffffff;

    while (len) {
        len--;
        DO1;
    }

    return c ^ 0xffffffff;
}

__attribute__((__target__(CRC_32_Z_TARGET_ISA)))
static inline void CRC32_FOLD(crc32_fold * crc, const uint8_t * src, size_t len, uint32_t init_crc) {
    unsigned long algn_diff;
    __m128i xmm_t0, xmm_t1, xmm_t2, xmm_t3;
    __m128i xmm_crc0, xmm_crc1, xmm_crc2, xmm_crc3;
    __m128i xmm_crc_part = _mm_setzero_si128();
    char ALIGNED_(16) partial_buf[16] = { 0 };
    __m128i xmm_initial = _mm_cvtsi32_si128(init_crc);
    int32_t first = init_crc != 0;

    /* The CRC functions don't call this for input < 16, as a minimum of 16 bytes of input is needed
     * for the aligning load that occurs.  If there's an initial CRC, to carry it forward through
     * the folded CRC there must be 16 - src % 16 + 16 bytes available, which by definition can be
     * up to 15 bytes + one full vector load. */
    Assert(len >= 16 || first == 0, "Invalid len or init_crc values.");

    crc32_fold_load((__m128i*)crc->fold, &xmm_crc0, &xmm_crc1, &xmm_crc2, &xmm_crc3);

    if (len < 16) {
        if (len == 0)
            return;

        memcpy(partial_buf, src, len);
        xmm_crc_part = _mm_load_si128((const __m128i*)partial_buf);
        goto partial;
    }

    algn_diff = ((uintptr_t)16 - ((uintptr_t)src & 0xF)) & 0xF;
    if (algn_diff) {
        xmm_crc_part = _mm_loadu_si128((__m128i*)src);
        XOR_INITIAL128(xmm_crc_part);

        if (algn_diff < 4 && init_crc != 0) {
            xmm_t0 = xmm_crc_part;
            if (len >= 32) {
                xmm_crc_part = _mm_loadu_si128((__m128i*)src + 1);
                fold_1(&xmm_crc0, &xmm_crc1, &xmm_crc2, &xmm_crc3);
                xmm_crc3 = _mm_xor_si128(xmm_crc3, xmm_t0);
            }
            else {
                memcpy(partial_buf, src + 16, len - 16);
                xmm_crc_part = _mm_load_si128((__m128i*)partial_buf);
                fold_1(&xmm_crc0, &xmm_crc1, &xmm_crc2, &xmm_crc3);
                xmm_crc3 = _mm_xor_si128(xmm_crc3, xmm_t0);
                src += 16;
                len -= 16;
                goto partial;
            }

            src += 16;
            len -= 16;
        }

        partial_fold(algn_diff, &xmm_crc0, &xmm_crc1, &xmm_crc2, &xmm_crc3, &xmm_crc_part);

        src += algn_diff;
        len -= algn_diff;
    }

#ifdef X86_VPCLMULQDQ
    if (len >= 256) {
        size_t n = fold_16_vpclmulqdq(&xmm_crc0, &xmm_crc1, &xmm_crc2, &xmm_crc3, src, len,
            xmm_initial, first);
        first = 0;
        len -= n;
        src += n;
    }
#endif

    while (len >= 64) {
        len -= 64;
        xmm_t0 = _mm_load_si128((__m128i*)src);
        xmm_t1 = _mm_load_si128((__m128i*)src + 1);
        xmm_t2 = _mm_load_si128((__m128i*)src + 2);
        xmm_t3 = _mm_load_si128((__m128i*)src + 3);
        src += 64;

        fold_4(&xmm_crc0, &xmm_crc1, &xmm_crc2, &xmm_crc3);
        XOR_INITIAL128(xmm_t0);

        xmm_crc0 = _mm_xor_si128(xmm_crc0, xmm_t0);
        xmm_crc1 = _mm_xor_si128(xmm_crc1, xmm_t1);
        xmm_crc2 = _mm_xor_si128(xmm_crc2, xmm_t2);
        xmm_crc3 = _mm_xor_si128(xmm_crc3, xmm_t3);
    }

    /*
     * len = num bytes left - 64
     */
    if (len >= 48) {
        len -= 48;

        xmm_t0 = _mm_load_si128((__m128i*)src);
        xmm_t1 = _mm_load_si128((__m128i*)src + 1);
        xmm_t2 = _mm_load_si128((__m128i*)src + 2);
        src += 48;
        XOR_INITIAL128(xmm_t0);
        fold_3(&xmm_crc0, &xmm_crc1, &xmm_crc2, &xmm_crc3);

        xmm_crc1 = _mm_xor_si128(xmm_crc1, xmm_t0);
        xmm_crc2 = _mm_xor_si128(xmm_crc2, xmm_t1);
        xmm_crc3 = _mm_xor_si128(xmm_crc3, xmm_t2);
    }
    else if (len >= 32) {
        len -= 32;

        xmm_t0 = _mm_load_si128((__m128i*)src);
        xmm_t1 = _mm_load_si128((__m128i*)src + 1);
        src += 32;
        XOR_INITIAL128(xmm_t0);
        fold_2(&xmm_crc0, &xmm_crc1, &xmm_crc2, &xmm_crc3);

        xmm_crc2 = _mm_xor_si128(xmm_crc2, xmm_t0);
        xmm_crc3 = _mm_xor_si128(xmm_crc3, xmm_t1);
    }
    else if (len >= 16) {
        len -= 16;
        xmm_t0 = _mm_load_si128((__m128i*)src);
        src += 16;
        XOR_INITIAL128(xmm_t0);
        fold_1(&xmm_crc0, &xmm_crc1, &xmm_crc2, &xmm_crc3);

        xmm_crc3 = _mm_xor_si128(xmm_crc3, xmm_t0);
    }

partial:
    if (len) {
        memcpy(&xmm_crc_part, src, len);
        partial_fold(len, &xmm_crc0, &xmm_crc1, &xmm_crc2, &xmm_crc3, &xmm_crc_part);
    }

    crc32_fold_save((__m128i*)crc->fold, &xmm_crc0, &xmm_crc1, &xmm_crc2, &xmm_crc3);
}

__attribute__((__target__(CRC_32_Z_TARGET_ISA)))
static inline uint32_t CRC_32_Z_X86(uint32_t crc32, const uint8_t* buf, size_t len) {
    /* For lens smaller than ~12, crc32_small method is faster.
     * But there are also minimum requirements for the pclmul functions due to alignment */
    if (len < 16)
        return crc32_small(crc32, buf, len);

    crc32_fold ALIGNED_(16) crc_state;
    CRC32_FOLD_RESET(&crc_state);
    CRC32_FOLD(&crc_state, buf, len, crc32);
    return CRC32_FOLD_FINAL(&crc_state);
}

#undef CRC_32_Z_TARGET_ISA
#undef CRC_32_Z_VARIANT
#endif /* CRC_32_Z_VARIANT */

#endif /*_CRC32_X86_ */
