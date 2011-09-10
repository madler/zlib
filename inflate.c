/* inflate.c -- zlib interface to inflate modules
 * Copyright (C) 1995 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h 
 */

#include "zutil.h"
#include "infblock.h"

struct inflate_blocks_state {int dummy;}; /* for buggy compilers */

/* inflate private state */
struct internal_state {

  /* mode */
  enum {
      METHOD,	/* waiting for method byte */
      FLAG,	/* waiting for flag byte */
      START,	/* make new blocks state */
      BLOCKS,	/* decompressing blocks */
      CHECK4,	/* four check bytes to go */
      CHECK3,	/* three check bytes to go */
      CHECK2,	/* two check bytes to go */
      CHECK1,	/* one check byte to go */
      DONE,	/* finished check, done */
      ERROR}	/* got an error--stay here */
    mode;		/* current inflate mode */

  int  no_header;
  uInt w_size;  /* LZ77 window size (32K by default) */
  uInt w_bits;  /* log2(w_size)  (8..16) */

  /* mode dependent information */
  union {
    uInt method;	/* if FLAGS, method byte */
    struct inflate_blocks_state
      *blocks;		/* if BLOCKS, current state */
    struct {
      uLong was;		/* computed check value */
      uLong need;		/* stream check value */
    } check;		/* if CHECK, check values to compare */
  } sub;	/* submode */
};


int inflateInit (strm)
z_stream *strm;
{
    return inflateInit2(strm, WBITS);
}

int inflateInit2(z, windowBits)
z_stream *z;
int windowBits;
{
  if (z == Z_NULL)
    return Z_STREAM_ERROR;
  if (z->zalloc == Z_NULL) z->zalloc = zcalloc;
  if (z->zfree == Z_NULL) z->zfree = zcfree;
  z->total_in = z->total_out = 0;
  z->msg = Z_NULL;
  if ((z->state = (struct internal_state *)
       ZALLOC(z,1,sizeof(struct internal_state))) == Z_NULL)
    return Z_MEM_ERROR;
  z->state->mode = METHOD;

  z->state->no_header = 0;
  if (windowBits < 0) { /* undocumented feature: no zlib header */
    windowBits = - windowBits;
    z->state->no_header = 1;
    z->state->sub.method = DEFLATED;
    z->state->mode = START;
  }
  if (windowBits < 8 || windowBits > 15) {
    inflateEnd(z);
    return Z_STREAM_ERROR;
  }
  z->state->w_bits = windowBits;
  z->state->w_size = 1<<windowBits;
  return Z_OK;
}


#define NEXTBYTE (z->avail_in--,z->total_in++,*z->next_in++)

int inflate(z, f)
z_stream *z;
int f;
{
  int r;
  uInt b;
  uLong c;

  if (z == Z_NULL || z->next_in == Z_NULL)
    return Z_STREAM_ERROR;
  r = Z_BUF_ERROR;
  while (1) switch (z->state->mode)
  {
    case METHOD:
      if (z->avail_in == 0) return r;  r = Z_OK;
      if (((z->state->sub.method = NEXTBYTE) & 0xf != DEFLATED))
      {
        z->state->mode = ERROR;
	z->msg = "unknown compression method";
	return Z_DATA_ERROR;
      }
      if ((z->state->sub.method >> 4) > z->state->w_bits)
      {
        z->state->mode = ERROR;
	z->msg = "invalid window size";
	return Z_DATA_ERROR;
      }
      z->state->mode = FLAG;
    case FLAG:
      if (z->avail_in == 0) return r;  r = Z_OK;
      if ((b = NEXTBYTE) & 0x20)
      {
        z->state->mode = ERROR;
	z->msg = "invalid reserved bit";
	return Z_DATA_ERROR;
      }
      if (((z->state->sub.method << 8) + b) % 31)
      {
        z->state->mode = ERROR;
	z->msg = "incorrect header check";
	return Z_DATA_ERROR;
      }
      z->state->mode = START;
    case START:
      if ((z->state->sub.blocks = inflate_blocks_new(z,z->state->w_size))
	  == Z_NULL)
        return Z_MEM_ERROR;
      z->state->mode = BLOCKS;
    case BLOCKS:
      if ((r = inflate_blocks(z->state->sub.blocks, z, r)) != Z_STREAM_END)
        return r;
      inflate_blocks_free(z->state->sub.blocks, z, &c, &r);
      if (z->state->no_header) {
	z->state->mode = DONE;
	return Z_STREAM_END;
      }
      z->state->sub.check.was = c;
      if (r != -1)
      {
	z->state->sub.check.need = (uLong)r << 24;
        z->state->mode = CHECK3;
        r = Z_OK;
	break;
      }
      r = Z_OK;
      z->state->mode = CHECK4;
    case CHECK4:
      if (z->avail_in == 0) return r;  r = Z_OK;
      z->state->sub.check.need = (uLong)NEXTBYTE << 24;
      z->state->mode = CHECK3;
    case CHECK3:
      if (z->avail_in == 0) return r;  r = Z_OK;
      z->state->sub.check.need += (uLong)NEXTBYTE << 16;
      z->state->mode = CHECK2;
    case CHECK2:
      if (z->avail_in == 0) return r;  r = Z_OK;
      z->state->sub.check.need += (uLong)NEXTBYTE << 8;
      z->state->mode = CHECK1;
    case CHECK1:
      if (z->avail_in == 0) return r;  r = Z_OK;
      z->state->sub.check.need += (uLong)NEXTBYTE;
      if (z->state->sub.check.was != z->state->sub.check.need)
      {
        z->state->mode = ERROR;
        z->msg = "incorrect data check";
	return Z_DATA_ERROR;
      }
      z->state->mode = DONE;
    case DONE:
      return Z_STREAM_END;
    case ERROR:
      return Z_DATA_ERROR;
    default:
      return Z_STREAM_ERROR;
  }
}


int inflateEnd(z)
z_stream *z;
{
  uLong c;
  int e;

  if (z == Z_NULL || z->state == Z_NULL || z->zfree == Z_NULL)
    return Z_STREAM_ERROR;
  if (z->state->mode == BLOCKS)
    inflate_blocks_free(z->state->sub.blocks, z, &c, &e);
  ZFREE(z, z->state);
  z->state = Z_NULL;
  return Z_OK;
}


/* inflateSync not implemented yet--this just consumes input */
int inflateSync(z)
z_stream *z;
{
  if (z == Z_NULL) return Z_STREAM_ERROR;
  if (z->avail_in == 0) return Z_BUF_ERROR;
  do {
    z->total_in++;
  } while (--z->avail_in);
  return Z_DATA_ERROR;
}


/* inflateReset not fully implemented yet--this frees and reallocates */
int inflateReset(z)
z_stream *z;
{
  int r;

  if ((r = inflateEnd(z)) != Z_OK)
    return r;
  return inflateInit(z);
}
