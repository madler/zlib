/*
* Copyright (C) 2018 Pierre G. Richard.
* Written: 06/22/2018
* This code is licensed under the terms of the MIT license.
*
* Deflate is a C++ implementation of the DEFLATE compression algorithm
* as specified in RFC 1951. It has both a Compressor and a Decompressor,
* which are standard iostream's.
*
* Deflate is simply a wrapper over the zlib interface compression library
* written by Jean-Loup Gailly and Mark Adler.
*
* Primarily intended for compressing or decompressing HTML5 websockets data,
* as described in RFC 7692, "Deflate" does "raw" deflate.
*
* Deflate depends on "MemStream", an early implementation of what is today
* known as "std::strstream" or "std::stringstream". The MemStream source
* is part of this distribution.
*
* This C++ code has been tested on Linux and g++. It should be easy to port it
* to other environments.
*
* Here is simple example of use:
*
* #include "Deflate.h"
*
* int main() {
*    Deflate::Compressor compressor;
*    Deflate::Decompressor decompressor;
*
*    compressor << "Hello, Hello?" << std::flush;
*    decompressor << compressor.rdbuf() << std::flush;
*
*    std::cout << '|' << decompressor.rdbuf() << '|' << std::endl;
*    return 0;
* }
*
* To compile and run:
* g++ -Wno-write-strings -Wall -std=c++0x -O0 -g -D _DEBUG \
* ZTest.cpp Deflate.cpp MemStream.cpp -o ZTest -lz && ZTest
*/

#ifndef _DEFLATE_H_INCLUDED
#define _DEFLATE_H_INCLUDED
#include <iostream>
#include "zlib.h"  // for z_stream
#include "MemStream.h"

class Deflate {
public:
   class StreamBuf : public MemStreamBuf {
   public:
      StreamBuf(int windowBits, int chunkSize);
      virtual ~StreamBuf();
      int rc() const;
   protected:
      void cleanUp();
   private:
      unsigned char * m_buffer;
      z_stream m_stream;
      MemStream m_out;
      int m_rc;
      int const m_windowBits;
      int const m_chunkSize;
      bool m_isInited;

      void breathe(int);
      virtual int overflow(int c);
      virtual int sync();
      virtual int init(z_stream *, int, int, int, int, int) = 0;
      virtual int xxflate(z_stream *, int) = 0;
      virtual int xxflateEnd(z_stream *) = 0;
      virtual int shrinkBy() = 0;
   };

   class CompressStreamBuf : public StreamBuf {
   public:
      CompressStreamBuf(int, int);
      virtual ~CompressStreamBuf();
   private:
      virtual int init(z_stream *, int, int, int, int, int);
      virtual int xxflate(z_stream *, int);
      virtual int shrinkBy();
      virtual int xxflateEnd(z_stream *);
   };

   class DecompressStreamBuf : public StreamBuf {
   public:
      DecompressStreamBuf(int, int);
      virtual ~DecompressStreamBuf();
   private:
      virtual int init(z_stream *, int, int, int, int, int);
      virtual int xxflate(z_stream *, int);
      virtual int shrinkBy();
      virtual int xxflateEnd(z_stream *);
   };

   class Compressor : public std::iostream {
   public:
      Compressor(int windowBits = 15, int chunkSize = 0x4000)
      : iostream(&m_buf), m_buf(windowBits, chunkSize) {}
      int pcount() { return m_buf.pcount(); }
      int rc() const { return m_buf.rc(); }
   private:
      CompressStreamBuf m_buf;
   };

   class Decompressor : public std::iostream {
   public:
      Decompressor(int windowBits = 15, int chunkSize = 0x4000)
      : iostream(&m_buf), m_buf(windowBits, chunkSize) {}
      int pcount() { return m_buf.pcount(); }
      int rc() const { return m_buf.rc(); }
   private:
      DecompressStreamBuf m_buf;
   };
};
#endif
/*===========================================================================*/
