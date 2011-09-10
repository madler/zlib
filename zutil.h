/* zutil.h -- internal interface and configuration of the compression library
 * Copyright (C) 1995 Jean-loup Gailly.
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

/* $Id: zutil.h,v 1.5 1995/04/14 21:22:38 jloup Exp $ */

#ifndef _Z_UTIL_H
#define _Z_UTIL_H

#include "zlib.h"

#ifdef MSDOS
#   include <stddef.h>
#else
    extern int errno;
#endif
#ifdef __STDC__
#  include <string.h>
#  include <memory.h>
#endif

#ifndef local
#  define local static
#endif
/* compile with -Dlocal if your debugger can't find static symbols */

typedef unsigned char  uch;
typedef unsigned short ush;
typedef unsigned long  ulg;

extern char *z_errmsg[]; /* indexed by 1-zlib_error */

#define ERR_RETURN(strm,err) return (strm->msg=z_errmsg[1-err], err)
/* To be used only when the state is known to be valid */

	/* common constants */

#define DEFLATED   8

#ifndef WBITS
# define WBITS   15 /* 32K window */
#endif

#ifndef MEM_LEVEL
# define MEM_LEVEL  8
#endif

#define STORED_BLOCK 0
#define STATIC_TREES 1
#define DYN_TREES    2
/* The three kinds of block type */

#define MIN_MATCH  3
#define MAX_MATCH  258
/* The minimum and maximum match lengths */

	/* target dependencies */

#ifdef MSDOS
#  define OS_CODE  0x00
#  ifdef __TURBOC__
#    include <alloc.h>
#    define exit(n) _exit(n)
#  else /* MSC */
#    include <malloc.h>
#  endif
#endif

#ifdef OS2
#  define OS_CODE  0x06
#endif

#ifdef WIN32 /* Windows NT */
#  define OS_CODE  0x0b
#endif

#if defined(VAXC) || defined(VMS)
#  define OS_CODE  0x02
#  define FOPEN(name, mode) \
     fopen((name), (mode), "mbc=60", "ctx=stm", "rfm=fix", "mrs=512")
#endif

#ifdef AMIGA
#  define OS_CODE  0x01
#endif

#if defined(ATARI) || defined(atarist)
#  define OS_CODE  0x05
#endif

#ifdef MACOS
#  define OS_CODE  0x07
#endif

#ifdef __50SERIES /* Prime/PRIMOS */
#  define OS_CODE  0x0F
#endif

#ifdef TOPS20
#  define OS_CODE  0x0a
#endif

	/* Common defaults */

#ifndef OS_CODE
#  define OS_CODE  0x03  /* assume Unix */
#endif

#ifndef FOPEN
#  define FOPEN(name, mode) fopen((name), (mode))
#endif

         /* functions */

#ifdef HAVE_STRERROR
   extern char *strerror __P((int));
#  define zstrerror(errnum) strerror(errnum)
#else
#  define zstrerror(errnum) ""
#endif

#if defined(__STDC__) && !defined(HAVE_MEMCPY)
#  define HAVE_MEMCPY
#endif
#ifdef HAVE_MEMCPY
#  define zmemcpy memcpy
#  define zmemzero(dest, len) memset(dest, 0, len)
#else
   extern void zmemcpy  __P((Byte* dest, Byte* source, uInt len));
   extern void zmemzero __P((Byte* dest, uInt len));
#endif

/* Diagnostic functions */
#ifdef DEBUG
#  include <stdio.h>
#  ifndef verbose
#    define verbose 0
#  endif
#  define Assert(cond,msg) {if(!(cond)) z_error(msg);}
#  define Trace(x) fprintf x
#  define Tracev(x) {if (verbose) fprintf x ;}
#  define Tracevv(x) {if (verbose>1) fprintf x ;}
#  define Tracec(c,x) {if (verbose && (c)) fprintf x ;}
#  define Tracecv(c,x) {if (verbose>1 && (c)) fprintf x ;}
#else
#  define Assert(cond,msg)
#  define Trace(x)
#  define Tracev(x)
#  define Tracevv(x)
#  define Tracec(c,x)
#  define Tracecv(c,x)
#endif


typedef uLong (*check_func) __P((uLong check, Byte *buf, uInt len));

extern void z_error    __P((char *m));

voidp zcalloc __P((voidp opaque, unsigned items, unsigned size));
void  zcfree  __P((voidp opaque, voidp ptr));

#define ZALLOC(strm, items, size) \
           (*((strm)->zalloc))((strm)->opaque, (items), (size))
#define ZFREE(strm, addr)  (*((strm)->zfree))((strm)->opaque, (voidp)(addr))
#define TRY_FREE(s, p) {if (p) ZFREE(s, p);}

#endif /* _Z_UTIL_H */
