</sys/src/ape/config

CC=pcc -I.
CFLAGS=-c -D_POSIX_SOURCE -D_BSD_EXTENSION -DHAVE_UNISTD_H -DHAVE_STDARG_H

LD=pcc

OFILES=\
	adler32.$O\
	compress.$O\
	crc32.$O\
	deflate.$O\
	gzclose.$O\
	gzlib.$O\
	gzread.$O\
	gzwrite.$O\
	infback.$O\
	inffast.$O\
	inflate.$O\
	inftrees.$O\
	trees.$O\
	uncompr.$O\
	zutil.$O\

HFILES=`{ls *.h}

LIB=libz.a

TESTOFILES=\
	test/example.$O\
	test/minigzip.$O\

TESTPROGS=\
	test/$O.example\
	test/$O.minigzip\

CLEANFILES=$TESTOFILES $TESTPROGS

</sys/src/cmd/mklib

%.$O: %.c
	$CC $CFLAGS -o $target $stem.c

test/$O.%: test/%.$O $LIB
	$LD $LDFLAGS -o $target $prereq

test:V: $TESTPROGS
	echo hello world | ./test/$O.minigzip | ./test/$O.minigzip -d
	./test/$O.example tmpst_$pid
