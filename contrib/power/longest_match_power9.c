/* Copyright (C) 2019 Matheus Castanho <msc@linux.ibm.com>, IBM
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#include <altivec.h>
#include "../../deflate.h"

local inline int vec_match OF((Bytef* scan, Bytef* match))
    __attribute__((always_inline));

local inline int vec_match(Bytef* scan, Bytef* match)
{
    vector unsigned char vscan, vmatch, vc;

    vscan  = *((vector unsigned char *) scan);
    vmatch = *((vector unsigned char *) match);

    /* Compare 16 bytes at a time.
     * Each byte of vc will be either all ones or all zeroes,
     * depending on the result of the comparison
     */
    vc = (vector unsigned char) vec_cmpne(vscan,vmatch);

    /* Since the index of matching bytes will contain only zeroes
     * on vc (since we used cmpne), counting the number of consecutive
     * bytes where LSB == 0 is the same as counting the length of the match.
     */
    return vec_cnttz_lsbb(vc);
}

uInt ZLIB_INTERNAL _longest_match_power9(deflate_state *s, IPos cur_match)
{
    unsigned chain_length = s->max_chain_length;/* max hash chain length */
    register Bytef *scan = s->window + s->strstart; /* current string */
    register Bytef *match;                      /* matched string */
    register int len;                           /* length of current match */
    int best_len = (int)s->prev_length;         /* best match length so far */
    int nice_match = s->nice_match;             /* stop if match long enough */
    int mbytes;                                 /* matched bytes inside loop */
    IPos limit = s->strstart > (IPos)MAX_DIST(s) ?
        s->strstart - (IPos)MAX_DIST(s) : 0;
    /* Stop when cur_match becomes <= limit. To simplify the code,
     * we prevent matches with the string of window index 0.
     */
    Posf *prev = s->prev;
    uInt wmask = s->w_mask;

#if (MAX_MATCH == 258)
    /* Compare the last two bytes at once. */
    register Bytef *strend2 = s->window + s->strstart + MAX_MATCH - 2;
    register ush scan_end   = *(ushf*)(scan+best_len-1);
#else
    register Bytef *strend = s->window + s->strstart + MAX_MATCH;
    register Byte scan_end1  = scan[best_len-1];
    register Byte scan_end   = scan[best_len];
#endif

    /* The code is optimized for HASH_BITS >= 8 and MAX_MATCH-2 multiple of 16.
     * It is easy to get rid of this optimization if necessary.
     */
    Assert(s->hash_bits >= 8 && MAX_MATCH == 258, "Code too clever");

    /* Do not waste too much time if we already have a good match: */
    if (s->prev_length >= s->good_match) {
        chain_length >>= 2;
    }
    /* Do not look for matches beyond the end of the input. This is necessary
     * to make deflate deterministic.
     */
    if ((uInt)nice_match > s->lookahead) nice_match = (int)s->lookahead;

    Assert((ulg)s->strstart <= s->window_size-MIN_LOOKAHEAD, "need lookahead");

    do {
        Assert(cur_match < s->strstart, "no future");
        match = s->window + cur_match;

        /* Skip to next match if the match length cannot increase
         * or if the match length is less than 2.  Note that the checks below
         * for insufficient lookahead only occur occasionally for performance
         * reasons.  Therefore uninitialized memory will be accessed, and
         * conditional jumps will be made that depend on those values.
         * However the length of the match is limited to the lookahead, so
         * the output of deflate is not affected by the uninitialized values.
         */

/* MAX_MATCH - 2 should be a multiple of 16 for this optimization to work. */
#if (MAX_MATCH == 258)

        /* Compare ending (2 bytes) and beginning of potential match.
         *
         * On Power processors, loading a 16-byte vector takes only 1 extra
         * cycle compared to a regular byte load. So instead of comparing the
         * first two bytes and then the rest later if they match, we can compare
         * the first 16 at once, and when we have a match longer than 2, we will
         * already have the result of comparing the first 16 bytes saved in mbytes.
         */
        if (*(ushf*)(match+best_len-1) != scan_end ||
             (mbytes = vec_match(scan,match)) < 3) continue;

        scan  += mbytes;
        match += mbytes;

        /* In case when we may have a match longer than 16, we perform further
         * comparisons in chunks of 16 and keep going while all bytes match.
         */
        while(mbytes == 16) {
            mbytes = vec_match(scan,match);
            scan += mbytes;
            match += mbytes;

            /* We also have to limit the maximum match based on MAX_MATCH.
             * Since we are comparing 16 bytes at a time and MAX_MATCH == 258 (to
             * comply with default implementation), we should stop comparing when
             * we have matched 256 bytes, which happens when scan == strend2.
             * In this ("rare") case, we have to check the remaining 2 bytes
             * individually using common load and compare operations.
             */
            if(scan >= strend2) {
                if(*scan == *match) {
                    if(*++scan == *++match)
                        scan++;
                }
                break;
            }
        }

        Assert(scan <= s->window+(unsigned)(s->window_size-1), "wild scan");

        len = (MAX_MATCH - 2) - (int)(strend2 - scan);
        scan = strend2 - (MAX_MATCH - 2);

#else /* MAX_MATCH == 258 */

        if (match[best_len]   != scan_end  ||
            match[best_len-1] != scan_end1 ||
            *match            != *scan     ||
            *++match          != scan[1])      continue;

        /* The check at best_len-1 can be removed because it will be made
         * again later. (This heuristic is not always a win.)
         * It is not necessary to compare scan[2] and match[2] since they
         * are always equal when the other bytes match, given that
         * the hash keys are equal and that HASH_BITS >= 8.
         */
        scan += 2, match++;
        Assert(*scan == *match, "match[2]?");

        /* We check for insufficient lookahead only every 8th comparison;
         * the 256th check will be made at strstart+258.
         */
        do {
        } while (*++scan == *++match && *++scan == *++match &&
                 *++scan == *++match && *++scan == *++match &&
                 *++scan == *++match && *++scan == *++match &&
                 *++scan == *++match && *++scan == *++match &&
                 scan < strend);

        Assert(scan <= s->window+(unsigned)(s->window_size-1), "wild scan");

        len = MAX_MATCH - (int)(strend - scan);
        scan = strend - MAX_MATCH;

#endif /* MAX_MATCH == 258 */

        if (len > best_len) {
            s->match_start = cur_match;
            best_len = len;
            if (len >= nice_match) break;
#if (MAX_MATCH == 258)
            scan_end = *(ushf*)(scan+best_len-1);
#else
            scan_end1  = scan[best_len-1];
            scan_end   = scan[best_len];
#endif
        }
    } while ((cur_match = prev[cur_match & wmask]) > limit
             && --chain_length != 0);

    if ((uInt)best_len <= s->lookahead) return (uInt)best_len;
    return s->lookahead;
}
