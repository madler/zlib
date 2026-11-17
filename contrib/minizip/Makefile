CPPFLAGS = -I../..

UNZ_OBJS = miniunz.o unzip.o ioapi.o ../../libz.a
ZIP_OBJS = minizip.o zip.o   ioapi.o ../../libz.a
TEST64_OBJS = testzip64.o zip.o unzip.o ioapi.o ../../libz.a

all: miniunz minizip

miniunz.o: miniunz.c unzip.h iowin32.h
minizip.o: minizip.c zip.h iowin32.h ints.h
testzip64.o: testzip64.c zip.h unzip.h
unzip.o: unzip.c unzip.h crypt.h
zip.o: zip.c zip.h crypt.h skipset.h ints.h
ioapi.o: ioapi.c ioapi.h ints.h
iowin32.o: iowin32.c iowin32.h ioapi.h
mztools.o: mztools.c unzip.h

miniunz: $(UNZ_OBJS)
	${CC} ${LDFLAGS} -o $@ $(UNZ_OBJS)

minizip: $(ZIP_OBJS)
	${CC} ${LDFLAGS} -o $@ $(ZIP_OBJS)

testzip64: $(TEST64_OBJS)
	${CC} ${LDFLAGS} -o $@ $(TEST64_OBJS)

test: miniunz minizip testzip64
	@rm -f test.* test64.zip
	@echo hello hello hello > test.txt
	./minizip test test.txt
	./miniunz -l test.zip
	@mv test.txt test.old
	./miniunz test.zip
	@cmp test.txt test.old
	@rm -f test.*
	./testzip64

clean:
	/bin/rm -f *.o *~ minizip miniunz testzip64 test.* test64.zip
