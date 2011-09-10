/* temporary kludge assuming single pass decompression */

/* $Id: inflate.h,v 1.2 1995/04/11 14:47:32 jloup Exp $ */

#include <stdio.h>

#define NEXTBYTE \
   (istrm->total_in++, istrm->avail_in-- == 0 ? \
      (z_error("too small"), 0) : *istrm->next_in++)

#define FLUSH(n) { \
   if (istrm->avail_out < n) z_error("too big"); \
   istrm->avail_out -= n; \
   memcpy(istrm->next_out, slide, n); \
   istrm->next_out += n; \
   istrm->total_out += n; \
}
#define WSIZE istrm->state->w_size
#define slide istrm->state->window
#define memzero(a,s) memset((a),0,(s))
#define inflate z_inflate
#define qflag 1
