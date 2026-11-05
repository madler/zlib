/*
* (C) Copyright 1997-2018 Pierre G. Richard.
* This code is licensed under the terms of the MIT license.
*
* Memory streams are streams where the ultimate consumer is the buffer itself.
* They are quite identical to strstream's, except this implementation is
* independent of the many differences, bugs, and missing functions found in
* most C++ libraries.
*
* MemStream objects are handy to create temporary or permanent buffers,
* getting the mind free about memory allocation and overflows.
* Very simple code, very efficient.  A variety of uses!
*
* MemStream is a derived class from iostream: all iostream methods
* can be used to manipulate MemStream's:
*
*     MemStream ds;              // define a MemStream
*     ds << "Hello world!";      // insert a few char;
*     ds << infile.rdbuf();      // add the contents of infile (istream)
*     ds >> outfile.rdbuf();     // save in outfile (ostream)
*     char *result = ds.str();   // keep copy in permanent storage
*     ds.reset();                // and reset (or delete the stream)
*
* Author:  Pierre G. Richard
* Written: 4/1/2002
*
* Migrated from yaxx, MemStream.h, written circa 1997.
* Original available on https://github.com/Jaxo/yaxx/tree/master/toolslib
*
*/

#ifndef COM_JAXO_TOOLS_MEMSTREA_H_INCLUDED
#define COM_JAXO_TOOLS_MEMSTREA_H_INCLUDED

/*---------+
| Includes |
+---------*/
#include <string.h>
#include <iostream>
using std::streambuf;
using std::filebuf;
using std::iostream;
using std::istream;
using std::ostream;
using std::streamoff;
using std::streampos;
using std::streamsize;
using std::ios;
using std::basic_istream;
using std::basic_ostream;
#define ios__openmode ios::openmode

/*---------------------------------------------------------------MemStreamBuf-+
| MemStreamBuf's shouldn't be used directly, but rather manipulated through   |
| an object of the MemStream class.  Except, in specific cases:               |
|                                                                             |
| freeze() freezes the buffer.  Any write operations will be lost,            |
|          until the buffer is again thawed using freeze(0).                  |
|          A frozzen stream should be thawed before it's destroyed,           |
|          or this will make a memory leak.  Unless you str() it...           |
|                                                                             |
| setincr() changes the amount of memory by which the buffer is               |
|          increased each time an overflow occurs.  Default: 100 bytes.       |
|          Can be reset at any time.                                          |
|                                                                             |
| Note: the pointer to the MemStreamBuf object of a MemStream is obtained by  |
|       calling the MemStream::rdbuf() method (non virtual!)                  |
+----------------------------------------------------------------------------*/
class MemStreamBuf : public streambuf {
public:
   enum e_Given    { Given };
   enum e_Constant { Constant };

   MemStreamBuf();                                           // extendable, RW
   MemStreamBuf(char * sp, int iLen, char * tp/*=0 LATER*/); // fixed, RW
   MemStreamBuf(char * pch, int iLen, e_Given own);          // extendable. RW
   MemStreamBuf(char const * pch, int iLen, e_Constant own); // fixed, RO

   virtual ~MemStreamBuf();

   virtual int overflow(int c);
   virtual int underflow();
   virtual int sync();
   virtual streampos seekoff(
      streamoff so,
      ios::seekdir dir,
      ios__openmode om
   );
   virtual streampos seekpos(streampos sp, ios__openmode om);

   int in_avail();                          // non virtual!

   /* PGR: 09/24/2002
   | out_waiting is deprecated.  classicaly, out_waiting was:
   | out_waiting = _M_out_cur - _M_out_beg;
   | this is too simple, and wrong after seekoff(-3, ios::cur, ios::out)
   */

   void freeze(int iFreeze=1);              // and no more write
   char * str();                            // get buffer and freeze
   void reset();                            // restart from scratch
   unsigned short setincr() const;          // returns current increment
   unsigned short setincr(unsigned short);  // set and return previous settings
   bool isFrozen() const;                   // true if frozen buffer
   bool isFixed() const;                    // true if fixed buffer
   int pcount();
   operator void *() const   { return isOk? (void *)this : 0; }
   bool operator!() const { return isOk? false : true; }

protected:
   #if __GNUC__ >= 4
   char_type * _M_buf;  // Pointer to the internally-allocated buffer
   size_t _M_buf_size;  // Actual size of internally-allocated buffer
   #endif
   void init(char * sp, int iLen, char * tp /* =0 LATER!! */);
   void init(char * pch, int iLen, e_Given);
   void init(char const * pch, int iLen, e_Constant);

private:
   enum { DEFAULT_EXTEND = 100 };
   unsigned short iExtend;
   unsigned int isOk      : 1;
   unsigned int bFixedBuf : 1;
   unsigned int bFrozen   : 1;
   unsigned int bConst    : 1;

   streamsize xsgetn(char * s, streamsize n);
   streamsize xsputn(char const * s, streamsize n);
   bool reallocate(int iBufLen);
   streampos seekIn(streamoff so, ios::seekdir dir);
   streampos seekOut(streamoff so, ios::seekdir dir);
   MemStreamBuf& operator=(MemStreamBuf const& source);  // no!
   MemStreamBuf(MemStreamBuf const& source);             // no!
};

/* -- INLINES -- */
inline int MemStreamBuf::in_avail()          { return pptr()-gptr(); }
// inline int MemStreamBuf::out_waiting()    { return pptr()-gptr(); }
inline MemStreamBuf::~MemStreamBuf()         { reset(); }
inline bool MemStreamBuf::isFrozen() const {
   if (bFrozen) return true; else return false;
}
inline bool MemStreamBuf::isFixed() const {
   if (bFixedBuf) return true; else return false;
}
inline unsigned short MemStreamBuf::setincr() const { return iExtend; }
inline unsigned short MemStreamBuf::setincr(unsigned short i) {
   unsigned short const t = iExtend;
   iExtend = i;
   return t;
}

/*------------------------------------------------------------------MemStream-+
| MemStream is a derived class from iostream: all iostream methods            |
| can be used to manipulate a MemStream, including positioning.               |
|                                                                             |
| A MemStream can be created from an existing string (asciiZ, or lengthed)    |
|  - the read pointers will be positioned at the beginning.                   |
|  - the write pointers will be positioned at the end (so write operations    |
|    will catenate)                                                           |
|  WARNING: caller should not make any assumption about the pointer to the    |
|           original string staying the same.  AND caller must guarantee      |
|           that the original string stays so for the life of the MemStream!  |
|                                                                             |
| `own' is an optional extra-argument to constructors from existing strings.  |
|  - when set to `MemStream::Given', it means that the caller owns the        |
|    string in the free-store, and transfers his ownership to MemStream.      |
|  - when set to `MemStream::Constant', it means that the caller wants the    |
|    string to be used asis, and no modification can be done to it.           |
|    The caller guarantees the validity of the string during the lifetime     |
|    of the MemStream object.                                                 |
| These options are provided for efficiency, and should be used with care.    |
|                                                                             |
|                                                                             |
| str()    get the current buffer and freezes the MemStream.                  |
|          It's a classical way to keep your results safe before to           |
|          delete or to reset() the MemStream.  Remember that you             |
|          are responsible to delete the buffer you got from str(),           |
|          using a classical delete[].                                        |
|          You shouldn't thaw the buffer -- freeze(0) -- after the stream     |
|          has been str'ed.  But it works.  However, no more rely on the      |
|          buffer address that str() returned before you thaw.                |
|          A buffer made from a MemStream::Constant string cannot be thawed.  |
|                                                                             |
| pcount() returns the number of bytes that have been stored in the           |
|          buffer. Mainly useful for binary data.                             |
|                                                                             |
| flush()  acts as if the stream had been read entirely.                      |
|                                                                             |
| reset()  creates a new empty buffer.  If you didn't str() the buffer,       |
|          the previous results are lost.  It's faster to reset() than        |
|          to delete/new.                                                     |
|                                                                             |
| putback(char c) doesn't care of what the value of 'c' is (thanks!)          |
|                                                                             |
| tellg(), seekg(), tellp() and seekp() can be safely used.                   |
|                                                                             |
+----------------------------------------------------------------------------*/
class MemStream : public iostream {
public:
   enum e_Given    { Given };
   enum e_Constant { Constant };

   MemStream();                                  // extendable, RW
   MemStream(char * sp);                         // fixed, RW, put at sp
   MemStream(char * sp, int iLen);               // fixed, RW, put at sp
   MemStream(char * pch, int iLen, e_Given own); // extendable, RW, put at +iLen
   MemStream(char const * pch, int iLen, e_Constant own); // fixed, RO
   MemStreamBuf * rdbuf() { return &buf; }
   int pcount() { return buf.pcount(); }
   char * str() { return buf.str(); }
   void reset() { buf.reset(); }
private:
   MemStreamBuf buf;
   MemStream& operator=(MemStream const& source);        // no!
   MemStream(MemStream const& source);                   // no!
};

/*-----------------------------------------------------------------NullStream-+
| This class is internal: reserved for invalid streams                        |
+----------------------------------------------------------------------------*/
class NullStreamBuf : public streambuf {
   int overflow(int)     { return -1; }
   int underflow()       { return -1; }
   int sync()            { return 0; }
   streampos seekoff(streamoff, ios::seekdir, ios__openmode) { return -1; }
   streampos seekpos(streampos sp, ios__openmode om)         { return -1; }
};

class NullStream : public iostream {
public:
   NullStream();
private:
   NullStreamBuf buf;
   NullStream& operator=(NullStream const& source);     // no!
   NullStream(NullStream const& source);                // no!
};

#endif
/*===========================================================================*/
