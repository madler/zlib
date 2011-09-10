/* infcodes.h -- header to use infcodes.c
 * Copyright (C) 1995 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h 
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

struct inflate_codes_state;

extern struct inflate_codes_state *inflate_codes_new __P((
    uInt, uInt,
    inflate_huft *, inflate_huft *,
    z_stream *));

extern int inflate_codes __P((
    struct inflate_blocks_state *,
    z_stream *,
    int));

extern void inflate_codes_free __P((
    struct inflate_codes_state *,
    z_stream *));
