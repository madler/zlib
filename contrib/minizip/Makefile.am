lib_LTLIBRARIES = libminizip.la

libminizip_la_SOURCES = \
	ioapi.c \
	mztools.c \
	unzip.c \
	zip.c

libminizip_la_CFLAGS = -I../.. -L../..
libminizip_la_LDFLAGS = -version-info 1:0:0 -lz

minizip_includedir = $(includedir)/minizip
minizip_include_HEADERS = \
	crypt.h \
	ioapi.h \
	mztools.h \
	unzip.h \
	zip.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = minizip.pc

EXTRA_PROGRAMS = miniunzip minizip

miniunzip_SOURCES = miniunz.c
miniunzip_LDADD = libminizip.la

minizip_SOURCES = minizip.c
