TEMPLATE = lib
TARGET = zlib
CONFIG += warn_off

DESTDIR = $$PWD/../../bin

INCLUDEPATH += ..

win32 {
    DEF_FILE = ../win32/zlib.def
    CONFIG += shared dll
} else {
    DEFINES += Z_HAVE_UNISTD_H
    CONFIG += staticlib
}

HEADERS = \
    ../crc32.h \
    ../deflate.h \
    ../gzguts.h \
    ../inffast.h \
    ../inffixed.h \
    ../inflate.h \
    ../inftrees.h \
    ../trees.h \
    ../zconf.h \
    ../zlib.h \
    ../zutil.h 

SOURCES = \
    ../adler32.c \
    ../compress.c \
    ../crc32.c \
    ../deflate.c \
    ../gzclose.c \
    ../gzlib.c \
    ../gzread.c \
    ../gzwrite.c \
    ../infback.c \
    ../inffast.c \
    ../inflate.c \
    ../inftrees.c \
    ../trees.c \
    ../uncompr.c \
    ../zutil.c

#
# Copies the given files to the destination directory
# http://stackoverflow.com/questions/3984104/qmake-how-to-copy-a-file-to-the-output
#
defineTest(copyToDestdir) {
    files = $$1

    for(FILE, files) {
        DDIR = $$DESTDIR

        # Replace slashes in paths with backslashes for Windows
        win32:FILE ~= s,/,\\,g
        win32:DDIR ~= s,/,\\,g

        QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$FILE) $$quote($$DDIR) $$escape_expand(\\n\\t)
    }

    export(QMAKE_POST_LINK)
}

copyToDestdir($$PWD/../zlib.h)
copyToDestdir($$PWD/../zconf.h)
