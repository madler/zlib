CC=cc
CFLAGS=-O
#CFLAGS=-g -DDEBUG
LDFLAGS=-L. -lgz

RANLIB=ranlib

OBJS = adler32.o compress.o crc32.o gzio.o uncompr.o deflate.o trees.o \
       zutil.o inflate.o infblock.o inftrees.o infcodes.o infutil.o

TEST_OBJS = example.o minigzip.o inftest.o

all: example minigzip inftest

test: all
	./example
	echo hello world | ./minigzip | ./minigzip -d 

libgz.a: $(OBJS)
	ar rc $@ $(OBJS)
	$(RANLIB) $@

example: example.o libgz.a
	$(CC) $(CFLAGS) -o $@ example.o $(LDFLAGS)

minigzip: minigzip.o libgz.a
	$(CC) $(CFLAGS) -o $@ minigzip.o $(LDFLAGS)

inftest: inftest.o libgz.a
	$(CC) $(CFLAGS) -o $@ inftest.o $(LDFLAGS)

clean:
	rm -f *.o example minigzip inftest libgz.a foo.gz

zip:
	zip -ul9 zlib README ChangeLog Makefile *.[ch]

tgz:
	cd ..; tar cfz zlib/zlib.tgz zlib/README zlib/ChangeLog zlib/Makefile \
		zlib/*.[ch]

# DO NOT DELETE THIS LINE -- make depend depends on it.

adler32.o: zutil.h zlib.h zconf.h
compress.o: zlib.h zconf.h
crc32.o: zutil.h zlib.h zconf.h
deflate.o: deflate.h zutil.h zlib.h zconf.h
example.o: zlib.h zconf.h
gzio.o: zutil.h zlib.h zconf.h
infblock.o: zutil.h zlib.h zconf.h infblock.h inftrees.h infcodes.h infutil.h
infcodes.o: zutil.h zlib.h zconf.h inftrees.h infutil.h infcodes.h
inflate.o: zutil.h zlib.h zconf.h infblock.h
inftest.o: zutil.h zlib.h zconf.h
inftrees.o: zutil.h zlib.h zconf.h inftrees.h
infutil.o: zutil.h zlib.h zconf.h inftrees.h infutil.h
minigzip.o: zlib.h zconf.h
trees.o: deflate.h zutil.h zlib.h zconf.h
uncompr.o: zlib.h zconf.h
zutil.o: zutil.h zlib.h zconf.h
