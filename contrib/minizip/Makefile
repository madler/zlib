# Makefile for minizip. From this directory, ../.. is the zlib tree.
# An out-of-tree zlib build sets SRCDIR to this directory and ZLIBDIR to
# the build directory that contains libz.a and zconf.h.
SRCDIR ?= .
ZLIBDIR ?= ../..
ZLIBSRC ?= $(SRCDIR)/../..

# -DZLIB_BUILD selects zlib.h's <zconf.h> include, so the header next to
# libz.a is used when the build directory is not the source directory.
CPPFLAGS = -I$(ZLIBDIR) -I$(SRCDIR) -I$(ZLIBSRC) -DZLIB_BUILD

UNZ_OBJS = miniunz.o unzip.o ioapi.o $(ZLIBDIR)/libz.a
ZIP_OBJS = minizip.o zip.o   ioapi.o $(ZLIBDIR)/libz.a

all: miniunz minizip

miniunz.o: $(SRCDIR)/miniunz.c $(SRCDIR)/unzip.h $(SRCDIR)/iowin32.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(SRCDIR)/miniunz.c

minizip.o: $(SRCDIR)/minizip.c $(SRCDIR)/zip.h $(SRCDIR)/iowin32.h $(SRCDIR)/ints.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(SRCDIR)/minizip.c

unzip.o: $(SRCDIR)/unzip.c $(SRCDIR)/unzip.h $(SRCDIR)/crypt.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(SRCDIR)/unzip.c

zip.o: $(SRCDIR)/zip.c $(SRCDIR)/zip.h $(SRCDIR)/crypt.h $(SRCDIR)/skipset.h $(SRCDIR)/ints.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(SRCDIR)/zip.c

ioapi.o: $(SRCDIR)/ioapi.c $(SRCDIR)/ioapi.h $(SRCDIR)/ints.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(SRCDIR)/ioapi.c

iowin32.o: $(SRCDIR)/iowin32.c $(SRCDIR)/iowin32.h $(SRCDIR)/ioapi.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(SRCDIR)/iowin32.c

mztools.o: $(SRCDIR)/mztools.c $(SRCDIR)/unzip.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(SRCDIR)/mztools.c

miniunz: $(UNZ_OBJS)
	${CC} ${LDFLAGS} -o $@ $(UNZ_OBJS)

minizip: $(ZIP_OBJS)
	${CC} ${LDFLAGS} -o $@ $(ZIP_OBJS)

test: miniunz minizip
	@rm -f test.*
	@echo hello hello hello > test.txt
	./minizip test test.txt
	./miniunz -l test.zip
	@mv test.txt test.old
	./miniunz test.zip
	@cmp test.txt test.old
	@rm -f test.*

clean:
	/bin/rm -f *.o *~ minizip miniunz test.*
