/*
* Copyright (C) 2018 Pierre G. Richard.
* Written: 06/25/2018
* This code is licensed under the terms of the MIT license.
*
* Example of use of the Deflate class
*
* To compile and run:
* g++ -Wno-write-strings -Wall -std=c++0x -O0 -g -D _DEBUG \
* ZTest.cpp Deflate.cpp MemStream.cpp -o ZTest -lz && ZTest
*/
#include "Deflate.h"

int main() {
   Deflate::Compressor compressor;
   Deflate::Decompressor decompressor;

   compressor << "Hello, Hello?" << std::flush;
   decompressor << compressor.rdbuf() << std::flush;

   std::cout << '|' << decompressor.rdbuf() << '|' << std::endl;
   return 0;
}
/*===========================================================================*/
