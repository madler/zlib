#!/bin/sh
# configure script for zlib. This script is needed only if
# you wish to build a shared library and your system supports them,
# of if you need special compiler, flags or install directory.
# Otherwise, you can just use directly "make test; make install"
#
# To impose specific compiler or flags or install directory, use for example:
#    prefix=$HOME CC=cc CFLAGS="-O4" ./configure
# or for csh/tcsh users:
#    (setenv prefix $HOME; setenv CC cc; setenv CFLAGS "-O4"; ./configure)
# LDSHARED is the command to be used to create a shared library

LIBS=libz.a
VER=`sed -n -e '/VERSION "/s/.*"\(.*\)".*/\1/p' < zlib.h`
AR=${AR-"ar rc"}
RANLIB=${RANLIB-"ranlib"}
prefix=${prefix-/usr/local}
exec_prefix=${exec_prefix-$prefix}

test -z "$CC" && echo Checking for gcc...
test=ztest$$
cat > $test.c <<EOF
int hello() { printf("hello\n"); }
EOF
if test -z "$CC" && (gcc -c -O3 $test.c) 2>/dev/null; then
  CC=gcc
  SFLAGS=${CFLAGS-"-fPIC -O3"}
  CFLAGS=${CFLAGS-"-O3"}
  LDSHARED=${LDSHARED-"gcc -shared"}
else
  # find system name and corresponding cc options
  CC=${CC-cc}
  case `(uname -sr || echo unknown) 2>/dev/null` in
  SunOS\ 5*) SFLAGS=${CFLAGS-"-fast -xcg89 -KPIC -R."}
             CFLAGS=${CFLAGS-"-fast -xcg89"}
	     LDSHARED=${LDSHARED-"cc -G"};;
  SunOS\ 4*) SFLAGS=${CFLAGS-"-O2 -PIC"}
	     CFLAGS=${CFLAGS-"-O2"}
	     LDSHARED=${LDSHARED-"ld"};;
  IRIX*)     SFLAGS=${CFLAGS-"-ansi -O2 -rpath ."}
	     CFLAGS=${CFLAGS-"-ansi -O2"}
	     LDSHARED=${LDSHARED-"cc -shared"};;
  QNX*)      SFLAGS=${CFLAGS-"-4 -O"}
             CFLAGS=${CFLAGS-"-4 -O"}
	     LDSHARED=${LDSHARED-"cc"}
             RANLIB=${RANLIB-"true"}
             AR="cc -A";;
  SCO_SV\ 3.2*) SFLAGS=${CFLAGS-"-O3 -dy -KPIC "}
	     CFLAGS=${CFLAGS-"-O3"}
	     LDSHARED=${LDSHARED-"cc -dy -KPIC -G"};;
  HP-UX*)    SFLAGS=${CFLAGS-"-O +z"}
	     CFLAGS=${CFLAGS-"-O"}
	     LDSHARED=${LDSHARED-"ld -b"}
	     SHAREDLIBS='libz.sl';;
  # send working options for other systems to gzip@prep.ai.mit.edu
  *)         SFLAGS=${CFLAGS-"-O"}
	     CFLAGS=${CFLAGS-"-O"}
	     LDSHARED=${LDSHARED-"cc -shared"};;
  esac
fi

echo Checking for shared library support...
# we must test in two steps (cc then ld), required at least on SunOS 4.x
if test "`($CC -c $SFLAGS $test.c) 2>&1`" = "" &&
   test "`($LDSHARED -o $test.so $test.o) 2>&1`" = ""; then
  CFLAGS="$SFLAGS"
  LIBS='libz.so.$(VER)'
  echo Building shared library libz.so.$VER with $CC.
else
  LDSHARED="$CC"
  echo Building static library $LIBS version $VER with $CC.
fi
rm -f $test.[co] $test.so

# udpate Makefile
sed < Makefile.in "
/^CC *=/s/=.*/=$CC/
/^CFLAGS *=/s/=.*/=$CFLAGS/
/^LDSHARED *=/s/=.*/=$LDSHARED/
/^LIBS *=/s,=.*,=$LIBS,
/^AR *=/s/=.*/=$AR/
/^RANLIB *=/s,=.*,=$RANLIB,
/^VER *=/s/=.*/=$VER/
/^prefix *=/s,=.*,=$prefix,
/^exec_prefix *=/s,=.*,=$exec_prefix,
" > Makefile
