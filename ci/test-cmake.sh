#!/bin/sh
# Build and test with CMake; output junit xml files.
set -ex

rm -rf btmp
mkdir btmp
cd btmp
  cmake -G Ninja -DJUNIT=ON ..
  ninja
  ctest -V
cd ..
