/* Copyright (C) 1995-2011, 2016 Mark Adler
 * Copyright (C) 2017 ARM Holdings Inc.
 * Authors: Adenilson Cavalcanti <adenilson.cavalcanti@arm.com>
 *          Simon Hosie <simon.hosie@arm.com>
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 1. The origin of this software must not be misrepresented; you must not
 *  claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#if (defined(__ARM_NEON__) || defined(__ARM_NEON))
#include <arm_neon.h>

static void NEON_accum32(uint32_t *s, const unsigned char *buf,
                         unsigned int len)
{
    static const uint8_t taps[32] = {
        32, 31, 30, 29, 28, 27, 26, 25,
        24, 23, 22, 21, 20, 19, 18, 17,
        16, 15, 14, 13, 12, 11, 10, 9,
        8, 7, 6, 5, 4, 3, 2, 1 };

    uint32x2_t adacc2, s2acc2, as;
    uint8x16_t t0 = vld1q_u8(taps), t1 = vld1q_u8(taps + 16);

    uint32x4_t adacc = vdupq_n_u32(0), s2acc = vdupq_n_u32(0);
    adacc = vsetq_lane_u32(s[0], adacc, 0);
    s2acc = vsetq_lane_u32(s[1], s2acc, 0);

    while (len >= 2) {
        uint8x16_t d0 = vld1q_u8(buf), d1 = vld1q_u8(buf + 16);
        uint16x8_t adler, sum2;
        s2acc = vaddq_u32(s2acc, vshlq_n_u32(adacc, 5));
        adler = vpaddlq_u8(       d0);
        adler = vpadalq_u8(adler, d1);
        sum2 = vmull_u8(      vget_low_u8(t0), vget_low_u8(d0));
        sum2 = vmlal_u8(sum2, vget_high_u8(t0), vget_high_u8(d0));
        sum2 = vmlal_u8(sum2, vget_low_u8(t1), vget_low_u8(d1));
        sum2 = vmlal_u8(sum2, vget_high_u8(t1), vget_high_u8(d1));
        adacc = vpadalq_u16(adacc, adler);
        s2acc = vpadalq_u16(s2acc, sum2);
        len -= 2;
        buf += 32;
    }

    while (len > 0) {
        uint8x16_t d0 = vld1q_u8(buf);
        uint16x8_t adler, sum2;
        s2acc = vaddq_u32(s2acc, vshlq_n_u32(adacc, 4));
        adler = vpaddlq_u8(d0);
        sum2 = vmull_u8(      vget_low_u8(t1), vget_low_u8(d0));
        sum2 = vmlal_u8(sum2, vget_high_u8(t1), vget_high_u8(d0));
        adacc = vpadalq_u16(adacc, adler);
        s2acc = vpadalq_u16(s2acc, sum2);
        buf += 16;
        len--;
    }

    adacc2 = vpadd_u32(vget_low_u32(adacc), vget_high_u32(adacc));
    s2acc2 = vpadd_u32(vget_low_u32(s2acc), vget_high_u32(s2acc));
    as = vpadd_u32(adacc2, s2acc2);
    s[0] = vget_lane_u32(as, 0);
    s[1] = vget_lane_u32(as, 1);
}

static void NEON_handle_tail(uint32_t *pair, const unsigned char *buf,
                             unsigned int len)
{
    /* Oldie K&R code integration. */
    unsigned int i;
    for (i = 0; i < len; ++i) {
        pair[0] += buf[i];
        pair[1] += pair[0];
    }
}

extern unsigned long NEON_adler32(unsigned long adler, const unsigned char *buf,
                                  const unsigned int len)
{
    /* initial Adler-32 value (deferred check for len == 1 speed) */
    if (!buf)
        return 1L;

    /* The largest prime smaller than 65536. */
    const uint32_t M_BASE = 65521;
    /* This is the threshold where doing accumulation may overflow. */
    const int M_NMAX = 5552;

    unsigned long sum2;
    uint32_t pair[2];
    int n = M_NMAX;
    unsigned int done = 0;
    /* Oldie K&R code integration. */
    unsigned int i;

    /* Split Adler-32 into component sums, it can be supplied by
     * the caller sites (e.g. in a PNG file).
     */
    sum2 = (adler >> 16) & 0xffff;
    adler &= 0xffff;
    pair[0] = adler;
    pair[1] = sum2;

    for (i = 0; i < len; i += n) {
        if ((i + n) > len)
            n = len - i;

        if (n < 16)
            break;

        NEON_accum32(pair, buf + i, n / 16);
        pair[0] %= M_BASE;
        pair[1] %= M_BASE;

        done += (n / 16) * 16;
    }

    /* Handle the tail elements. */
    if (done < len) {
        NEON_handle_tail(pair, (buf + done), len - done);
        pair[0] %= M_BASE;
        pair[1] %= M_BASE;
    }

    /* D = B * 65536 + A, see: https://en.wikipedia.org/wiki/Adler-32. */
    return (pair[1] << 16) | pair[0];
}
#endif
