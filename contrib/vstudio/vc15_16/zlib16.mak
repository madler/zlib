# Microsoft Visual C++ generated build script - Do not modify

PROJ = ZLIB16
DEBUG = 0
PROGTYPE = 1
CALLER = 
ARGS = 
DLLS = 
D_RCDEFINES = -d_DEBUG
R_RCDEFINES = -dNDEBUG
ORIGIN = MSVC
ORIGIN_VER = 1.00
PROJPATH = c:\zlib\
USEMFC = 0
CC = cl
CPP = cl
CXX = cl
CCREATEPCHFLAG = 
CPPCREATEPCHFLAG = 
CUSEPCHFLAG = 
CPPUSEPCHFLAG = 
FIRSTC = ADLER32.C   
FIRSTCPP =             
RC = rc
CFLAGS_D_WDLL = /nologo /G2 /W3 /Zi /ALw /Od /D "_DEBUG" /D "WINDOWS" /D "ZLIB_DLL" /FR /GD /Fd"ZLIB.PDB"
CFLAGS_R_WDLL = /nologo /W3 /ALw /O1 /D "NDEBUG" /D "WINDOWS" /D "ZLIB_DLL" /FR /GD 
LFLAGS_D_WDLL = /NOLOGO /ONERROR:NOEXE /NOD /PACKC:61440 /CO /NOE /ALIGN:16 /MAP:FULL
LFLAGS_R_WDLL = /NOLOGO /ONERROR:NOEXE /NOD /PACKC:61440 /NOE /ALIGN:16 /MAP:FULL
LIBS_D_WDLL = oldnames libw commdlg shell olecli olesvr ldllcew
LIBS_R_WDLL = oldnames libw commdlg shell olecli olesvr ldllcew
RCFLAGS = /nologo
RESFLAGS = /nologo
RUNFLAGS = 
DEFFILE = ZLIB16.DEF
OBJS_EXT = 
LIBS_EXT = 
!if "$(DEBUG)" == "1"
CFLAGS = $(CFLAGS_D_WDLL)
LFLAGS = $(LFLAGS_D_WDLL)
LIBS = $(LIBS_D_WDLL)
MAPFILE = nul
RCDEFINES = $(D_RCDEFINES)
!else
CFLAGS = $(CFLAGS_R_WDLL)
LFLAGS = $(LFLAGS_R_WDLL)
LIBS = $(LIBS_R_WDLL)
MAPFILE = nul
RCDEFINES = $(R_RCDEFINES)
!endif
!if [if exist MSVC.BND del MSVC.BND]
!endif
SBRS = ADLER32.SBR \
		COMPRESS.SBR \
		CRC32.SBR \
		DEFLATE.SBR \
		GZIO.SBR \
		INFFAST.SBR \
		INFLATE.SBR \
		TREES.SBR \
		UNCOMPR.SBR \
		ZUTIL.SBR \
		ZIP.SBR \
		UNZIP.SBR \
		INFBACK.SBR \
		IOAPI.SBR \
		INFTREES.SBR


ADLER32_DEP = c:\zlib\zlib.h \
	c:\zlib\zconf.h


COMPRESS_DEP = c:\zlib\zlib.h \
	c:\zlib\zconf.h


CRC32_DEP = c:\zlib\zlib.h \
	c:\zlib\zconf.h \
	c:\zlib\crc32.h


DEFLATE_DEP = c:\zlib\deflate.h \
	c:\zlib\zutil.h \
	c:\zlib\zlib.h \
	c:\zlib\zconf.h


GZIO_DEP = c:\zlib\zutil.h \
	c:\zlib\zlib.h \
	c:\zlib\zconf.h


INFFAST_DEP = c:\zlib\zutil.h \
	c:\zlib\zlib.h \
	c:\zlib\zconf.h \
	c:\zlib\inftrees.h \
	c:\zlib\inflate.h \
	c:\zlib\inffast.h


INFLATE_DEP = c:\zlib\zutil.h \
	c:\zlib\zlib.h \
	c:\zlib\zconf.h \
	c:\zlib\inftrees.h \
	c:\zlib\inflate.h \
	c:\zlib\inffast.h \
	c:\zlib\inffixed.h


TREES_DEP = c:\zlib\deflate.h \
	c:\zlib\zutil.h \
	c:\zlib\zlib.h \
	c:\zlib\zconf.h \
	c:\zlib\trees.h


UNCOMPR_DEP = c:\zlib\zlib.h \
	c:\zlib\zconf.h


ZUTIL_DEP = c:\zlib\zutil.h \
	c:\zlib\zlib.h \
	c:\zlib\zconf.h


ZLIB16_RCDEP = 

ZIP_DEP = c:\zlib\zlib.h \
	c:\zlib\zconf.h \
	c:\zlib\zip.h \
	c:\zlib\ioapi.h


UNZIP_DEP = c:\zlib\zlib.h \
	c:\zlib\zconf.h \
	c:\zlib\unzip.h \
	c:\zlib\ioapi.h


INFBACK_DEP = c:\zlib\zutil.h \
	c:\zlib\zlib.h \
	c:\zlib\zconf.h \
	c:\zlib\inftrees.h \
	c:\zlib\inflate.h \
	c:\zlib\inffast.h \
	c:\zlib\inffixed.h


IOAPI_DEP = c:\zlib\zlib.h \
	c:\zlib\zconf.h \
	c:\zlib\ioapi.h


INFTREES_DEP = c:\zlib\zutil.h \
	c:\zlib\zlib.h \
	c:\zlib\zconf.h \
	c:\zlib\inftrees.h


all:	$(PROJ).DLL $(PROJ).BSC

ADLER32.OBJ:	ADLER32.C $(ADLER32_DEP)
	$(CC) $(CFLAGS) $(CCREATEPCHFLAG) /c ADLER32.C

COMPRESS.OBJ:	COMPRESS.C $(COMPRESS_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c COMPRESS.C

CRC32.OBJ:	CRC32.C $(CRC32_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c CRC32.C

DEFLATE.OBJ:	DEFLATE.C $(DEFLATE_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c DEFLATE.C

GZIO.OBJ:	GZIO.C $(GZIO_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c GZIO.C

INFFAST.OBJ:	INFFAST.C $(INFFAST_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c INFFAST.C

INFLATE.OBJ:	INFLATE.C $(INFLATE_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c INFLATE.C

TREES.OBJ:	TREES.C $(TREES_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c TREES.C

UNCOMPR.OBJ:	UNCOMPR.C $(UNCOMPR_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c UNCOMPR.C

ZUTIL.OBJ:	ZUTIL.C $(ZUTIL_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ZUTIL.C

ZLIB16.RES:	ZLIB16.RC $(ZLIB16_RCDEP)
	$(RC) $(RCFLAGS) $(RCDEFINES) -r ZLIB16.RC

ZIP.OBJ:	ZIP.C $(ZIP_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ZIP.C

UNZIP.OBJ:	UNZIP.C $(UNZIP_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c UNZIP.C

INFBACK.OBJ:	INFBACK.C $(INFBACK_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c INFBACK.C

IOAPI.OBJ:	IOAPI.C $(IOAPI_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c IOAPI.C

INFTREES.OBJ:	INFTREES.C $(INFTREES_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c INFTREES.C


$(PROJ).DLL::	ZLIB16.RES

$(PROJ).DLL::	ADLER32.OBJ COMPRESS.OBJ CRC32.OBJ DEFLATE.OBJ GZIO.OBJ INFFAST.OBJ \
	INFLATE.OBJ TREES.OBJ UNCOMPR.OBJ ZUTIL.OBJ ZIP.OBJ UNZIP.OBJ INFBACK.OBJ IOAPI.OBJ \
	INFTREES.OBJ $(OBJS_EXT) $(DEFFILE)
	echo >NUL @<<$(PROJ).CRF
ADLER32.OBJ +
COMPRESS.OBJ +
CRC32.OBJ +
DEFLATE.OBJ +
GZIO.OBJ +
INFFAST.OBJ +
INFLATE.OBJ +
TREES.OBJ +
UNCOMPR.OBJ +
ZUTIL.OBJ +
ZIP.OBJ +
UNZIP.OBJ +
INFBACK.OBJ +
IOAPI.OBJ +
INFTREES.OBJ +
$(OBJS_EXT)
$(PROJ).DLL
$(MAPFILE)
C:\MSVC\LIB\+
C:\MSVC\MFC\LIB\+
E:\PROGRAMFILES\MICROSOFTVISUALSTUDIO.NET\FRAMEWORKSDK\LIB\+
$(LIBS)
$(DEFFILE);
<<
	link $(LFLAGS) @$(PROJ).CRF
	$(RC) $(RESFLAGS) ZLIB16.RES $@
	@copy $(PROJ).CRF MSVC.BND
	implib /nowep $(PROJ).LIB $(PROJ).DLL

$(PROJ).DLL::	ZLIB16.RES
	if not exist MSVC.BND 	$(RC) $(RESFLAGS) ZLIB16.RES $@

run: $(PROJ).DLL
	$(PROJ) $(RUNFLAGS)


$(PROJ).BSC: $(SBRS)
	bscmake @<<
/o$@ $(SBRS)
<<
