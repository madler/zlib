/* inftrees.h -- header to use inftrees.c
 * Copyright (C) 1995-2026 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

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
   the current table to the next table.  Each entry is four bytes.

   Where Z_ENTRY32 is 1, the 64-bit loop in inffast.c reads each entry as one
   little-endian 32-bit word, and bits comes first in the entry, so that the
   bit buffer can be shifted by the word itself, as a 64-bit shift uses only
   the low six bits of its count. That is the default on little-endian 64-bit
   ARM. Elsewhere op comes first, and the fields are read through a pointer.
   Define Z_ENTRY32 as 1 or 0 to choose. 1 needs a little-endian machine.
   CODE() writes an entry in either order, for inffixed.h. */
#ifndef Z_ENTRY32
#  if (defined(__aarch64__) && defined(__BYTE_ORDER__) && \
       __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || defined(_M_ARM64)
#    define Z_ENTRY32 1
#  else
#    define Z_ENTRY32 0
#  endif
#endif
#if Z_ENTRY32
typedef struct {
    unsigned char bits;         /* bits in this part of the code */
    unsigned char op;           /* operation, extra bits, table bits */
    unsigned short val;         /* offset in table or code value */
} code;
#  define CODE(op, bits, val) {bits, op, val}
#else
typedef struct {
    unsigned char op;           /* operation, extra bits, table bits */
    unsigned char bits;         /* bits in this part of the code */
    unsigned short val;         /* offset in table or code value */
} code;
#  define CODE(op, bits, val) {op, bits, val}
#endif

/* op values as set by inflate_table():
    00000000 - literal
    0000tttt - table link, tttt != 0 is the number of table index bits
    0001eeee - length or distance, eeee is the number of extra bits
    01100000 - end of block
    01000000 - invalid code
 */

/* Maximum size of the dynamic table. The maximum number of code structures is
   1732, which is the sum of 1332 for literal/length codes and 400 for distance
   codes. These values were calculated using the program enough.c found in
   examples in the zlib distribution. The arguments to that program are the
   number of symbols, the initial root table size, and the maximum bit length
   of a code. "enough 286 10 15" for literal/length codes returns 1332, and
   "enough 30 8 15" for distance codes returns 400. The root table sizes (10
   and 8) are defined here. If the root table sizes here are changed, then
   these enough sizes must be recalculated and updated. */
#define LENBITS 10
#define DISTBITS 8
#ifndef PKZIP_BUG_WORKAROUND
#  define ENOUGH_LENS 1332
#  define ENOUGH_DISTS 400
#else
/* That workaround permits 288 literal/length codes and 32 distance codes. */
#  define ENOUGH_LENS 1334
#  define ENOUGH_DISTS 402
#endif
#define ENOUGH (ENOUGH_LENS+ENOUGH_DISTS)

/* Type of code to build for inflate_table() */
typedef enum {
    CODES,
    LENS,
    DISTS
} codetype;

int ZLIB_INTERNAL inflate_table(codetype type, unsigned short FAR *lens,
                                unsigned codes, code FAR * FAR *table,
                                unsigned FAR *bits, unsigned short FAR *work);
struct inflate_state;
void ZLIB_INTERNAL inflate_fixed(struct inflate_state FAR *state);
