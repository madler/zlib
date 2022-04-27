Building instructions for the DLL versions of Zlib 1.2.12
========================================================

This directory contains projects that build zlib and minizip using
Microsoft Visual C++ 9.0/10.0.

You don't need to build these projects yourself. You can download the
binaries from:
  http://www.winimage.com/zLibDll

More information can be found at this site.





Build instructions for Visual Studio 2008 (32 bits or 64 bits)
--------------------------------------------------------------
- Decompress current zlib, including all contrib/* files
- Compile assembly code (with Visual Studio Command Prompt) by running:
   bld_ml64.bat (in contrib\masmx64)
   bld_ml32.bat (in contrib\masmx86)
- Open contrib\vstudio\vc9\zlibvc.sln with Microsoft Visual C++ 2008
- Or run: vcbuild /rebuild contrib\vstudio\vc9\zlibvc.sln "Release|Win32"

Build instructions for Visual Studio 2010 (32 bits or 64 bits)
--------------------------------------------------------------
- Decompress current zlib, including all contrib/* files
- Open contrib\vstudio\vc10\zlibvc.sln with Microsoft Visual C++ 2010

Build instructions for Visual Studio 2012 (32 bits or 64 bits)
--------------------------------------------------------------
- Decompress current zlib, including all contrib/* files
- Open contrib\vstudio\vc11\zlibvc.sln with Microsoft Visual C++ 2012

Build instructions for Visual Studio 2013 (32 bits or 64 bits)
--------------------------------------------------------------
- Decompress current zlib, including all contrib/* files
- Open contrib\vstudio\vc12\zlibvc.sln with Microsoft Visual C++ 2013

Build instructions for Visual Studio 2015 (32 bits or 64 bits)
--------------------------------------------------------------
- Decompress current zlib, including all contrib/* files
- Open contrib\vstudio\vc14\zlibvc.sln with Microsoft Visual C++ 2015

Build instructions for Visual Studio 2022 (32 bits or 64 bits)
--------------------------------------------------------------
- Open contrib\vstudio\vc14\zlibvc.sln with Microsoft Visual C++ 2022 and
  build any of the available outputs.
  
- The build outputs are:
   zlib.lib - static link library
   zlib1.dll - DLL
   zdll.lib - link library for zlib1.dll
   minizip.exe - Mini zip program
   miniunz.exe - Mini unzip program
   testzlib.exe - test program for static and DLL library
   
- Link libraries are placed in a folder named: zlib/lib/<platform>/<configuration>
  Example:
    64-bit debug static library is: zlib/lib/x64/debug/zlib.lib
    32-bit release DLL import library is: zlib/lib/x86/DLL_release/zdll.lib
    
- Programs and DLLs are placed in a folder named: zlib/exe/<platform>/<configuration>
  Example:
    32-bit release DLL is: zlib/exe/x86/DLL_release/zlib1.dll
    64-bit debug static lib test program is: zlib/exe/x64/debug/testzlib.exe
    
- zlib1.dll created from this build is fully compatible with zlib1.dll
  created using Makefile.msc in win32 folder.
  

Important
---------
- To use zlibwapi.dll in your application, you must define the
  macro ZLIB_WINAPI when compiling your application's source files.


Additional notes
----------------
- This DLL, named zlibwapi.dll, is compatible to the old zlib.dll built
  by Gilles Vollant from the zlib 1.1.x sources, and distributed at
    http://www.winimage.com/zLibDll
  It uses the WINAPI calling convention for the exported functions, and
  includes the minizip functionality. If your application needs that
  particular build of zlib.dll, you can rename zlibwapi.dll to zlib.dll.

- The new DLL was renamed because there exist several incompatible
  versions of zlib.dll on the Internet.

- There is also an official DLL build of zlib, named zlib1.dll. This one
  is exporting the functions using the CDECL convention. See the file
  win32\DLL_FAQ.txt found in this zlib distribution.

- There used to be a ZLIB_DLL macro in zlib 1.1.x, but now this symbol
  has a slightly different effect. To avoid compatibility problems, do
  not define it here.


Gilles Vollant
info@winimage.com

Visual Studio 2013 and 2015 Projects from Sean Hunt
seandhunt_7@yahoo.com

Visual Studio 2022 from Mircea Neacsu
mircea@neacsu.net