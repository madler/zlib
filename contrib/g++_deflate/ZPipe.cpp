/*
* Copyright (C) 2018 Pierre G. Richard.
* Written: 06/25/2018
* This code is licensed under the terms of the MIT license.
*
* Example of use of the Deflate class
*
* To compile:
  g++ -Wno-write-strings -Wall -std=c++0x -O0 -g -D _DEBUG \
  ZPipe.cpp Deflate.cpp MemStream.cpp -o ZPipe -lz
*
* A run example (cut and paste to console)
*
  echo \
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do " \
    "eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim " \
    "ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut " \
    "aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit " \
    "in voluptate velit esse cillum dolore eu fugiat nulla pariatur. " \
    "Excepteur sint occaecat cupidatat non proident, sunt in culpa qui " \
    "officia deserunt mollit anim id est laborum." \
    >/tmp/toto && \
  ZPipe < /tmp/toto | ZPipe -d | diff -s - /tmp/toto  && \
  rm /tmp/toto
*
*/
#include "Deflate.h"

// compress or decompress from std::cin to std::cout
int main(int argc, char * const * const argv) {
   Deflate::Compressor compressor;
   Deflate::Decompressor decompressor;
   int ret;

   // do compression if no arguments
   if (argc == 1) {
      compressor << std::cin.rdbuf() << std::flush;
      if (ret = compressor.rc(), ret != 0) {
         std::cerr << "Compressor RC:" << ret << " Exiting." << std::endl;
      }else {
         std::cout << compressor.rdbuf();
      }

   // do decompression if -d specified
   }else if ((argc == 2) && strcmp(argv[1], "-d") == 0) {
      decompressor << std::cin.rdbuf() << std::flush;
      if (ret = decompressor.rc(), ret != 0) {
         std::cerr << "Decompressor RC:" << ret << " Exiting." << std::endl;
      }else {
         std::cout << decompressor.rdbuf();
      }

   // otherwise, report usage
   }else {
      std::cerr << "Usage: " << argv[0] << " [-d] < source > dest" << std::endl;
      return 1;
   }
   return 0;
}
/*===========================================================================*/
