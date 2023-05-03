/* inffast.c -- fast decoding
 * Copyright (C) 1995-2017 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#include "zutil.h"
#include "inftrees.h"
#include "inflate.h"
#include "inffast.h"
#include <stdio.h> 

#define DEBUG 0

#ifdef ASMINF
#  pragma message("Assembler code may have bugs -- use at your own risk")
#else

 /*
    Decode literal, length, and distance codes and write out the resulting
    literal and match bytes until either not enough input or output is
    available, an end-of-block is encountered, or a data error is encountered.
    When large enough input and output buffers are supplied to inflate(), for
    example, a 16K input buffer and a 64K output buffer, more than 95% of the
    inflate execution time is spent in this routine.

    Entry assumptions:

         state->mode == LEN
         strm->avail_in >= 8
         strm->avail_out >= 8
         start >= strm->avail_out
         state->usedBits < 8

    On return, state->mode is one of:

         LEN -- ran out of enough output space or enough available input
         TYPE -- reached end of block code, inflate() to interpret next block
         BAD -- error in block data

    Notes:

     - The maximum input usedBits used by a length/distance pair is 15 usedBits for the
       length code, 5 usedBits for the length extra, 15 usedBits for the distance code,
       and 13 usedBits for the distance extra.  This totals 48 usedBits, or six bytes.
       Therefore if strm->avail_in >= 6, then there is enough input to avoid
       checking for available input while decoding.

     - The maximum bytes that a single length/distance pair can output is 258
       bytes, which is the maximum length that can be coded.  inflate_fast()
       requires strm->avail_out >= 258 for each loop to avoid checking for
       output space.
  */

inline void MemWildCopy_Overlap(void* dst, const void* src, void* const dstEnd)
{
    const unsigned char* ip = (const unsigned char*)src;
    unsigned char* op = (unsigned char*)dst;
    unsigned char* const oend = (unsigned char*)dstEnd;
    do {
        memcpy(op, ip, 8);
        op += 8;
        ip += 8;
    } while (op < oend);
}

static unsigned MEM_IsLittleEndian(void)
{
    const union { unsigned int u; unsigned char c[4]; } one = { 1 };   /* don't use static : performance detrimental  */
    return one.c[0];
}

static unsigned int MemSwap4(unsigned int in)
{
#if defined(_MSC_VER)     /* Visual Studio */
    return _byteswap_ulong(in);
#elif (defined (__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ >= 403)) \
  || (defined(__clang__) && __has_builtin(__builtin_bswap32))
    return __builtin_bswap32(in);
#else
    return  ((in << 24) & 0xff000000) |
        ((in << 8) & 0x00ff0000) |
        ((in >> 8) & 0x0000ff00) |
        ((in >> 24) & 0x000000ff);
#endif
}

static size_t MemSwap8(size_t in)
{
#if defined(_MSC_VER)     /* Visual Studio */
    return _byteswap_uint64(in);
#elif (defined (__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ >= 403)) \
  || (defined(__clang__) && __has_builtin(__builtin_bswap64))
    return __builtin_bswap64(in);
#else
    return  ((in << 56) & 0xff00000000000000ULL) |
        ((in << 40) & 0x00ff000000000000ULL) |
        ((in << 24) & 0x0000ff0000000000ULL) |
        ((in << 8) & 0x000000ff00000000ULL) |
        ((in >> 8) & 0x00000000ff000000ULL) |
        ((in >> 24) & 0x0000000000ff0000ULL) |
        ((in >> 40) & 0x000000000000ff00ULL) |
        ((in >> 56) & 0x00000000000000ffULL);
#endif
}

/* Fast read "size_t val" in the Little Endian order */
static size_t MemReadLEST(const void* memPtr)
{
    size_t val;
    memcpy(&val, memPtr, sizeof(size_t));

    if (MEM_IsLittleEndian()) {
        return val;
    }
    else {
        if (sizeof(size_t) == 4)
            return (size_t)MemSwap4(val);
        else
            return (size_t)MemSwap8(val);
    }
}

#define HUFF_DECODE_LEN()    {                         \
    op = (hold >> usedBits) & ((1 << LEN_ROOT) - 1);   \
    here = lenDec +op;                                 \
    if (unlikely( ((op+1) & lenMask) == 0 && here->allBits==0 )) {                          \
        here = lenDec2 + (here->val + ((hold >> (usedBits + LEN_ROOT)) & here->hufBits));   \
    } /* note .hufBits is repurposed as mask */                                             \
}

#define HUFF_DECODE_DIST()  {                          \
   op = (hold >> usedBits) & ((1 << DIST_ROOT) - 1);   \
   here = distDec +op;                                 \
   if (unlikely(isDistDec2 && ((op+1) & distMask) == 0 && here->allBits==0)) {              \
      here = distDec2 + (here->val + ((hold >> (usedBits + DIST_ROOT)) & here->hufBits));   \
   }  /* note ->hufBits is repurposed as mask */                                            \
   dist = here->val + (((hold >> usedBits) & ((1 << here->allBits) - 1)) >> here->hufBits); \
}

/* Optimized LZ copy operation 
 * 1. fast overlapped memcpy-8-byte 
 * 2. memset for single-run
 * 3. initiated with 2-byte copy among distances 2-7
 */
#define FAST_LZ_COPY() {                                               \
if (likely(dist >= 8)) { MemWildCopy_Overlap(out, from, outStop); }    \
else if (dist == 1) { memset(out, *(out - 1), len); }                  \
else {                                                                 \
    outPtr = (unsigned short*)out;                                     \
    fromPtr = (unsigned short*)from;                                   \
    *outPtr++ = *fromPtr++;                                            \
    *outPtr++ = *fromPtr++;                                            \
    *outPtr++ = *fromPtr++;                                            \
    *outPtr = *fromPtr;                                                \
    if (unlikely(len > 8))                                             \
        MemWildCopy_Overlap(out + 8, from + inc8table[dist], outStop); \
}                                                                      \
out = outStop;                                                         \
if (DEBUG) {                                                           \
  fprintf(fptr, "%d: (%d, %d)-", (int)(outStop - 1 - strm->next_out), len, dist);         \
  for (unsigned i = 0; i < len; i++) fprintf(fptr, "%c", *(char*)(outStop - len + i));    \
  fprintf(fptr, "\n");  fflush(fptr);                                                     \
}                                                                                         \
}

void ZLIB_INTERNAL inflate_fast(strm, start)
z_streamp strm;
unsigned start;         /* inflate()'s starting value for strm->avail_out */
{
    z_const unsigned char FAR* in;      /* local strm->next_in */
    unsigned char FAR* out;     /* local strm->next_out */
    unsigned char FAR* outStop;     /* local strm->next_out */
    unsigned char FAR* beg;     /* inflate()'s initial strm->next_out */
#ifdef INFLATE_STRICT
    unsigned dmax;              /* maximum distance from zlib header */
#endif
    unsigned winSize;           /* window size or zero if not using window */
    unsigned whave;             /* valid bytes in the window */
    unsigned wnext;             /* window write index */
    unsigned char FAR* window;  /* allocated sliding window, if wsize != 0 */
    register size_t hold;       /* local strm->hold */
    register unsigned usedBits;              /* local strm->usedBits */
    DecodeStr* here;            /* retrieved table entry */
    register unsigned op;       /* code usedBits, operation, extra usedBits, or */
                                /*  window position, window bytes to copy */
    unsigned len;               /* match length, unused bytes */
    unsigned dist;              /* match distance */

    unsigned char FAR* from;    /* where to copy match from */
    unsigned short FAR* outPtr, * fromPtr;
    FILE* fptr;
    if (DEBUG) {
        fptr = fopen("dec_trace.txt", "w");
    }

    /* copy state to local variables */
    struct inflate_state FAR* const state = (struct inflate_state FAR*)strm->state;
    const DecodeStr FAR* const lenDec = state->lenDec;
    const DecodeStr FAR* const distDec = state->distDec;
    const unsigned lenMask = state->lenMask;
    const unsigned distMask = state->distMask;
    const unsigned isLenDec2 = (lenMask < (1 << LEN_ROOT));
    const unsigned isDistDec2 = (distMask < (1 << DIST_ROOT));
    const DecodeStr FAR* const lenDec2 = isLenDec2 ? state->lenDec2 : NULL;
    const DecodeStr FAR* const distDec2 = isDistDec2 ? state->distDec2 : NULL;

    in = strm->next_in;
    out = strm->next_out;
    beg = out - (start - strm->avail_out);
    winSize = state->wsize;
    whave = state->whave;
    wnext = state->wnext;
    window = state->window;

    /* If window is continuous toward output stream, then treat as expanded output stream */
    if ( beg == window + winSize && wnext==0 && whave == winSize) {   
        whave = 0;
        winSize = 0;
        beg = window;
    }

    const unsigned char FAR* const last = in + (strm->avail_in - 8);        /* have enough input while in < last */
    const unsigned char FAR* const litEnd = out + strm->avail_out;         /* literal decoding ending position */
    const unsigned char FAR* const copyEnd = out + strm->avail_out - 8;     /* LZ copy decoding ending position */

#ifdef INFLATE_STRICT
    dmax = state->dmax;
#endif

    static const int inc8table[8] = { 0, 1, 0,  2,  0,  3, 2, 1 };

    in -= (state->bits + 7) >> 3;
    usedBits = (8 - (state->bits & 7)) & 7;
    hold = MemReadLEST(in);

    /* decode literals and length/distances until beyond dictionary window or not enough input data or output space */
    while (1) {
        HUFF_DECODE_LEN();

        while ((here->val & 0xFF00) == 0x8000) {
            /* while -- an uncompressed litereral is likely followed by another literal */
            if (unlikely(out >= litEnd)) goto WrapUp;
            *out++ = (unsigned char)(here->val);
            if (DEBUG) {
                fprintf(fptr, "%d: %c\n", (int)(out - 1 - strm->next_out), *(char*)(out - 1));  fflush(fptr);
            }

            usedBits += here->allBits;
            if (usedBits > sizeof(state->hold) * 8 - MAX_HUFBITS) {  
                in += usedBits >> 3;    /* update the input stream pointer */
                usedBits &= 7;
                if (unlikely(in > last)) {
                    goto WrapUp;
                }
                hold = MemReadLEST(in);
            }
            HUFF_DECODE_LEN();
        }

        if (likely((here->val & 0x8000) == 0)) {     /* LZ copy */
            in += usedBits >> 3;    /* update the input stream pointer */
            usedBits &= 7;
            if (unlikely(in > last)) {   /* insufficient data, reverse update*/
                break;
            }
            hold = MemReadLEST(in);

            len = here->val + (((hold >> usedBits) & ((1 << here->allBits) - 1)) >> here->hufBits);
            outStop = out + len;
            if (unlikely(outStop > copyEnd)) break;
            usedBits += here->allBits;

            if (sizeof(size_t) == 4) {
                in += usedBits >> 3;    /* update the input stream pointer */
                usedBits &= 7;
                hold = MemReadLEST(in);
            }
            HUFF_DECODE_DIST();
            usedBits += here->allBits;

#ifdef INFLATE_STRICT
            if (dist > dmax) {
                strm->msg = (char*)"invalid distance too far back";
                state->mode = BAD;
                break;
            }
#endif

            if (winSize && dist > (unsigned)(out - beg)) {                /* see if copy from window */
                op = dist - (out - beg);             /* distance back in window */
                if (unlikely(op > whave)) {
                    if (state->sane) {
                        strm->msg =
                            (char*)"invalid distance too far back";
                        state->mode = BAD;
                        break;
                    }
#ifdef INFLATE_ALLOW_INVALID_DISTANCE_TOOFAR_ARRR
                    if (len <= op - whave) {
                        memset(out, 0, len);
                        out += len;
                        continue;
                    }
                    len -= op - whave;
                    memset(out, 0, op - whave);
                    out += op - whave;

                    if (op == 0) {
                        from = out - dist;
                        memcpy(out, from, len);
                        out += len;
                        continue;
                    }
#endif
                }
                /* We increase the margin of 7B to enable MemWildCopy on the remaining match */
                from = window;
                if (wnext == 0) {           /* very common case */
                    from += winSize - op;
                    if (unlikely(op < len + 7)) {         /* some from window */
                        op = min(op, len);
                        len -= op;
                        memcpy(out, from, op);
                        out += op;
                        from = out - dist;  /* rest from output */
                    }
                }
                else if (wnext < op) {      /* wrap around window */
                    from += winSize + wnext - op;
                    op -= wnext;
                    if (unlikely(op < len + 7)) {         /* some from end of window */
                        op = min(op, len);
                        len -= op;
                        memcpy(out, from, op);
                        out += op;
                        from = window;
                        if (unlikely(wnext < len + 7)) {  /* some from start of window */
                            op = min(wnext, len);
                            len -= op;
                            memcpy(out, from, op);
                            out += op;
                            from = out - dist;      /* rest from output */
                        }
                    }
                }
                else {                      /* contiguous in window */
                    from += wnext - op;
                    if (unlikely(op < len + 7)) {         /* some from window */
                        op = min(op, len);
                        len -= op;
                        memcpy(out, from, op);
                        out += op;
                        from = out - dist;  /* rest from output */
                    }
                }
            }
            else {
                from = out - dist;          /* copy direct from output */
            }

            FAST_LZ_COPY();
        }
        else {
            if (here->val == 0xFFFF) { /* end-of-block */
                state->mode = TYPE;
            }
            else {
                strm->msg = (char*)"invalid literal/length code";
                state->mode = BAD;
            }
            usedBits += here->allBits;
            break;
        }
    }
    if (DEBUG) { fclose(fptr); }

WrapUp:
    /* return unused bytes (on entry, usedBits < 8, so in won't go too far back) */
    in += usedBits >> 3;    /* update the input stream pointer */
    usedBits &= 7;
    if (usedBits) {
        state->bits = (8 - usedBits) & 7;
        state->hold = (*in++) >> usedBits;
    }
    else {
        state->bits = 0;
        state->hold = 0;
    }

    /* update state and return */
    strm->next_in = in;
    strm->next_out = out;
    strm->avail_in = (unsigned)(8 + last - in);
    strm->avail_out = (unsigned)(litEnd - out);
}

#endif /* !ASMINF */
