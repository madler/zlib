@echo off
REM Set up \Microsoft Visual Studio 2015, where <arch> is \c amd64, \c x86, etc.
reg Query "HKLM\Hardware\Description\System\CentralProcessor\0" | find /i "x86" > NUL && set MACH_ARCH=Win32 || set MACH_ARCH=x64

if %MACH_ARCH% == %1 (set ARCH=x86) else (set ARCH=amd64_x86)

CALL "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" %ARCH%

ml /coff /Zi /c /safeseh /Flmatch686.lst match686.asm
ml /coff /Zi /c /safeseh /Flinffas32.lst inffas32.asm
