#!/bin/sh
# Build and test with Make
set -ex

rm -rf btmp
mkdir btmp
cd btmp1
  ../configure
  make
  make test
cd ..
