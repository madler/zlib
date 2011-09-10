/* zconf.h -- configuration of the zlib compression library
 * Copyright (C) 1995 Jean-loup Gailly.
 * For conditions of distribution and use, see copyright notice in zlib.h 
 */

/* $Id: zconf.h,v 1.12 1995/05/03 17:27:12 jloup Exp $ */

#ifndef _ZCONF_H
#define _ZCONF_H

/*
     The library does not install any signal handler. It is recommended to
  add at least a handler for SIGSEGV when decompressing; the library checks
  the consistency of the input data whenever possible but may go nuts
  for some forms of corrupted input.
 */

/*
 * Compile with -DMAXSEG_64K if the alloc function cannot allocate more
 * than 64k bytes at a time (needed on systems with 16-bit int).
 */
#if defined(_GNUC__) && !defined(__32BIT__)
#  define __32BIT__
#endif
#if defined(__MSDOS__) && !defined(MSDOS)
#  define MSDOS
#endif
#if defined(MSDOS) && !defined(__32BIT__)
#  define MAXSEG_64K
#endif
#ifdef MSDOS
#  define UNALIGNED_OK
#endif

#ifndef STDC
#  if defined(MSDOS) || defined(__STDC__) || defined(__cplusplus)
#    define STDC
#  endif
#endif

#ifndef STDC
#  ifndef const
#    define const
#  endif
#endif

#ifdef	__MWERKS__ /* Metrowerks CodeWarrior declares fileno() in unix.h */
#  include <unix.h>
#endif

/* Maximum value for memLevel in deflateInit2 */
#ifndef MAX_MEM_LEVEL
#  ifdef MAXSEG_64K
#    define MAX_MEM_LEVEL 8
#  else
#    define MAX_MEM_LEVEL 9
#  endif
#endif

/* Maximum value for windowBits in deflateInit2 and inflateInit2 */
#ifndef MAX_WBITS
#  define MAX_WBITS   15 /* 32K LZ77 window */
#endif

/* The memory requirements for deflate are (in bytes):
            1 << (windowBits+2)   +  1 << (memLevel+9)
 that is: 128K for windowBits=15  +  128K for memLevel = 8  (default values)
 plus a few kilobytes for small objects. For example, if you want to reduce
 the default memory requirements from 256K to 128K, compile with
     make CFLAGS="-O -DMAX_WBITS=14 -DMAX_MEM_LEVEL=7"
 Of course this will generally degrade compression (there's no free lunch).

   The memory requirements for inflate are (in bytes) 1 << windowBits
 that is, 32K for windowBits=15 (default value) plus a few kilobytes
 for small objects.
*/

                        /* Type declarations */

#ifndef OF /* function prototypes */
#  ifdef STDC
#    define OF(args)  args
#  else
#    define OF(args)  ()
#  endif
#endif

/* The following definitions for FAR are needed only for MSDOS mixed
 * model programming (small or medium model with some far allocations).
 * This was tested only with MSC; for other MSDOS compilers you may have
 * to define NO_MEMCPY in zutil.h.  If you don't need the mixed model,
 * just define FAR to be empty.
 */
#if defined(M_I86SM) || defined(M_I86MM) /* MSC small or medium model */
#  ifdef _MSC_VER
#    define FAR __far
#  else
#    define FAR far
#  endif
#endif
#if defined(__BORLANDC__) && (defined(__SMALL__) || defined(__MEDIUM__))
#    define FAR __far /* completely untested, just a best guess */
#endif
#ifndef FAR
#   define FAR
#endif

typedef unsigned char  Byte;  /* 8 bits */
typedef unsigned int   uInt;  /* 16 bits or more */
typedef unsigned long  uLong; /* 32 bits or more */

typedef Byte FAR Bytef;
typedef char FAR charf;
typedef int FAR intf;
typedef uInt FAR uIntf;
typedef uLong FAR uLongf;

#ifdef STDC
   typedef void FAR *voidpf;
   typedef void     *voidp;
#else
   typedef Byte FAR *voidpf;
   typedef Byte     *voidp;
#endif

#endif /* _ZCONF_H */
