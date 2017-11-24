#!/bin/bash
#
# Environment Variables
#
# CMAKE_BUILD_TYPE sets the cmake variable by that name (default if not set: use ./configure)
# CMAKE_C_COMPILER sets the cmake variable by that name (default if not set: $CC)
# CTEST_DASHBOARD sets the dashboard suite to run (default if not set: ExperimentalTest)
# 

set -ex

SRCDIR=`pwd`
if [[ "${CTEST_BASHBOARD}" == "ExperimentalMemCheck" ]]; then
    RUNNER=valgrind
fi
mkdir /tmp/build
cd /tmp/build

if [[ ! -z "$CMAKE_BUILD_TYPE" ]]; then
  cmake $SRCDIR -DCMAKE_INSTALL_PREFIX=/tmp/install \
                -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE \
                -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER:-$CC}
  VERBOSE=1 cmake --build . --target install
  ctest -D ${CTEST_DASHBOARD:-ExperimentalTest}
  ls -lsR /tmp/install
else
  $SRCDIR/configure $CONFIGURE --prefix=/tmp/install
  make -j2 install
  make test
  ls -lsR /tmp/install
fi

cp libz.a libz.a.orig
$RUNNER ./minigzip64 -9 libz.a
$RUNNER ./minigzip64 -d libz.a.gz
cmp libz.a libz.a.orig
