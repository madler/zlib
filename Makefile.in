# Makefile for zlib
# Copyright (C) 1995-1996 Jean-loup Gailly.
# For conditions of distribution and use, see copyright notice in zlib.h 

# To compile and test, type:
#   ./configure; make test
# The call of configure is optional if you don't have special requirements

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

LDFLAGS=-L. -lz
LDSHARED=$(CC)

VER=1.0.4
LIBS=libz.a

AR=ar rc
RANLIB=ranlib
TAR=tar

prefix=/usr/local
exec_prefix = $(prefix)

OBJS = adler32.o compress.o crc32.o gzio.o uncompr.o deflate.o trees.o \
       zutil.o inflate.o infblock.o inftrees.o infcodes.o infutil.o inffast.o

TEST_OBJS = example.o minigzip.o

DISTFILES = README INDEX ChangeLog configure Make*[a-z0-9] descrip.mms \
	    zlib.def zlib.rc algorithm.doc  *.[ch]

all: example minigzip

test: all
	./example
	echo hello world | ./minigzip | ./minigzip -d 

libz.a: $(OBJS)
	$(AR) $@ $(OBJS)
	-@ ($(RANLIB) $@ || true) 2>/dev/null

libz.so.$(VER): $(OBJS)
	$(LDSHARED) -o $@ $(OBJS)
	rm -f libz.so; ln -s $@ libz.so

example: example.o $(LIBS)
	$(CC) $(CFLAGS) -o $@ example.o $(LDFLAGS)

minigzip: minigzip.o $(LIBS)
	$(CC) $(CFLAGS) -o $@ minigzip.o $(LDFLAGS)

install: $(LIBS)
	-@if [ ! $(prefix)/include  ]; then mkdir $(prefix)/include; fi
	-@if [ ! $(exec_prefix)/lib ]; then mkdir $(exec_prefix)/lib; fi
	cp zlib.h zconf.h $(prefix)/include
	chmod 644 $(prefix)/include/zlib.h $(prefix)/include/zconf.h
	cp $(LIBS) $(exec_prefix)/lib
	cd $(exec_prefix)/lib; chmod 644 $(LIBS)
	-@(cd $(exec_prefix)/lib; $(RANLIB) libz.a || true) >/dev/null 2>&1
	cd $(exec_prefix)/lib; if test -f libz.so.$(VER); then \
	  ln -s libz.so.$(VER) libz.so; \
	fi
# The ranlib in install is needed on NeXTSTEP which checks file times

uninstall:
	cd $(exec_prefix)/lib; rm -f $(LIBS); \
	if test -f libz.so; then \
	 v=`sed -n '/VERSION "/s/.*"\(.*\)".*/\1/p'<$(prefix)/include/zlib.h`;\
	 rm -f libz.so.$$v libz.so; \
	fi
	cd $(prefix)/include; rm -f zlib.h zconf.h

clean:
	rm -f *.o *~ example minigzip libz.a libz.so* foo.gz

zip:
	mv Makefile Makefile~; cp -p Makefile.in Makefile
	v=`sed -n -e 's/\.//g' -e '/VERSION "/s/.*"\(.*\)".*/\1/p' < zlib.h`;\
	zip -ul9 zlib$$v $(DISTFILES)
	mv Makefile~ Makefile

dist:
	mv Makefile Makefile~; cp -p Makefile.in Makefile
	d=zlib-`sed -n '/VERSION "/s/.*"\(.*\)".*/\1/p' < zlib.h`;\
	rm -f $$d.tar.gz; \
	if test ! -d ../$$d; then rm -f ../$$d; ln -s `pwd` ../$$d; fi; \
	files=""; \
	for f in $(DISTFILES); do files="$$files $$d/$$f"; done; \
	cd ..; \
	GZIP=-9 $(TAR) chofz $$d/$$d.tar.gz $$files; \
	if test ! -d $$d; then rm -f $$d; fi
	mv Makefile~ Makefile

tags:	
	etags *.[ch]

depend:
	makedepend -- $(CFLAGS) -- *.[ch]

# DO NOT DELETE THIS LINE -- make depend depends on it.

adler32.o: zlib.h zconf.h
compress.o: zlib.h zconf.h
crc32.o: zlib.h zconf.h
deflate.o: deflate.h zutil.h zlib.h zconf.h
example.o: zlib.h zconf.h
gzio.o: zutil.h zlib.h zconf.h
infblock.o: infblock.h inftrees.h infcodes.h infutil.h zutil.h zlib.h zconf.h
infcodes.o: zutil.h zlib.h zconf.h
infcodes.o: inftrees.h infblock.h infcodes.h infutil.h inffast.h
inffast.o: zutil.h zlib.h zconf.h inftrees.h
inffast.o: infblock.h infcodes.h infutil.h inffast.h
inflate.o: zutil.h zlib.h zconf.h infblock.h
inftrees.o: zutil.h zlib.h zconf.h inftrees.h
infutil.o: zutil.h zlib.h zconf.h infblock.h inftrees.h infcodes.h infutil.h
minigzip.o:  zlib.h zconf.h 
trees.o: deflate.h zutil.h zlib.h zconf.h 
uncompr.o: zlib.h zconf.h
zutil.o: zutil.h zlib.h zconf.h  
