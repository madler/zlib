For create the 16 and 32 bits DLL of Zlib 1.20

For the 16 bits :
unzip zlib120.zip and copy file from contrib\vstudio\vc15_16 and from contrib\minizip in the same directory
open zlib16.mak with Microsoft Visual C++ 1.52


For the 32 bits :
unzip zlib120.zip and copy file from contrib\vstudio\vc70_32 and from contrib\minizip in the same directory
You can also need unzip http://www.winimage.com/zLibDll/crtdll.zip

If you are using x86, use target Release
open zlibvc.sln with Microsoft Visual C++ 7.0 (Visual Studio .net)


Note : You don't need recompile yourself. There is compiled .LIB in
  http://www.winimage.com/zLibDll
