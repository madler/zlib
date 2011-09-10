/* zutil.c -- target dependent utility functions for the compression library
 * Copyright (C) 1995 Jean-loup Gailly.
 * For conditions of distribution and use, see copyright notice in zlib.h 
 */

/* $Id: zutil.c,v 1.5 1995/04/14 21:30:23 jloup Exp $ */

#include <stdio.h>

#include "zutil.h"

extern void exit __P((int));

char *zlib_version = ZLIB_VERSION;

char *z_errmsg[] = {
"stream end",          /* Z_STREAM_END    1 */
"",                    /* Z_OK            0 */
"file error",          /* Z_ERRNO        (-1) */
"stream error",        /* Z_STREAM_ERROR (-2) */
"data error",          /* Z_DATA_ERROR   (-3) */
"insufficient memory", /* Z_MEM_ERROR    (-4) */
"buffer error",        /* Z_BUF_ERROR    (-5) */
""};


void z_error (m)
    char *m;
{
    fprintf(stderr, "%s\n", m);
    exit(1);
}

#ifndef HAVE_MEMCPY

void zmemcpy(dest, source, len)
    Byte* dest;
    Byte* source;
    uInt  len;
{
    if (len == 0) return;
    do {
	*dest++ = *source++; /* ??? to be unrolled */
    } while (--len != 0);
}

void zmemzero(dest, len)
    Byte* dest;
    uInt  len;
{
    if (len == 0) return;
    do {
	*dest++ = 0;  /* ??? to be unrolled */
    } while (--len != 0);
}
#endif

#if defined(MSDOS) && !defined(USE_CALLOC)
#  ifdef __TURBOC__

/* Turbo C malloc() does not allow dynamic allocation of 64K bytes
 * and farmalloc(64K) returns a pointer with an offset of 8, so we
 * must fix the pointer. Warning: the pointer must be put back to its
 * original form in order to free it, use zcfree().
 */

#define MAX_PTR 10
/* 10*64K = 640K */

local int next_ptr = 0;

typedef struct ptr_table_s {
    voidp org_ptr;
    voidp new_ptr;
} ptr_table;

local ptr_table table[MAX_PTR];
/* This table is used to remember the original form of pointers
 * to large buffers (64K). Such pointers are normalized with a zero offset.
 * Since MSDOS is not a preemptive multitasking OS, this table is not
 * protected from concurrent access. This hack doesn't work anyway on
 * a protected system like OS/2. Use Microsoft C instead.
 */

voidp zcalloc (voidp opaque, unsigned items, unsigned size)
{
    voidp buf;
    ulg bsize = (ulg)items*size;

    if (bsize < 65536L) {
	buf = farmalloc(bsize);
	if (*(ush*)&buf != 0) return buf;
    } else {
	buf = farmalloc(bsize + 16L);
    }
    if (buf == NULL || next_ptr >= MAX_PTR) return NULL;
    table[next_ptr].org_ptr = buf;

    /* Normalize the pointer to seg:0 */
    *((ush*)&buf+1) += ((ush)((uch*)buf-0) + 15) >> 4;
    *(ush*)&buf = 0;
    table[next_ptr++].new_ptr = buf;
    return buf;
}

void  zcfree (voidp opaque, voidp ptr)
{
    int n;
    if (*(ush*)&ptr != 0) { /* object < 64K */
	farfree(ptr);
	return;
    }
    /* Find the original pointer */
    for (n = 0; n < next_ptr; n++) {
	if (ptr != table[n].new_ptr) continue;

	farfree(table[n].org_ptr);
	while (++n < next_ptr) {
	    table[n-1] = table[n];
	}
	next_ptr--;
	return;
    }
    z_error("zcfree: ptr not found");
}

#  else /* MSC */

#if (!defined(_MSC_VER) || (_MSC_VER < 600))
#  define _halloc  halloc
#  define _hfree   hfree
#endif

voidp zcalloc (voidp opaque, unsigned items, unsigned size)
{
    return _halloc((long)items, size);
}

void  zcfree (voidp opaque, voidp ptr)
{
    _hfree(ptr);
}

#  endif /* __TURBOC__ ? */

#else /* !MSDOS */

extern voidp calloc __P((uInt items, uInt size));
extern void  free   __P((voidp ptr));

voidp zcalloc (opaque, items, size)
    voidp opaque;
    unsigned items;
    unsigned size;
{
    return calloc(items, size);
}

void  zcfree (opaque, ptr)
    voidp opaque;
    voidp ptr;
{
    free(ptr);
}

#endif /* MSDOS */
