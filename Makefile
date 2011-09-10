# Makefile for zlib
# Copyright (C) 1995-2006 Jean-loup Gailly.
# For conditions of distribution and use, see copyright notice in zlib.h

# To compile and test, type:
#    ./configure; make test
# The call of configure is optional if you don't have special requirements
# If you wish to build zlib as a shared library, use: ./configure -s

# To use the asm code, type:
#    cp contrib/asm?86/match.S ./match.S
#    make LOC=-DASMV OBJA=match.o

# To install /usr/local/lib/libz.* and /usr/local/include/zlib.h, type:
#    make install
# To install in $HOME instead of /usr/local, use:
#    make install prefix=$HOME

CC=cc

CFLAGS=-O
#CFLAGS=-O -DMAX_WBITS=14 -DMAX_MEM_LEVEL=7
#CFLAGS=-g -DDEBUG
#CFLAGS=-O3 -Wall -Wwrite-strings -Wpointer-arith -Wconversion \
#           -Wstrict-prototypes -Wmissing-prototypes

SFLAGS=-O

LDFLAGS=libz.a
LDSHARED=$(CC)
CPP=$(CC) -E

LIBS=libz.a
SHAREDLIB=libz.so
SHAREDLIBV=libz.so.1.2.3.3
SHAREDLIBM=libz.so.1

AR=ar
RANLIB=ranlib
TAR=tar
SHELL=/bin/sh
EXE=

prefix = /usr/local
exec_prefix = ${prefix}
libdir = ${exec_prefix}/lib
includedir = ${prefix}/include
mandir = ${prefix}/share/man
man3dir = ${mandir}/man3
pkgconfigdir = ${libdir}/pkgconfig

OBJC = adler32.o compress.o crc32.o gzio.o uncompr.o deflate.o trees.o \
       zutil.o inflate.o infback.o inftrees.o inffast.o

OBJA =
# to use the asm code: make OBJA=match.o

OBJS = $(OBJC) $(OBJA)

PIC_OBJS = $(OBJS:%.o=%.lo)

TEST_OBJS = example.o minigzip.o

allstatic: example$(EXE) minigzip$(EXE)

allshared: examplesh$(EXE) minigzipsh$(EXE)

all: allstatic allshared

teststatic: allstatic
	@echo hello world | ./minigzip | ./minigzip -d || \
	  echo '		*** minigzip test FAILED ***' ; \
	if ./example; then \
	  echo '		*** zlib test OK ***'; \
	else \
	  echo '		*** zlib test FAILED ***'; \
	fi

testshared: allshared
	@LD_LIBRARY_PATH=`pwd`:$(LD_LIBRARY_PATH) ; export LD_LIBRARY_PATH; \
	DYLD_LIBRARY_PATH=`pwd`:$(DYLD_LIBRARY_PATH) ; export DYLD_LIBRARY_PATH; \
	SHLIB_PATH=`pwd`:$(SHLIB_PATH) ; export SHLIB_PATH; \
	echo hello world | ./minigzipsh | ./minigzipsh -d || \
	  echo '		*** minigzip shared test FAILED ***' ; \
	if ./examplesh; then \
	  echo '		*** zlib shared test OK ***'; \
	else \
	  echo '		*** zlib shared test FAILED ***'; \
	fi

test: teststatic testshared

check: test

libz.a: $(OBJS)
	$(AR) $@ $(OBJS)
	-@ ($(RANLIB) $@ || true) >/dev/null 2>&1

match.o: match.S
	$(CPP) match.S > _match.s
	$(CC) -c _match.s
	mv _match.o match.o
	rm -f _match.s

match.lo: match.S
	$(CPP) match.S > _match.s
	$(CC) -c -fPIC _match.s
	mv _match.o match.lo
	rm -f _match.s

%.lo: %.c
	$(CC) $(SFLAGS) -DPIC -c $< -o $@

$(SHAREDLIBV): $(PIC_OBJS)
	$(LDSHARED) -o $@ $(PIC_OBJS) -lc
	rm -f $(SHAREDLIB) $(SHAREDLIBM)
	ln -s $@ $(SHAREDLIB)
	ln -s $@ $(SHAREDLIBM)

example$(EXE): example.o $(LIBS)
	$(CC) $(CFLAGS) -o $@ example.o $(LDFLAGS)

minigzip$(EXE): minigzip.o $(LIBS)
	$(CC) $(CFLAGS) -o $@ minigzip.o $(LDFLAGS)

examplesh$(EXE): example.o $(LIBS)
	$(CC) $(CFLAGS) -o $@ example.o -L. $(SHAREDLIB)

minigzipsh$(EXE): minigzip.o $(LIBS)
	$(CC) $(CFLAGS) -o $@ minigzip.o -L. $(SHAREDLIB)

install-libs: $(LIBS)
	-@if [ ! -d $(DESTDIR)$(exec_prefix)  ]; then mkdir -p $(DESTDIR)$(exec_prefix); fi
	-@if [ ! -d $(DESTDIR)$(libdir)       ]; then mkdir -p $(DESTDIR)$(libdir); fi
	-@if [ ! -d $(DESTDIR)$(man3dir)      ]; then mkdir -p $(DESTDIR)$(man3dir); fi
	-@if [ ! -d $(DESTDIR)$(pkgconfigdir) ]; then mkdir -p $(DESTDIR)$(pkgconfigdir); fi
	cp $(LIBS) $(DESTDIR)$(libdir)
	cd $(DESTDIR)$(libdir); chmod 755 $(LIBS)
	-@(cd $(DESTDIR)$(libdir); $(RANLIB) libz.a || true) >/dev/null 2>&1
	cd $(DESTDIR)$(libdir); if test -f $(SHAREDLIBV); then \
	  rm -f $(SHAREDLIB) $(SHAREDLIBM); \
	  ln -s $(SHAREDLIBV) $(SHAREDLIB); \
	  ln -s $(SHAREDLIBV) $(SHAREDLIBM); \
	  (ldconfig || true)  >/dev/null 2>&1; \
	fi
	cp zlib.3 $(DESTDIR)$(man3dir)
	chmod 644 $(DESTDIR)$(man3dir)/zlib.3
	cp zlib.pc $(DESTDIR)$(pkgconfigdir)
	chmod 644 $(DESTDIR)$(pkgconfigdir)/zlib.pc
# The ranlib in install is needed on NeXTSTEP which checks file times
# ldconfig is for Linux

install: install-libs
	-@if [ ! -d $(DESTDIR)$(includedir)   ]; then mkdir -p $(DESTDIR)$(includedir); fi
	cp zlib.h zconf.h zlibdefs.h $(DESTDIR)$(includedir)
	chmod 644 $(DESTDIR)$(includedir)/zlib.h $(DESTDIR)$(includedir)/zconf.h $(DESTDIR)$(includedir)/zlibdefs.h

uninstall:
	cd $(DESTDIR)$(includedir); rm -f zlib.h zconf.h zlibdefs.h
	cd $(DESTDIR)$(libdir); rm -f libz.a; \
	if test -f $(SHAREDLIBV); then \
	  rm -f $(SHAREDLIBV) $(SHAREDLIB) $(SHAREDLIBM); \
	fi
	cd $(DESTDIR)$(man3dir); rm -f zlib.3
	cd $(DESTDIR)$(pkgconfigdir); rm -f zlib.pc

mostlyclean: clean
clean:
	rm -f *.o *.lo *~ \
	   example$(EXE) minigzip$(EXE) examplesh$(EXE) minigzipsh$(EXE) \
	   libz.* foo.gz so_locations \
	   _match.s maketree contrib/infback9/*.o

maintainer-clean: distclean
distclean: clean
	cp -p Makefile.in Makefile
	rm zlibdefs.h
	touch -r configure zlibdefs.h
	rm -f zlib.pc .DS_Store

tags:
	etags *.[ch]

depend:
	makedepend -- $(CFLAGS) -- *.[ch]

# DO NOT DELETE THIS LINE -- make depend depends on it.

adler32.o: zlib.h zconf.h zlibdefs.h
compress.o: zlib.h zconf.h zlibdefs.h
crc32.o: crc32.h zlib.h zconf.h zlibdefs.h
deflate.o: deflate.h zutil.h zlib.h zconf.h zlibdefs.h
example.o: zlib.h zconf.h zlibdefs.h
gzio.o: zutil.h zlib.h zconf.h zlibdefs.h
inffast.o: zutil.h zlib.h zconf.h zlibdefs.h inftrees.h inflate.h inffast.h
inflate.o: zutil.h zlib.h zconf.h zlibdefs.h inftrees.h inflate.h inffast.h inffixed.h
infback.o: zutil.h zlib.h zconf.h zlibdefs.h inftrees.h inflate.h inffast.h inffixed.h
inftrees.o: zutil.h zlib.h zconf.h zlibdefs.h inftrees.h
minigzip.o: zlib.h zconf.h zlibdefs.h
trees.o: deflate.h zutil.h zlib.h zconf.h zlibdefs.h trees.h
uncompr.o: zlib.h zconf.h zlibdefs.h
zutil.o: zutil.h zlib.h zconf.h zlibdefs.h

adler32.lo: zlib.h zconf.h zlibdefs.h
compress.lo: zlib.h zconf.h zlibdefs.h
crc32.lo: crc32.h zlib.h zconf.h zlibdefs.h
deflate.lo: deflate.h zutil.h zlib.h zconf.h zlibdefs.h
gzio.lo: zutil.h zlib.h zconf.h zlibdefs.h
inffast.lo: zutil.h zlib.h zconf.h zlibdefs.h inftrees.h inflate.h inffast.h
inflate.lo: zutil.h zlib.h zconf.h zlibdefs.h inftrees.h inflate.h inffast.h inffixed.h
infback.lo: zutil.h zlib.h zconf.h zlibdefs.h inftrees.h inflate.h inffast.h inffixed.h
inftrees.lo: zutil.h zlib.h zconf.h zlibdefs.h inftrees.h
trees.lo: deflate.h zutil.h zlib.h zconf.h zlibdefs.h trees.h
uncompr.lo: zlib.h zconf.h zlibdefs.h
zutil.lo: zutil.h zlib.h zconf.h zlibdefs.h
