/*
* Copyright (C) 2018 Pierre G. Richard.
* Written: 06/22/2018
* This code is licensed under the terms of the MIT license.
*
* C++ implementation of the DEFLATE compression algorithm over the zlib
* compression library written by Jean-Loup Gailly and Mark Adler.
*/
#include "zlib.h"
#include "Deflate.h"

/*----------------------------------------------Deflate::StreamBuf::StreamBuf-+
| Constructor                                                                            |
+----------------------------------------------------------------------------*/
Deflate::StreamBuf::StreamBuf(int windowBits, int chunkSize) :
m_buffer(0), m_rc(Z_OK),
m_windowBits(windowBits), m_chunkSize(chunkSize), m_isInited(false)
{
   m_stream.zalloc = 0;
   m_stream.zfree = 0;
   m_stream.opaque = 0;
}

/*---------------------------------------------Deflate::StreamBuf::~StreamBuf-+
| Destructor                                                                            |
+----------------------------------------------------------------------------*/
Deflate::StreamBuf::~StreamBuf() {}


/*-----------------------------------------------------Deflate::StreamBuf::rc-+
| Get the actual return code                                                  |
+----------------------------------------------------------------------------*/
int Deflate::StreamBuf::rc() const { return m_rc; }

/*------------------------------------------------Deflate::StreamBuf::cleanUp-+
| Cleanup (when the derived StreamBuf's are destroyed)                        |
+----------------------------------------------------------------------------*/
void Deflate::StreamBuf::cleanUp() {
   if (m_buffer) delete[] m_buffer, m_buffer = 0;
   if (m_isInited) xxflateEnd(&m_stream), m_isInited = false;
}

/*-----------------------------------------------Deflate::StreamBuf::overflow-+
| The underlying ostream is full                                              |
+----------------------------------------------------------------------------*/
int Deflate::StreamBuf::overflow(int c) {
   if (pcount() < m_chunkSize) {   // then, extend the buffer
      return MemStreamBuf::overflow(c);
   }else {                         // then, empty the buffer
      breathe(Z_NO_FLUSH);         // compress, or decompress its contents
      if (m_rc == Z_OK) {
         return sputc(c);          // shouldn't trigger an overflow
      }else {
         return -1;
      }
   }
}

/*------------------------------------------------Deflate::StreamBuf::breathe-+
| This is where most processing is done: breathe (deflate or inflate)         |
+----------------------------------------------------------------------------*/
void Deflate::StreamBuf::breathe(int flush) {
   m_stream.next_in = (unsigned char *)str();
   m_stream.avail_in = pcount();
   if (!m_isInited) {              // lazy initialization at first breath
      m_buffer = new unsigned char[m_chunkSize];
      m_rc = init(
         &m_stream,
         Z_DEFAULT_COMPRESSION,    // 0..9  Default to 6
         Z_DEFLATED,               // must be Z_DEFLATED
         -m_windowBits, //<-minus! // raw window size -15..-8  Default to -15
         8,                        // memory level 1..9 Default to 8
         Z_DEFAULT_STRATEGY
      );
      m_isInited = (m_rc == Z_OK);
   }
   if (m_buffer && (m_rc == Z_OK)) do {
      m_stream.next_out = m_buffer;
      m_stream.avail_out = m_chunkSize;
   }while (
      (
         m_rc = xxflate(&m_stream, flush),
         m_rc == Z_OK
      ) && (
         m_rc = (
            m_out.write((char const *)m_buffer, m_chunkSize-m_stream.avail_out)?
            Z_OK : Z_ERRNO
         ),
         m_rc == Z_OK
      ) && (
         m_stream.avail_out == 0
      )
   );
   freeze(0);                      // thaw our str'ed buffer
   pubseekpos(0);                  // Reset the stream pointers
}

/*---------------------------------------------------Deflate::StreamBuf::sync-+
| Take the ownership of the compress buffer:                                  |
| - m_out.str freezes the buffer so that the m_out destructor does nada       |
| - init with `Given' let "this" destructor destroy the buffer                |
| Why shrinkBy? Despite what Mark says, deflate produces the trailer.         |
+----------------------------------------------------------------------------*/
int Deflate::StreamBuf::sync() {   // no more data (flush)
   if (breathe(Z_SYNC_FLUSH), m_rc == Z_OK) { // Z_FINISH is unfit for Deflate
      reset();                     // free the decompress buffer
      MemStreamBuf::init(
         m_out.str(), m_out.pcount()-shrinkBy(), MemStreamBuf::Given
      );
      xxflateEnd(&m_stream);
      delete[] m_buffer;
      m_buffer = 0;
      return 0;
   }else {
      return -1;
   }
}

/*------------------------------Deflate::CompressStreamBuf::CompressStreamBuf-+
|                                                                             |
+----------------------------------------------------------------------------*/
Deflate::CompressStreamBuf::CompressStreamBuf(int windowBits, int chunkSize)
 : StreamBuf(windowBits, chunkSize) {}

/*-----------------------------Deflate::CompressStreamBuf::~CompressStreamBuf-+
|                                                                             |
+----------------------------------------------------------------------------*/
Deflate::CompressStreamBuf::~CompressStreamBuf() {
   cleanUp();
}

/*-------------------------------------------Deflate::CompressStreamBuf::init-+
|                                                                             |
+----------------------------------------------------------------------------*/
int Deflate::CompressStreamBuf::init(
   z_stream * stream, int level, int method,
   int windowBits, int memLevel, int strategy
) {
   return ::deflateInit2(stream, level, method, windowBits, memLevel, strategy);
}

/*----------------------------------------Deflate::CompressStreamBuf::xxflate-+
|                                                                             |
+----------------------------------------------------------------------------*/
int Deflate::CompressStreamBuf::xxflate(z_stream * stream, int flush) {
   return ::deflate(stream, flush);
}

/*---------------------------------------Deflate::CompressStreamBuf::shrinkBy-+
|                                                                             |
+----------------------------------------------------------------------------*/
int Deflate::CompressStreamBuf::shrinkBy() {
   return 4; // trailer size (0x0000FFFF)
}

/*-------------------------------------Deflate::CompressStreamBuf::xxflateEnd-+
|                                                                             |
+----------------------------------------------------------------------------*/
int Deflate::CompressStreamBuf::xxflateEnd(z_stream * stream) {
   return ::deflateEnd(stream);
}

/*--------------------------Deflate::DecompressStreamBuf::DecompressStreamBuf-+
|                                                                             |
+----------------------------------------------------------------------------*/
Deflate::DecompressStreamBuf::DecompressStreamBuf(int windowBits, int chunkSize)
: StreamBuf(windowBits, chunkSize) {}

/*-------------------------Deflate::DecompressStreamBuf::~DecompressStreamBuf-+
|                                                                             |
+----------------------------------------------------------------------------*/
Deflate::DecompressStreamBuf::~DecompressStreamBuf() {
   cleanUp();
}

/*-----------------------------------------Deflate::DecompressStreamBuf::init-+
|                                                                             |
+----------------------------------------------------------------------------*/
int Deflate::DecompressStreamBuf::init(
   z_stream * stream, int, int, int windowBits, int, int
) {
   return ::inflateInit2(stream, windowBits);
}

/*--------------------------------------Deflate::DecompressStreamBuf::xxflate-+
|                                                                             |
+----------------------------------------------------------------------------*/
int Deflate::DecompressStreamBuf::xxflate(z_stream * stream, int flush) {
   return ::inflate(stream, flush);
}

/*-------------------------------------Deflate::DecompressStreamBuf::shrinkBy-+
|                                                                             |
+----------------------------------------------------------------------------*/
int Deflate::DecompressStreamBuf::shrinkBy() {
   return 0;
}

/*-----------------------------------Deflate::DecompressStreamBuf::xxflateEnd-+
|                                                                             |
+----------------------------------------------------------------------------*/
int Deflate::DecompressStreamBuf::xxflateEnd(z_stream * stream) {
   return ::inflateEnd(stream);
}

/*===========================================================================*/
