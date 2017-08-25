/* inffast64.c is a 64 bit version of inffast.c. Pure C, no asm.
 * Copyright (C) 2017 Google.
 *
 * It benchmarks 1.5x faster than the top-level inffast.c, on x86 64. See
 * https://gist.github.com/nigeltao/5deba67065a302ebe9ac342235a709c2
 *
 * The diff between this file and inffast.c can be seen at
 * https://gist.github.com/nigeltao/2884d66f16fd3109184967a78cfea7a5
 */

/* inffast.c -- fast decoding
 * Copyright (C) 1995-2017 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#include "zutil.h"
#include "inftrees.h"
#include "inflate.h"
#include "inffast.h"

#ifdef ASMINF
#  pragma message("Assembler code may have bugs -- use at your own risk")
#elif INFLATE_FAST64

#if defined(ZLIB_INTERNAL_HAVE_64_BIT_UNALIGNED_LOADS) || \
    defined(ZLIB_INTERNAL_HAVE_64_BIT_UNALIGNED_LOADS_LITTLE_ENDIAN)
#include <stdint.h>
#endif

/*
   Decode literal, length, and distance codes and write out the resulting
   literal and match bytes until either not enough input or output is
   available, an end-of-block is encountered, or a data error is encountered.
   When large enough input and output buffers are supplied to inflate(), for
   example, a 16K input buffer and a 64K output buffer, more than 95% of the
   inflate execution time is spent in this routine.

   Entry assumptions:

        state->mode == LEN
        strm->avail_in >= INFLATE_FAST_MIN_HAVE
        strm->avail_out >= INFLATE_FAST_MIN_LEFT
        start >= strm->avail_out
        state->bits < 8
        (state->hold >> state->bits) == 0

   On return, state->mode is one of:

        LEN -- ran out of enough output space or enough available input
        TYPE -- reached end of block code, inflate() to interpret next block
        BAD -- error in block data

   Notes:

    - The maximum input bits used by a length/distance pair is 15 bits for the
      length code, 5 bits for the length extra, 15 bits for the distance code,
      and 13 bits for the distance extra.  This totals 48 bits, or six bytes.
      Therefore if strm->avail_in >= 6, then there is enough input to avoid
      checking for available input while decoding.

    - On some architectures, it can be significantly faster (e.g. up to 1.2x
      faster on x86_64) to load from strm->next_in 64 bits, or 8 bytes, at a
      time, so INFLATE_FAST_MIN_HAVE == 8. This requires a little endian
      architecture.

    - The maximum bytes that a single length/distance pair can output is 258
      bytes, which is the maximum length that can be coded.  inflate_fast()
      requires strm->avail_out >= 258 for each loop to avoid checking for
      output space.

    - On some architectures, for length-distance copies, we similarly load and
      store 8 bytes at a time, if the distance is at least 8. Again, this can
      be significantly faster (e.g. up to 1.3x faster on x86_64). Rounding up
      to a multiple of 8 gives INFLATE_FAST_MIN_LEFT == 264. This does not
      require a little endian architecture. This always copies 8*L bytes (where
      L is the smallest integer such that 8*L >= len, i.e. we round length up
      to a multiple of 8), instead of only len bytes, but that's OK, as
      subsequent iterations will fix the overrun.

    - Combining those two optimizations for 64 bit unaligned loads gives up to
      a 1.5x throughput improvement on x86_64.

 */
void ZLIB_INTERNAL inflate_fast(strm, start)
z_streamp strm;
unsigned start;         /* inflate()'s starting value for strm->avail_out */
{
    struct inflate_state FAR *state;
    z_const unsigned char FAR *in;      /* local strm->next_in */
    z_const unsigned char FAR *last;    /* have enough input while in < last */
    unsigned char FAR *out;     /* local strm->next_out */
    unsigned char FAR *beg;     /* inflate()'s initial strm->next_out */
    unsigned char FAR *end;     /* while out < end, enough space available */
#ifdef INFLATE_STRICT
    unsigned dmax;              /* maximum distance from zlib header */
#endif
    unsigned wsize;             /* window size or zero if not using window */
    unsigned whave;             /* valid bytes in the window */
    unsigned wnext;             /* window write index */
    unsigned char FAR *window;  /* allocated sliding window, if wsize != 0 */

    /* hold is a local copy of strm->hold. By default, hold satisfies the same
       invariants that strm->hold does, namely that (hold >> bits) == 0. This
       invariant is kept by loading bits into hold one byte at a time, like:

       hold |= next_byte_of_input << bits; in++; bits += 8;

       If we need to ensure that bits >= 15 then this code snippet is simply
       repeated. Over one iteration of the outermost do/while loop, this
       happens up to six times (48 bits of input), as described in the NOTES
       above.

       However, on some little endian architectures, it can be significantly
       faster to load 64 bits once instead of 8 bits six times:

       if (bits <= 16) {
         hold |= next_8_bytes_of_input << bits; in += 6; bits += 48;
       }

       Unlike the simpler one byte load, shifting the next_8_bytes_of_input
       by bits will overflow and lose those high bits, up to 2 bytes' worth.
       The conservative estimate is therefore that we have read only 6 bytes
       (48 bits). Again, as per the NOTES above, 48 bits is sufficient for the
       rest of the iteration, and we will not need to load another 8 bytes.

       Inside this function, we no longer satisfy (hold >> bits) == 0, but
       this is not problematic, even if that overflow does not land on an 8 bit
       byte boundary. Those excess bits will eventually shift down lower as the
       Huffman decoder consumes input, and when new input bits need to be loaded
       into the bits variable, the same input bits will be or'ed over those
       existing bits. A bitwise or is idempotent: (a | b | b) equals (a | b).
       Note that we therefore write that load operation as "hold |= etc" and not
       "hold += etc".

       Outside that loop, at the end of the function, hold is bitwise and'ed
       with (1<<bits)-1 to drop those excess bits so that, on function exit, we
       keep the invariant that (state->hold >> state->bits) == 0.

       TODO: rename the bits variable to nbits, so that this block comment
       is less confusing when discussing bits (the variable) and bits (one
       eighth of a byte).
       */
#ifdef ZLIB_INTERNAL_HAVE_64_BIT_UNALIGNED_LOADS_LITTLE_ENDIAN
    uint64_t hold;
#else
    unsigned long hold;
#endif

    unsigned bits;              /* local strm->bits */
    code const FAR *lcode;      /* local strm->lencode */
    code const FAR *dcode;      /* local strm->distcode */
    unsigned lmask;             /* mask for first level of length codes */
    unsigned dmask;             /* mask for first level of distance codes */
    code here;                  /* retrieved table entry */
    unsigned op;                /* code bits, operation, extra bits, or */
                                /*  window position, window bytes to copy */
    unsigned len;               /* match length, unused bytes */
    unsigned dist;              /* match distance */
    unsigned char FAR *from;    /* where to copy match from */

    /* copy state to local variables */
    state = (struct inflate_state FAR *)strm->state;
    in = strm->next_in;
    last = in + (strm->avail_in - (INFLATE_FAST_MIN_HAVE - 1));
    out = strm->next_out;
    beg = out - (start - strm->avail_out);
    end = out + (strm->avail_out - (INFLATE_FAST_MIN_LEFT - 1));
#ifdef INFLATE_STRICT
    dmax = state->dmax;
#endif
    wsize = state->wsize;
    whave = state->whave;
    wnext = state->wnext;
    window = state->window;
    hold = state->hold;
    bits = state->bits;
    lcode = state->lencode;
    dcode = state->distcode;
    lmask = (1U << state->lenbits) - 1;
    dmask = (1U << state->distbits) - 1;

    /* decode literals and length/distances until end-of-block or not enough
       input data or output space */
    do {
        if (bits < 15) {
#ifdef ZLIB_INTERNAL_HAVE_64_BIT_UNALIGNED_LOADS_LITTLE_ENDIAN
            /* Example disassembly on __x86_64__:
               49 8b 06              mov    (%r14),%rax
               89 f1                 mov    %esi,%ecx
               49 83 c6 06           add    $0x6,%r14
               83 c6 30              add    $0x30,%esi
               48 d3 e0              shl    %cl,%rax
               48 09 c2              or     %rax,%rdx
             */
            hold |= *((uint64_t *)(in)) << bits;
            in += 6;
            bits += 48;
#else
            /* Example disassembly on __x86_64__:
               45 0f b6 45 00        movzbl 0x0(%r13),%r8d
               89 f1                 mov    %esi,%ecx
               49 83 c5 02           add    $0x2,%r13
               49 d3 e0              shl    %cl,%r8
               8d 4e 08              lea    0x8(%rsi),%ecx
               83 c6 10              add    $0x10,%esi
               49 01 d0              add    %rdx,%r8
               41 0f b6 55 ff        movzbl -0x1(%r13),%edx
               48 d3 e2              shl    %cl,%rdx
               4c 01 c2              add    %r8,%rdx
             */
            /* TODO: replace "hold += etc" with "hold |= etc", here and below,
               to be consistent with the 64 bit unaligned code path. This is
               only a comment for now so that the commit that introduced this
               comment has no effect whatsoever on architectures without
               ZLIB_INTERNAL_HAVE_64_BIT_UNALIGNED_LOADS_LITTLE_ENDIAN. */
            hold += (unsigned long)(*in++) << bits;
            bits += 8;
            hold += (unsigned long)(*in++) << bits;
            bits += 8;
#endif
        }
        here = lcode[hold & lmask];
      dolen:
        op = (unsigned)(here.bits);
        hold >>= op;
        bits -= op;
        op = (unsigned)(here.op);
        if (op == 0) {                          /* literal */
            Tracevv((stderr, here.val >= 0x20 && here.val < 0x7f ?
                    "inflate:         literal '%c'\n" :
                    "inflate:         literal 0x%02x\n", here.val));
            *out++ = (unsigned char)(here.val);
        }
        else if (op & 16) {                     /* length base */
            len = (unsigned)(here.val);
            op &= 15;                           /* number of extra bits */
            if (op) {
                if (bits < op) {
#ifdef ZLIB_INTERNAL_HAVE_64_BIT_UNALIGNED_LOADS_LITTLE_ENDIAN
                    hold |= *((uint64_t *)(in)) << bits;
                    in += 6;
                    bits += 48;
#else
                    hold += (unsigned long)(*in++) << bits;
                    bits += 8;
#endif
                }
                len += (unsigned)hold & ((1U << op) - 1);
                hold >>= op;
                bits -= op;
            }
            Tracevv((stderr, "inflate:         length %u\n", len));
            if (bits < 15) {
#ifdef ZLIB_INTERNAL_HAVE_64_BIT_UNALIGNED_LOADS_LITTLE_ENDIAN
                hold |= *((uint64_t *)(in)) << bits;
                in += 6;
                bits += 48;
#else
                hold += (unsigned long)(*in++) << bits;
                bits += 8;
                hold += (unsigned long)(*in++) << bits;
                bits += 8;
#endif
            }
            here = dcode[hold & dmask];
          dodist:
            op = (unsigned)(here.bits);
            hold >>= op;
            bits -= op;
            op = (unsigned)(here.op);
            if (op & 16) {                      /* distance base */
                dist = (unsigned)(here.val);
                op &= 15;                       /* number of extra bits */
                if (bits < op) {
#ifdef ZLIB_INTERNAL_HAVE_64_BIT_UNALIGNED_LOADS_LITTLE_ENDIAN
                    hold |= *((uint64_t *)(in)) << bits;
                    in += 6;
                    bits += 48;
#else
                    hold += (unsigned long)(*in++) << bits;
                    bits += 8;
                    if (bits < op) {
                        hold += (unsigned long)(*in++) << bits;
                        bits += 8;
                    }
#endif
                }
                dist += (unsigned)hold & ((1U << op) - 1);
#ifdef INFLATE_STRICT
                if (dist > dmax) {
                    strm->msg = (char *)"invalid distance too far back";
                    state->mode = BAD;
                    break;
                }
#endif
                hold >>= op;
                bits -= op;
                Tracevv((stderr, "inflate:         distance %u\n", dist));
                op = (unsigned)(out - beg);     /* max distance in output */
                if (dist > op) {                /* see if copy from window */
                    op = dist - op;             /* distance back in window */
                    if (op > whave) {
                        if (state->sane) {
                            strm->msg =
                                (char *)"invalid distance too far back";
                            state->mode = BAD;
                            break;
                        }
#ifdef INFLATE_ALLOW_INVALID_DISTANCE_TOOFAR_ARRR
                        if (len <= op - whave) {
                            do {
                                *out++ = 0;
                            } while (--len);
                            continue;
                        }
                        len -= op - whave;
                        do {
                            *out++ = 0;
                        } while (--op > whave);
                        if (op == 0) {
                            from = out - dist;
                            do {
                                *out++ = *from++;
                            } while (--len);
                            continue;
                        }
#endif
                    }
                    from = window;
                    if (wnext == 0) {           /* very common case */
                        from += wsize - op;
                        if (op < len) {         /* some from window */
                            len -= op;
                            do {
                                *out++ = *from++;
                            } while (--op);
                            from = out - dist;  /* rest from output */
                        }
                    }
                    else if (wnext < op) {      /* wrap around window */
                        from += wsize + wnext - op;
                        op -= wnext;
                        if (op < len) {         /* some from end of window */
                            len -= op;
                            do {
                                *out++ = *from++;
                            } while (--op);
                            from = window;
                            if (wnext < len) {  /* some from start of window */
                                op = wnext;
                                len -= op;
                                do {
                                    *out++ = *from++;
                                } while (--op);
                                from = out - dist;      /* rest from output */
                            }
                        }
                    }
                    else {                      /* contiguous in window */
                        from += wnext - op;
                        if (op < len) {         /* some from window */
                            len -= op;
                            do {
                                *out++ = *from++;
                            } while (--op);
                            from = out - dist;  /* rest from output */
                        }
                    }

#ifdef ZLIB_INTERNAL_HAVE_64_BIT_UNALIGNED_LOADS
                    if (dist >= 8) {
                        while (1) {
                            *((uint64_t*)(out)) = *((uint64_t*)(from));
                            if (len <= 8) {
                                out += len;
                                break;
                            }
                            out += 8;
                            from += 8;
                            len -= 8;
                        }
                        continue;
                    }
#endif
                    while (len > 2) {
                        *out++ = *from++;
                        *out++ = *from++;
                        *out++ = *from++;
                        len -= 3;
                    }
                    if (len) {
                        *out++ = *from++;
                        if (len > 1)
                            *out++ = *from++;
                    }
                }
                else {
                    from = out - dist;          /* copy direct from output */
#ifdef ZLIB_INTERNAL_HAVE_64_BIT_UNALIGNED_LOADS
                    if (dist >= 8) {
                        while (1) {
                            *((uint64_t*)(out)) = *((uint64_t*)(from));
                            if (len <= 8) {
                                out += len;
                                break;
                            }
                            out += 8;
                            from += 8;
                            len -= 8;
                        }
                        continue;
                    }
#endif
                    do {                        /* minimum length is three */
                        *out++ = *from++;
                        *out++ = *from++;
                        *out++ = *from++;
                        len -= 3;
                    } while (len > 2);
                    if (len) {
                        *out++ = *from++;
                        if (len > 1)
                            *out++ = *from++;
                    }
                }
            }
            else if ((op & 64) == 0) {          /* 2nd level distance code */
                here = dcode[here.val + (hold & ((1U << op) - 1))];
                goto dodist;
            }
            else {
                strm->msg = (char *)"invalid distance code";
                state->mode = BAD;
                break;
            }
        }
        else if ((op & 64) == 0) {              /* 2nd level length code */
            here = lcode[here.val + (hold & ((1U << op) - 1))];
            goto dolen;
        }
        else if (op & 32) {                     /* end-of-block */
            Tracevv((stderr, "inflate:         end of block\n"));
            state->mode = TYPE;
            break;
        }
        else {
            strm->msg = (char *)"invalid literal/length code";
            state->mode = BAD;
            break;
        }
    } while (in < last && out < end);

    /* return unused bytes (on entry, bits < 8, so in won't go too far back) */
    len = bits >> 3;
    in -= len;
    bits -= len << 3;
    hold &= (1U << bits) - 1;

    /* update state and return */
    strm->next_in = in;
    strm->next_out = out;
    strm->avail_in = (unsigned)(in < last ?
        (INFLATE_FAST_MIN_HAVE - 1) + (last - in) :
        (INFLATE_FAST_MIN_HAVE - 1) - (in - last));
    strm->avail_out = (unsigned)(out < end ?
        (INFLATE_FAST_MIN_LEFT - 1) + (end - out) :
        (INFLATE_FAST_MIN_LEFT - 1) - (out - end));
    state->hold = hold;
    state->bits = bits;
    return;
}

/*
   inflate_fast() speedups that turned out slower (on a PowerPC G3 750CXe):
   - Using bit fields for code structure
   - Different op definition to avoid & for extra bits (do & for table bits)
   - Three separate decoding do-loops for direct, window, and wnext == 0
   - Special case for distance > 1 copies to do overlapped load and store copy
   - Explicit branch predictions (based on measured branch probabilities)
   - Deferring match copy and interspersed it with decoding subsequent codes
   - Swapping literal/length else
   - Swapping window/direct else
   - Larger unrolled copy loops (three is about right)
   - Moving len -= 3 statement into middle of loop
 */

#endif /* !ASMINF && INFLATE_FAST64 */
