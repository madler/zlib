# Makefile for zlib
# OpenWatcom large model
# Last updated: 28-Dec-2005

# To use, do "wmake -f watcom_l.mak"

C_SOURCE =  adler32.c  compress.c crc32.c   deflate.c    &
            gzio.c     infback.c  inffast.c inflate.c    &
            inftrees.c trees.c    uncompr.c zutil.c

OBJS =      adler32.obj  compress.obj crc32.obj   deflate.obj    &
            gzio.obj     infback.obj  inffast.obj inflate.obj    &
            inftrees.obj trees.obj    uncompr.obj zutil.obj

CC       = wcc
LINKER   = wcl
CFLAGS   = -zq -ml -s -bt=dos -oilrtfm -fr=nul -wx
ZLIB_LIB = zlib_l.lib

.C.OBJ:	
        $(CC) $(CFLAGS) $[@

all: $(ZLIB_LIB) example.exe minigzip.exe

$(ZLIB_LIB): $(OBJS)
	wlib -b -c $(ZLIB_LIB) -+adler32.obj  -+compress.obj -+crc32.obj
        wlib -b -c $(ZLIB_LIB) -+deflate.obj  -+gzio.obj     -+infback.obj
        wlib -b -c $(ZLIB_LIB) -+inffast.obj  -+inflate.obj  -+inftrees.obj
        wlib -b -c $(ZLIB_LIB) -+trees.obj    -+uncompr.obj  -+zutil.obj

example.exe: $(ZLIB_LIB) example.obj
	$(LINKER) -fe=example.exe example.obj $(ZLIB_LIB)

minigzip.exe: $(ZLIB_LIB) minigzip.obj
	$(LINKER) -fe=minigzip.exe minigzip.obj $(ZLIB_LIB)

clean: .SYMBOLIC
          del *.obj
          del $(ZLIB_LIB)
          @echo Cleaning done
