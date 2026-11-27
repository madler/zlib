/* adler32.c -- compute the Adler-32 checksum of a data stream
 * Copyright (C) 1995-2011, 2016 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* @(#) $Id$ */

#include "zutil.h"

#define BASE 65521U     /* largest prime smaller than 65536 */
#define NMAX 5552
/* NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */

#define DO1(buf,i)  {adler += (buf)[i]; sum2 += adler;}
#define DO2(buf,i)  DO1(buf,i); DO1(buf,i+1);
#define DO4(buf,i)  DO2(buf,i); DO2(buf,i+2);
#define DO8(buf,i)  DO4(buf,i); DO4(buf,i+4);
#define DO16(buf)   DO8(buf,0); DO8(buf,8);

/* use NO_DIVIDE if your processor does not do division in hardware --
   try it both ways to see which is faster */
#ifdef NO_DIVIDE
/* note that this assumes BASE is 65521, where 65536 % 65521 == 15
   (thank you to John Reiser for pointing this out) */
#  define CHOP(a) \
    do { \
        unsigned long tmp = a >> 16; \
        a &= 0xffffUL; \
        a += (tmp << 4) - tmp; \
    } while (0)
#  define MOD28(a) \
    do { \
        CHOP(a); \
        if (a >= BASE) a -= BASE; \
    } while (0)
#  define MOD(a) \
    do { \
        CHOP(a); \
        MOD28(a); \
    } while (0)
#  define MOD63(a) \
    do { /* this assumes a is not negative */ \
        z_off64_t tmp = a >> 32; \
        a &= 0xffffffffL; \
        a += (tmp << 8) - (tmp << 5) + tmp; \
        tmp = a >> 16; \
        a &= 0xffffL; \
        a += (tmp << 4) - tmp; \
        tmp = a >> 16; \
        a &= 0xffffL; \
        a += (tmp << 4) - tmp; \
        if (a >= BASE) a -= BASE; \
    } while (0)
#else
#  define MOD(a) a %= BASE
#  define MOD28(a) a %= BASE
#  define MOD63(a) a %= BASE
#endif

/* Z_ADLER_WORDS chooses whether adler32_z() adds up eight bytes at a time in
   64-bit words. That is the default where the machine has 64-bit registers
   and fast unaligned loads, Z_REG64 and Z_UNALIGNED_FAST in zutil.h, as for
   the 64-bit loop in inffast.c. Define Z_ADLER_WORDS as 0 to use the byte
   loop below instead, or as 1 to use words on any machine with a 64-bit
   type. */
#ifndef Z_ADLER_WORDS
#  if defined(Z_REG64) && defined(Z_UNALIGNED_FAST)
#    define Z_ADLER_WORDS 1
#  else
#    define Z_ADLER_WORDS 0
#  endif
#endif
#if Z_ADLER_WORDS && !defined(Z_U8)
#  error Z_ADLER_WORDS needs a 64-bit type (Z_U8)
#endif

/* ========================================================================= */
uLong ZEXPORT adler32_z(uLong adler, const Bytef *buf, z_size_t len) {
    unsigned long sum2;
#if !Z_ADLER_WORDS
    unsigned n;
#endif

    /* split Adler-32 into component sums */
    sum2 = (adler >> 16) & 0xffff;
    adler &= 0xffff;

    /* in case user likes doing a byte at a time, keep it fast */
    if (len == 1) {
        adler += buf[0];
        if (adler >= BASE)
            adler -= BASE;
        sum2 += adler;
        if (sum2 >= BASE)
            sum2 -= BASE;
        return adler | (sum2 << 16);
    }

    /* initial Adler-32 value (deferred check for len == 1 speed) */
    if (buf == Z_NULL)
        return 1L;

    /* in case short lengths are provided, keep it somewhat fast */
    if (len < 16) {
        while (len--) {
            adler += *buf++;
            sum2 += adler;
        }
        if (adler >= BASE)
            adler -= BASE;
        MOD28(sum2);            /* only added so many BASE's */
        return adler | (sum2 << 16);
    }

#if Z_ADLER_WORDS
    /* Eight bytes at a time. With the even and the odd bytes of a word in
       16-bit lanes, one multiplication adds them up and two weight them: the
       top 16 bits of v * (c0 + c1 2^16 + c2 2^32 + c3 2^48) are
       v0 c3 + v1 c2 + v2 c1 + v3 c0, as no partial sum reaches 2^16 (at most
       255 * 20). A word adds 8 * adler + sum((8 - i) * byte[i]) to sum2. In
       64 bits the sums need a modulo only every 2^20 bytes. */
    {
        Z_U8 one = 1, a = adler, s = sum2, x, e, o, we, wo;
        Z_U8 m = ((Z_U8)0xff00ff << 32) | 0xff00ff;
        Z_U8 ones = ((Z_U8)0x10001 << 32) | 0x10001;
        z_size_t k;

        /* The weights of the lanes of e and o, which hold bytes 0, 2, 4, 6
           and 1, 3, 5, 7 of the word on a little-endian machine, and bytes 7,
           5, 3, 1 and 6, 4, 2, 0 on a big-endian one. */
        if (*(unsigned char *)&one) {
            we = ((Z_U8)0x80006 << 32) | 0x40002;
            wo = ((Z_U8)0x70005 << 32) | 0x30001;
        }
        else {
            we = ((Z_U8)0x10003 << 32) | 0x50007;
            wo = ((Z_U8)0x20004 << 32) | 0x60008;
        }
        do {
            k = len < 0x100000 ? len >> 3 : 0x20000;
            len -= k << 3;
            do {
                zmemcpy(&x, buf, 8);
                e = x & m;
                o = (x >> 8) & m;
                s += (a << 3) + ((e * we) >> 48) + ((o * wo) >> 48);
                a += ((e + o) * ones) >> 48;
                buf += 8;
            } while (--k);
            a %= BASE;
            s %= BASE;
        } while (len >= 8);
        adler = (unsigned long)a;
        sum2 = (unsigned long)s;
        while (len--) {
            adler += *buf++;
            sum2 += adler;
        }
        if (adler >= BASE)
            adler -= BASE;
        MOD28(sum2);
        return adler | (sum2 << 16);
    }
#else
    /* do length NMAX blocks -- requires just one modulo operation */
    while (len >= NMAX) {
        len -= NMAX;
        n = NMAX / 16;          /* NMAX is divisible by 16 */
        do {
            DO16(buf);          /* 16 sums unrolled */
            buf += 16;
        } while (--n);
        MOD(adler);
        MOD(sum2);
    }

    /* do remaining bytes (less than NMAX, still just one modulo) */
    if (len) {                  /* avoid modulos if none remaining */
        while (len >= 16) {
            len -= 16;
            DO16(buf);
            buf += 16;
        }
        while (len--) {
            adler += *buf++;
            sum2 += adler;
        }
        MOD(adler);
        MOD(sum2);
    }

    /* return recombined sums */
    return adler | (sum2 << 16);
#endif
}

/* ========================================================================= */
uLong ZEXPORT adler32(uLong adler, const Bytef *buf, uInt len) {
    return adler32_z(adler, buf, len);
}

/* ========================================================================= */
local uLong adler32_combine_(uLong adler1, uLong adler2, z_off64_t len2) {
    unsigned long sum1;
    unsigned long sum2;
    unsigned rem;

    /* for negative len, return invalid adler32 as a clue for debugging */
    if (len2 < 0)
        return 0xffffffffUL;

    /* the derivation of this formula is left as an exercise for the reader */
    MOD63(len2);                /* assumes len2 >= 0 */
    rem = (unsigned)len2;
    sum1 = adler1 & 0xffff;
    sum2 = rem * sum1;
    MOD(sum2);
    sum1 += (adler2 & 0xffff) + BASE - 1;
    sum2 += ((adler1 >> 16) & 0xffff) + ((adler2 >> 16) & 0xffff) + BASE - rem;
    if (sum1 >= BASE) sum1 -= BASE;
    if (sum1 >= BASE) sum1 -= BASE;
    if (sum2 >= ((unsigned long)BASE << 1)) sum2 -= ((unsigned long)BASE << 1);
    if (sum2 >= BASE) sum2 -= BASE;
    return sum1 | (sum2 << 16);
}

/* ========================================================================= */
uLong ZEXPORT adler32_combine(uLong adler1, uLong adler2, z_off_t len2) {
    return adler32_combine_(adler1, adler2, len2);
}

uLong ZEXPORT adler32_combine64(uLong adler1, uLong adler2, z_off64_t len2) {
    return adler32_combine_(adler1, adler2, len2);
}
