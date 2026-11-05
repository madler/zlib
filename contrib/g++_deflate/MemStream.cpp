/*
* (C) Copyright 1997-2018 Pierre G. Richard.
* This code is licensed under the terms of the MIT license.
*
* Memory Streams
*
* Author:  Pierre G. Richard
* Written: 4/1/2002  (migrated from memstream.cpp, written circa 1997).
* Original available on https://github.com/Jaxo/yaxx/tree/master/toolslib
*/

/*--------------+
| Include Files |
+--------------*/
#include "MemStream.h"

#define base()        _M_buf
#define setb(b,e)     _M_buf = (b), _M_buf_size = ((e)-(b))
#define ebuf()        (_M_buf + _M_buf_size)
#define blen()        _M_buf_size
// #define in_avail()    (_M_in_end - _M_in_cur)

#define myZapeof(c) ((unsigned char)(c))

/*------------------------------------------------------------------MemStream-+
|                                                                             |
+----------------------------------------------------------------------------*/
MemStream::MemStream() :                                // extendable, RW
   iostream(&buf)
{
}
MemStream::MemStream(char * sp) :                       // fixed, RW
   iostream(&buf), buf(sp, strlen(sp), sp)              // put at sp
{
   if (!buf) clear(rdstate() | ios::failbit);
}
MemStream::MemStream(char * sp, int iLen) :             // fixed, RW
   iostream(&buf), buf(sp, iLen, sp)                    // put at sp
{
   if (!buf) clear(rdstate() | ios::failbit);
}
MemStream::MemStream(char * pch, int iLen, e_Given) :   // extendable, RW
   iostream(&buf), buf(pch, iLen, MemStreamBuf::Given)  // put at pch+iLen
{
}
MemStream::MemStream(char const * pch, int iLen, e_Constant) : // fixed, RO
   iostream(&buf), buf(pch, iLen, MemStreamBuf::Constant)
{
}

/*---------------------------------------------------------MemStreamBuf::init-+
| init(char *sp, int iLen, char * tp)                                         |
|    makes a fixed buffer.  Get pointer at sp, put pointer at tp,             |
| init(char const * pch, int iLen, e_Given)                                   |
|    makes an extendable buffer, initially filled with (pch, iLen)            |
|    Get pointer at pch, put pointer at pch + iLen (so it catenates)          |
| init(char const * pch, int iLen, e_Constant)                                |  |
|    makes a fixed buffer and prohibits any write.  Get pointer at pch.       |
+----------------------------------------------------------------------------*/
void MemStreamBuf::init(char * sp, int iLen, char * tp)
{
   iExtend = DEFAULT_EXTEND;
   bFrozen = 0;
   bConst = 0;
   bFixedBuf = 1;
   isOk = 1;
   if (!tp) tp = sp;
   setb(sp, sp+iLen);
   setp(sp, sp+iLen);
   setg(sp, sp, sp+iLen);
   pbump(tp-sp);             // setp(tp, sp+iLen);
}

void MemStreamBuf::init(char * pch, int iLen, e_Given)
{
   iExtend = DEFAULT_EXTEND;
   bFrozen = 0;
   bFixedBuf = 0;
   bConst = 0;
   isOk = 1;
   setb(pch, pch+iLen);
   setp(pch, pch+iLen);
   setg(pch, pch, pch+iLen);
   pbump(iLen);             // setp(pch+iLen, pch+iLen);
}

void MemStreamBuf::init(char const * pch, int iLen, e_Constant)
{
   iExtend = DEFAULT_EXTEND;
   bFrozen = 1;
   bFixedBuf = 1;
   bConst = 1;
   isOk = 1;
   setb((char *)pch, (char *)pch+iLen);
   setp((char *)pch, (char *)pch+iLen);
   setg((char *)pch, (char *)pch, (char *)pch+iLen);
   pbump(iLen);             // setp((char *)pch+iLen, (char *)pch+iLen);
}

/*-------------------------------------------------MemStreamBuf::MemStreamBuf-+
| MemStreamBuf()                                                              |
|    makes an extendable buffer (initially empty)                             |
|                                                                             |
| MemStreamBuf(char * sp, int iLen, char * tp)                                |
|    makes a fixed, writable, caller-owned buffer at sp, iLen;                |
|    get pointer at sp,                                                       |
|    put pointer at tp.                                                       |
|                                                                             |
| MemStreamBuf(char * pch, int iLen, e_Given)                                 |
|    Assumption: the buffer: pch, iLen has been char[] new'ed and is now      |
|                owned by the MemStreamBuf (and may be delete[]'ed)           |
|    makes a extendable buffer initialized with pch, iLen;                    |
|    get pointer at pch,                                                      |
|    put pointer at pch + iLen (so it catenates).                             |
|                                                                             |
| MemStreamBuf(char const * pch, int iLen, e_Constant)                        |
|    makes a fixed caller-owned buffer and prohibits any write to it;         |
|    get pointer at pch.                                                      |
|                                                                             |
+----------------------------------------------------------------------------*/
MemStreamBuf::MemStreamBuf() {
   iExtend = DEFAULT_EXTEND;
   bFrozen = 0;
   bFixedBuf = 0;
   bConst = 0;
   setb(0, 0);
   setp(0, 0);
   setg(0, 0, 0);
   isOk = 1;
}
MemStreamBuf::MemStreamBuf(char * sp, int iLen, char * tp) {
   init(sp, iLen, tp);
}
MemStreamBuf::MemStreamBuf(char * pch, int iLen, e_Given own) {
   init(pch, iLen, own);
}
MemStreamBuf::MemStreamBuf(char const * pch, int iLen, e_Constant own) {
   init(pch, iLen, own);
}

/*--------------------------------------------------------MemStreamBuf::reset-+
| bConst bFixedBuf bFrozen  Action:                                           |
|   0       0        0      free the buffer; reset all to 0; thaw.            |
|   0       0        1      do not free current buffer; reset all to 0; thaw. |
|   0       1        0      set the pointer like it was at init.              |
|   0       1        1      set the pointer like it was at init; thaw.        |
|   1       1        1      set the pointer like it was at init.              |
| and keep same extend                                                        |
+----------------------------------------------------------------------------*/
void MemStreamBuf::reset()
{
   if (!bFixedBuf) {
      if (!bFrozen) delete [] base(); else bFrozen=0;
      setb(0, 0);
      setp(0, 0);
      setg(0, 0, 0);
   }else {
      if (!bConst) {
         setp(base(), ebuf());
         bFrozen = 0;
      }
      setg(base(), base(), ebuf());
   }
}

/*---------------------------------------------------MemStreamBuf::reallocate-+
|                                                                             |
+----------------------------------------------------------------------------*/
bool MemStreamBuf::reallocate(int iBufLen)
{
   if (bFixedBuf) return false;
   char *pBuf = new char[iBufLen];
   if (!pBuf) {
      isOk = 0;
      return false;
   }
   isOk = 1;
   memcpy(pBuf, base(), ((int)blen() < iBufLen)? blen() : iBufLen);
   int iPutOffset = pptr() - base();
   int iGetOffset = gptr() - base();
   delete [] base();
   setb(pBuf, pBuf+iBufLen);
   setp(pBuf, pBuf+iBufLen);
   setg(pBuf, pBuf+iGetOffset, pBuf+iPutOffset);
   pbump(iPutOffset);   // setp(pBuf+iPutOffset, pBuf+iBufLen);
   return true;
}

/*---------------------------------------------------------MemStreamBuf::sync-+
| Being its own ultimate producer/consumer, a MemStreamBuf is always synch'ed.|
+----------------------------------------------------------------------------*/
int MemStreamBuf::sync() {
   return 0;
}

/*----------------------------------------------------MemStreamBuf::underflow-+
|                                                                             |
+----------------------------------------------------------------------------*/
int MemStreamBuf::underflow()
{
   if (gptr() < pptr()) {
      setg(base(), gptr(), pptr());
      return myZapeof(*gptr());

   }
   return -1;
}

/*-------------------------------------------------------MemStreamBuf::freeze-+
|                                                                             |
+----------------------------------------------------------------------------*/
void MemStreamBuf::freeze(int iFreeze)
{
   if (!bConst) {
      char * putPtr = pptr();
      if (iFreeze) {              // setp(pptr(), pptr());
         setp(base(), putPtr);    // so overflow is called each time
         bFrozen = 1;
      }else {                     // setp(pptr(), ebuf());
         setp(base(), ebuf());    // thaw
         bFrozen = 0;
      }
      pbump(putPtr - base());     // restore the put pointer
   }
}

/*-----------------------------------------------------MemStreamBuf::overflow-+
|                                                                             |
+----------------------------------------------------------------------------*/
int MemStreamBuf::overflow(int c)
{
   if (bFrozen || !reallocate(blen() + iExtend) || (c == -1)) {
      return -1;
   }else {
      sputc(c);
      return myZapeof(c);
   }
}

/*-------------------------------------------------------MemStreamBuf::xsgetn-+
|                                                                             |
+----------------------------------------------------------------------------*/
streamsize MemStreamBuf::xsgetn(char * pchBuf, streamsize n)
{
#if 1                                   // <== Optimized
   char * get = gptr();
   int count = egptr() - get;
   if (count > n) {
      switch (n) {
      case 12: *pchBuf++ = *get++;
      case 11: *pchBuf++ = *get++;
      case 10: *pchBuf++ = *get++;
      case  9: *pchBuf++ = *get++;
      case  8: *pchBuf++ = *get++;
      case  7: *pchBuf++ = *get++;
      case  6: *pchBuf++ = *get++;
      case  5: *pchBuf++ = *get++;
      case  4: *pchBuf++ = *get++;
      case  3: *pchBuf++ = *get++;
      case  2: *pchBuf++ = *get++;
      case  1:
         *pchBuf++ = *get++;
         gbump(n);
         return n;
      default:
         count = n;
         break;
      }
   }
   memcpy(pchBuf, get, count);
   setg(base(), get + count, pptr());
#else
   int count = pptr() - gptr();
   if (count > n) count = n;
   memcpy(pchBuf, gptr(), count);
   setg(base(), gptr() + count, pptr());
#endif
   return count;
}

/*-------------------------------------------------------MemStreamBuf::xsputn-+
|                                                                             |
+----------------------------------------------------------------------------*/
streamsize MemStreamBuf::xsputn(char const * s, streamsize n) {
   if (!bFrozen) {
      int extend = n - (ebuf()-pptr());
      if ((extend <= 0) || reallocate(blen() + extend)) {
         memcpy(pptr(), s, n);
         pbump(n);
         return n;
      }
   }
   return 0;
}

/*----------------------------------------------------------MemStreamBuf::str-+
|                                                                             |
+----------------------------------------------------------------------------*/
char * MemStreamBuf::str()
{
   bFrozen = 1;       // bConst => bFrozen and bFixedBuf (no reallocate)
   reallocate(pptr()-base()); // try to shrink.  Don't check.
   return base();
}

/*------------------------------------------------------MemStreamBuf::seekoff-+
|                                                                             |
+----------------------------------------------------------------------------*/
streampos MemStreamBuf::seekoff(
   streamoff so,
   ios::seekdir dir,
   ios__openmode om
) {
   streampos ret = -1;
   if (om & ios::in) {
      ret = seekIn(so, dir);
//    if (ret == -1) return -1;      Unix chokes on this one
      if (ret < 0) return -1;
   }
   if (om & ios::out) {
      ret = seekOut(so, dir);
//    if (ret == -1) return -1;      Unix chokes on this one
      if (ret < 0) return -1;
   }
   return ret;
}

/*-------------------------------------------------------MemStreamBuf::seekIn-+
|                                                                             |
+----------------------------------------------------------------------------*/
streampos MemStreamBuf::seekIn(streamoff so, ios::seekdir dir)
{
   char * newPtr;
   switch (dir) {
   case ios::beg:
      newPtr = base() + so;
      break;
   case ios::end:
      newPtr = pptr() + so;
      break;
   default: // ios::cur
      newPtr = gptr() + so;
      break;
   }
   if ((newPtr >= base()) && (newPtr <= pptr())) {
      setg(base(), newPtr, pptr());
      return newPtr - base();
   }
   return -1;
}

/*------------------------------------------------------MemStreamBuf::seekOut-+
|                                                                             |
+----------------------------------------------------------------------------*/
streampos MemStreamBuf::seekOut(streamoff so, ios::seekdir dir)
{
   if (!bFrozen) {
      char * newPtr;
      switch (dir) {
      case ios::beg:
         newPtr = base() + so;
         break;
      default: // ios::cur, ios::end
         newPtr = pptr() + so;
         break;
      }
      if (newPtr >= gptr()) {
         if (newPtr > ebuf()) {
            if (!reallocate(newPtr - base())) return -1;
            newPtr = epptr();
         }
         setp(base(), ebuf());     // setp(newPtr, ebuf());
         pbump(newPtr - base());
         setg(base(), gptr(), newPtr);
         return newPtr - base();
      }
   }
   return -1;
}

/*------------------------------------------------------MemStreamBuf::seekpos-+
|                                                                             |
+----------------------------------------------------------------------------*/
streampos MemStreamBuf::seekpos(
   streampos sp,
   ios__openmode om
) {
   return seekoff((streamoff)sp, ios::beg, om);
}

/*-------------------------------------------------------MemStreamBuf::pcount-+
|                                                                             |
+----------------------------------------------------------------------------*/
int MemStreamBuf::pcount() {
   return pptr()-base();
}

/*-----------------------------------------------------------------NullStream-+
| Creates the object: invalid stream                                          |
+----------------------------------------------------------------------------*/
NullStream::NullStream() : iostream(&buf) {
   clear(failbit | badbit | eofbit);
}

/*===========================================================================*/
