/*
 * Adler32 for POWER 8+ using VSX instructions.
 *
 * Calculate adler32 checksum for 16 bytes at once using POWER8+ VSX (vector)
 * instructions.
 *
 * If adler32 do 1 byte at time on the first iteration s1 is s1_0 (_n means
 * iteration n) is the initial value of adler - at start  _0 is 1 unless
 * adler initial value is different than 1. So s1_1 = s1_0 + c[0] after
 * the first calculation. For the iteration s1_2 = s1_1 + c[1] and so on.
 * Hence, for iteration N, s1_N = s1_(N-1) + c[N] is the value of s1 on
 * after iteration N.
 *
 * Therefore, for s2 and iteration N, s2_N = s2_0 + N*s1_N + N*c[0] +
 * N-1*c[1] + ... + c[N]
 *
 * In a more general way:
 *
 * s1_N = s1_0 + sum(i=1 to N)c[i]
 * s2_N = s2_0 + N*s1 + sum (i=1 to N)(N-i+1)*c[i]
 *
 * Where s1_N, s2_N are the values for s1, s2 after N iterations. So if we
 * can process N-bit at time we can do this at once.
 *
 * Since VSX can support 16-bit vector instructions, we can process
 * 16-bit at time using N = 16 we have:
 *
 * s1 = s1_16 = s1_(16-1) + c[16] = s1_0 + sum(i=1 to 16)c[i]
 * s2 = s2_16 = s2_0 + 16*s1 + sum(i=1 to 16)(16-i+1)*c[i]
 *
 * After the first iteration we calculate the adler32 checksum for 16 bytes.
 *
 * For more background about adler32 please check the RFC:
 * https://www.ietf.org/rfc/rfc1950.txt
 *
 * Copyright (C) 2019 Rogerio Alves <rcardoso@linux.ibm.com>, IBM
 * For conditions of distribution and use, see copyright notice in zlib.h
 *
 */

#include "../../zutil.h"
#include <altivec.h>

/* Largest prime smaller than 65536.  */
#define BASE 65521U
#define NMAX 5552
/* NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1.  */

#define DO1(s1,s2,buf,i)  {(s1) += buf[(i)]; (s2) += (s1);}
#define DO2(s1,s2,buf,i)  {DO1(s1,s2,buf,i); DO1(s1,s2,buf,i+1);}
#define DO4(s1,s2,buf,i)  {DO2(s1,s2,buf,i); DO2(s1,s2,buf,i+2);}
#define DO8(s1,s2,buf,i)  {DO4(s1,s2,buf,i); DO4(s1,s2,buf,i+4);}
#define DO16(s1,s2,buf)   {DO8(s1,s2,buf,0); DO8(s1,s2,buf,8);}

/* Vector across sum unsigned int (saturate).  */
inline vector unsigned int vec_sumsu (vector unsigned int __a,
        vector unsigned int __b)
{
  __b = vec_sld(__a, __a, 8);
  __b = vec_add(__b, __a);
  __a = vec_sld(__b, __b, 4);
  __a = vec_add(__a, __b);

  return __a;
}

uLong ZLIB_INTERNAL _adler32_power8 (uLong adler, const Bytef* buf, uInt len)
{
  /* If buffer is empty or len=0 we need to return adler initial value.  */
  if (buf == NULL)
      return 1;

  unsigned int s1 = adler & 0xffff;
  unsigned int s2 = (adler >> 16) & 0xffff;

  /* in case user likes doing a byte at a time, keep it fast */
  if (len == 1) {
      s1 += buf[0];
      if (s1 >= BASE)
          s1 -= BASE;
      s2 += s1;
      if (s2 >= BASE)
          s2 -= BASE;
      return (s2 << 16) | s1;
  }

  /* Keep it fast for short length buffers. */
  if (len < 16) {
      while (len--) {
          s1 += *buf++;
          s2 += s1;
      }
      if (s1 >= BASE)
          s1 -= BASE;
      s2 %= BASE;
      return (s2 << 16) | s1;
  }

  /* This is faster than VSX code for len < 64.  */
  if (len < 64) {
      while (len >= 16) {
          len -= 16;
          DO16(s1,s2,buf);
          buf += 16;
      }
  } else {
      /* Use POWER VSX instructions for len >= 64. */
      const vector unsigned int v_zeros = { 0 };
      const vector unsigned char v_mul = {16, 15, 14, 13, 12, 11, 10, 9, 8, 7,
           6, 5, 4, 3, 2, 1};
      const vector unsigned char vsh = vec_splat_u8(4);
      const vector unsigned int vmask = {0xffffffff, 0x0, 0x0, 0x0};
      vector unsigned int vs1 = { 0 };
      vector unsigned int vs2 = { 0 };
      vector unsigned int vs1_save = { 0 };
      vector unsigned int vsum1, vsum2;
      vector unsigned char vbuf;
      int n;

      vs1[0] = s1;
      vs2[0] = s2;

      /* Do length bigger than NMAX in blocks of NMAX size.  */
      while (len >= NMAX) {
          len -= NMAX;
          n = NMAX / 16;
          do {
             vbuf = vec_xl(0, (unsigned char *) buf);
             vsum1 = vec_sum4s(vbuf, v_zeros); /* sum(i=1 to 16) buf[i].  */
             /* sum(i=1 to 16) buf[i]*(16-i+1).  */
             vsum2 = vec_msum(vbuf, v_mul, v_zeros);
             /* Save vs1.  */
             vs1_save = vec_add(vs1_save, vs1);
             /* Accumulate the sums.  */
             vs1 = vec_add(vsum1, vs1);
             vs2 = vec_add(vsum2, vs2);

             buf += 16;
          } while (--n);
          /* Once each block of NMAX size.  */
          vs1 = vec_sumsu(vs1, vsum1);
          vs1_save = vec_sll(vs1_save, vsh); /* 16*vs1_save.  */
          vs2 = vec_add(vs1_save, vs2);
          vs2 = vec_sumsu(vs2, vsum2);

          /* vs1[0] = (s1_i + sum(i=1 to 16)buf[i]) mod 65521.  */
          vs1[0] = vs1[0] % BASE;
          /* vs2[0] = s2_i + 16*s1_save +
             sum(i=1 to 16)(16-i+1)*buf[i] mod 65521.  */
          vs2[0] = vs2[0] % BASE;

          vs1 = vec_and(vs1, vmask);
          vs2 = vec_and(vs2, vmask);
          vs1_save = v_zeros;
      }

      /* len is less than NMAX one modulo is needed.  */
      if (len >= 16) {
          while (len >= 16) {
              len -= 16;

              vbuf = vec_xl(0, (unsigned char *) buf);

              vsum1 = vec_sum4s(vbuf, v_zeros); /* sum(i=1 to 16) buf[i].  */
              /* sum(i=1 to 16) buf[i]*(16-i+1).  */
              vsum2 = vec_msum(vbuf, v_mul, v_zeros);
              /* Save vs1.  */
              vs1_save = vec_add(vs1_save, vs1);
              /* Accumulate the sums.  */
              vs1 = vec_add(vsum1, vs1);
              vs2 = vec_add(vsum2, vs2);

              buf += 16;
          }
          /* Since the size will be always less than NMAX we do this once.  */
          vs1 = vec_sumsu(vs1, vsum1);
          vs1_save = vec_sll(vs1_save, vsh); /* 16*vs1_save.  */
          vs2 = vec_add(vs1_save, vs2);
          vs2 = vec_sumsu(vs2, vsum2);
      }
      /* Copy result back to s1, s2 (mod 65521).  */
      s1 = vs1[0] % BASE;
      s2 = vs2[0] % BASE;
   }

  /* Process tail (len < 16).  */
  while (len--) {
      s1 += *buf++;
      s2 += s1;
  }
  s1 %= BASE;
  s2 %= BASE;

  return (s2 << 16) | s1;
}
