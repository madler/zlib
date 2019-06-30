Building instructions for the DLL versions of Zlib 1.2.11.1
===========================================================

This directory contains projects that build zlib and minizip using
Microsoft Visual C++.

Build instructions
------------------
Since most modern versions of Visual Studio are now capable of 
building against multiple versions of the Visual C++ toolchain, 
and because the VC runtime used is what determines binary 
compatibility, the VS projects have been reorganized by runtime 
version rather than IDE version. You will still need to have 
the original Visual Studio version installed to get the SDK 
for a given runtime version, however given this the project 
will likely compile on a wide range of Visual Studio versions.

The mapping between projects, original VS versions, and runtimes is
as follows:

CRT Version		VS Project							Original VS version
90				contrib\vstudio\v90\zlibvc.sln		Visual Studio 2008
100				contrib\vstudio\v100\zlibvc.sln	Visual Studio 2010
110				contrib\vstudio\v110\zlibvc.sln	Visual Studio 2012
120				contrib\vstudio\v120\zlibvc.sln	Visual Studio 2013
140				contrib\vstudio\v140\zlibvc.sln	Visual Studio 2015
142				contrib\vstudio\v142\zlibvc.sln	Visual Studio 2019

Be aware that the currently extant versions of Microsoft's Universal 
CRT, versions 140 and above, share the same ABI (Application-Binary 
Interface) and can therefore be used interchangeably. The ABI may 
change again in future revisions.

Important note on calling conventions
-------------------------------------

The official build of zlib uses the CDECL calling convention for all 
functions. However, for compatibility with various Windows applications, 
zlibwapi.dll uses the WINAPI (__stdcall) calling convention for all 
functions except those with variable arguments, which remain CDECL.
This is controlled by the ZLIB_WINAPI symbol - see zconf.h for details.

To use this DLL correctly, you must define the ZLIB_WINAPI symbol before 
including the zlib headers in your code.

For more detailed technical notes on this topic, see win32\DLL_FAQ.txt 
found in this zlib distribution.

Additional/historical notes
---------------------------
- This DLL, named zlibwapi.dll, is compatible to the old zlib.dll built
  by Gilles Vollant from the zlib 1.1.x sources, and distributed at
    http://www.winimage.com/zLibDll
  It uses the WINAPI calling convention for the exported functions, and
  includes the minizip functionality. If your application needs that
  particular build of zlib.dll, you can rename zlibwapi.dll to zlib.dll.

- The ZLIB_DLL symbol referenced in zconf.h is intentionally not defined 
  in these projects due to reported compatibility issues in the past. 
  For more information about this, see the notes in win32\DLL_FAQ.txt 
  found in this zlib distribution.

- The hand-tuned assembler modules have been removed from zlib as of 
  version 1.2.11.1. However, only the VS 2019 project has been fully
  updated to reflect this. Others will either need to be modified by 
  hand or built in the "ReleaseWithoutAsm" configuration only.


Gilles Vollant
info@winimage.com

Visual Studio 2013 and 2015 Projects from Sean Hunt
seandhunt_7@yahoo.com

Visual Studio 2019 projects contributed by Andrew King