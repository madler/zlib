#!/bin/sh
# Build and test with CMake
set -ex

rm -rf btmp
mkdir btmp
cd btmp
  cmake -G Ninja ..
  ninja
  ctest -V
cd ..
