/* inffast.c -- process literals and length/distance pairs fast
 * Copyright (C) 1995 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h 
 */

#include "zutil.h"
#include "inftrees.h"
#include "infutil.h"
#include "inffast.h"

struct inflate_codes_state {int dummy;}; /* for buggy compilers */

/* simplify the use of the inflate_huft type with some defines */
#define base more.Base
#define next more.Next
#define exop word.what.Exop
#define bits word.what.Bits

/* macros for bit input with no checking and for returning unused bytes */
#ifdef DEBUG
#  undef NEXTBYTE
#  define NEXTBYTE (n--?0:fprintf(stderr,"inffast underrun\n"),*p++)
#endif
#define GRABBITS(j) {while(k<(j)){b|=((uLong)NEXTBYTE)<<k;k+=8;}}
#define UNGRAB {n+=(c=k>>3);p-=c;k&=7;}

/* Called with number of bytes left to write in window at least 258
   (the maximum string length) and number of input bytes available
   at least ten.  The ten bytes are six bytes for the longest length/
   distance pair plus four bytes for overloading the bit buffer. */

int inflate_fast(bl, bd, tl, td, s, z)
uInt bl, bd;
inflate_huft *tl, *td;
struct inflate_blocks_state *s;
z_stream *z;
{
  inflate_huft *t;      /* temporary pointer */
  int e;                /* extra bits or operation */
  uLong b;              /* bit buffer */
  uInt k;               /* bits in bit buffer */
  Byte *p;              /* input data pointer */
  uInt n;               /* bytes available there */
  Byte *q;              /* output window write pointer */
  uInt m;               /* bytes to end of window or read pointer */
  uInt ml;              /* mask for literal/length tree */
  uInt md;              /* mask for distance tree */
  uInt c;               /* bytes to copy */
  uInt d;               /* distance back to copy from */
  Byte *r;              /* copy source pointer */

  /* load input, output, bit values */
  LOAD

  /* initialize masks in registers */
  ml = inflate_mask[bl];
  md = inflate_mask[bd];

  /* do until not enough input or output space for fast loop */
  do {                          /* assume called with m >= 258 && n >= 10 */
    /* get literal/length code */
    GRABBITS(20)                /* max bits for literal/length code */
    if ((e = (t = tl + ((uInt)b & ml))->exop) < 0)
      do {
        if (e == -128)
        {
          z->msg = "invalid literal/length code";
          UNGRAB
          UPDATE
          return Z_DATA_ERROR;
        }
        DUMPBITS(t->bits)
        e = -e;
        if (e & 64)             /* end of block */
        {
          Tracevv((stderr, "inflate:         * end of block\n"));
          UNGRAB
          UPDATE
          return Z_STREAM_END;
        }
      } while ((e = (t = t->next + ((uInt)b & inflate_mask[e]))->exop) < 0);
    DUMPBITS(t->bits)

    /* process literal or length (end of block already trapped) */
    if (e & 16)                 /* then it's a literal */
    {
      Tracevv((stderr, t->base >= 0x20 && t->base < 0x7f ?
                "inflate:         * literal '%c'\n" :
                "inflate:         * literal 0x%02x\n", t->base));
      *q++ = (Byte)t->base;
      m--;
    }
    else                        /* it's a length */
    {
      /* get length of block to copy (already have extra bits) */
      c = t->base + ((uInt)b & inflate_mask[e]);
      DUMPBITS(e);
      Tracevv((stderr, "inflate:         * length %u\n", c));

      /* decode distance base of block to copy */
      GRABBITS(15);             /* max bits for distance code */
      if ((e = (t = td + ((uInt)b & md))->exop) < 0)
        do {
          if (e == -128)
          {
            z->msg = "invalid distance code";
            UNGRAB
            UPDATE
            return Z_DATA_ERROR;
          }
          DUMPBITS(t->bits)
          e = -e;
        } while ((e = (t = t->next + ((uInt)b & inflate_mask[e]))->exop) < 0);
      DUMPBITS(t->bits)

      /* get extra bits to add to distance base */
      GRABBITS((uInt)e)         /* get extra bits (up to 13) */
      d = t->base + ((uInt)b & inflate_mask[e]);
      DUMPBITS(e)
      Tracevv((stderr, "inflate:         * distance %u\n", d));

      /* do the copy */
      m -= c;
      if ((uInt)(q - s->window) >= d)   /* if offset before destination, */
      {                                 /*  just copy */
        r = q - d;
        *q++ = *r++;  c--;              /* minimum count is three, */
        *q++ = *r++;  c--;              /*  so unroll loop a little */
        do {
          *q++ = *r++;
        } while (--c);
      }
      else                              /* else offset after destination */
      {
        e = d - (q - s->window);        /* bytes from offset to end */
        r = s->end - e;                 /* pointer to offset */
        if (c > (uInt)e)                /* if source crosses, */
        {
          c -= e;                       /* copy to end of window */
          do {
            *q++ = *r++;
          } while (--e);
          r = s->window;                /* copy rest from start of window */
        }
        do {                            /* copy all or what's left */
          *q++ = *r++;
        } while (--c);
      }
    }
  } while (m >= 258 && n >= 10);

  /* not enough input or output--restore pointers and return */
  UNGRAB
  UPDATE
  return Z_OK;
}
