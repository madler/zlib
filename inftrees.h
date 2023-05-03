/* inftrees.h -- header to use inftrees.c
 * Copyright (C) 1995-2005, 2010 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

/* XXX_ROOT is the number of the first look-up bits;  
 * ENOUGH_XXX is the space for the second loop-up 
 */
#define LLEN_ROOT 7
#define LEN_ROOT  11
#define DIST_ROOT 8
#define ENOUGH_LENS   292          /*9:340, 10:308, 11:292, 12:72 */
#define ENOUGH_DISTS  144           /* 7:272, 8:144, 9:80,  10:48 */ 
#define MAX_HUFBITS   15 


 /* Structure for decoding tables.  Each entry provides either the
    information needed to do the operation requested by the code that
    indexed that table entry, or it provides a pointer to another
    table that indexes more bits of the code.  op indicates whether
    the entry is a pointer to another table, a literal, a length or
    distance, an end-of-block, or an invalid code.  For a table
    pointer, the low four bits of op is the number of index bits of
    that table.  For a length or distance, the low four bits of op
    is the number of extra bits to get after the code.  bits is
    the number of bits in this code or part of the code to drop off
    of the bit buffer.  val is the actual byte to output in the case
    of a literal, the base length or distance, or the offset from
    the current table to the next table.  Each entry is four bytes. */

typedef struct {
    unsigned short val;          /* offset in table or code value */
    unsigned char allBits;       /* Huffman + extension bits */
    unsigned char hufBits;       /* Huffman code bits */
} DecodeStr;


/* Type of code to build for inflate_table() */
typedef enum {
    LLENS,
    LENS,
    DISTS
} codetype;

int ZLIB_INTERNAL inflate_table OF((codetype type, unsigned char FAR* lens, unsigned codeSize,
    unsigned FAR* rootMaskPtr, DecodeStr FAR* rootTable, DecodeStr FAR* extTable));
