zlib 0.93 is a beta version of a general purpose compression library.

The data format used by the zlib library is described in the
files zlib-3.1.doc, deflate-1.1.doc and gzip-4.1.doc, available
in ftp.uu.net:/pub/archiving/zip/doc.

All functions of the compression library are documented in the file
zlib.h. A usage example of the library is given in the file example.c
which also tests that the library is working correctly.

To compile all files and run the test program, just type: make test
(For MSDOS, use one of the special makefiles such as Makefile.msc.)
To install the zlib library (libgz.a) in /usr/local/lib, type: make install
To install in a different directory, use for example: make install prefix=$HOME
This will install in $HOME/lib instead of /usr/local/lib.

The changes made in version 0.93 are documented in the file ChangeLog.
The main changes since 0.9 are:
- temporarily disable inline functions
- make deflate deterministic
- don't use signed char in inflate (not portable enough)
- fix inflate memory leak for segmented architectures
- Default MEM_LEVEL is 8 (not 9 for Unix) as documented in zlib.h
- Document the memory requirements in zconf.h
- added "make install"
- added support for DJGPP and Pyramid
- fix an inflate bug for stored blocks.
- various speedups

On MSDOS, this version works in both large and small model. However
small model compression works only for small values of MAX_MEM_LEVEL
and MAX_WBITS (see zconf.h). Small model decompression should work up
to MAX_WBITS=15.  This version of zlib does not support small or
medium model with far allocation of big objects.


  Copyright (C) 1995 Jean-loup Gailly and Mark Adler

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Jean-loup Gailly        Mark Adler
  gzip@prep.ai.mit.edu    madler@alumni.caltech.edu

If you use the zlib library in a product, we would appreciate *not*
receiving lengthy legal documents to sign. The sources are provided
for free but without warranty of any kind.  The library has been
entirely written by Jean-loup Gailly and Mark Adler; it does not
include third-party code.

If you redistribute modified sources, we would appreciate that you include
in the file ChangeLog history information documenting your changes.
