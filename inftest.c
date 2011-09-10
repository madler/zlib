#include <stdio.h>
#include <stdlib.h>
#include "zutil.h"

/* This test is in honor of Ed Hamrick who suggested that the interface
   to inflate be a byte at a time--this implements that, and is, of course,
   monumentally slow.  It has the virtue though of stressing the push-pull
   interface for testing purposes. */

void main()
{
  int a, r;
  char c;
  z_stream z;

  z.zalloc = Z_NULL;
  z.zfree = Z_NULL;
  r = inflateInit(&z);
  if (r != Z_OK)
    fprintf(stderr, "init error: %s\n", z_errmsg[1 - r]);
  while ((a = getchar()) != EOF)
  {
    /* feed one byte of input */
    z.avail_out = 0;
    c = (char)a;
    z.next_in = (Byte*)&c;
    z.avail_in = 1;
    r = inflate(&z, 0);
    if (r == Z_STREAM_END)
      break;
    if (r != Z_OK)
    {
      fprintf(stderr, "inflate error: %s\n", z_errmsg[1 - r]);
      break;
    }
    if (z.avail_in != 0)
    {
      fprintf(stderr, "inflate didn't eat byte and didn't say buf err!\n");
      break;
    }

    /* empty output one byte at a time */
    while (1)
    {
      z.next_out = (Byte*)&c;
      z.avail_out = 1;
      r = inflate(&z, 0);
      if (r == Z_STREAM_END)
	break;
      if (r != Z_OK && r != Z_BUF_ERROR)
      {
	fprintf(stderr, "inflate error: %s\n", z_errmsg[1 - r]);
	break;
      }
      if (z.avail_out == 0)
        putchar(c);
      else
        break;
    }
    if (r != Z_OK && r != Z_BUF_ERROR)
      break;
  }
  inflateEnd(&z);
  fprintf(stderr, "%d bytes in, %d bytes out\n", z.total_in, z.total_out);
  if (z.msg != NULL)
    fprintf(stderr, "msg is <%s>\n", z.msg);
}
