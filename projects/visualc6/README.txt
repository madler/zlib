Microsoft Developer Studio Project Files, Format Version 6.00 for zlib.

Copyright (C) 2000-2004 Simon-Pierre Cadieux.
Copyright (C) 2004 Cosmin Truta.
For conditions of distribution and use, see copyright notice in zlib.h.


To use:

1) On the main menu, select "File | Open Workspace".
   Open "zlib.dsw".

2) Select "Build | Set Active Configuration".
   Choose the configuration you wish to build.

3) Select "Build | Clean".

4) Select "Build | Build ... (F7)".  Ignore warning messages about
   not being able to find certain include files (e.g. alloc.h).

5) If you built one of the sample programs (example or minigzip),
   select "Build | Execute ... (Ctrl+F5)".


This project builds the zlib binaries as follows:

* Win32_DLL_Release\zlib1.dll       DLL build
* Win32_DLL_Debug\zlib1d.dll        DLL build (debug version)
* Win32_DLL_ASM_Release\zlib1.dll   DLL build using ASM code
* Win32_DLL_ASM_Debug\zlib1d.dll    DLL build using ASM code (debug version)
* Win32_LIB_Release\zlib.lib        static build
* Win32_LIB_Debug\zlibd.lib         static build (debug version)
* Win32_LIB_ASM_Release\zlib.lib    static build using ASM code
* Win32_LIB_ASM_Debug\zlibd.lib     static build using ASM code (debug version)


For more information regarding the DLL builds, please see the DLL FAQ
in ..\..\win32\DLL_FAQ.txt.
