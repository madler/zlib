/* infblock.h -- header to use infblock.c
 * Copyright (C) 1995 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h 
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

struct inflate_blocks_state;

extern struct inflate_blocks_state * inflate_blocks_new __P((
    z_stream *,
    check_func,                 /* check function */
    uInt));                     /* window size */

extern int inflate_blocks __P((
    struct inflate_blocks_state *,
    z_stream *,
    int));			/* initial return code */

extern void inflate_blocks_reset __P((
    struct inflate_blocks_state *,
    z_stream *,
    uLong *));                  /* check value on output */

extern int inflate_blocks_free __P((
    struct inflate_blocks_state *,
    z_stream *,
    uLong *));                  /* check value on output */
