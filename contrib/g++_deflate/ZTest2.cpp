/*
* Copyright (C) 2018 Pierre G. Richard.
* Written: 06/25/2018
* This code is licensed under the terms of the MIT license.
*
* Example of use of the Deflate class
*
* To compile and run:
* g++ -Wno-write-strings -Wall -std=c++0x -O0 -g -D _DEBUG \
* ZTest2.cpp Deflate.cpp MemStream.cpp -o ZTest2 -lz && ZTest2
*/
#include <iomanip>
#include "Deflate.h"

char const * test = (
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do "
    "eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim "
    "ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut "
    "aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit "
    "in voluptate velit esse cillum dolore eu fugiat nulla pariatur. "
    "Excepteur sint occaecat cupidatat non proident, sunt in culpa qui "
    "officia deserunt mollit anim id est laborum."
);

int main(int argc, char * const * const argv)
{
   Deflate::Compressor compressor(12, 0x10);
   Deflate::Decompressor decompressor;

   compressor << test << std::flush;
   decompressor << compressor.rdbuf() << std::flush;

   std::cout <<
      std::endl << "zlib version " << ZLIB_VERSION <<
      " = 0x" << std::setfill('0') << std::setw(4) << std::hex << ZLIB_VERNUM <<
      ", compile flags = 0x" << std::hex << zlibCompileFlags() <<
      std::endl <<
      std::dec <<  decompressor.pcount() << " => " << compressor.pcount() <<
      " bytes. Compression: " << std::fixed << std::setprecision(2) <<
      (100.0*(decompressor.pcount()-compressor.pcount()))/decompressor.pcount() <<
      '%' <<
      std::endl << "---" << std::endl <<
      '|' << test << '|' <<
      std::endl << "---" << std::endl <<
      '|' << decompressor.rdbuf() << '|' <<
      std::endl;
   return 0;
}
