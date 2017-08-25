/* inffast.h -- header to use inffast.c
 * Copyright (C) 1995-2003, 2010 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */



#ifdef INFLATE_FAST64

/* __x86_64__ is used by Clang and GCC. _M_X64 is used by MSVC.
   __ppc64__ is used by Clang. __PPC64__ is used by GCC.
   __aarch64__ is used by Clang and GCC.
 */
#if defined(__x86_64__) || defined(_M_X64) || \
    defined(__ppc64__) || defined(__PPC__64) || defined(__aarch64__)

#define ZLIB_INTERNAL_HAVE_64_BIT_UNALIGNED_LOADS 1

#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define ZLIB_INTERNAL_HAVE_64_BIT_UNALIGNED_LOADS_LITTLE_ENDIAN 1
#endif

#endif

#endif /* INFFLATE_FAST64 */



/* If using 64 bit loads, round INFLATE_FAST_MIN_LEFT and INFLATE_FAST_MIN_HAVE
   up to a multiple of 8.
 */

/* INFLATE_FAST_MIN_LEFT is the minimum number of output bytes that are left,
   so that we can call inflate_fast safely with only one up front bounds check.
   One length-distance code pair can copy up to 258 bytes.
 */
#ifdef ZLIB_INTERNAL_HAVE_64_BIT_UNALIGNED_LOADS
#define INFLATE_FAST_MIN_LEFT 264
#else
#define INFLATE_FAST_MIN_LEFT 258
#endif

/* INFLATE_FAST_MIN_HAVE is the minimum number of input bytes that we have, so
   that we can call inflate_fast safely with only one up front bounds check.
   One length-distance code pair (as two Huffman encoded values of up to 15
   bits each) plus any additional bits (up to 5 for length and 13 for distance)
   can require reading up to 48 bits, or 6 bytes.
 */
#ifdef ZLIB_INTERNAL_HAVE_64_BIT_UNALIGNED_LOADS_LITTLE_ENDIAN
#define INFLATE_FAST_MIN_HAVE 8
#else
#define INFLATE_FAST_MIN_HAVE 6
#endif



void ZLIB_INTERNAL inflate_fast OF((z_streamp strm, unsigned start));
