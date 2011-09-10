/* zconf.h -- configuration of the zlib compression library
 * Copyright (C) 1995 Jean-loup Gailly.
 * For conditions of distribution and use, see copyright notice in zlib.h 
 */

/* $Id: zconf.h,v 1.10 1995/04/30 19:27:14 jloup Exp $ */

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
#if !defined(STDC) && (defined(MSDOS) || defined(__STDC__))
#  define STDC
#endif

#ifndef MAX_MEM_LEVEL
#  ifdef MAXSEG_64K
#    define MAX_MEM_LEVEL 8
#  else
#    define MAX_MEM_LEVEL 9
#  endif
#endif

#ifndef MAX_WBITS
#  define MAX_WBITS   15 /* 32K LZ77 window */
#endif

			/* Type declarations */

#ifndef __P /* function prototypes */
#  ifdef STDC
#    define __P(args)  args
#  else
#    define __P(args)  ()
#  endif
#endif

#ifndef Byte
  typedef unsigned char  Byte;  /* 8 bits */
#endif
#ifndef uInt
  typedef unsigned int   uInt;  /* 16 bits or more */
#endif
#ifndef uLong
  typedef unsigned long  uLong; /* 32 bits or more */
#endif
#ifndef voidp
#  ifdef STDC
     typedef void *voidp;
#  else
     typedef Byte *voidp;
#  endif
#endif

#endif /* _ZCONF_H */

