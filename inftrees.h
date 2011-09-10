/* inftrees.h -- header to use inftrees.c
 * Copyright (C) 1995 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h 
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

/* Huffman code lookup table entry--this entry is four bytes for machines
   that have 16-bit pointers (e.g. PC's in the small or medium model).
   Valid extra bits (exop) are 0..13.  exop == -64 is EOB (end of block),
   exop == 16 means that v is a literal, exop < 0 means that v is a pointer
   to the next table, which codes -exop bits, and lastly exop == -128
   indicates an unused code.  If a code with exop == -128 is looked up,
   this implies an error in the data. */

typedef struct inflate_huft_s inflate_huft;
struct inflate_huft_s {
  union {
    struct {
      char Exop;        /* number of extra bits or operation */
      char Bits;        /* number of bits in this code or subcode */
    } what;
    Byte *pad;          /* pad structure to a power of 2 (4 bytes for */
  } word;               /*  16-bit, 8 bytes for 32-bit machines) */
  union {
    uInt Base;          /* literal, length base, or distance base */
    inflate_huft *Next; /* pointer to next level of table */
  } more;
};

#ifdef DEBUG
  extern uInt inflate_hufts;
#endif

extern int inflate_trees_bits __P((
    uInt *,			/* 19 code lengths */
    uInt *,			/* bits tree desired/actual depth */
    inflate_huft **,		/* bits tree result */
    z_stream *));		/* for zalloc, zfree functions */

extern int inflate_trees_dynamic __P((
    uInt,			/* number of literal/length codes */
    uInt,			/* number of distance codes */
    uInt *,			/* that many (total) code lengths */
    uInt *,			/* literal desired/actual bit depth */
    uInt *,			/* distance desired/actual bit depth */
    inflate_huft **,		/* literal/length tree result */
    inflate_huft **,		/* distance tree result */
    z_stream *));		/* for zalloc, zfree functions */

extern int inflate_trees_fixed __P((
    uInt *,			/* literal desired/actual bit depth */
    uInt *,			/* distance desired/actual bit depth */
    inflate_huft **,		/* literal/length tree result */
    inflate_huft **));		/* distance tree result */

extern int inflate_trees_free __P((
    inflate_huft *,		/* tables to free */
    z_stream *));		/* for zfree function */
