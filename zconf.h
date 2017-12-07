/* zconf.h -- configuration of the zlib compression library
 * Copyright (C) 1995-2016 Jean-loup Gailly, Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* @(#) $Id$ */

#ifndef ZCONF_H
#define ZCONF_H

/*
 * Defining custom Z_PREFIX_ implies using Z_PREFIX.
 */
#if defined(Z_PREFIX_) && !defined(Z_PREFIX)
#  define Z_PREFIX 1
#endif

/*
 * If you *really* need a unique prefix for all types and library functions,
 * compile with -DZ_PREFIX. The "standard" zlib should be compiled without it.
 * Even better than compiling with -DZ_PREFIX would be to use configure to set
 * this permanently in zconf.h using "./configure --zprefix".
 */
#ifdef Z_PREFIX     /* may be set to #if 1 by ./configure */
#  define Z_PREFIX_SET

#define ZLIB_CONCAT2(x, y) x ## y
#define ZLIB_CONCAT(x, y) ZLIB_CONCAT2(x, y)

/*
 * If a custom prefix is defined, use it. Otherwise use the default "z_".
 */
#ifdef Z_PREFIX_
#  define Z_ADD_PREFIX(name) ZLIB_CONCAT(Z_PREFIX_, name)
#else
#  define Z_ADD_PREFIX(name) ZLIB_CONCAT(z_, name)
#endif

/*
 * For macros the standard prefix is always used, as we can't define them
 * using names constructed using arbitrary Z_PREFIX_ value -- but luckily this
 * doesn't matter anyhow, because we can't have link-time conflicts for macros.
 */
#define Z_ADD_PREFIX_STD(name) ZLIB_CONCAT(z_, name)

/* all linked symbols and init macros */
#  define _dist_code            Z_ADD_PREFIX(_dist_code)
#  define _length_code          Z_ADD_PREFIX(_length_code)
#  define _tr_align             Z_ADD_PREFIX(_tr_align)
#  define _tr_flush_bits        Z_ADD_PREFIX(_tr_flush_bits)
#  define _tr_flush_block       Z_ADD_PREFIX(_tr_flush_block)
#  define _tr_init              Z_ADD_PREFIX(_tr_init)
#  define _tr_stored_block      Z_ADD_PREFIX(_tr_stored_block)
#  define _tr_tally             Z_ADD_PREFIX(_tr_tally)
#  define adler32               Z_ADD_PREFIX(adler32)
#  define adler32_combine       Z_ADD_PREFIX(adler32_combine)
#  define adler32_combine64     Z_ADD_PREFIX(adler32_combine64)
#  define adler32_z             Z_ADD_PREFIX(adler32_z)
#  ifndef Z_SOLO
#    define compress              Z_ADD_PREFIX(compress)
#    define compress2             Z_ADD_PREFIX(compress2)
#    define compressBound         Z_ADD_PREFIX(compressBound)
#  endif
#  define crc32                 Z_ADD_PREFIX(crc32)
#  define crc32_combine         Z_ADD_PREFIX(crc32_combine)
#  define crc32_combine64       Z_ADD_PREFIX(crc32_combine64)
#  define crc32_z               Z_ADD_PREFIX(crc32_z)
#  define deflate               Z_ADD_PREFIX(deflate)
#  define deflateBound          Z_ADD_PREFIX(deflateBound)
#  define deflateCopy           Z_ADD_PREFIX(deflateCopy)
#  define deflateEnd            Z_ADD_PREFIX(deflateEnd)
#  define deflateGetDictionary  Z_ADD_PREFIX(deflateGetDictionary)
#  define deflateInit           Z_ADD_PREFIX_STD(deflateInit)
#  define deflateInit2          Z_ADD_PREFIX_STD(deflateInit2)
#  define deflateInit2_         Z_ADD_PREFIX(deflateInit2_)
#  define deflateInit_          Z_ADD_PREFIX(deflateInit_)
#  define deflateParams         Z_ADD_PREFIX(deflateParams)
#  define deflatePending        Z_ADD_PREFIX(deflatePending)
#  define deflatePrime          Z_ADD_PREFIX(deflatePrime)
#  define deflateReset          Z_ADD_PREFIX(deflateReset)
#  define deflateResetKeep      Z_ADD_PREFIX(deflateResetKeep)
#  define deflateSetDictionary  Z_ADD_PREFIX(deflateSetDictionary)
#  define deflateSetHeader      Z_ADD_PREFIX(deflateSetHeader)
#  define deflateTune           Z_ADD_PREFIX(deflateTune)
#  define deflate_copyright     Z_ADD_PREFIX(deflate_copyright)
#  define get_crc_table         Z_ADD_PREFIX(get_crc_table)
#  ifndef Z_SOLO
#    define gz_error              Z_ADD_PREFIX(gz_error)
#    define gz_intmax             Z_ADD_PREFIX(gz_intmax)
#    define gz_strwinerror        Z_ADD_PREFIX(gz_strwinerror)
#    define gzbuffer              Z_ADD_PREFIX(gzbuffer)
#    define gzclearerr            Z_ADD_PREFIX(gzclearerr)
#    define gzclose               Z_ADD_PREFIX(gzclose)
#    define gzclose_r             Z_ADD_PREFIX(gzclose_r)
#    define gzclose_w             Z_ADD_PREFIX(gzclose_w)
#    define gzdirect              Z_ADD_PREFIX(gzdirect)
#    define gzdopen               Z_ADD_PREFIX(gzdopen)
#    define gzeof                 Z_ADD_PREFIX(gzeof)
#    define gzerror               Z_ADD_PREFIX(gzerror)
#    define gzflush               Z_ADD_PREFIX(gzflush)
#    define gzfread               Z_ADD_PREFIX(gzfread)
#    define gzfwrite              Z_ADD_PREFIX(gzfwrite)
#    define gzgetc                Z_ADD_PREFIX(gzgetc)
#    define gzgetc_               Z_ADD_PREFIX(gzgetc_)
#    define gzgets                Z_ADD_PREFIX(gzgets)
#    define gzoffset              Z_ADD_PREFIX(gzoffset)
#    define gzoffset64            Z_ADD_PREFIX(gzoffset64)
#    define gzopen                Z_ADD_PREFIX(gzopen)
#    define gzopen64              Z_ADD_PREFIX(gzopen64)
#    ifdef _WIN32
#      define gzopen_w              Z_ADD_PREFIX(gzopen_w)
#    endif
#    define gzprintf              Z_ADD_PREFIX(gzprintf)
#    define gzputc                Z_ADD_PREFIX(gzputc)
#    define gzputs                Z_ADD_PREFIX(gzputs)
#    define gzread                Z_ADD_PREFIX(gzread)
#    define gzrewind              Z_ADD_PREFIX(gzrewind)
#    define gzseek                Z_ADD_PREFIX(gzseek)
#    define gzseek64              Z_ADD_PREFIX(gzseek64)
#    define gzsetparams           Z_ADD_PREFIX(gzsetparams)
#    define gztell                Z_ADD_PREFIX(gztell)
#    define gztell64              Z_ADD_PREFIX(gztell64)
#    define gzungetc              Z_ADD_PREFIX(gzungetc)
#    define gzvprintf             Z_ADD_PREFIX(gzvprintf)
#    define gzwrite               Z_ADD_PREFIX(gzwrite)
#  endif
#  define inflate               Z_ADD_PREFIX(inflate)
#  define inflateBack           Z_ADD_PREFIX(inflateBack)
#  define inflateBackEnd        Z_ADD_PREFIX(inflateBackEnd)
#  define inflateBackInit       Z_ADD_PREFIX_STD(inflateBackInit)
#  define inflateBackInit_      Z_ADD_PREFIX(inflateBackInit_)
#  define inflateCodesUsed      Z_ADD_PREFIX(inflateCodesUsed)
#  define inflateCopy           Z_ADD_PREFIX(inflateCopy)
#  define inflateEnd            Z_ADD_PREFIX(inflateEnd)
#  define inflateGetDictionary  Z_ADD_PREFIX(inflateGetDictionary)
#  define inflateGetHeader      Z_ADD_PREFIX(inflateGetHeader)
#  define inflateInit           Z_ADD_PREFIX_STD(inflateInit)
#  define inflateInit2          Z_ADD_PREFIX_STD(inflateInit2)
#  define inflateInit2_         Z_ADD_PREFIX(inflateInit2_)
#  define inflateInit_          Z_ADD_PREFIX(inflateInit_)
#  define inflateMark           Z_ADD_PREFIX(inflateMark)
#  define inflatePrime          Z_ADD_PREFIX(inflatePrime)
#  define inflateReset          Z_ADD_PREFIX(inflateReset)
#  define inflateReset2         Z_ADD_PREFIX(inflateReset2)
#  define inflateResetKeep      Z_ADD_PREFIX(inflateResetKeep)
#  define inflateSetDictionary  Z_ADD_PREFIX(inflateSetDictionary)
#  define inflateSync           Z_ADD_PREFIX(inflateSync)
#  define inflateSyncPoint      Z_ADD_PREFIX(inflateSyncPoint)
#  define inflateUndermine      Z_ADD_PREFIX(inflateUndermine)
#  define inflateValidate       Z_ADD_PREFIX(inflateValidate)
#  define inflate_copyright     Z_ADD_PREFIX(inflate_copyright)
#  define inflate_fast          Z_ADD_PREFIX(inflate_fast)
#  define inflate_table         Z_ADD_PREFIX(inflate_table)
#  ifndef Z_SOLO
#    define uncompress            Z_ADD_PREFIX(uncompress)
#    define uncompress2           Z_ADD_PREFIX(uncompress2)
#  endif
#  define zError                Z_ADD_PREFIX(zError)
#  ifndef Z_SOLO
#    define zcalloc               Z_ADD_PREFIX(zcalloc)
#    define zcfree                Z_ADD_PREFIX(zcfree)
#  endif
#  define zlibCompileFlags      Z_ADD_PREFIX(zlibCompileFlags)
#  define zlibVersion           Z_ADD_PREFIX(zlibVersion)

/* all zlib typedefs in zlib.h and zconf.h */
#  define Byte                  Z_ADD_PREFIX(Byte)
#  define Bytef                 Z_ADD_PREFIX(Bytef)
#  define alloc_func            Z_ADD_PREFIX(alloc_func)
#  define charf                 Z_ADD_PREFIX(charf)
#  define free_func             Z_ADD_PREFIX(free_func)
#  ifndef Z_SOLO
#    define gzFile                Z_ADD_PREFIX(gzFile)
#  endif
#  define gz_header             Z_ADD_PREFIX(gz_header)
#  define gz_headerp            Z_ADD_PREFIX(gz_headerp)
#  define in_func               Z_ADD_PREFIX(in_func)
#  define intf                  Z_ADD_PREFIX(intf)
#  define out_func              Z_ADD_PREFIX(out_func)
#  define uInt                  Z_ADD_PREFIX(uInt)
#  define uIntf                 Z_ADD_PREFIX(uIntf)
#  define uLong                 Z_ADD_PREFIX(uLong)
#  define uLongf                Z_ADD_PREFIX(uLongf)
#  define voidp                 Z_ADD_PREFIX(voidp)
#  define voidpc                Z_ADD_PREFIX(voidpc)
#  define voidpf                Z_ADD_PREFIX(voidpf)

/* all zlib structs in zlib.h and zconf.h */
#  define gz_header_s           Z_ADD_PREFIX(gz_header_s)
#  define internal_state        Z_ADD_PREFIX(internal_state)

/* variable from zutil.h */
#  define z_errmsg              Z_ADD_PREFIX(z_errmsg)
#endif

#if defined(__MSDOS__) && !defined(MSDOS)
#  define MSDOS
#endif
#if (defined(OS_2) || defined(__OS2__)) && !defined(OS2)
#  define OS2
#endif
#if defined(_WINDOWS) && !defined(WINDOWS)
#  define WINDOWS
#endif
#if defined(_WIN32) || defined(_WIN32_WCE) || defined(__WIN32__)
#  ifndef WIN32
#    define WIN32
#  endif
#endif
#if (defined(MSDOS) || defined(OS2) || defined(WINDOWS)) && !defined(WIN32)
#  if !defined(__GNUC__) && !defined(__FLAT__) && !defined(__386__)
#    ifndef SYS16BIT
#      define SYS16BIT
#    endif
#  endif
#endif

/*
 * Compile with -DMAXSEG_64K if the alloc function cannot allocate more
 * than 64k bytes at a time (needed on systems with 16-bit int).
 */
#ifdef SYS16BIT
#  define MAXSEG_64K
#endif
#ifdef MSDOS
#  define UNALIGNED_OK
#endif

#ifdef __STDC_VERSION__
#  ifndef STDC
#    define STDC
#  endif
#  if __STDC_VERSION__ >= 199901L
#    ifndef STDC99
#      define STDC99
#    endif
#  endif
#endif
#if !defined(STDC) && (defined(__STDC__) || defined(__cplusplus))
#  define STDC
#endif
#if !defined(STDC) && (defined(__GNUC__) || defined(__BORLANDC__))
#  define STDC
#endif
#if !defined(STDC) && (defined(MSDOS) || defined(WINDOWS) || defined(WIN32))
#  define STDC
#endif
#if !defined(STDC) && (defined(OS2) || defined(__HOS_AIX__))
#  define STDC
#endif

#if defined(__OS400__) && !defined(STDC)    /* iSeries (formerly AS/400). */
#  define STDC
#endif

#ifndef STDC
#  ifndef const /* cannot use !defined(STDC) && !defined(const) on Mac */
#    define const       /* note: need a more gentle solution here */
#  endif
#endif

#if defined(ZLIB_CONST) && !defined(z_const)
#  define z_const const
#else
#  define z_const
#endif

#ifdef Z_SOLO
   typedef unsigned long z_size_t;
#else
#  define z_longlong long long
#  if defined(NO_SIZE_T)
     typedef unsigned NO_SIZE_T z_size_t;
#  elif defined(STDC)
#    include <stddef.h>
     typedef size_t z_size_t;
#  else
     typedef unsigned long z_size_t;
#  endif
#  undef z_longlong
#endif

/* Maximum value for memLevel in deflateInit2 */
#ifndef MAX_MEM_LEVEL
#  ifdef MAXSEG_64K
#    define MAX_MEM_LEVEL 8
#  else
#    define MAX_MEM_LEVEL 9
#  endif
#endif

/* Maximum value for windowBits in deflateInit2 and inflateInit2.
 * WARNING: reducing MAX_WBITS makes minigzip unable to extract .gz files
 * created by gzip. (Files created by minigzip can still be extracted by
 * gzip.)
 */
#ifndef MAX_WBITS
#  define MAX_WBITS   15 /* 32K LZ77 window */
#endif

/* The memory requirements for deflate are (in bytes):
            (1 << (windowBits+2)) +  (1 << (memLevel+9))
 that is: 128K for windowBits=15  +  128K for memLevel = 8  (default values)
 plus a few kilobytes for small objects. For example, if you want to reduce
 the default memory requirements from 256K to 128K, compile with
     make CFLAGS="-O -DMAX_WBITS=14 -DMAX_MEM_LEVEL=7"
 Of course this will generally degrade compression (there's no free lunch).

   The memory requirements for inflate are (in bytes) 1 << windowBits
 that is, 32K for windowBits=15 (default value) plus about 7 kilobytes
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

#ifndef Z_ARG /* function prototypes for stdarg */
#  if defined(STDC) || defined(Z_HAVE_STDARG_H)
#    define Z_ARG(args)  args
#  else
#    define Z_ARG(args)  ()
#  endif
#endif

/* The following definitions for FAR are needed only for MSDOS mixed
 * model programming (small or medium model with some far allocations).
 * This was tested only with MSC; for other MSDOS compilers you may have
 * to define NO_MEMCPY in zutil.h.  If you don't need the mixed model,
 * just define FAR to be empty.
 */
#ifdef SYS16BIT
#  if defined(M_I86SM) || defined(M_I86MM)
     /* MSC small or medium model */
#    define SMALL_MEDIUM
#    ifdef _MSC_VER
#      define FAR _far
#    else
#      define FAR far
#    endif
#  endif
#  if (defined(__SMALL__) || defined(__MEDIUM__))
     /* Turbo C small or medium model */
#    define SMALL_MEDIUM
#    ifdef __BORLANDC__
#      define FAR _far
#    else
#      define FAR far
#    endif
#  endif
#endif

#if defined(WINDOWS) || defined(WIN32)
   /* If building or using zlib as a DLL, define ZLIB_DLL.
    * This is not mandatory, but it offers a little performance increase.
    */
#  ifdef ZLIB_DLL
#    if defined(WIN32) && (!defined(__BORLANDC__) || (__BORLANDC__ >= 0x500))
#      ifdef ZLIB_INTERNAL
#        define ZEXTERN extern __declspec(dllexport)
#      else
#        define ZEXTERN extern __declspec(dllimport)
#      endif
#    endif
#  endif  /* ZLIB_DLL */
   /* If building or using zlib with the WINAPI/WINAPIV calling convention,
    * define ZLIB_WINAPI.
    * Caution: the standard ZLIB1.DLL is NOT compiled using ZLIB_WINAPI.
    */
#  ifdef ZLIB_WINAPI
#    ifdef FAR
#      undef FAR
#    endif
#    include <windows.h>
     /* No need for _export, use ZLIB.DEF instead. */
     /* For complete Windows compatibility, use WINAPI, not __stdcall. */
#    define ZEXPORT WINAPI
#    ifdef WIN32
#      define ZEXPORTVA WINAPIV
#    else
#      define ZEXPORTVA FAR CDECL
#    endif
#  endif
#endif

#if defined (__BEOS__)
#  ifdef ZLIB_DLL
#    ifdef ZLIB_INTERNAL
#      define ZEXPORT   __declspec(dllexport)
#      define ZEXPORTVA __declspec(dllexport)
#    else
#      define ZEXPORT   __declspec(dllimport)
#      define ZEXPORTVA __declspec(dllimport)
#    endif
#  endif
#endif

#ifndef ZEXTERN
#  define ZEXTERN extern
#endif
#ifndef ZEXPORT
#  define ZEXPORT
#endif
#ifndef ZEXPORTVA
#  define ZEXPORTVA
#endif

#ifndef FAR
#  define FAR
#endif

#if !defined(__MACTYPES__)
typedef unsigned char  Byte;  /* 8 bits */
#endif
typedef unsigned int   uInt;  /* 16 bits or more */
typedef unsigned long  uLong; /* 32 bits or more */

#ifdef SMALL_MEDIUM
   /* Borland C/C++ and some old MSC versions ignore FAR inside typedef */
#  define Bytef Byte FAR
#else
   typedef Byte  FAR Bytef;
#endif
typedef char  FAR charf;
typedef int   FAR intf;
typedef uInt  FAR uIntf;
typedef uLong FAR uLongf;

#ifdef STDC
   typedef void const *voidpc;
   typedef void FAR   *voidpf;
   typedef void       *voidp;
#else
   typedef Byte const *voidpc;
   typedef Byte FAR   *voidpf;
   typedef Byte       *voidp;
#endif

#if !defined(Z_U4) && !defined(Z_SOLO) && defined(STDC)
#  include <limits.h>
#  if (UINT_MAX == 0xffffffffUL)
#    define Z_U4 unsigned
#  elif (ULONG_MAX == 0xffffffffUL)
#    define Z_U4 unsigned long
#  elif (USHRT_MAX == 0xffffffffUL)
#    define Z_U4 unsigned short
#  endif
#endif

#ifdef Z_U4
   typedef Z_U4 z_crc_t;
#else
   typedef unsigned long z_crc_t;
#endif

#ifdef HAVE_UNISTD_H    /* may be set to #if 1 by ./configure */
#  define Z_HAVE_UNISTD_H
#endif

#ifdef HAVE_STDARG_H    /* may be set to #if 1 by ./configure */
#  define Z_HAVE_STDARG_H
#endif

#ifdef STDC
#  ifndef Z_SOLO
#    include <sys/types.h>      /* for off_t */
#  endif
#endif

#if defined(STDC) || defined(Z_HAVE_STDARG_H)
#  ifndef Z_SOLO
#    include <stdarg.h>         /* for va_list */
#  endif
#endif

#ifdef _WIN32
#  ifndef Z_SOLO
#    include <stddef.h>         /* for wchar_t */
#  endif
#endif

/* a little trick to accommodate both "#define _LARGEFILE64_SOURCE" and
 * "#define _LARGEFILE64_SOURCE 1" as requesting 64-bit operations, (even
 * though the former does not conform to the LFS document), but considering
 * both "#undef _LARGEFILE64_SOURCE" and "#define _LARGEFILE64_SOURCE 0" as
 * equivalently requesting no 64-bit operations
 */
#if defined(_LARGEFILE64_SOURCE) && -_LARGEFILE64_SOURCE - -1 == 1
#  undef _LARGEFILE64_SOURCE
#endif

#if defined(__WATCOMC__) && !defined(Z_HAVE_UNISTD_H)
#  define Z_HAVE_UNISTD_H
#endif
#ifndef Z_SOLO
#  if defined(Z_HAVE_UNISTD_H) || defined(_LARGEFILE64_SOURCE)
#    include <unistd.h>         /* for SEEK_*, off_t, and _LFS64_LARGEFILE */
#    ifdef VMS
#      include <unixio.h>       /* for off_t */
#    endif
#    ifndef z_off_t
#      define z_off_t off_t
#    endif
#  endif
#endif

#if defined(_LFS64_LARGEFILE) && _LFS64_LARGEFILE-0
#  define Z_LFS64
#endif

#if defined(_LARGEFILE64_SOURCE) && defined(Z_LFS64)
#  define Z_LARGE64
#endif

#if defined(_FILE_OFFSET_BITS) && _FILE_OFFSET_BITS-0 == 64 && defined(Z_LFS64)
#  define Z_WANT64
#endif

#if !defined(ZLIB_INTERNAL) && defined(Z_WANT64) && defined(Z_PREFIX_)
/*
 * It turns out that some of the functions defined above using custom prefix
 * will actually be macros in this library build, so they must use standard
 * prefix instead. Ugly, but we need to redefine them as we couldn't define
 * them correctly from the beginning as Z_WANT64 wasn't available then yet.
 */
#  undef adler32_combine
#  define adler32_combine       Z_ADD_PREFIX_STD(adler32_combine)
#  undef crc32_combine
#  define crc32_combine         Z_ADD_PREFIX_STD(crc32_combine)
#  ifndef Z_SOLO
#    undef gzoffset
#    define gzoffset              Z_ADD_PREFIX_STD(gzoffset)
#    undef gzopen
#    define gzopen                Z_ADD_PREFIX_STD(gzopen)
#    undef gzseek
#    define gzseek                Z_ADD_PREFIX_STD(gzseek)
#    undef gztell
#    define gztell                Z_ADD_PREFIX_STD(gztell)
#  endif
#endif

#if !defined(SEEK_SET) && !defined(Z_SOLO)
#  define SEEK_SET        0       /* Seek from beginning of file.  */
#  define SEEK_CUR        1       /* Seek from current position.  */
#  define SEEK_END        2       /* Set file pointer to EOF plus "offset" */
#endif

#ifndef z_off_t
#  define z_off_t long
#endif

#if !defined(_WIN32) && defined(Z_LARGE64)
#  define z_off64_t off64_t
#else
#  if defined(_WIN32) && !defined(__GNUC__) && !defined(Z_SOLO)
#    define z_off64_t __int64
#  else
#    define z_off64_t z_off_t
#  endif
#endif

/* MVS linker does not support external names larger than 8 bytes */
#if defined(__MVS__)
  #pragma map(deflateInit_,"DEIN")
  #pragma map(deflateInit2_,"DEIN2")
  #pragma map(deflateEnd,"DEEND")
  #pragma map(deflateBound,"DEBND")
  #pragma map(inflateInit_,"ININ")
  #pragma map(inflateInit2_,"ININ2")
  #pragma map(inflateEnd,"INEND")
  #pragma map(inflateSync,"INSY")
  #pragma map(inflateSetDictionary,"INSEDI")
  #pragma map(compressBound,"CMBND")
  #pragma map(inflate_table,"INTABL")
  #pragma map(inflate_fast,"INFA")
  #pragma map(inflate_copyright,"INCOPY")
#endif

#endif /* ZCONF_H */
