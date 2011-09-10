# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=zlibvc - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to zlibvc - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "zlibvc - Win32 Release" && "$(CFG)" != "zlibvc - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "zlibvc.mak" CFG="zlibvc - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "zlibvc - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "zlibvc - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "zlibvc - Win32 Debug"
CPP=cl.exe
RSC=rc.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "zlibvc - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\zlib.dll" "$(OUTDIR)\zlibvc.bsc"

CLEAN : 
	-@erase "$(INTDIR)\adler32.obj"
	-@erase "$(INTDIR)\adler32.sbr"
	-@erase "$(INTDIR)\compress.obj"
	-@erase "$(INTDIR)\compress.sbr"
	-@erase "$(INTDIR)\crc32.obj"
	-@erase "$(INTDIR)\crc32.sbr"
	-@erase "$(INTDIR)\deflate.obj"
	-@erase "$(INTDIR)\deflate.sbr"
	-@erase "$(INTDIR)\gvmat32c.obj"
	-@erase "$(INTDIR)\gvmat32c.sbr"
	-@erase "$(INTDIR)\gzio.obj"
	-@erase "$(INTDIR)\gzio.sbr"
	-@erase "$(INTDIR)\infblock.obj"
	-@erase "$(INTDIR)\infblock.sbr"
	-@erase "$(INTDIR)\infcodes.obj"
	-@erase "$(INTDIR)\infcodes.sbr"
	-@erase "$(INTDIR)\inffast.obj"
	-@erase "$(INTDIR)\inffast.sbr"
	-@erase "$(INTDIR)\inflate.obj"
	-@erase "$(INTDIR)\inflate.sbr"
	-@erase "$(INTDIR)\inftrees.obj"
	-@erase "$(INTDIR)\inftrees.sbr"
	-@erase "$(INTDIR)\infutil.obj"
	-@erase "$(INTDIR)\infutil.sbr"
	-@erase "$(INTDIR)\trees.obj"
	-@erase "$(INTDIR)\trees.sbr"
	-@erase "$(INTDIR)\uncompr.obj"
	-@erase "$(INTDIR)\uncompr.sbr"
	-@erase "$(INTDIR)\zlib.res"
	-@erase "$(INTDIR)\zutil.obj"
	-@erase "$(INTDIR)\zutil.sbr"
	-@erase "$(OUTDIR)\zlib.dll"
	-@erase "$(OUTDIR)\zlib.exp"
	-@erase "$(OUTDIR)\zlib.lib"
	-@erase "$(OUTDIR)\zlib.map"
	-@erase "$(OUTDIR)\zlibvc.bsc"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D fdopen=_fdopen /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_WIN32" /D "BUILD_ZLIBDLL" /D "ZLIB_DLL" /D "ASMV" /FR /YX /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "NDEBUG" /D fdopen=_fdopen /D "WIN32" /D\
 "_WINDOWS" /D "_WINDLL" /D "_WIN32" /D "BUILD_ZLIBDLL" /D "ZLIB_DLL" /D "ASMV"\
 /FR"$(INTDIR)/" /Fp"$(INTDIR)/zlibvc.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\Release/
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /d "NDEBUG"
RSC_PROJ=/l 0x40c /fo"$(INTDIR)/zlib.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/zlibvc.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\adler32.sbr" \
	"$(INTDIR)\compress.sbr" \
	"$(INTDIR)\crc32.sbr" \
	"$(INTDIR)\deflate.sbr" \
	"$(INTDIR)\gvmat32c.sbr" \
	"$(INTDIR)\gzio.sbr" \
	"$(INTDIR)\infblock.sbr" \
	"$(INTDIR)\infcodes.sbr" \
	"$(INTDIR)\inffast.sbr" \
	"$(INTDIR)\inflate.sbr" \
	"$(INTDIR)\inftrees.sbr" \
	"$(INTDIR)\infutil.sbr" \
	"$(INTDIR)\trees.sbr" \
	"$(INTDIR)\uncompr.sbr" \
	"$(INTDIR)\zutil.sbr"

"$(OUTDIR)\zlibvc.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib crtdll.lib /nologo /subsystem:windows /dll /map /machine:I386 /nodefaultlib /out:"Release/zlib.dll"
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib crtdll.lib /nologo\
 /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)/zlib.pdb"\
 /map:"$(INTDIR)/zlib.map" /machine:I386 /nodefaultlib /def:".\zlib.def"\
 /out:"$(OUTDIR)/zlib.dll" /implib:"$(OUTDIR)/zlib.lib" 
DEF_FILE= \
	".\zlib.def"
LINK32_OBJS= \
	"$(INTDIR)\adler32.obj" \
	"$(INTDIR)\compress.obj" \
	"$(INTDIR)\crc32.obj" \
	"$(INTDIR)\deflate.obj" \
	"$(INTDIR)\gvmat32c.obj" \
	"$(INTDIR)\gzio.obj" \
	"$(INTDIR)\infblock.obj" \
	"$(INTDIR)\infcodes.obj" \
	"$(INTDIR)\inffast.obj" \
	"$(INTDIR)\inflate.obj" \
	"$(INTDIR)\inftrees.obj" \
	"$(INTDIR)\infutil.obj" \
	"$(INTDIR)\trees.obj" \
	"$(INTDIR)\uncompr.obj" \
	"$(INTDIR)\zlib.res" \
	"$(INTDIR)\zutil.obj" \
	".\GVMAT32.obj"

"$(OUTDIR)\zlib.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\zlib.dll"

CLEAN : 
	-@erase "$(INTDIR)\adler32.obj"
	-@erase "$(INTDIR)\compress.obj"
	-@erase "$(INTDIR)\crc32.obj"
	-@erase "$(INTDIR)\deflate.obj"
	-@erase "$(INTDIR)\gvmat32c.obj"
	-@erase "$(INTDIR)\gzio.obj"
	-@erase "$(INTDIR)\infblock.obj"
	-@erase "$(INTDIR)\infcodes.obj"
	-@erase "$(INTDIR)\inffast.obj"
	-@erase "$(INTDIR)\inflate.obj"
	-@erase "$(INTDIR)\inftrees.obj"
	-@erase "$(INTDIR)\infutil.obj"
	-@erase "$(INTDIR)\trees.obj"
	-@erase "$(INTDIR)\uncompr.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\zlib.res"
	-@erase "$(INTDIR)\zutil.obj"
	-@erase "$(OUTDIR)\zlib.dll"
	-@erase "$(OUTDIR)\zlib.exp"
	-@erase "$(OUTDIR)\zlib.ilk"
	-@erase "$(OUTDIR)\zlib.lib"
	-@erase "$(OUTDIR)\zlib.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_WIN32" /D "BUILD_ZLIBDLL" /D "ZLIB_DLL" /YX /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_WINDLL" /D "_WIN32" /D "BUILD_ZLIBDLL" /D "ZLIB_DLL"\
 /Fp"$(INTDIR)/zlibvc.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /d "_DEBUG"
RSC_PROJ=/l 0x40c /fo"$(INTDIR)/zlib.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/zlibvc.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"Debug/zlib.dll"
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo\
 /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)/zlib.pdb" /debug\
 /machine:I386 /def:".\zlib.def" /out:"$(OUTDIR)/zlib.dll"\
 /implib:"$(OUTDIR)/zlib.lib" 
DEF_FILE= \
	".\zlib.def"
LINK32_OBJS= \
	"$(INTDIR)\adler32.obj" \
	"$(INTDIR)\compress.obj" \
	"$(INTDIR)\crc32.obj" \
	"$(INTDIR)\deflate.obj" \
	"$(INTDIR)\gvmat32c.obj" \
	"$(INTDIR)\gzio.obj" \
	"$(INTDIR)\infblock.obj" \
	"$(INTDIR)\infcodes.obj" \
	"$(INTDIR)\inffast.obj" \
	"$(INTDIR)\inflate.obj" \
	"$(INTDIR)\inftrees.obj" \
	"$(INTDIR)\infutil.obj" \
	"$(INTDIR)\trees.obj" \
	"$(INTDIR)\uncompr.obj" \
	"$(INTDIR)\zlib.res" \
	"$(INTDIR)\zutil.obj" \
	".\GVMAT32.obj"

"$(OUTDIR)\zlib.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "zlibvc - Win32 Release"
# Name "zlibvc - Win32 Debug"

!IF  "$(CFG)" == "zlibvc - Win32 Release"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\adler32.c
DEP_CPP_ADLER=\
	".\zconf.h"\
	".\zlib.h"\
	

!IF  "$(CFG)" == "zlibvc - Win32 Release"


"$(INTDIR)\adler32.obj" : $(SOURCE) $(DEP_CPP_ADLER) "$(INTDIR)"

"$(INTDIR)\adler32.sbr" : $(SOURCE) $(DEP_CPP_ADLER) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"


"$(INTDIR)\adler32.obj" : $(SOURCE) $(DEP_CPP_ADLER) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\compress.c
DEP_CPP_COMPR=\
	".\zconf.h"\
	".\zlib.h"\
	

!IF  "$(CFG)" == "zlibvc - Win32 Release"


"$(INTDIR)\compress.obj" : $(SOURCE) $(DEP_CPP_COMPR) "$(INTDIR)"

"$(INTDIR)\compress.sbr" : $(SOURCE) $(DEP_CPP_COMPR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"


"$(INTDIR)\compress.obj" : $(SOURCE) $(DEP_CPP_COMPR) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\crc32.c
DEP_CPP_CRC32=\
	".\zconf.h"\
	".\zlib.h"\
	

!IF  "$(CFG)" == "zlibvc - Win32 Release"


"$(INTDIR)\crc32.obj" : $(SOURCE) $(DEP_CPP_CRC32) "$(INTDIR)"

"$(INTDIR)\crc32.sbr" : $(SOURCE) $(DEP_CPP_CRC32) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"


"$(INTDIR)\crc32.obj" : $(SOURCE) $(DEP_CPP_CRC32) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\deflate.c

!IF  "$(CFG)" == "zlibvc - Win32 Release"

DEP_CPP_DEFLA=\
	".\deflate.h"\
	".\zconf.h"\
	".\zlib.h"\
	".\zutil.h"\
	

"$(INTDIR)\deflate.obj" : $(SOURCE) $(DEP_CPP_DEFLA) "$(INTDIR)"

"$(INTDIR)\deflate.sbr" : $(SOURCE) $(DEP_CPP_DEFLA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"

DEP_CPP_DEFLA=\
	".\deflate.h"\
	".\zconf.h"\
	".\zlib.h"\
	".\zutil.h"\
	
NODEP_CPP_DEFLA=\
	".\local"\
	

"$(INTDIR)\deflate.obj" : $(SOURCE) $(DEP_CPP_DEFLA) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\gzio.c
DEP_CPP_GZIO_=\
	".\zconf.h"\
	".\zlib.h"\
	".\zutil.h"\
	

!IF  "$(CFG)" == "zlibvc - Win32 Release"


"$(INTDIR)\gzio.obj" : $(SOURCE) $(DEP_CPP_GZIO_) "$(INTDIR)"

"$(INTDIR)\gzio.sbr" : $(SOURCE) $(DEP_CPP_GZIO_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"


"$(INTDIR)\gzio.obj" : $(SOURCE) $(DEP_CPP_GZIO_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\infblock.c
DEP_CPP_INFBL=\
	".\infblock.h"\
	".\infcodes.h"\
	".\inftrees.h"\
	".\infutil.h"\
	".\zconf.h"\
	".\zlib.h"\
	".\zutil.h"\
	

!IF  "$(CFG)" == "zlibvc - Win32 Release"


"$(INTDIR)\infblock.obj" : $(SOURCE) $(DEP_CPP_INFBL) "$(INTDIR)"

"$(INTDIR)\infblock.sbr" : $(SOURCE) $(DEP_CPP_INFBL) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"


"$(INTDIR)\infblock.obj" : $(SOURCE) $(DEP_CPP_INFBL) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\infcodes.c
DEP_CPP_INFCO=\
	".\infblock.h"\
	".\infcodes.h"\
	".\inffast.h"\
	".\inftrees.h"\
	".\infutil.h"\
	".\zconf.h"\
	".\zlib.h"\
	".\zutil.h"\
	

!IF  "$(CFG)" == "zlibvc - Win32 Release"


"$(INTDIR)\infcodes.obj" : $(SOURCE) $(DEP_CPP_INFCO) "$(INTDIR)"

"$(INTDIR)\infcodes.sbr" : $(SOURCE) $(DEP_CPP_INFCO) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"


"$(INTDIR)\infcodes.obj" : $(SOURCE) $(DEP_CPP_INFCO) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\inffast.c
DEP_CPP_INFFA=\
	".\infblock.h"\
	".\infcodes.h"\
	".\inffast.h"\
	".\inftrees.h"\
	".\infutil.h"\
	".\zconf.h"\
	".\zlib.h"\
	".\zutil.h"\
	

!IF  "$(CFG)" == "zlibvc - Win32 Release"


"$(INTDIR)\inffast.obj" : $(SOURCE) $(DEP_CPP_INFFA) "$(INTDIR)"

"$(INTDIR)\inffast.sbr" : $(SOURCE) $(DEP_CPP_INFFA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"


"$(INTDIR)\inffast.obj" : $(SOURCE) $(DEP_CPP_INFFA) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\inflate.c
DEP_CPP_INFLA=\
	".\infblock.h"\
	".\zconf.h"\
	".\zlib.h"\
	".\zutil.h"\
	

!IF  "$(CFG)" == "zlibvc - Win32 Release"


"$(INTDIR)\inflate.obj" : $(SOURCE) $(DEP_CPP_INFLA) "$(INTDIR)"

"$(INTDIR)\inflate.sbr" : $(SOURCE) $(DEP_CPP_INFLA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"


"$(INTDIR)\inflate.obj" : $(SOURCE) $(DEP_CPP_INFLA) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\inftrees.c
DEP_CPP_INFTR=\
	".\inftrees.h"\
	".\zconf.h"\
	".\zlib.h"\
	".\zutil.h"\
	

!IF  "$(CFG)" == "zlibvc - Win32 Release"


"$(INTDIR)\inftrees.obj" : $(SOURCE) $(DEP_CPP_INFTR) "$(INTDIR)"

"$(INTDIR)\inftrees.sbr" : $(SOURCE) $(DEP_CPP_INFTR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"


"$(INTDIR)\inftrees.obj" : $(SOURCE) $(DEP_CPP_INFTR) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\infutil.c
DEP_CPP_INFUT=\
	".\infblock.h"\
	".\infcodes.h"\
	".\inftrees.h"\
	".\infutil.h"\
	".\zconf.h"\
	".\zlib.h"\
	".\zutil.h"\
	

!IF  "$(CFG)" == "zlibvc - Win32 Release"


"$(INTDIR)\infutil.obj" : $(SOURCE) $(DEP_CPP_INFUT) "$(INTDIR)"

"$(INTDIR)\infutil.sbr" : $(SOURCE) $(DEP_CPP_INFUT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"


"$(INTDIR)\infutil.obj" : $(SOURCE) $(DEP_CPP_INFUT) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\trees.c
DEP_CPP_TREES=\
	".\deflate.h"\
	".\zconf.h"\
	".\zlib.h"\
	".\zutil.h"\
	

!IF  "$(CFG)" == "zlibvc - Win32 Release"


"$(INTDIR)\trees.obj" : $(SOURCE) $(DEP_CPP_TREES) "$(INTDIR)"

"$(INTDIR)\trees.sbr" : $(SOURCE) $(DEP_CPP_TREES) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"


"$(INTDIR)\trees.obj" : $(SOURCE) $(DEP_CPP_TREES) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\uncompr.c

!IF  "$(CFG)" == "zlibvc - Win32 Release"

DEP_CPP_UNCOM=\
	".\zconf.h"\
	".\zlib.h"\
	

"$(INTDIR)\uncompr.obj" : $(SOURCE) $(DEP_CPP_UNCOM) "$(INTDIR)"

"$(INTDIR)\uncompr.sbr" : $(SOURCE) $(DEP_CPP_UNCOM) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"

DEP_CPP_UNCOM=\
	".\zconf.h"\
	".\zlib.h"\
	
NODEP_CPP_UNCOM=\
	".\uncompress"\
	

"$(INTDIR)\uncompr.obj" : $(SOURCE) $(DEP_CPP_UNCOM) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\zutil.c
DEP_CPP_ZUTIL=\
	".\zconf.h"\
	".\zlib.h"\
	".\zutil.h"\
	

!IF  "$(CFG)" == "zlibvc - Win32 Release"


"$(INTDIR)\zutil.obj" : $(SOURCE) $(DEP_CPP_ZUTIL) "$(INTDIR)"

"$(INTDIR)\zutil.sbr" : $(SOURCE) $(DEP_CPP_ZUTIL) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"


"$(INTDIR)\zutil.obj" : $(SOURCE) $(DEP_CPP_ZUTIL) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\zlib.rc

"$(INTDIR)\zlib.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\zlib.def

!IF  "$(CFG)" == "zlibvc - Win32 Release"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\GVMAT32.obj

!IF  "$(CFG)" == "zlibvc - Win32 Release"

!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\gvmat32c.c

!IF  "$(CFG)" == "zlibvc - Win32 Release"

DEP_CPP_GVMAT=\
	".\deflate.h"\
	".\zconf.h"\
	".\zlib.h"\
	".\zutil.h"\
	

"$(INTDIR)\gvmat32c.obj" : $(SOURCE) $(DEP_CPP_GVMAT) "$(INTDIR)"

"$(INTDIR)\gvmat32c.sbr" : $(SOURCE) $(DEP_CPP_GVMAT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "zlibvc - Win32 Debug"

DEP_CPP_GVMAT=\
	".\deflate.h"\
	".\zconf.h"\
	".\zlib.h"\
	".\zutil.h"\
	

"$(INTDIR)\gvmat32c.obj" : $(SOURCE) $(DEP_CPP_GVMAT) "$(INTDIR)"


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
