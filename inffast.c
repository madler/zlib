/* inffast.c -- fast decoding
 * Copyright (C) 1995-2026 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#include "zutil.h"
#include "inftrees.h"
#include "inflate.h"
#include "inffast.h"

#ifdef ASMINF
#  pragma message("Assembler code may have bugs -- use at your own risk")
#else

#ifdef Z_U8
/* Fast loop: with a 64-bit type the bit buffer is refilled eight bytes at a
   time, and matches are copied eight or sixteen bytes at a time. */

#define TRACE_LITERAL(here) \
    Tracevv((stderr, (here)->val >= 0x20 && (here)->val < 0x7f ? \
            "inflate:         literal '%c'\n" : \
            "inflate:         literal 0x%02x\n", (here)->val))

/* The eight bytes at p, little-endian: one load on a little-endian machine,
   the bytes one by one otherwise. */
local Z_U8 load64(z_const unsigned char FAR *p) {
    Z_U8 v = 1;

    if (*(unsigned char *)&v) {
        zmemcpy(&v, p, 8);
        return v;
    }
    return (Z_U8)p[0] | ((Z_U8)p[1] << 8) | ((Z_U8)p[2] << 16) |
           ((Z_U8)p[3] << 24) | ((Z_U8)p[4] << 32) | ((Z_U8)p[5] << 40) |
           ((Z_U8)p[6] << 48) | ((Z_U8)p[7] << 56);
}

/* Copy len >= 1 bytes from dist bytes back in the output to out, and return
   out + len. It may write up to fifteen bytes after out + len, so the caller
   must have that room. */
local unsigned char FAR *copy_back(unsigned char FAR *out, unsigned dist,
                                   unsigned len) {
    unsigned char FAR *from = out - dist;
    unsigned char FAR *stop = out + len;

    if (dist >= 16) {                   /* sixteen at a time, no overlap */
        do {
            zmemcpy(out, from, 16);
            out += 16;
            from += 16;
        } while (out < stop);
        return stop;
    }
    if (dist >= 8) {                    /* eight at a time, no overlap */
        do {
            zmemcpy(out, from, 8);
            out += 8;
            from += 8;
        } while (out < stop);
        return stop;
    }
    if (dist == 1) {                    /* a run of one byte, eight at a time */
        Z_U8 run = *from;
        run |= run << 8;
        run |= run << 16;
        run |= run << 32;
        do {
            zmemcpy(out, &run, 8);
            out += 8;
        } while (out < stop);
        return stop;
    }
    /* dist 2..7: eight bytes per step, of which the first dist are right;
       the next step rewrites the rest */
    do {
        Z_U8 v;
        zmemcpy(&v, from, 8);
        zmemcpy(out, &v, 8);
        out += dist;
        from += dist;
    } while (out < stop);
    return stop;
}
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
        strm->avail_in >= 6
        strm->avail_out >= 258
        start >= strm->avail_out
        state->bits < 8

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

    - The maximum bytes that a single length/distance pair can output is 258
      bytes, which is the maximum length that can be coded.  inflate_fast()
      requires strm->avail_out >= 258 for each loop to avoid checking for
      output space.
 */
void ZLIB_INTERNAL inflate_fast(z_streamp strm, unsigned start) {
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
    unsigned long hold;         /* local strm->hold */
    unsigned bits;              /* local strm->bits */
    code const FAR *lcode;      /* local strm->lencode */
    code const FAR *dcode;      /* local strm->distcode */
    unsigned lmask;             /* mask for first level of length codes */
    unsigned dmask;             /* mask for first level of distance codes */
    code const *here;           /* retrieved table entry */
    unsigned op;                /* code bits, operation, extra bits, or */
                                /*  window position, window bytes to copy */
    unsigned len;               /* match length, unused bytes */
    unsigned dist;              /* match distance */
    unsigned char FAR *from;    /* where to copy match from */
#ifdef Z_U8
    Z_U8 hold8;                 /* 64-bit bit buffer of the fast loop */
    z_const unsigned char FAR *last8;   /* two refills fit after in */
    unsigned char FAR *end8;    /* room for a match and fifteen more */
#endif

    /* copy state to local variables */
    state = (struct inflate_state FAR *)strm->state;
    in = strm->next_in;
    last = in + (strm->avail_in - 5);
    out = strm->next_out;
    beg = out - (start - strm->avail_out);
    end = out + (strm->avail_out - 257);
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

#ifdef Z_U8
    /* Fast loop, while sixteen bytes can be read at in (two refills of
       eight bytes, the first advancing in by up to seven) and a match plus
       fifteen bytes fits in the output. Each length/literal code starts with
       at least 56 bits in hold8, enough for a length/distance pair (at most
       48 bits). */
    if (strm->avail_in >= 16 && strm->avail_out >= 258 + 15) {
        last8 = in + (strm->avail_in - 16);
        end8 = out + (strm->avail_out - (258 + 15));
        hold8 = hold;
        do {
            hold8 |= load64(in) << bits;
            in += (63 - bits) >> 3;
            bits |= 56;
            here = lcode + (hold8 & lmask);
            if (here->op == 0) {        /* up to three literals a refill */
                hold8 >>= here->bits;
                bits -= here->bits;
                TRACE_LITERAL(here);
                *out++ = (unsigned char)(here->val);
                here = lcode + (hold8 & lmask);
                if (here->op == 0) {
                    hold8 >>= here->bits;
                    bits -= here->bits;
                    TRACE_LITERAL(here);
                    *out++ = (unsigned char)(here->val);
                    here = lcode + (hold8 & lmask);
                    if (here->op == 0) {
                        hold8 >>= here->bits;
                        bits -= here->bits;
                        TRACE_LITERAL(here);
                        *out++ = (unsigned char)(here->val);
                        continue;
                    }
                }
                hold8 |= load64(in) << bits;    /* 56 bits for a pair */
                in += (63 - bits) >> 3;
                bits |= 56;
            }
          dolen8:
            op = (unsigned)(here->bits);
            hold8 >>= op;
            bits -= op;
            op = (unsigned)(here->op);
            if (op == 0) {                      /* literal */
                TRACE_LITERAL(here);
                *out++ = (unsigned char)(here->val);
            }
            else if (op & 16) {                 /* length base */
                len = (unsigned)(here->val);
                op &= 15;                       /* number of extra bits */
                len += (unsigned)hold8 & ((1U << op) - 1);
                hold8 >>= op;
                bits -= op;
                Tracevv((stderr, "inflate:         length %u\n", len));
                here = dcode + (hold8 & dmask);
              dodist8:
                op = (unsigned)(here->bits);
                hold8 >>= op;
                bits -= op;
                op = (unsigned)(here->op);
                if (op & 16) {                  /* distance base */
                    dist = (unsigned)(here->val);
                    op &= 15;                   /* number of extra bits */
                    dist += (unsigned)hold8 & ((1U << op) - 1);
#ifdef INFLATE_STRICT
                    if (dist > dmax) {
                        strm->msg = (z_const char *)
                            "invalid distance too far back";
                        state->mode = BAD;
                        break;
                    }
#endif
                    hold8 >>= op;
                    bits -= op;
                    Tracevv((stderr, "inflate:         distance %u\n", dist));
                    op = (unsigned)(out - beg); /* max distance in output */
                    if (dist > op) {            /* see if copy from window */
                        op = dist - op;         /* distance back in window */
                        if (op > whave) {
                            if (state->sane) {
                                strm->msg = (z_const char *)
                                    "invalid distance too far back";
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
                                out = copy_back(out, dist, len);
                                continue;
                            }
#endif
                        }
                        from = window;
                        if (wnext == 0)         /* very common case */
                            from += wsize - op;
                        else if (wnext < op) {  /* wrap around window */
                            from += wsize + wnext - op;
                            op -= wnext;
                            if (op < len) {     /* end of window, then start */
                                zmemcpy(out, from, op);
                                out += op;
                                len -= op;
                                from = window;
                                op = wnext;
                            }
                        }
                        else                    /* contiguous in window */
                            from += wnext - op;
                        if (op > len)
                            op = len;
                        zmemcpy(out, from, op); /* from the window */
                        out += op;
                        len -= op;
                        if (len)                /* rest from output */
                            out = copy_back(out, dist, len);
                    }
                    else
                        out = copy_back(out, dist, len);
                }
                else if ((op & 64) == 0) {      /* 2nd level distance code */
                    here = dcode + here->val + (hold8 & ((1U << op) - 1));
                    goto dodist8;
                }
                else {
                    strm->msg = (z_const char *)"invalid distance code";
                    state->mode = BAD;
                    break;
                }
            }
            else if ((op & 64) == 0) {          /* 2nd level length code */
                here = lcode + here->val + (hold8 & ((1U << op) - 1));
                goto dolen8;
            }
            else if (op & 32) {                 /* end-of-block */
                Tracevv((stderr, "inflate:         end of block\n"));
                state->mode = TYPE;
                break;
            }
            else {
                strm->msg = (z_const char *)"invalid literal/length code";
                state->mode = BAD;
                break;
            }
        } while (in <= last8 && out < end8);

        /* return the whole bytes not used, leaving fewer than eight bits */
        len = bits >> 3;
        in -= len;
        bits -= len << 3;
        hold = (unsigned long)(hold8 & ((1U << bits) - 1));
        if (state->mode != LEN || in >= last || out >= end)
            goto leave;
    }
#endif

    /* decode literals and length/distances until end-of-block or not enough
       input data or output space */
    do {
        if (bits < 15) {
            hold += (unsigned long)(*in++) << bits;
            bits += 8;
            hold += (unsigned long)(*in++) << bits;
            bits += 8;
        }
        here = lcode + (hold & lmask);
      dolen:
        op = (unsigned)(here->bits);
        hold >>= op;
        bits -= op;
        op = (unsigned)(here->op);
        if (op == 0) {                          /* literal */
            Tracevv((stderr, here->val >= 0x20 && here->val < 0x7f ?
                    "inflate:         literal '%c'\n" :
                    "inflate:         literal 0x%02x\n", here->val));
            *out++ = (unsigned char)(here->val);
        }
        else if (op & 16) {                     /* length base */
            len = (unsigned)(here->val);
            op &= 15;                           /* number of extra bits */
            if (op) {
                if (bits < op) {
                    hold += (unsigned long)(*in++) << bits;
                    bits += 8;
                }
                len += (unsigned)hold & ((1U << op) - 1);
                hold >>= op;
                bits -= op;
            }
            Tracevv((stderr, "inflate:         length %u\n", len));
            if (bits < 15) {
                hold += (unsigned long)(*in++) << bits;
                bits += 8;
                hold += (unsigned long)(*in++) << bits;
                bits += 8;
            }
            here = dcode + (hold & dmask);
          dodist:
            op = (unsigned)(here->bits);
            hold >>= op;
            bits -= op;
            op = (unsigned)(here->op);
            if (op & 16) {                      /* distance base */
                dist = (unsigned)(here->val);
                op &= 15;                       /* number of extra bits */
                if (bits < op) {
                    hold += (unsigned long)(*in++) << bits;
                    bits += 8;
                    if (bits < op) {
                        hold += (unsigned long)(*in++) << bits;
                        bits += 8;
                    }
                }
                dist += (unsigned)hold & ((1U << op) - 1);
#ifdef INFLATE_STRICT
                if (dist > dmax) {
                    strm->msg = (z_const char *)
                        "invalid distance too far back";
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
                            strm->msg = (z_const char *)
                                "invalid distance too far back";
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
                here = dcode + here->val + (hold & ((1U << op) - 1));
                goto dodist;
            }
            else {
                strm->msg = (z_const char *)"invalid distance code";
                state->mode = BAD;
                break;
            }
        }
        else if ((op & 64) == 0) {              /* 2nd level length code */
            here = lcode + here->val + (hold & ((1U << op) - 1));
            goto dolen;
        }
        else if (op & 32) {                     /* end-of-block */
            Tracevv((stderr, "inflate:         end of block\n"));
            state->mode = TYPE;
            break;
        }
        else {
            strm->msg = (z_const char *)"invalid literal/length code";
            state->mode = BAD;
            break;
        }
    } while (in < last && out < end);

#ifdef Z_U8
  leave:
#endif
    /* return unused bytes (on entry, bits < 8, so in won't go too far back) */
    len = bits >> 3;
    in -= len;
    bits -= len << 3;
    hold &= (1U << bits) - 1;

    /* update state and return */
    strm->next_in = in;
    strm->next_out = out;
    strm->avail_in = (unsigned)(in < last ? 5 + (last - in) : 5 - (in - last));
    strm->avail_out = (unsigned)(out < end ?
                                 257 + (end - out) : 257 - (out - end));
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

#endif /* !ASMINF */
