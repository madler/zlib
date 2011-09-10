/* zconf.h -- configuration of the zlib compression library
 * Copyright (C) 1995 Jean-loup Gailly.
 * For conditions of distribution and use, see copyright notice in zlib.h 
 */

/* $Id: zconf.h,v 1.8 1995/04/14 20:59:22 jloup Exp $ */

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

#ifdef MAXSEG_64K
#  define MAX_MEM_LEVEL 8
#else
#  define MAX_MEM_LEVEL 9
#endif

			/* Type declarations */

#ifndef __P /* function prototypes */
#  if defined(__STDC__) || defined(MSDOS)
#    define __P(args)  args
#  else
#    define __P(args)  ()
#  endif
#endif

#ifndef Byte
  typedef unsigned char  Byte;  /* 8 bits */
#endif
#ifndef uInt
  typedef unsigned int   uInt;  /* may be 16 or 32 bits */
#endif
#ifndef uLong
  typedef unsigned long  uLong; /* 32 bits or more */
#endif
#ifndef voidp
#  if defined(__STDC__) || defined(MSDOS)
     typedef void *voidp;
#  else
     typedef Byte *voidp;
#  endif
#endif

#endif /* _ZCONF_H */

