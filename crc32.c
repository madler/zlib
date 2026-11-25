/* crc32.c -- compute the CRC-32 of a data stream
 * Copyright (C) 1995-2026 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 *
 * This interleaved implementation of a CRC makes use of pipelined multiple
 * arithmetic-logic units, commonly found in modern CPU cores. It is due to
 * Kadatch and Jenkins (2010). See doc/crc-doc.1.0.pdf in this distribution.
 */

/* @(#) $Id$ */

/*
  Note on the use of DYNAMIC_CRC_TABLE: there is no mutex or semaphore
  protection on the static variables used to control the first-use generation
  of the crc tables. Therefore, if you #define DYNAMIC_CRC_TABLE, you should
  first call get_crc_table() to initialize the tables before allowing more than
  one thread to use crc32().

  MAKECRCH can be #defined to write out crc32.h. A main() routine is also
  produced, so that this one source file can be compiled to an executable.
 */

#ifdef MAKECRCH
#  include <stdio.h>
#  ifndef DYNAMIC_CRC_TABLE
#    define DYNAMIC_CRC_TABLE
#  endif
#endif
#ifdef DYNAMIC_CRC_TABLE
#  define Z_ONCE
#endif

#include "zutil.h"      /* for Z_U4, Z_U8, z_crc_t, and FAR definitions */

#ifdef HAVE_S390X_VX
#  include "contrib/crc32vx/crc32_vx_hooks.h"
#endif

/*
  A CRC of a message is computed on N braids of words in the message, where
  each word consists of W bytes (4 or 8). If N is 3, for example, then three
  running sparse CRCs are calculated respectively on each braid, at these
  indices in the array of words: 0, 3, 6, ..., 1, 4, 7, ..., and 2, 5, 8, ...
  This is done starting at a word boundary, and continues until as many blocks
  of N * W bytes as are available have been processed. The results are combined
  into a single CRC at the end. For this code, N must be in the range 1..6 and
  W must be 4 or 8. The upper limit on N can be increased if desired by adding
  more #if blocks, extending the patterns apparent in the code. In addition,
  crc32.h would need to be regenerated, if the maximum N value is increased.

  N and W are chosen empirically by benchmarking the execution time on a given
  processor. The choices below were based on testing on Intel Haswell, AMD Zen
  4, Apple M1, ARM Cortex-A76, another ARM64 server, IBM POWER10, UltraSPARC
  T5, LoongArch64, and ARM Cortex-A7 (32-bit) processors, with gcc and clang,
  and MSVC and clang-cl on Windows, at -O2 and -O3. W=4 was faster than W=8 on
  all of them for lengths up to 1K, and as fast or nearly so on most of them
  for long lengths. With 64-bit registers (see Z_REG64 in zutil.h), N=5 was the
  fastest, or within 10% of the fastest, for lengths of 1K and more. For
  lengths less than about 320 bytes, N=2 was faster, due to the cost of
  combining the N braids at the end. So both are used, divided at BRAID_CUT.
  With 32-bit registers on the Cortex-A7, more than two braids ran out of
  registers, and N=2 was the fastest at all lengths. Your mileage may vary.
 */

/*
  These can be defined when compiling, to change the calculation:

    Z_REG64         Use the choices for 64-bit registers below. zutil.h
                    defines Z_REG64 when the architecture has 64-bit general
                    registers, unless Z_REG32 is defined. It requires an
                    eight-byte unsigned integer type, Z_U8.
    Z_REG32         Use the choices for 32-bit registers below, even if the
                    architecture has 64-bit registers. Z_REG64 and Z_REG32
                    cannot both be defined.
    Z_TESTW=w       Use words of w bytes, 4 or 8, for the braids, in place of
                    DEF_W. 8 requires Z_U8, and 4 is used without it. If w is
                    0 or empty (-DZ_TESTW=), then the braided calculation is
                    not compiled, and the CRC is computed a byte at a time.
                    Any other value is taken as 4.
    Z_TESTN=n       Use n braids, 1..6, in place of DEF_N. If Z_TESTCUT is not
                    defined, then n braids are used at all lengths.
    Z_TESTCUT=len   Use Z_TESTNLOW braids for lengths less than len, and N
                    braids for lengths of len or more, in place of DEF_CUT and
                    DEF_NLOW. Z_TESTCUT and Z_TESTNLOW must be defined
                    together.
    Z_TESTNLOW=n    The number of braids, 1..6, for lengths less than
                    Z_TESTCUT. It may be more or less than N.
    DYNAMIC_CRC_TABLE
                    Compute the tables at run time, instead of using those in
                    crc32.h. Only the braid tables for N and NLOW are made.
    MAKECRCH        Compile a program that writes crc32.h, with the braid
                    tables for N from 1 to 6, for both word sizes, and for
                    both little and big-endian. It requires Z_U8.
    Z_U4, Z_U8      The four and eight-byte unsigned integer types, if they are
                    not found by zconf.h and zutil.h. Without Z_U4, the CRC is
                    computed a byte at a time. Without Z_U8, Z_REG64 is not
                    defined, and W is 4.

  These are defined by the compiler or the build, and also change the
  calculation:

    __ARM_FEATURE_CRC32
                    On 64-bit ARM, use the CRC32 instructions instead of the
                    braided calculation. The compiler defines this when the
                    target architecture has them, e.g. with
                    -march=armv8-a+crc, -march=armv8.1-a, or -march=native on
                    a machine that has them.
    HAVE_S390X_VX   On s390x, crc32() uses the vector instruction code in
                    contrib/crc32vx.
 */

/*
  The default choices, used unless overridden by the defines above:

    DEF_W       The size in bytes of the words that the braids are computed
                on, 4 or 8.
    DEF_N       The number of braids, 1..6, for lengths of DEF_CUT or more, or
                for all lengths if there is no cutoff.
    DEF_CUT     The length below which DEF_NLOW braids are used instead of
                DEF_N. If it is empty or zero, then there is no cutoff, and
                DEF_N braids are used at all lengths.
    DEF_NLOW    The number of braids, 1..6, for lengths less than DEF_CUT. It
                must be set if DEF_CUT is set. If it is equal to DEF_N, then
                there is no cutoff.
 */
#ifdef Z_REG64
#  define DEF_W 4
#  define DEF_N 5
#  define DEF_CUT 320
#  define DEF_NLOW 2
#else
#  define DEF_W 4
#  define DEF_N 2
#  define DEF_CUT
#  define DEF_NLOW
#endif

/*
  Define N, and if there is a cutoff, BRAID_CUT and NLOW, from the defaults,
  or from Z_TESTN, Z_TESTCUT, and Z_TESTNLOW. If there is no cutoff, then NLOW
  is N.
 */
#ifdef Z_TESTN
#  define N Z_TESTN
#else
#  define N DEF_N
#endif
#if defined(Z_TESTCUT) != defined(Z_TESTNLOW)
#  error Z_TESTCUT and Z_TESTNLOW must be set together
#endif
#ifdef Z_TESTCUT
#  define BRAID_CUT Z_TESTCUT
#  define NLOW Z_TESTNLOW
#elif !defined(Z_TESTN) && DEF_CUT-1 != -1
#  if DEF_NLOW-1 == -1
#    error DEF_NLOW must be set when DEF_CUT is
#    define NLOW N
#  elif DEF_NLOW != DEF_N
#    define BRAID_CUT DEF_CUT
#    define NLOW DEF_NLOW
#  else
#    define NLOW N
#  endif
#else
#  define NLOW N
#endif
#if N < 1 || N > 6 || NLOW < 1 || NLOW > 6
#  error N and NLOW must be in 1..6
#endif
#if NLOW < N
#  define NMIN NLOW     /* the fewest braids used */
#else
#  define NMIN N
#endif

/* The names of the braid tables for n braids, in crc32.h or made by
   make_crc_table(). */
#define BRAID_TABLE_(n) crc_braid##n##_table
#define BRAID_TABLE(n) BRAID_TABLE_(n)
#define BRAID_BIG_TABLE_(n) crc_braid##n##_big_table
#define BRAID_BIG_TABLE(n) BRAID_BIG_TABLE_(n)

/*
  z_crc_t must be at least 32 bits. z_word_t must be at least as long as
  z_crc_t. It is assumed here that z_word_t is either 32 bits or 64 bits, and
  that bytes are eight bits.
 */

/*
  Define W and the associated z_word_t type. If W is not defined, then a
  braided calculation is not used, and the associated tables and code are not
  compiled.
 */
#ifdef Z_TESTW
#  if Z_TESTW-1 != -1
#    define W Z_TESTW
#  endif
#else
#  ifdef MAKECRCH
#    define W 8         /* required for MAKECRCH */
#  else
#    define W DEF_W
#  endif
#endif
#ifdef W
#  if W == 8 && defined(Z_U8)
     typedef Z_U8 z_word_t;
#  elif defined(Z_U4)
#    undef W
#    define W 4
     typedef Z_U4 z_word_t;
#  else
#    undef W
#  endif
#endif

/*
  Determine if ARM CRC-32 hardware instructions are to be used, and how.

  If the compiler targets a processor that has the instructions, then it
  defines __ARM_FEATURE_CRC32 and they are used unconditionally, with only the
  code to use the hardware instructions compiled. Otherwise, if the compiler
  can produce CRC-32 instructions and if there is a way to ask the operating
  system if the instructions are present, then both the hardware and software
  CRC-32 calculations are compiled, and which to use is decided at run time.
  Otherwise the CRC-32 is calculated in software. If the target is big-endian,
  then the calculation is in software regardless.

  Define ARMCRC32 to compile the code to use the hardware instructions. Define
  ARMCRC32_WORD as the number of bytes to be processed by the word CRC-32
  instruction. Define ARMCRC32_RT to decide at run time whether or not to use
  the hardware instructions.

  Define ARMCRC32_ASK to ask the operating system if the feature is present, in
  which case define one of ARMCRC32_AUXV, ARMCRC32_ELFAUX, ARMCRC32_SYSCTL,
  ARMCRC32_NETBSD, or ARMCRC32_WIN to use, respectively, getauxval() on Linux,
  elf_aux_info() on FreeBSD and OpenBSD, sysctlbyname() on Apple systems, the
  ID registers from sysctlbyname() on NetBSD, or IsProcessorFeaturePresent() on
  Windows.

  The build can define Z_NO_ARMCRC32_RT to disallow the run-time check.
 */
#if (defined(__aarch64__) || defined(_M_ARM64)) && defined(Z_U8)
#  define ARMCRC32_WORD 8               /* eight bytes per instruction */
#elif defined(__arm__) && defined(Z_U4)
#  define ARMCRC32_WORD 4               /* four bytes per instruction */
#endif
#if defined(ARMCRC32_WORD) && (defined(__ARMEB__) || \
                               defined(__AARCH64EB__) || \
                               defined(__ARM_BIG_ENDIAN))
#  undef ARMCRC32_WORD
#endif

#ifdef ARMCRC32_WORD
#  ifndef Z_NO_ARMCRC32_RT
#    if defined(_MSC_VER) && !defined(__clang__)
#      if ARMCRC32_WORD == 8
#        define ARMCRC32_CC
#      endif
#    elif defined(__clang__)
#      define ARMCRC32_CC
#    elif defined(__GNUC__) && __GNUC__ >= (ARMCRC32_WORD == 8 ? 6 : 8)
#      define ARMCRC32_CC
#    endif
#    if defined(__linux__)
#      if defined(__has_include)
#        if __has_include(<sys/auxv.h>)
#          define ARMCRC32_AUXV         /* getauxval() */
#        endif
#      elif defined(__GLIBC__) && (__GLIBC__ > 2 || \
                                   (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 16))
#        define ARMCRC32_AUXV
#      endif
#      if defined(__ANDROID__) && defined(__ANDROID_API__) && \
          __ANDROID_API__ < 18
#        undef ARMCRC32_AUXV
#      endif
#      ifdef ARMCRC32_AUXV
#        define ARMCRC32_OS
#      endif
#    elif defined(__FreeBSD__) && __FreeBSD__ >= 12
#      define ARMCRC32_ELFAUX           /* elf_aux_info() */
#      define ARMCRC32_OS
#    elif defined(__OpenBSD__) && defined(__has_include)
#      if __has_include(<sys/auxv.h>)   /* OpenBSD 7.6 and later */
#        define ARMCRC32_ELFAUX         /* elf_aux_info() */
#        define ARMCRC32_OS
#      endif
#    elif defined(__NetBSD__) && ARMCRC32_WORD == 8
#      define ARMCRC32_NETBSD           /* sysctlbyname() for ID registers */
#      define ARMCRC32_OS
#    elif defined(__APPLE__)
#      define ARMCRC32_SYSCTL           /* sysctlbyname() */
#      define ARMCRC32_OS
#    elif defined(_WIN32)
#      define ARMCRC32_WIN              /* IsProcessorFeaturePresent() */
#      define ARMCRC32_OS
#    endif
#    if defined(ARMCRC32_CC) && defined(ARMCRC32_OS)
#      define ARMCRC32_CAN_ASK
#    endif
#  endif
#  ifdef __ARM_FEATURE_CRC32
#    define ARMCRC32                    /* just hardware instructions code */
#  elif defined(ARMCRC32_CAN_ASK)
#    define ARMCRC32                    /* hardware and software code, ask */
#    define ARMCRC32_RT                 /* the operating system at run time */
#  endif
   /* Define ARMPMULL to use the PMULL 64-bit carry-less multiply instruction,
      along with the CRC32 instruction, to combine the CRCs of interleaved
      strings. Define ARMPMULL_RT to check at run time if the instruction is
      available. The builder can define Z_NO_ARMPMULL to prevent the use of the
      instruction even if present. */
#  if defined(ARMCRC32) && ARMCRC32_WORD == 8 && !defined(Z_NO_ARMPMULL) && \
      !defined(Z_NO_ARMCRC32_BATCH)
#    if defined(__ARM_FEATURE_AES) || defined(__ARM_FEATURE_CRYPTO)
#      define ARMPMULL
#    elif defined(ARMCRC32_CAN_ASK)
#      define ARMPMULL
#      define ARMPMULL_RT
#    endif
#  endif
#  if defined(ARMCRC32_RT) || defined(ARMPMULL_RT)
#    define ARMCRC32_ASK                /* ask the operating system */
#  endif
#  if defined(ARMCRC32) && !defined(Z_NO_ARMCRC32_BATCH) && \
      (!defined(ARMPMULL) || defined(ARMPMULL_RT) || \
       defined(Z_TEST_ARMPMULL_ABSENT))
#    define ARMCRC32_SOFT               /* the software multiply is used */
#  endif
#endif

#if defined(W) && (!defined(ARMCRC32) || defined(ARMCRC32_RT) || \
                   defined(DYNAMIC_CRC_TABLE))
/*
  Swap the bytes in a z_word_t to convert between little and big endian. Any
  self-respecting compiler will optimize this to a single machine byte-swap
  instruction, if one is available. This assumes that word_t is either 32 bits
  or 64 bits.
 */
local z_word_t byte_swap(z_word_t word) {
#  if W == 8
    return
        (word & 0xff00000000000000) >> 56 |
        (word & 0xff000000000000) >> 40 |
        (word & 0xff0000000000) >> 24 |
        (word & 0xff00000000) >> 8 |
        (word & 0xff000000) << 8 |
        (word & 0xff0000) << 24 |
        (word & 0xff00) << 40 |
        (word & 0xff) << 56;
#  else   /* W == 4 */
    return
        (word & 0xff000000) >> 24 |
        (word & 0xff0000) >> 8 |
        (word & 0xff00) << 8 |
        (word & 0xff) << 24;
#  endif
}
#endif

#if (defined(ARMCRC32) && !defined(Z_NO_ARMCRC32_BATCH)) || defined(MAKECRCH)
/*
  batch[] is the count of bytes to process in each of the three interleaved
  streams. Stepping down through these counts provides the benefits of
  interleaving across whatever length is provided, while avoiding on-the-fly
  calculations of multipliers. Instead, constant multipliers in batch_t[] are
  used, which correspond to the fixed counts. The batch[] counts are each a bit
  less than a power of two, so as to spread out the use of new cache lines,
  with the amounts less (172 and 48) determined empirically on a Cortex-A72.
  The batch_t[] multipliers are calculated from these batch lengths. If these
  counts are changed, then MAKECRCH must be run to rebuild crc32.h.
 */
local const unsigned batch[] = {32592, 16208, 8016, 3920, 2000, 976, 464};
#  define BATCHES (sizeof(batch) / sizeof(batch[0]))
#endif

#ifdef DYNAMIC_CRC_TABLE
/* =========================================================================
 * Table of powers of x for combining CRC-32s, and tables for combining the
 * CRCs of the ARM batches, filled in by make_crc_table() below.
 */
   local z_crc_t FAR x2n_table[32];
#  if defined(ARMCRC32_SOFT) || defined(MAKECRCH)
   local Z_U8 FAR batch_t[BATCHES][2][16];
#  endif
#  if defined(ARMPMULL) || defined(MAKECRCH)
   local z_crc_t FAR batch_k[BATCHES][2];
#  endif
#else
/* =========================================================================
 * Tables for byte-wise and braided CRC-32 calculations, a table of powers of x
 * for combining CRC-32s, and tables for combining the CRCs of the ARM batches,
 * all made by make_crc_table().
 */
#  include "crc32.h"
#endif

/* CRC polynomial. */
#define POLY 0xedb88320         /* p(x) reflected, with x^32 implied */

/*
  Return a(x) multiplied by b(x) modulo p(x), where p(x) is the CRC polynomial,
  reflected. For speed, this requires that a not be zero.
 */
local uLong multmodp(uLong a, uLong b) {
    uLong m, p;

    m = (uLong)1 << 31;
    p = 0;
    for (;;) {
        if (a & m) {
            p ^= b;
            if ((a & (m - 1)) == 0)
                break;
        }
        m >>= 1;
        b = b & 1 ? (b >> 1) ^ POLY : b >> 1;
    }
    return p;
}

/*
  Return x^(n * 2^k) modulo p(x). Requires that x2n_table[] has been
  initialized. n must not be negative.
 */
local uLong x2nmodp(z_off64_t n, unsigned k) {
    uLong p;

    p = (uLong)1 << 31;             /* x^0 == 1 */
    while (n) {
        if (n & 1)
            p = multmodp(x2n_table[k & 31], p);
        n >>= 1;
        k++;
    }
    return p;
}

#ifdef DYNAMIC_CRC_TABLE
/* =========================================================================
 * Build the tables for byte-wise and braided CRC-32 calculations, and a table
 * of powers of x for combining CRC-32s.
 */
local z_crc_t FAR crc_table[256];
#ifdef W
   local z_word_t FAR crc_big_table[256];
   local z_crc_t FAR BRAID_TABLE(N)[W][256];
   local z_word_t FAR BRAID_BIG_TABLE(N)[W][256];
#  if NLOW != N
   local z_crc_t FAR BRAID_TABLE(NLOW)[W][256];
   local z_word_t FAR BRAID_BIG_TABLE(NLOW)[W][256];
#  endif
   local void braid(z_crc_t [][256], z_word_t [][256], int, int);
#endif
#ifdef MAKECRCH
   local void write_table(FILE *, const z_crc_t FAR *, int);
   local void write_table32hi(FILE *, const z_word_t FAR *, int);
   local void write_table64(FILE *, const z_word_t FAR *, int);
#endif /* MAKECRCH */

#if defined(ARMCRC32_SOFT) || defined(MAKECRCH)
/* Set t[0..15] to the carry-less products of 0..15 and k. t is a table entry
   in batch_t[] that represents the multiplier k. It is used in place of k to
   speed up the multiplication. */
local void batch_products(Z_U8 FAR *t, z_crc_t k) {
    unsigned i, j;

    for (i = 0; i < 16; i++) {
        t[i] = 0;
        for (j = 0; j < 4; j++)
            if ((i >> j) & 1)
                t[i] ^= (Z_U8)k << j;
    }
}
#endif

/* State for once(). */
local z_once_t made = Z_ONCE_INIT;

/*
  Generate tables for a byte-wise 32-bit CRC calculation on the polynomial:
  x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x+1.

  Polynomials over GF(2) are represented in binary, one bit per coefficient,
  with the lowest powers in the most significant bit. Then adding polynomials
  is just exclusive-or, and multiplying a polynomial by x is a right shift by
  one. If we call the above polynomial p, and represent a byte as the
  polynomial q, also with the lowest power in the most significant bit (so the
  byte 0xb1 is the polynomial x^7+x^3+x^2+1), then the CRC is (q*x^32) mod p,
  where a mod b means the remainder after dividing a by b.

  This calculation is done using the shift-register method of multiplying and
  taking the remainder. The register is initialized to zero, and for each
  incoming bit, x^32 is added mod p to the register if the bit is a one (where
  x^32 mod p is p+x^32 = x^26+...+1), and the register is multiplied mod p by x
  (which is shifting right by one and adding x^32 mod p if the bit shifted out
  is a one). We start with the highest power (least significant bit) of q and
  repeat for all eight bits of q.

  The table is simply the CRC of all possible eight bit values. This is all the
  information needed to generate CRCs on data a byte at a time for all
  combinations of CRC register values and incoming bytes.
 */

local void make_crc_table(void) {
    unsigned i, j, n;
    z_crc_t p;

    /* initialize the CRC of bytes tables */
    for (i = 0; i < 256; i++) {
        p = i;
        for (j = 0; j < 8; j++)
            p = p & 1 ? (p >> 1) ^ POLY : p >> 1;
        crc_table[i] = p;
#ifdef W
        crc_big_table[i] = byte_swap(p);
#endif
    }

    /* initialize the x^2^n mod p(x) table */
    p = (z_crc_t)1 << 30;         /* x^1 */
    x2n_table[0] = p;
    for (n = 1; n < 32; n++)
        x2n_table[n] = p = (z_crc_t)multmodp(p, p);

#if defined(ARMCRC32_SOFT) || defined(ARMPMULL) || defined(MAKECRCH)
    /* initialize the tables for combining the CRCs of the ARM batches */
    for (n = 0; n < BATCHES; n++) {
        z_crc_t k1 = (z_crc_t)x2nmodp(((z_off64_t)batch[n] << 3) - 33, 0);
        z_crc_t k2 = (z_crc_t)x2nmodp(((z_off64_t)batch[n] << 4) - 33, 0);
#  if defined(ARMPMULL) || defined(MAKECRCH)
        batch_k[n][0] = k1;
        batch_k[n][1] = k2;
#  endif
#  if defined(ARMCRC32_SOFT) || defined(MAKECRCH)
        batch_products(batch_t[n][0], k1);
        batch_products(batch_t[n][1], k2);
#  endif
    }
#endif

#ifdef W
    /* initialize the braiding tables -- needs x2n_table[] */
    braid(BRAID_TABLE(N), BRAID_BIG_TABLE(N), N, W);
#  if NLOW != N
    braid(BRAID_TABLE(NLOW), BRAID_BIG_TABLE(NLOW), NLOW, W);
#  endif
#endif

#ifdef MAKECRCH
    {
        /*
          The crc32.h header file contains tables for both 32-bit and 64-bit
          z_word_t's, and so requires a 64-bit type be available. In that case,
          z_word_t must be defined to be 64-bits. This code then also generates
          and writes out the tables for the case that z_word_t is 32 bits.
         */
#if !defined(W) || W != 8
#  error Need a 64-bit integer type in order to generate crc32.h.
#endif
        FILE *out;
        int k, n;
        z_crc_t ltl[8][256];
        z_word_t big[8][256];

        out = fopen("crc32.h", "w");
        if (out == NULL) return;

        /* write out little-endian CRC table to crc32.h */
        fprintf(out,
            "/* crc32.h -- tables for rapid CRC calculation\n"
            " * Generated automatically by crc32.c\n */\n"
            "\n"
            "local const z_crc_t FAR crc_table[] = {\n"
            "    ");
        write_table(out, crc_table, 256);
        fprintf(out,
            "};\n");

        /* write out big-endian CRC table for 64-bit z_word_t to crc32.h */
        fprintf(out,
            "\n"
            "#ifdef W\n"
            "\n"
            "#if W == 8\n"
            "\n"
            "local const z_word_t FAR crc_big_table[] = {\n"
            "    ");
        write_table64(out, crc_big_table, 256);
        fprintf(out,
            "};\n");

        /* write out big-endian CRC table for 32-bit z_word_t to crc32.h */
        fprintf(out,
            "\n"
            "#else /* W == 4 */\n"
            "\n"
            "local const z_word_t FAR crc_big_table[] = {\n"
            "    ");
        write_table32hi(out, crc_big_table, 256);
        fprintf(out,
            "};\n"
            "\n"
            "#endif\n");

        /* write out braid tables for each value of N, and of NLOW */
        for (n = 1; n <= 6; n++) {
            fprintf(out,
            "\n"
            "#if N == %d || NLOW == %d\n", n, n);

            /* compute braid tables for this N and 64-bit word_t */
            braid(ltl, big, n, 8);

            /* write out braid tables for 64-bit z_word_t to crc32.h */
            fprintf(out,
            "\n"
            "#if W == 8\n"
            "\n"
            "local const z_crc_t FAR crc_braid%d_table[][256] = {\n", n);
            for (k = 0; k < 8; k++) {
                fprintf(out, "   {");
                write_table(out, ltl[k], 256);
                fprintf(out, "}%s", k < 7 ? ",\n" : "");
            }
            fprintf(out,
            "};\n"
            "\n"
            "local const z_word_t FAR crc_braid%d_big_table[][256] = {\n", n);
            for (k = 0; k < 8; k++) {
                fprintf(out, "   {");
                write_table64(out, big[k], 256);
                fprintf(out, "}%s", k < 7 ? ",\n" : "");
            }
            fprintf(out,
            "};\n");

            /* compute braid tables for this N and 32-bit word_t */
            braid(ltl, big, n, 4);

            /* write out braid tables for 32-bit z_word_t to crc32.h */
            fprintf(out,
            "\n"
            "#else /* W == 4 */\n"
            "\n"
            "local const z_crc_t FAR crc_braid%d_table[][256] = {\n", n);
            for (k = 0; k < 4; k++) {
                fprintf(out, "   {");
                write_table(out, ltl[k], 256);
                fprintf(out, "}%s", k < 3 ? ",\n" : "");
            }
            fprintf(out,
            "};\n"
            "\n"
            "local const z_word_t FAR crc_braid%d_big_table[][256] = {\n", n);
            for (k = 0; k < 4; k++) {
                fprintf(out, "   {");
                write_table32hi(out, big[k], 256);
                fprintf(out, "}%s", k < 3 ? ",\n" : "");
            }
            fprintf(out,
            "};\n"
            "\n"
            "#endif\n"
            "\n"
            "#endif\n");
        }
        fprintf(out,
            "\n"
            "#endif\n");

        /* write out zeros operator table to crc32.h */
        fprintf(out,
            "\n"
            "local const z_crc_t FAR x2n_table[] = {\n"
            "    ");
        write_table(out, x2n_table, 32);
        fprintf(out,
            "};\n");

        /* write out the tables for combining the CRCs of the ARM batches */
        fprintf(out,
            "\n"
            "#ifdef ARMCRC32_SOFT\n"
            "\n"
            "local const Z_U8 FAR batch_t[][2][16] = {\n");
        for (n = 0; n < (int)BATCHES; n++) {
            fprintf(out, "   {{");
            write_table64(out, batch_t[n][0], 16);
            fprintf(out, "},\n    {");
            write_table64(out, batch_t[n][1], 16);
            fprintf(out, "}}%s", n < (int)BATCHES - 1 ? ",\n" : "");
        }
        fprintf(out,
            "};\n"
            "\n"
            "#endif\n");
        fprintf(out,
            "\n"
            "#ifdef ARMPMULL\n"
            "\n"
            "local const z_crc_t FAR batch_k[][2] = {\n");
        for (n = 0; n < (int)BATCHES; n++)
            fprintf(out, "    {0x%08lx, 0x%08lx}%s",
                    (unsigned long)batch_k[n][0], (unsigned long)batch_k[n][1],
                    n < (int)BATCHES - 1 ? ",\n" : "\n");
        fprintf(out,
            "};\n"
            "\n"
            "#endif\n");
        fclose(out);
    }
#endif /* MAKECRCH */
}

#ifdef MAKECRCH

/*
   Write the 32-bit values in table[0..k-1] to out, five per line in
   hexadecimal separated by commas.
 */
local void write_table(FILE *out, const z_crc_t FAR *table, int k) {
    int n;

    for (n = 0; n < k; n++)
        fprintf(out, "%s0x%08lx%s", n == 0 || n % 5 ? "" : "    ",
                (unsigned long)(table[n]),
                n == k - 1 ? "" : (n % 5 == 4 ? ",\n" : ", "));
}

/*
   Write the high 32-bits of each value in table[0..k-1] to out, five per line
   in hexadecimal separated by commas.
 */
local void write_table32hi(FILE *out, const z_word_t FAR *table, int k) {
    int n;

    for (n = 0; n < k; n++)
        fprintf(out, "%s0x%08lx%s", n == 0 || n % 5 ? "" : "    ",
                (unsigned long)(table[n] >> 32),
                n == k - 1 ? "" : (n % 5 == 4 ? ",\n" : ", "));
}

/*
  Write the 64-bit values in table[0..k-1] to out, three per line in
  hexadecimal separated by commas. This assumes that if there is a 64-bit
  type, then there is also a long long integer type, and it is at least 64
  bits. If not, then the type cast and format string can be adjusted
  accordingly.
 */
local void write_table64(FILE *out, const z_word_t FAR *table, int k) {
    int n;

    for (n = 0; n < k; n++)
        fprintf(out, "%s0x%016llx%s", n == 0 || n % 3 ? "" : "    ",
                (unsigned long long)(table[n]),
                n == k - 1 ? "" : (n % 3 == 2 ? ",\n" : ", "));
}

/* Actually do the deed. */
int main(void) {
    make_crc_table();
    return 0;
}

#endif /* MAKECRCH */

#ifdef W
/*
  Generate the little and big-endian braid tables for the given n and z_word_t
  size w. Each array must have room for w blocks of 256 elements.
 */
local void braid(z_crc_t ltl[][256], z_word_t big[][256], int n, int w) {
    int k;
    z_crc_t i, p, q;
    for (k = 0; k < w; k++) {
        p = (z_crc_t)x2nmodp((n * w + 3 - k) << 3, 0);
        ltl[k][0] = 0;
        big[w - 1 - k][0] = 0;
        for (i = 1; i < 256; i++) {
            ltl[k][i] = q = (z_crc_t)multmodp(i << 24, p);
            big[w - 1 - k][i] = byte_swap(q);
        }
    }
}
#endif

#endif /* DYNAMIC_CRC_TABLE */

/* =========================================================================
 * This function can be used by asm versions of crc32(), and to force the
 * generation of the CRC tables in a threaded application.
 */
const z_crc_t FAR * ZEXPORT get_crc_table(void) {
#ifdef DYNAMIC_CRC_TABLE
    z_once(&made, make_crc_table);
#endif /* DYNAMIC_CRC_TABLE */
    return (const z_crc_t FAR *)crc_table;
}

#ifdef ARMCRC32_ASK
/*
  Ask the operating system whether the processor has the CRC32 instructions, as
  well as the 64-bit carry-less multiply, PMULL. The capability bits are fixed
  by the Linux ABI (FreeBSD uses the same), so they are defined here, in case
  the headers are too old to have them.
 */
#define ARM_HAVE_CRC32 1
#define ARM_HAVE_PMULL 2
#if defined(ARMCRC32_AUXV) || defined(ARMCRC32_ELFAUX)
#  include <sys/auxv.h>
#  ifndef AT_HWCAP
#    define AT_HWCAP 16
#  endif
#  ifndef AT_HWCAP2
#    define AT_HWCAP2 26
#  endif
#  if ARMCRC32_WORD == 8
#    define ARM_AT AT_HWCAP
#    define ARM_BIT_CRC32 7         /* HWCAP_CRC32 */
#    define ARM_BIT_PMULL 4         /* HWCAP_PMULL */
#  else
#    define ARM_AT AT_HWCAP2
#    define ARM_BIT_CRC32 4         /* HWCAP2_CRC32 */
#    define ARM_BIT_PMULL 1         /* HWCAP2_PMULL */
#  endif
#elif defined(ARMCRC32_SYSCTL)
#  include <sys/types.h>
#  include <sys/sysctl.h>
#elif defined(ARMCRC32_NETBSD)
#  include <sys/types.h>
#  include <sys/sysctl.h>
#  include <aarch64/armreg.h>       /* for struct aarch64_sysctl_cpu_id */
#elif defined(ARMCRC32_WIN) && !defined(_WINBASE_)
__declspec(dllimport) int __stdcall IsProcessorFeaturePresent(unsigned long);
#endif
#ifndef PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE
#  define PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE 30
#endif
#ifndef PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE
#  define PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE 31
#endif

#ifdef ARMCRC32_SYSCTL
local int arm_sysctl(const char *name) {
    int have = 0;
    size_t size = sizeof(have);
    return sysctlbyname(name, &have, &size, NULL, 0) == 0 && have;
}
#endif

/* Ask for the capabilities identified by the bits set in want. Return those
   bits set if the respective capapility is present. */
local int arm_probe(int want) {
    int caps = 0;
#if defined(ARMCRC32_AUXV) || defined(ARMCRC32_ELFAUX)
    unsigned long bits;
#  ifdef ARMCRC32_AUXV
    bits = getauxval(ARM_AT);
#  else
    bits = 0;
    if (elf_aux_info(ARM_AT, &bits, sizeof(bits)) != 0)
        bits = 0;
#  endif
    if ((bits >> ARM_BIT_CRC32) & 1)
        caps |= ARM_HAVE_CRC32;
    if ((bits >> ARM_BIT_PMULL) & 1)
        caps |= ARM_HAVE_PMULL;
    (void)want;
#elif defined(ARMCRC32_SYSCTL)
#  ifdef ARMCRC32_RT
    if ((want & ARM_HAVE_CRC32) && arm_sysctl("hw.optional.armv8_crc32"))
        caps |= ARM_HAVE_CRC32;
#  endif
#  ifdef ARMPMULL_RT
    if ((want & ARM_HAVE_PMULL) && arm_sysctl("hw.optional.arm.FEAT_PMULL"))
        caps |= ARM_HAVE_PMULL;
#  endif
#elif defined(ARMCRC32_NETBSD)
    /* ID_AA64ISAR0_EL1 of cpu0: CRC32 in bits 16..19 (1 if present), and AES
       in bits 4..7 (2 if PMULL is present) */
    struct aarch64_sysctl_cpu_id id;
    size_t size = sizeof(id);
    if (sysctlbyname("machdep.cpu0.cpu_id", &id, &size, NULL, 0) == 0 &&
        size == sizeof(id)) {
        if (((id.ac_aa64isar0 >> 16) & 15) >= 1)
            caps |= ARM_HAVE_CRC32;
        if (((id.ac_aa64isar0 >> 4) & 15) >= 2)
            caps |= ARM_HAVE_PMULL;
    }
    (void)want;
#else
    if ((want & ARM_HAVE_CRC32) &&
        IsProcessorFeaturePresent(PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE))
        caps |= ARM_HAVE_CRC32;
    if ((want & ARM_HAVE_PMULL) &&
        IsProcessorFeaturePresent(PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE))
        caps |= ARM_HAVE_PMULL;
#endif
    return caps;
}

/* Z_TEST_ARMCRC32_ABSENT and Z_TEST_ARMPMULL_ABSENT ask, but answer no, to
   test the code used without them, and to measure the cost of asking. */
local int arm_ask(int want) {
    volatile int caps = arm_probe(want);
#ifdef Z_TEST_ARMCRC32_ABSENT
    caps &= ~ARM_HAVE_CRC32;
#endif
#ifdef Z_TEST_ARMPMULL_ABSENT
    caps &= ~ARM_HAVE_PMULL;
#endif
    return caps;
}

/*
  If lock-free C11 atomics are available, arm_probe() is only called once and
  the answer is cached. If not, then arm_probe() is called for every crc32()
  call above a data-length threshold (Z_ARMCRC32_PROBE_LEN).
 */
#if !defined(Z_ARMCRC32_NOCACHE) && defined(__STDC__) && \
    defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && \
    !defined(__STDC_NO_ATOMICS__)
#  include <stdatomic.h>
#  if ATOMIC_INT_LOCK_FREE == 2
#    define ARMCRC32_CACHE
#  endif
#endif

#ifdef ARMCRC32_CACHE
local atomic_int arm_known;

/* Ask the question and cache the answer. */
local int arm_learn(void) {
    int caps = arm_ask(ARM_HAVE_CRC32 | ARM_HAVE_PMULL) | 4;
    atomic_store_explicit(&arm_known, caps, memory_order_relaxed);
    return caps;
}

/* If there are lock-free C11 atomics, then the cache is filled by asking for
   both of the capabilities, which is returned. Otherwise only the capabilities
   with set bits in want are asked for. The return value has bits set for the
   requested and available capabilities. The two bits are ARM_HAVE_CRC32 and
   ARM_HAVE_PMULL. */
local inline int arm_caps(int want) {
    int caps = atomic_load_explicit(&arm_known, memory_order_relaxed);
    (void)want;
    if (caps == 0)
        caps = arm_learn();
    return caps & 3;
}
#else
#  define arm_caps(want) arm_ask(want)
#endif
#endif /* ARMCRC32_ASK */

/* =========================================================================
 * Use the ARM CRC32 instructions, if available. This computes the CRC about
 * ten times faster than the braided calculation.
 */
#ifdef ARMCRC32

/* Define ARMCRC32_ATTR to attribute to the function with hardware CRC
   instructions, as needed to tell the compiler it can produce them. */
#ifndef ARMCRC32_RT
#  define ARMCRC32_ATTR
#elif defined(Z_ARMCRC32_TARGET)
#  define ARMCRC32_ATTR __attribute__((target(Z_ARMCRC32_TARGET)))
#elif defined(_MSC_VER) && !defined(__clang__)
#  define ARMCRC32_ATTR
#elif defined(__clang__)
#  define ARMCRC32_ATTR __attribute__((target("crc")))
#elif ARMCRC32_WORD == 8
#  define ARMCRC32_ATTR __attribute__((target("+crc")))
#elif defined(__ARM_PCS_VFP)
   /* gcc needs a floating-point unit for the hard-float calling convention */
#  define ARMCRC32_ATTR __attribute__((target("arch=armv8-a+crc+simd")))
#else
#  define ARMCRC32_ATTR __attribute__((target("arch=armv8-a+crc")))
#endif
#ifdef ARMPMULL
#  if !defined(ARMCRC32_RT) && !defined(ARMPMULL_RT)
#    define ARMPMULL_ATTR
#  elif defined(Z_ARMPMULL_TARGET)
#    define ARMPMULL_ATTR __attribute__((target(Z_ARMPMULL_TARGET)))
#  elif defined(_MSC_VER) && !defined(__clang__)
#    define ARMPMULL_ATTR
#  elif defined(__clang__)
#    define ARMPMULL_ATTR __attribute__((target("crc,crypto")))
#  else
#    define ARMPMULL_ATTR __attribute__((target("+crc+crypto")))
#  endif
#endif

/* Define CRC32W() to operate on a word and CRC32B() to operate on a byte. */
#if ARMCRC32_WORD == 8
typedef Z_U8 arm_word_t;
#  if defined(_MSC_VER) && !defined(__clang__)
#    include <intrin.h>
#    ifdef ARMPMULL
#      include <arm64_neon.h>
#    endif
#    define CRC32B(c, b) (c = __crc32b((unsigned)(c), (unsigned char)(b)))
#    define CRC32W(c, w) (c = __crc32d((unsigned)(c), w))
#  else
#    define CRC32B(c, b) \
        __asm__ volatile("crc32b %w0, %w0, %w1" : "+r"(c) : "r"(b))
#    define CRC32W(c, w) \
        __asm__ volatile("crc32x %w0, %w0, %x1" : "+r"(c) : "r"(w))
#  endif
   /* set c to p times x^32, modulo the CRC polynomial */
#  define CRC32_REDUCE(c, p) do { \
        c = 0; \
        CRC32W(c, p); \
    } while (0)
#else
typedef Z_U4 arm_word_t;
#  define CRC32B(c, b) (c = __builtin_arm_crc32b((unsigned)(c), \
                                                 (unsigned char)(b)))
#  define CRC32W(c, w) (c = __builtin_arm_crc32w((unsigned)(c), w))
    /* set c to p times x^32, modulo the CRC polynomial */
#  define CRC32_REDUCE(c, p) do { \
        c = 0; \
        CRC32W(c, (Z_U4)(p)); \
        CRC32W(c, (Z_U4)((p) >> 32)); \
    } while (0)
#endif

/*
  The CRC is computed on three interleaved streams of words at once, to realize
  a throughput of one CRC instruction per cycle, where each can have a latency
  of up to three cycles. Do successively smaller batches so that any length
  that is at least the smallest batch size will start with one of them. Below
  that, do one word at a time in serial, and then one byte at a time for the
  last few. The CRC from the first group is the CRC of all of the data through
  that group. The other two CRCs are of those groups in isolation. Those two
  are combined with the first CRC by multiplying them by the appropriate x^n
  modulo the CRC polynomial, and then exclusive-oring the three together.

  If Z_NO_ARMCRC32_BATCH is defined, then the interleaving is skipped, and the
  CRC is calculated a word at a time. Some ARM processors pipeline that to one
  cycle per CRC instruction, so nothing is gained by the interleaving, and in
  fact a little bit is lost due to the interleaving overhead. This is the case
  for the Cortex-A76 and Cortex-A720 for example. The builder can measure this
  on their processor and compiler and elect to define it if its faster.
 */
#ifndef Z_NO_ARMCRC32_BATCH
#ifdef ARMCRC32_SOFT
/* Return the carry-less product of c and and the value represented by the
   table t from batch_t[]. The table speeds up the multiplication by doing
   four bits of c at a time. */
local Z_U8 clmul_soft(z_crc_t c, const Z_U8 *t) {
    Z_U8 p;
    int j;

    p = 0;
    for (j = 0; j < 32; j += 4)
        p ^= t[(c >> j) & 15] << j;
    return p;
}

/* Return the CRC through three sequential streams of batch[k] bytes, given
   crc, the running CRC through the first stream, and crc1 and crc2, the CRCs
   of the second and third streams each starting from zero. */
local ARMCRC32_ATTR uLong batch_soft(uLong crc, uLong crc1, uLong crc2,
                                     unsigned k) {
    Z_U8 p;
    uLong c;

    p = clmul_soft((z_crc_t)crc, batch_t[k][1]) ^
        clmul_soft((z_crc_t)crc1, batch_t[k][0]);
    CRC32_REDUCE(c, p);
    return c ^ crc2;
}
#endif

#ifdef ARMPMULL
local ARMPMULL_ATTR uLong batch_pmull(uLong crc, uLong crc1, uLong crc2,
                                      unsigned k) {
    Z_U8 p;
    uLong c;
#if defined(_MSC_VER) && !defined(__clang__)
    __n64 a0, a1, b0, b1;

    a0.n64_u64[0] = crc;
    b0.n64_u64[0] = batch_k[k][1];
    a1.n64_u64[0] = crc1;
    b1.n64_u64[0] = batch_k[k][0];
    p = vmull_p64(a0, b0).n128_u64[0] ^ vmull_p64(a1, b1).n128_u64[0];
#else
    Z_U8 a, b;

    __asm__("pmull %0.1q, %1.1d, %2.1d" : "=w"(a)
            : "w"((Z_U8)crc), "w"((Z_U8)batch_k[k][1]));
    __asm__("pmull %0.1q, %1.1d, %2.1d" : "=w"(b)
            : "w"((Z_U8)crc1), "w"((Z_U8)batch_k[k][0]));
    p = a ^ b;
#endif
    CRC32_REDUCE(c, p);
    return c ^ crc2;
}
#endif

#ifdef ARMPMULL
/* Return true to use the ARM carry-less multiply instruction. */
local int arm_pmull(void) {
#if defined(Z_TEST_ARMPMULL_ABSENT)
    return 0;
#elif !defined(ARMPMULL_RT)
    return 1;
#else
    return (arm_caps(ARM_HAVE_PMULL) & ARM_HAVE_PMULL) != 0;
#endif
}
#endif
#endif /* !Z_NO_ARMCRC32_BATCH */

/* Return the CRC-32 of len bytes at buf using ARM hardware instructions. */
local ARMCRC32_ATTR uLong crc32_arm(uLong crc, const unsigned char FAR *buf,
                                    z_size_t len) {
#ifdef ARMPMULL
    int pm;
#endif
    uLong val;
#ifndef Z_NO_ARMCRC32_BATCH
    uLong crc1, crc2;
    z_size_t n;
    unsigned k;
#endif
    const arm_word_t *word;
    arm_word_t val0, val1, val2;
    z_size_t i;
    z_size_t num;

    /* Return initial CRC, if requested. */
    if (buf == Z_NULL) return 0;

#ifdef DYNAMIC_CRC_TABLE
    z_once(&made, make_crc_table);
#endif /* DYNAMIC_CRC_TABLE */

    /* Pre-condition the CRC */
    crc = (~crc) & 0xffffffff;

    /* Compute the CRC up to a word boundary. */
    while (len && ((z_size_t)buf & (ARMCRC32_WORD - 1)) != 0) {
        len--;
        val = *buf++;
        CRC32B(crc, val);
    }

    /* Prepare to compute the CRC on full words word[0..num-1]. */
    word = (arm_word_t const *)buf;
    num = len / ARMCRC32_WORD;
    len &= ARMCRC32_WORD - 1;

#ifndef Z_NO_ARMCRC32_BATCH
    /* Do batches of three interleaved CRCs, from the largest size down. */
    if (num >= 3 * (batch[BATCHES - 1] / ARMCRC32_WORD)) {
#ifdef ARMPMULL
        pm = arm_pmull();
#endif
        for (k = 0; k < BATCHES; k++) {
            n = batch[k] / ARMCRC32_WORD;
            while (num >= 3 * n) {
                crc1 = 0;
                crc2 = 0;
                for (i = 0; i < n; i++) {
                    val0 = word[i];
                    val1 = word[i + n];
                    val2 = word[i + 2 * n];
                    CRC32W(crc, val0);
                    CRC32W(crc1, val1);
                    CRC32W(crc2, val2);
                }
                word += 3 * n;
                num -= 3 * n;
#ifdef ARMPMULL
                if (pm)
                    crc = batch_pmull(crc, crc1, crc2, k);
#  ifdef ARMCRC32_SOFT
                else
                    crc = batch_soft(crc, crc1, crc2, k);
#  endif
#else
                crc = batch_soft(crc, crc1, crc2, k);
#endif
            }
        }
    }
#endif

    /* Compute the CRC on any remaining words, four at a time, and then one at
       a time. The four-at-a-time loop reduces the loop overhead and avoids
       a code alignment issue observed on one processor that would randomly
       halve the speed of the single-word loop. */
    for (i = 3; i < num; i += 4) {
        val0 = word[i - 3];
        val1 = word[i - 2];
        val2 = word[i - 1];
        CRC32W(crc, val0);
        CRC32W(crc, val1);
        CRC32W(crc, val2);
        val0 = word[i];
        CRC32W(crc, val0);
    }
    for (i -= 3; i < num; i++) {
        val0 = word[i];
        CRC32W(crc, val0);
    }
    word += num;

    /* Complete the CRC on any remaining bytes. */
    buf = (const unsigned char FAR *)word;
    while (len) {
        len--;
        val = *buf++;
        CRC32B(crc, val);
    }

    /* Return the CRC, post-conditioned. */
    return crc ^ 0xffffffff;
}

#ifndef ARMCRC32_RT
/* ========================================================================= */
uLong ZEXPORT crc32_z(uLong crc, const unsigned char FAR *buf, z_size_t len) {
    return crc32_arm(crc, buf, len);
}
#endif

#endif /* ARMCRC32 */

#if !defined(ARMCRC32) || defined(ARMCRC32_RT)

#ifdef W

/*
  Return the CRC of the W bytes in the word_t data, taking the
  least-significant byte of the word as the first byte of data, without any pre
  or post conditioning. This is used to combine the CRCs of each braid.
 */
local z_crc_t crc_word(z_word_t data) {
    int k;
    for (k = 0; k < W; k++)
        data = (data >> 8) ^ crc_table[data & 0xff];
    return (z_crc_t)data;
}

local z_word_t crc_word_big(z_word_t data) {
    int k;
    for (k = 0; k < W; k++)
        data = (data << 8) ^
            crc_big_table[(data >> ((W - 1) << 3)) & 0xff];
    return data;
}

/*
  The CRC of one braid's word w, using braid table t: the table entries for
  its W bytes, combined in pairs. This is written out rather than looped over
  the bytes, since gcc at -O2 does not unroll such a loop, which leaves the
  shift and the table offset to be computed for each byte, and is up to twice
  as slow.
 */
#define CRCB(t, w, k) t[k][((w) >> ((k) << 3)) & 0xff]
#if W == 8
#  define CRCWORD(t, w) (((CRCB(t, w, 0) ^ CRCB(t, w, 1)) ^ \
                          (CRCB(t, w, 2) ^ CRCB(t, w, 3))) ^ \
                         ((CRCB(t, w, 4) ^ CRCB(t, w, 5)) ^ \
                          (CRCB(t, w, 6) ^ CRCB(t, w, 7))))
#else
#  define CRCWORD(t, w) ((CRCB(t, w, 0) ^ CRCB(t, w, 1)) ^ \
                         (CRCB(t, w, 2) ^ CRCB(t, w, 3)))
#endif

#ifdef BRAID_CUT
/*
  Return the CRC of buf[0..len-1] starting with crc, computed on NLOW braids.
  crc is pre-conditioned, and the result is not post-conditioned. crc32_z()
  uses this for lengths less than BRAID_CUT, and N braids for longer lengths.
  The N braids are in crc32_z() itself, since compiled there, clang does not
  spill a braid's CRC to the stack in the loop on x86-64 for N=5, as it did
  when the N braids were here. A compiler can be expected to inline this, so
  that short lengths do not incur a function call.
 */
local uLong crc32_braid(uLong crc, const unsigned char FAR *buf,
                        z_size_t len) {
    /* If provided enough bytes, do a braided CRC calculation. */
    if (len >= NLOW * W + W - 1) {
        z_size_t blks;
        z_word_t const *words;
        unsigned endian;

        /* Compute the CRC up to a z_word_t boundary. */
        while (len && ((z_size_t)buf & (W - 1)) != 0) {
            len--;
            crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
        }

        /* Compute the CRC on as many NLOW z_word_t blocks as are available. */
        blks = len / (NLOW * W);
        len -= blks * NLOW * W;
        words = (z_word_t const *)buf;

        /* Do endian check at execution time instead of compile time, since ARM
           processors can change the endianness at execution time. If the
           compiler knows what the endianness will be, it can optimize out the
           check and the unused branch. */
        endian = 1;
        if (*(unsigned char *)&endian) {
            /* Little endian. */

            z_crc_t crc0;
            z_word_t word0;
#if NLOW > 1
            z_crc_t crc1;
            z_word_t word1;
#if NLOW > 2
            z_crc_t crc2;
            z_word_t word2;
#if NLOW > 3
            z_crc_t crc3;
            z_word_t word3;
#if NLOW > 4
            z_crc_t crc4;
            z_word_t word4;
#if NLOW > 5
            z_crc_t crc5;
            z_word_t word5;
#endif
#endif
#endif
#endif
#endif

            /* Initialize the CRC for each braid. */
            crc0 = (z_crc_t)crc;
#if NLOW > 1
            crc1 = 0;
#if NLOW > 2
            crc2 = 0;
#if NLOW > 3
            crc3 = 0;
#if NLOW > 4
            crc4 = 0;
#if NLOW > 5
            crc5 = 0;
#endif
#endif
#endif
#endif
#endif

            /*
              Process the first blks-1 blocks, computing the CRCs on each braid
              independently.
             */
            while (--blks) {
                /* Load the word for each braid into registers. */
                word0 = crc0 ^ words[0];
#if NLOW > 1
                word1 = crc1 ^ words[1];
#if NLOW > 2
                word2 = crc2 ^ words[2];
#if NLOW > 3
                word3 = crc3 ^ words[3];
#if NLOW > 4
                word4 = crc4 ^ words[4];
#if NLOW > 5
                word5 = crc5 ^ words[5];
#endif
#endif
#endif
#endif
#endif
                words += NLOW;

                /* Compute and update the CRC for each word. */
                crc0 = CRCWORD(BRAID_TABLE(NLOW), word0);
#if NLOW > 1
                crc1 = CRCWORD(BRAID_TABLE(NLOW), word1);
#if NLOW > 2
                crc2 = CRCWORD(BRAID_TABLE(NLOW), word2);
#if NLOW > 3
                crc3 = CRCWORD(BRAID_TABLE(NLOW), word3);
#if NLOW > 4
                crc4 = CRCWORD(BRAID_TABLE(NLOW), word4);
#if NLOW > 5
                crc5 = CRCWORD(BRAID_TABLE(NLOW), word5);
#endif
#endif
#endif
#endif
#endif
            }

            /*
              Process the last block, combining the CRCs of the NLOW braids at
              the same time.
             */
            crc = crc_word(crc0 ^ words[0]);
#if NLOW > 1
            crc = crc_word(crc1 ^ words[1] ^ crc);
#if NLOW > 2
            crc = crc_word(crc2 ^ words[2] ^ crc);
#if NLOW > 3
            crc = crc_word(crc3 ^ words[3] ^ crc);
#if NLOW > 4
            crc = crc_word(crc4 ^ words[4] ^ crc);
#if NLOW > 5
            crc = crc_word(crc5 ^ words[5] ^ crc);
#endif
#endif
#endif
#endif
#endif
            words += NLOW;
        }
        else {
            /* Big endian. */

            z_word_t crc0, word0, comb;
#if NLOW > 1
            z_word_t crc1, word1;
#if NLOW > 2
            z_word_t crc2, word2;
#if NLOW > 3
            z_word_t crc3, word3;
#if NLOW > 4
            z_word_t crc4, word4;
#if NLOW > 5
            z_word_t crc5, word5;
#endif
#endif
#endif
#endif
#endif

            /* Initialize the CRC for each braid. */
            crc0 = byte_swap(crc);
#if NLOW > 1
            crc1 = 0;
#if NLOW > 2
            crc2 = 0;
#if NLOW > 3
            crc3 = 0;
#if NLOW > 4
            crc4 = 0;
#if NLOW > 5
            crc5 = 0;
#endif
#endif
#endif
#endif
#endif

            /*
              Process the first blks-1 blocks, computing the CRCs on each braid
              independently.
             */
            while (--blks) {
                /* Load the word for each braid into registers. */
                word0 = crc0 ^ words[0];
#if NLOW > 1
                word1 = crc1 ^ words[1];
#if NLOW > 2
                word2 = crc2 ^ words[2];
#if NLOW > 3
                word3 = crc3 ^ words[3];
#if NLOW > 4
                word4 = crc4 ^ words[4];
#if NLOW > 5
                word5 = crc5 ^ words[5];
#endif
#endif
#endif
#endif
#endif
                words += NLOW;

                /* Compute and update the CRC for each word. */
                crc0 = CRCWORD(BRAID_BIG_TABLE(NLOW), word0);
#if NLOW > 1
                crc1 = CRCWORD(BRAID_BIG_TABLE(NLOW), word1);
#if NLOW > 2
                crc2 = CRCWORD(BRAID_BIG_TABLE(NLOW), word2);
#if NLOW > 3
                crc3 = CRCWORD(BRAID_BIG_TABLE(NLOW), word3);
#if NLOW > 4
                crc4 = CRCWORD(BRAID_BIG_TABLE(NLOW), word4);
#if NLOW > 5
                crc5 = CRCWORD(BRAID_BIG_TABLE(NLOW), word5);
#endif
#endif
#endif
#endif
#endif
            }

            /*
              Process the last block, combining the CRCs of the NLOW braids at
              the same time.
             */
            comb = crc_word_big(crc0 ^ words[0]);
#if NLOW > 1
            comb = crc_word_big(crc1 ^ words[1] ^ comb);
#if NLOW > 2
            comb = crc_word_big(crc2 ^ words[2] ^ comb);
#if NLOW > 3
            comb = crc_word_big(crc3 ^ words[3] ^ comb);
#if NLOW > 4
            comb = crc_word_big(crc4 ^ words[4] ^ comb);
#if NLOW > 5
            comb = crc_word_big(crc5 ^ words[5] ^ comb);
#endif
#endif
#endif
#endif
#endif
            words += NLOW;
            crc = byte_swap(comb);
        }

        /*
          Update the pointer to the remaining bytes to process.
         */
        buf = (unsigned char const *)words;
    }

    /* Complete the computation of the CRC on any remaining bytes. */
    while (len >= 8) {
        len -= 8;
        crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
        crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
        crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
        crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
        crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
        crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
        crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
        crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
    }
    while (len) {
        len--;
        crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
    }

    return crc;
}
#endif

#endif

/* ========================================================================= */
#ifdef ARMCRC32_RT
local uLong crc32_soft(uLong crc, const unsigned char FAR *buf,
                       z_size_t len) {
#else
uLong ZEXPORT crc32_z(uLong crc, const unsigned char FAR *buf, z_size_t len) {
#endif
    /* Return initial CRC, if requested. */
    if (buf == Z_NULL) return 0;

#ifdef DYNAMIC_CRC_TABLE
    z_once(&made, make_crc_table);
#endif /* DYNAMIC_CRC_TABLE */

    /* Pre-condition the CRC */
    crc = (~crc) & 0xffffffff;

#ifdef W

    /* If provided enough bytes, do a braided CRC calculation. */
    if (len >= NMIN * W + W - 1) {
        z_size_t blks;
        z_word_t const *words;
        unsigned endian;

#ifdef BRAID_CUT
        /* For shorter lengths, use NLOW braids. */
        if (len < BRAID_CUT || len < N * W + W - 1)
            return crc32_braid(crc, buf, len) ^ 0xffffffff;
#endif

        /* Compute the CRC up to a z_word_t boundary. */
        while (len && ((z_size_t)buf & (W - 1)) != 0) {
            len--;
            crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
        }

        /* Compute the CRC on as many N z_word_t blocks as are available. */
        blks = len / (N * W);
        len -= blks * N * W;
        words = (z_word_t const *)buf;

        /* Do endian check at execution time instead of compile time, since ARM
           processors can change the endianness at execution time. If the
           compiler knows what the endianness will be, it can optimize out the
           check and the unused branch. */
        endian = 1;
        if (*(unsigned char *)&endian) {
            /* Little endian. */

            z_crc_t crc0;
            z_word_t word0;
#if N > 1
            z_crc_t crc1;
            z_word_t word1;
#if N > 2
            z_crc_t crc2;
            z_word_t word2;
#if N > 3
            z_crc_t crc3;
            z_word_t word3;
#if N > 4
            z_crc_t crc4;
            z_word_t word4;
#if N > 5
            z_crc_t crc5;
            z_word_t word5;
#endif
#endif
#endif
#endif
#endif

            /* Initialize the CRC for each braid. */
            crc0 = (z_crc_t)crc;
#if N > 1
            crc1 = 0;
#if N > 2
            crc2 = 0;
#if N > 3
            crc3 = 0;
#if N > 4
            crc4 = 0;
#if N > 5
            crc5 = 0;
#endif
#endif
#endif
#endif
#endif

            /*
              Process the first blks-1 blocks, computing the CRCs on each braid
              independently.
             */
            while (--blks) {
                /* Load the word for each braid into registers. */
                word0 = crc0 ^ words[0];
#if N > 1
                word1 = crc1 ^ words[1];
#if N > 2
                word2 = crc2 ^ words[2];
#if N > 3
                word3 = crc3 ^ words[3];
#if N > 4
                word4 = crc4 ^ words[4];
#if N > 5
                word5 = crc5 ^ words[5];
#endif
#endif
#endif
#endif
#endif
                words += N;

                /* Compute and update the CRC for each word. */
                crc0 = CRCWORD(BRAID_TABLE(N), word0);
#if N > 1
                crc1 = CRCWORD(BRAID_TABLE(N), word1);
#if N > 2
                crc2 = CRCWORD(BRAID_TABLE(N), word2);
#if N > 3
                crc3 = CRCWORD(BRAID_TABLE(N), word3);
#if N > 4
                crc4 = CRCWORD(BRAID_TABLE(N), word4);
#if N > 5
                crc5 = CRCWORD(BRAID_TABLE(N), word5);
#endif
#endif
#endif
#endif
#endif
            }

            /*
              Process the last block, combining the CRCs of the N braids at the
              same time.
             */
            crc = crc_word(crc0 ^ words[0]);
#if N > 1
            crc = crc_word(crc1 ^ words[1] ^ crc);
#if N > 2
            crc = crc_word(crc2 ^ words[2] ^ crc);
#if N > 3
            crc = crc_word(crc3 ^ words[3] ^ crc);
#if N > 4
            crc = crc_word(crc4 ^ words[4] ^ crc);
#if N > 5
            crc = crc_word(crc5 ^ words[5] ^ crc);
#endif
#endif
#endif
#endif
#endif
            words += N;
        }
        else {
            /* Big endian. */

            z_word_t crc0, word0, comb;
#if N > 1
            z_word_t crc1, word1;
#if N > 2
            z_word_t crc2, word2;
#if N > 3
            z_word_t crc3, word3;
#if N > 4
            z_word_t crc4, word4;
#if N > 5
            z_word_t crc5, word5;
#endif
#endif
#endif
#endif
#endif

            /* Initialize the CRC for each braid. */
            crc0 = byte_swap(crc);
#if N > 1
            crc1 = 0;
#if N > 2
            crc2 = 0;
#if N > 3
            crc3 = 0;
#if N > 4
            crc4 = 0;
#if N > 5
            crc5 = 0;
#endif
#endif
#endif
#endif
#endif

            /*
              Process the first blks-1 blocks, computing the CRCs on each braid
              independently.
             */
            while (--blks) {
                /* Load the word for each braid into registers. */
                word0 = crc0 ^ words[0];
#if N > 1
                word1 = crc1 ^ words[1];
#if N > 2
                word2 = crc2 ^ words[2];
#if N > 3
                word3 = crc3 ^ words[3];
#if N > 4
                word4 = crc4 ^ words[4];
#if N > 5
                word5 = crc5 ^ words[5];
#endif
#endif
#endif
#endif
#endif
                words += N;

                /* Compute and update the CRC for each word. */
                crc0 = CRCWORD(BRAID_BIG_TABLE(N), word0);
#if N > 1
                crc1 = CRCWORD(BRAID_BIG_TABLE(N), word1);
#if N > 2
                crc2 = CRCWORD(BRAID_BIG_TABLE(N), word2);
#if N > 3
                crc3 = CRCWORD(BRAID_BIG_TABLE(N), word3);
#if N > 4
                crc4 = CRCWORD(BRAID_BIG_TABLE(N), word4);
#if N > 5
                crc5 = CRCWORD(BRAID_BIG_TABLE(N), word5);
#endif
#endif
#endif
#endif
#endif
            }

            /*
              Process the last block, combining the CRCs of the N braids at the
              same time.
             */
            comb = crc_word_big(crc0 ^ words[0]);
#if N > 1
            comb = crc_word_big(crc1 ^ words[1] ^ comb);
#if N > 2
            comb = crc_word_big(crc2 ^ words[2] ^ comb);
#if N > 3
            comb = crc_word_big(crc3 ^ words[3] ^ comb);
#if N > 4
            comb = crc_word_big(crc4 ^ words[4] ^ comb);
#if N > 5
            comb = crc_word_big(crc5 ^ words[5] ^ comb);
#endif
#endif
#endif
#endif
#endif
            words += N;
            crc = byte_swap(comb);
        }

        /*
          Update the pointer to the remaining bytes to process.
         */
        buf = (unsigned char const *)words;
    }

#endif /* W */

    /* Complete the computation of the CRC on any remaining bytes. */
    while (len >= 8) {
        len -= 8;
        crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
        crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
        crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
        crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
        crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
        crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
        crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
        crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
    }
    while (len) {
        len--;
        crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
    }

    /* Return the CRC, post-conditioned. */
    return crc ^ 0xffffffff;
}

#endif /* !ARMCRC32 || ARMCRC32_RT */

#ifdef ARMCRC32_RT
#ifndef Z_ARMCRC32_PROBE_LEN
#  define Z_ARMCRC32_PROBE_LEN 8
#endif

/* ========================================================================= */
uLong ZEXPORT crc32_z(uLong crc, const unsigned char FAR *buf, z_size_t len) {
    int caps;

#if Z_ARMCRC32_PROBE_LEN > 0
    /* For a few bytes, the byte table is faster than finding out whether the
       instructions can be used. */
    if (len < Z_ARMCRC32_PROBE_LEN) {
        if (buf == Z_NULL) return 0;
#ifdef DYNAMIC_CRC_TABLE
        z_once(&made, make_crc_table);
#endif /* DYNAMIC_CRC_TABLE */
        crc = (~crc) & 0xffffffff;
        while (len) {
            len--;
            crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xff];
        }
        return crc ^ 0xffffffff;
    }
#endif
    caps = arm_caps(ARM_HAVE_CRC32);
    if (caps & ARM_HAVE_CRC32)
        return crc32_arm(crc, buf, len);
    return crc32_soft(crc, buf, len);
}
#endif /* ARMCRC32_RT */

/* ========================================================================= */
uLong ZEXPORT crc32(uLong crc, const unsigned char FAR *buf, uInt len) {
    #ifdef HAVE_S390X_VX
    return crc32_z_hook(crc, buf, len);
    #endif
    return crc32_z(crc, buf, len);
}

/* ========================================================================= */
uLong ZEXPORT crc32_combine_gen64(z_off64_t len2) {
    if (len2 < 0)
        return 0;
#ifdef DYNAMIC_CRC_TABLE
    z_once(&made, make_crc_table);
#endif /* DYNAMIC_CRC_TABLE */
    return x2nmodp(len2, 3);
}

/* ========================================================================= */
uLong ZEXPORT crc32_combine_gen(z_off_t len2) {
    return crc32_combine_gen64((z_off64_t)len2);
}

/* ========================================================================= */
uLong ZEXPORT crc32_combine_op(uLong crc1, uLong crc2, uLong op) {
    if (op == 0)
        return 0;
    return multmodp(op, crc1 & 0xffffffff) ^ (crc2 & 0xffffffff);
}

/* ========================================================================= */
uLong ZEXPORT crc32_combine64(uLong crc1, uLong crc2, z_off64_t len2) {
    return crc32_combine_op(crc1, crc2, crc32_combine_gen64(len2));
}

/* ========================================================================= */
uLong ZEXPORT crc32_combine(uLong crc1, uLong crc2, z_off_t len2) {
    return crc32_combine64(crc1, crc2, (z_off64_t)len2);
}
