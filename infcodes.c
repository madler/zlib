/* infcodes.c -- process literals and length/distance pairs
 * Copyright (C) 1995 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h 
 */

#include "zutil.h"
#include "inftrees.h"
#include "infutil.h"
#include "inffast.h"
#include "infcodes.h"

/* simplify the use of the inflate_huft type with some defines */
#define base more.Base
#define next more.Next
#define exop word.what.Exop
#define bits word.what.Bits

/* inflate codes private state */
struct inflate_codes_state {

  /* mode */
  enum {	/* waiting for "i:"=input, "o:"=output, "x:"=nothing */
      START,	/* x: set up for LEN */
      LEN,	/* i: get length/literal/eob next */
      LENEXT,	/* i: getting length extra (have base) */
      DIST,	/* i: get distance next */
      DISTEXT,	/* i: getting distance extra */
      COPY,	/* o: copying bytes in window, waiting for space */
      LIT,	/* o: got literal, waiting for output space */
      WASH,	/* o: got eob, possibly still output waiting */
      END,	/* x: got eob and all data flushed */
      BADCODE}	/* x: got error */
    mode;		/* current inflate_codes mode */

  /* mode dependent information */
  uInt len;
  union {
    struct {
      inflate_huft *tree;	/* pointer into tree */
      uInt need;		/* bits needed */
    } code;		/* if LEN or DIST, where in tree */
    uInt lit;		/* if LIT, literal */
    struct {
      uInt get;			/* bits to get for extra */
      uInt dist;		/* distance back to copy from */
    } copy;		/* if EXT or COPY, where and how much */
  } sub;		/* submode */

  /* mode independent information */
  Byte lbits;		/* ltree bits decoded per branch */
  Byte dbits;		/* dtree bits decoder per branch */
  inflate_huft *ltree;		/* literal/length/eob tree */
  inflate_huft *dtree;		/* distance tree */

};


struct inflate_codes_state *inflate_codes_new(bl, bd, tl, td, z)
uInt bl, bd;
inflate_huft *tl, *td;
z_stream *z;
{
  struct inflate_codes_state *c;

  if ((c = (struct inflate_codes_state *)
       ZALLOC(z,1,sizeof(struct inflate_codes_state))) != Z_NULL)
  {
    c->mode = START;
    c->lbits = (Byte)bl;
    c->dbits = (Byte)bd;
    c->ltree = tl;
    c->dtree = td;
    Tracev((stderr, "inflate:       codes new\n"));
  }
  return c;
}


int inflate_codes(s, z, r)
struct inflate_blocks_state *s;
z_stream *z;
int r;
{
  uInt j;		/* temporary storage */
  inflate_huft *t;	/* temporary pointer */
  int e;		/* extra bits or operation */
  uLong b;		/* bit buffer */
  uInt k;		/* bits in bit buffer */
  Byte *p;		/* input data pointer */
  uInt n;		/* bytes available there */
  Byte *q;		/* output window write pointer */
  uInt m;		/* bytes to end of window or read pointer */
  Byte *f;		/* pointer to copy strings from */
  struct inflate_codes_state *c = s->sub.codes;	/* codes state */

  /* copy input/output information to locals (UPDATE macro restores) */
  LOAD

  /* process input and output based on current state */
  while (1) switch (c->mode)
  {		/* waiting for "i:"=input, "o:"=output, "x:"=nothing */
    case START:		/* x: set up for LEN */
#ifndef SLOW
      if (m >= 258 && n >= 10)
      {
        UPDATE
	r = inflate_fast(c->lbits, c->dbits, c->ltree, c->dtree, s, z);
	LOAD
	if (r != Z_OK)
	{
	  c->mode = r == Z_STREAM_END ? WASH : BADCODE;
	  break;
	}
      }
#endif /* !SLOW */
      c->sub.code.need = c->lbits;
      c->sub.code.tree = c->ltree;
      c->mode = LEN;
    case LEN:		/* i: get length/literal/eob next */
      j = c->sub.code.need;
      NEEDBITS(j)
      t = c->sub.code.tree + ((uInt)b & inflate_mask[j]);
      DUMPBITS(t->bits)
      if ((e = (int)(t->exop)) < 0)
      {
        if (e == -128)		/* invalid code */
	{
	  c->mode = BADCODE;
	  z->msg = "invalid literal/length code";
	  r = Z_DATA_ERROR;
	  LEAVE
	}
	e = -e;
	if (e & 64)		/* end of block */
	{
	  Tracevv((stderr, "inflate:         end of block\n"));
	  c->mode = WASH;
	  break;
	}
	c->sub.code.need = e;
	c->sub.code.tree = t->next;
	break;
      }
      if (e & 16)		/* literal */
      {
	c->sub.lit = t->base;
	Tracevv((stderr, t->base >= 0x20 && t->base < 0x7f ?
		 "inflate:         literal '%c'\n" :
		 "inflate:         literal 0x%02x\n", t->base));
	c->mode = LIT;
	break;
      }
      c->sub.copy.get = e;
      c->len = t->base;
      c->mode = LENEXT;
    case LENEXT:	/* i: getting length extra (have base) */
      j = c->sub.copy.get;
      NEEDBITS(j)
      c->len += (uInt)b & inflate_mask[j];
      DUMPBITS(j)
      c->sub.code.need = c->dbits;
      c->sub.code.tree = c->dtree;
      Tracevv((stderr, "inflate:         length %u\n", c->len));
      c->mode = DIST;
    case DIST:		/* i: get distance next */
      j = c->sub.code.need;
      NEEDBITS(j)
      t = c->sub.code.tree + ((uInt)b & inflate_mask[j]);
      DUMPBITS(t->bits)
      if ((e = (int)(t->exop)) < 0)
      {
        if (e == -128)
	{
	  c->mode = BADCODE;
	  z->msg = "invalid distance code";
	  r = Z_DATA_ERROR;
	  LEAVE
	}
	c->sub.code.need = -e;
	c->sub.code.tree = t->next;
	break;
      }
      c->sub.copy.dist = t->base;
      c->sub.copy.get = e;
      c->mode = DISTEXT;
    case DISTEXT:	/* i: getting distance extra */
      j = c->sub.copy.get;
      NEEDBITS(j)
      c->sub.copy.dist += (uInt)b & inflate_mask[j];
      DUMPBITS(j)
      Tracevv((stderr, "inflate:         distance %u\n", c->sub.copy.dist));
      c->mode = COPY;
    case COPY:		/* o: copying bytes in window, waiting for space */
      f = (uInt)(q - s->window) < c->sub.copy.dist ?
	  s->end - (c->sub.copy.dist - (q - s->window)) :
	  q - c->sub.copy.dist;
      while (c->len)
      {
	NEEDOUT
	OUTBYTE(*f++)
	if (f == s->end)
	  f = s->window;
	c->len--;
      }
      c->mode = START;
      break;
    case LIT:		/* o: got literal, waiting for output space */
      NEEDOUT
      OUTBYTE(c->sub.lit)
      c->mode = START;
      break;
    case WASH:		/* o: got eob, possibly more output */
      FLUSH
      if (s->read != s->write)
        LEAVE
      c->mode = END;
    case END:
      r = Z_STREAM_END;
      LEAVE
    case BADCODE:	/* x: got error */
      r = Z_DATA_ERROR;
      LEAVE
    default:
      r = Z_STREAM_ERROR;
      LEAVE
  }
}


void inflate_codes_free(c, z)
struct inflate_codes_state *c;
z_stream *z;
{
  inflate_trees_free(c->dtree, z);
  inflate_trees_free(c->ltree, z);
  ZFREE(z, c);
  Tracev((stderr, "inflate:       codes free\n"));
}
