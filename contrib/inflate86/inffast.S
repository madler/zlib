/*
 * inffast.S is a hand tuned assembler version of:
 *
 * inffast.c -- fast decoding
 * Copyright (C) 1995-2003 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h 
 *
 * Copyright (C) 2003 Chris Anderson <christop@charm.net>
 * Please use the copyright conditions above.
 * 
 * This version (Jan-23-2003) of inflate_fast was coded and tested under
 * GNU/Linux on a pentium 3, using the gcc-3.2 compiler distribution.  On that
 * machine, I found that gzip style archives decompressed about 20% faster than
 * the gcc-3.2 -O3 -fomit-frame-pointer compiled version.  Your results will
 * depend on how large of a buffer is used for z_stream.next_in & next_out
 * (8K-32K worked best for my 256K cpu cache) and how much overhead there is in
 * stream processing I/O and crc32/addler32.  In my case, this routine used
 * 70% of the cpu time and crc32 used 20%.
 *
 * I am confident that this version will work in the general case, but I have
 * not tested a wide variety of datasets or a wide variety of platforms.
 *
 * Jan-24-2003 -- Added -DUSE_MMX define for slightly faster inflating.
 * It should be a runtime flag instead of compile time flag...
 */

.file "inffast.S"

.globl inflate_fast

.text
.align 4,0
.L_invalid_literal_length_code_msg:
.string "invalid literal/length code"

.align 4,0
.L_invalid_distance_code_msg:
.string "invalid distance code"

.align 4,0
.L_invalid_distance_too_far_msg:
.string "invalid distance too far back"

#if defined( USE_MMX )
.align 4,0
.L_mask: /* mask[N] = ( 1 << N ) - 1 */
.long 0
.long 1
.long 3
.long 7
.long 15
.long 31
.long 63
.long 127
.long 255
.long 511
.long 1023
.long 2047
.long 4095
.long 8191
.long 16383
.long 32767
.long 65535
.long 131071
.long 262143
.long 524287
.long 1048575
.long 2097151
.long 4194303
.long 8388607
.long 16777215
.long 33554431
.long 67108863
.long 134217727
.long 268435455
.long 536870911
.long 1073741823
.long 2147483647
.long 4294967295
#endif

.text

/*
 * struct z_stream offsets, in zlib.h
 */
#define next_in_strm   0   /* strm->next_in */
#define avail_in_strm  4   /* strm->avail_in */
#define next_out_strm  12  /* strm->next_out */
#define avail_out_strm 16  /* strm->avail_out */
#define msg_strm       24  /* strm->msg */
#define state_strm     28  /* strm->state */

/*
 * struct inflate_state offsets, in inflate.h
 */
#define mode_state     0   /* state->mode */
#define wsize_state    32  /* state->wsize */
#define write_state    36  /* state->write */
#define window_state   40  /* state->window */
#define hold_state     44  /* state->hold */
#define bits_state     48  /* state->bits */
#define lencode_state  64  /* state->lencode */
#define distcode_state 68  /* state->distcode */
#define lenbits_state  72  /* state->lenbits */
#define distbits_state 76  /* state->distbits */

/*
 * inflate_fast's activation record
 */
#define local_var_size 56 /* how much local space for vars */
#define strm_sp        80 /* first arg: z_stream * (local_var_size + 24) */
#define start_sp       84 /* second arg: unsigned int (local_var_size + 28) */

/*
 * offsets for local vars on stack
 */
#define out            52  /* unsigned char* */
#define window         48  /* unsigned char* */
#define wsize          44  /* unsigned int */
#define write          40  /* unsigned int */
#define in             36  /* unsigned char* */
#define beg            32  /* unsigned char* */
#define dist           28  /* unsigned int */
#define len            24  /* unsigned int */
#define last           20  /* unsigned char* */
#define end            16  /* unsigned char* */
#define dcode          12  /* code* */
#define lcode           8  /* code* */
#define dmask           4  /* unsigned int */
#define lmask           0  /* unsigned int */

/*
 * typedef enum inflate_mode consts, in inflate.h
 */
#ifndef NO_GUNZIP
#define GUNZIP
#endif

#ifdef GUNZIP
#define INFLATE_MODE_TYPE 11  /* state->mode flags enum-ed in inflate.h */
#define INFLATE_MODE_BAD  26
#else
#define INFLATE_MODE_TYPE 3
#define INFLATE_MODE_BAD  17
#endif


.align 16,0x90
inflate_fast:
        pushl   %edi
        pushl   %esi
        pushl   %ebp
        pushl   %ebx
        pushf   /* save eflags (strm_sp, state_sp assumes this is 32 bits) */
        subl    $local_var_size, %esp
        cld
#if defined( USE_MMX )
        emms
#endif

#define strm_r  %esi
#define state_r %edi

        movl    strm_sp(%esp), strm_r
        movl    state_strm(strm_r), state_r

        /* in = strm->next_in;
         * out = strm->next_out;
         * last = in + strm->avail_in - 5;
         * beg = out - (start - strm->avail_out);
         * end = out + (strm->avail_out - 257);
         */
        movl    next_in_strm(strm_r), %eax
        movl    next_out_strm(strm_r), %ebx
        movl    avail_in_strm(strm_r), %edx
        movl    avail_out_strm(strm_r), %ecx
        movl    start_sp(%esp), %ebp

        addl    %eax, %edx      /* avail_in += next_in */
        subl    $5, %edx        /* avail_in -= 5 */

        subl    %ecx, %ebp      /* start -= avail_out */
        negl    %ebp            /* start = -start */
        addl    %ebx, %ebp      /* start += next_out */

        subl    $257, %ecx      /* avail_out -= 257 */
        addl    %ebx, %ecx      /* avail_out += out */

        movl    %eax, in(%esp)
        movl    %ebx, out(%esp)
        movl    %edx, last(%esp)
        movl    %ebp, beg(%esp)
        movl    %ecx, end(%esp)

        /* wsize = state->wsize;
         * write = state->write;
         * window = state->window;
         * hold = state->hold;
         * bits = state->bits;
         * lcode = state->lencode;
         * dcode = state->distcode;
         * lmask = ( 1 << state->lenbits ) - 1;
         * dmask = ( 1 << state->distbits ) - 1;
         */

        movl    lencode_state(state_r), %eax
        movl    distcode_state(state_r), %ecx

        movl    %eax, lcode(%esp)
        movl    %ecx, dcode(%esp)

        movl    $1, %eax
        movl    lenbits_state(state_r), %ecx
        shll    %cl, %eax
        decl    %eax
        movl    %eax, lmask(%esp)

        movl    $1, %eax
        movl    distbits_state(state_r), %ecx
        shll    %cl, %eax
        decl    %eax
        movl    %eax, dmask(%esp)

        movl    wsize_state(state_r), %eax
        movl    write_state(state_r), %ecx
        movl    window_state(state_r), %edx

        movl    %eax, wsize(%esp)
        movl    %ecx, write(%esp)
        movl    %edx, window(%esp)

#if ! defined( USE_MMX )

#define hold_r     %ebp
#define bits_r     %bl
#define bitslong_r %ebx

        movl    hold_state(state_r), hold_r
        movl    bits_state(state_r), bitslong_r

#else /* USE_MMX */

#define hold_mm    %mm0
#define bits_r     %ebp
#define bitslong_r %ebp

        movl    hold_state(state_r), %ebx
        movl    bits_state(state_r), bitslong_r

#endif

#undef strm_r
#undef state_r
#define in_r       %esi
#define from_r     %esi
#define out_r      %edi

        movl    in(%esp), in_r

#if ! defined ( USE_MMX )

        /* align in_r on word boundary */
        testl   $1, in_r
        jz      .L_is_aligned
        xorl    %eax, %eax
        movb    (in_r), %al
        incl    in_r
        movb    bits_r, %cl
        addb    $8, bits_r
        shll    %cl, %eax
        orl     %eax, hold_r

#else
        /* align in_r on long boundary */
.L_align_long:
        testl   $3, in_r
        jz      .L_is_aligned
        xorl    %eax, %eax
        movb    (in_r), %al
        incl    in_r
        movl    bits_r, %ecx
        addl    $8, bits_r
        shll    %cl, %eax
        orl     %eax, %ebx
        jmp     .L_align_long

#endif

.L_is_aligned:
        movl    out(%esp), out_r

#if defined ( USE_MMX )

#define used_mm   %mm1
#define dmask2_mm %mm2
#define lmask2_mm %mm3
#define lmask_mm  %mm4
#define dmask_mm  %mm5
#define tmp_mm    %mm6

        movl    out(%esp), out_r
        movd    lmask(%esp), lmask_mm 
        movq    lmask_mm, lmask2_mm
        movd    dmask(%esp), dmask_mm
        movq    dmask_mm, dmask2_mm
        movd    %ebx, hold_mm
        pxor    used_mm, used_mm
        movl    lcode(%esp), %ebx       /* ebx = lcode */
#endif

        jmp     .L_do_loop

.align 16,0x90

#if ! defined ( USE_MMX )

.L_do_loop:
        /* regs: %esi = in, %ebp = hold, %bl = bits, %edi = out
         *
         * do { 
         *   if (bits < 15) {
         *     hold |= *((unsigned short *)in)++ << bits;
         *     bits += 16
         *   }
         *   this = lcode[hold & lmask]
         */
        cmpb    $15, bits_r
        ja      .L_get_length_code      /* if (15 < bits) */

        xorl    %eax, %eax
        lodsw                           /* al = *(ushort *)in++ */
        movb    bits_r, %cl             /* cl = bits, needs it for shifting */
        addb    $16, bits_r             /* bits += 16 */
        shll    %cl, %eax
        orl     %eax, hold_r            /* hold |= *((ushort *)in)++ << bits */

.L_get_length_code:
        movl    lmask(%esp), %edx       /* edx = lmask */
        movl    lcode(%esp), %ecx       /* ecx = lcode */
        andl    hold_r, %edx            /* edx &= hold */
        movl    (%ecx,%edx,4), %eax     /* eax = lcode[hold & lmask] */

#else /* USE_MMX */

.L_do_loop:
        psrlq   used_mm, hold_mm        /* hold_mm >>= last bit length */

        cmpl    $32, bits_r
        ja      .L_get_length_code      /* if (32 < bits) */

        movd    bits_r, tmp_mm
        movd    (in_r), %mm7
        addl    $4, in_r
        psllq   tmp_mm, %mm7
        addl    $32, bits_r
        por     %mm7, hold_mm           /* hold_mm |= *((uint *)in)++ << bits */

.L_get_length_code:
        pand    hold_mm, lmask_mm
        movd    lmask_mm, %eax
        movq    lmask2_mm, lmask_mm
        movl    (%ebx,%eax,4), %eax     /* eax = lcode[hold & lmask] */

#endif

#if ! defined( USE_MMX )

.L_dolen:
        /* regs: %esi = in, %ebp = hold, %bl = bits, %edi = out
         *
         * dolen:
         *    bits -= this.bits;
         *    hold >>= this.bits
         */
        movb    %ah, %cl                /* cl = this.bits */
        subb    %ah, bits_r             /* bits -= this.bits */
        shrl    %cl, hold_r             /* hold >>= this.bits */

        /* check if op is a literal
         * if (op == 0) {
         *    PUP(out) = this.val;
         *  }
         */
        testb   %al, %al
        jnz     .L_test_for_length_base /* if (op != 0) 45.7% */

        shrl    $16, %eax               /* output this.val char */
        stosb

#else /* USE_MMX */

#define len_r  %edx

.L_dolen:
        movzbl  %ah, %ecx               /* ecx = this.bits */
        movl    %eax, len_r             /* len = this */
        shrl    $16, len_r              /* len = this.val */
        movd    %ecx, used_mm
        subl    %ecx, bits_r            /* bits -= this.bits */

        testb   %al, %al
        jnz     .L_test_for_length_base /* if (op != 0) 45.7% */

        movb    %dl, (out_r)
        incl    out_r

#endif

.L_while_test:
        /* while (in < last && out < end)
         */
        cmpl    out_r, end(%esp)
        jbe     .L_break_loop           /* if (out >= end) */

        cmpl    in_r, last(%esp)
        ja      .L_do_loop              /* if (in < last) */
        jmp     .L_break_loop

#if ! defined( USE_MMX )

.L_test_for_length_base:
        /* regs: %esi = in, %ebp = hold, %bl = bits, %edi = out, %edx = len
         *
         * else if (op & 16) {
         *   len = this.val
         *   op &= 15
         *   if (op) {
         *     if (op > bits) {
         *       hold |= *((unsigned short *)in)++ << bits;
         *       bits += 16
         *     }
         *     len += hold & mask[op];
         *     bits -= op;
         *     hold >>= op;
         *   }
         */
#define len_r %edx
        movl    %eax, len_r             /* len = this */
        shrl    $16, len_r              /* len = this.val */
        movb    %al, %cl

        testb   $16, %al
        jz      .L_test_for_second_level_length /* if ((op & 16) == 0) 8% */
        andb    $15, %cl                /* op &= 15 */
        jz      .L_save_len             /* if (!op) */
        cmpb    %cl, bits_r
        jae     .L_add_bits_to_len      /* if (op <= bits) */

        movb    %cl, %ch                /* stash op in ch, freeing cl */
        xorl    %eax, %eax
        lodsw                           /* al = *(ushort *)in++ */
        movb    bits_r, %cl             /* cl = bits, needs it for shifting */
        addb    $16, bits_r             /* bits += 16 */
        shll    %cl, %eax
        orl     %eax, hold_r            /* hold |= *((ushort *)in)++ << bits */
        movb    %ch, %cl                /* move op back to ecx */

.L_add_bits_to_len:
        movl    $1, %eax
        shll    %cl, %eax
        decl    %eax
        subb    %cl, bits_r
        andl    hold_r, %eax            /* eax &= hold */
        shrl    %cl, hold_r
        addl    %eax, len_r             /* len += hold & mask[op] */

.L_save_len:
        movl    len_r, len(%esp)        /* save len */
#undef  len_r

.L_decode_distance:
        /* regs: %esi = in, %ebp = hold, %bl = bits, %edi = out, %edx = dist
         *
         *   if (bits < 15) {
         *     hold |= *((unsigned short *)in)++ << bits;
         *     bits += 16
         *   }
         *   this = dcode[hold & dmask];
         * dodist:
         *   bits -= this.bits;
         *   hold >>= this.bits;
         *   op = this.op;
         */

        cmpb    $15, bits_r
        ja      .L_get_distance_code    /* if (15 < bits) */

        xorl    %eax, %eax
        lodsw                           /* al = *(ushort *)in++ */
        movb    bits_r, %cl             /* cl = bits, needs it for shifting */
        addb    $16, bits_r             /* bits += 16 */
        shll    %cl, %eax
        orl     %eax, hold_r            /* hold |= *((ushort *)in)++ << bits */

.L_get_distance_code:
        movl    dmask(%esp), %edx       /* edx = dmask */
        movl    dcode(%esp), %ecx       /* ecx = dcode */
        andl    hold_r, %edx            /* edx &= hold */
        movl    (%ecx,%edx,4), %eax     /* eax = dcode[hold & dmask] */

#else /* USE_MMX */

.L_test_for_length_base:
        testb   $16, %al
        jz      .L_test_for_second_level_length /* if ((op & 16) == 0) 8% */
        andl    $15, %eax               /* op &= 15 */
        jz      .L_decode_distance      /* if (!op) */

        psrlq   used_mm, hold_mm        /* hold_mm >>= last bit length */
        movd    %eax, used_mm
        movd    hold_mm, %ecx
        subl    %eax, bits_r
        andl    .L_mask(,%eax,4), %ecx
        addl    %ecx, len_r             /* len += hold & mask[op] */

.L_decode_distance:

        psrlq   used_mm, hold_mm        /* hold_mm >>= last bit length */

        cmpl    $32, bits_r
        ja      .L_get_dist_code        /* if (32 < bits) */

        movd    bits_r, tmp_mm
        movd    (in_r), %mm7
        addl    $4, in_r
        psllq   tmp_mm, %mm7
        addl    $32, bits_r
        por     %mm7, hold_mm           /* hold_mm |= *((uint *)in)++ << bits */

.L_get_dist_code:
        movl    dcode(%esp), %ebx       /* ebx = dcode */
        pand    hold_mm, dmask_mm
        movd    dmask_mm, %eax
        movq    dmask2_mm, dmask_mm
        movl    (%ebx,%eax,4), %eax     /* eax = dcode[hold & lmask] */

#endif

#if ! defined( USE_MMX )

#define dist_r %edx
.L_dodist:
        movl    %eax, dist_r            /* dist = this */
        shrl    $16, dist_r             /* dist = this.val */
        movb    %ah, %cl
        subb    %ah, bits_r             /* bits -= this.bits */
        shrl    %cl, hold_r             /* hold >>= this.bits */

        /* if (op & 16) {
         *   dist = this.val
         *   op &= 15
         *   if (op > bits) {
         *     hold |= *((unsigned short *)in)++ << bits;
         *     bits += 16
         *   }
         *   dist += hold & mask[op];
         *   bits -= op;
         *   hold >>= op;
         */
        movb    %al, %cl                /* cl = this.op */

        testb   $16, %al                /* if ((op & 16) == 0) */
        jz      .L_test_for_second_level_dist
        andb    $15, %cl                /* op &= 15 */
        jz      .L_check_dist_one
        cmpb    %cl, bits_r
        jae     .L_add_bits_to_dist     /* if (op <= bits) 97.6% */

        movb    %cl, %ch                /* stash op in ch, freeing cl */
        xorl    %eax, %eax
        lodsw                           /* al = *(ushort *)in++ */
        movb    bits_r, %cl             /* cl = bits, needs it for shifting */
        addb    $16, bits_r             /* bits += 16 */
        shll    %cl, %eax
        orl     %eax, hold_r            /* hold |= *((ushort *)in)++ << bits */
        movb    %ch, %cl                /* move op back to ecx */

.L_add_bits_to_dist:
        movl    $1, %eax
        shll    %cl, %eax
        decl    %eax                    /* (1 << op) - 1 */
        subb    %cl, bits_r
        andl    hold_r, %eax            /* eax &= hold */
        shrl    %cl, hold_r
        addl    %eax, dist_r            /* dist += hold & ((1 << op) - 1) */
        jmp     .L_check_window

#else /* USE_MMX */

#define dist_r %ebx
.L_dodist:
        movzbl  %ah, %ecx               /* ecx = this.bits */
        movl    %eax, dist_r
        shrl    $16, dist_r             /* dist  = this.val */
        subl    %ecx, bits_r            /* bits -= this.bits */
        movd    %ecx, used_mm

        testb   $16, %al                /* if ((op & 16) == 0) */
        jz      .L_test_for_second_level_dist 
        andl    $15, %eax               /* op &= 15 */
        jz      .L_check_dist_one

.L_add_bits_to_dist:
        psrlq   used_mm, hold_mm        /* hold_mm >>= last bit length */
        movd    %eax, used_mm           /* save bit length of current op */
        movd    hold_mm, %ecx           /* get the next bits on input stream */
        subl    %eax, bits_r            /* bits -= op bits */
        andl    .L_mask(,%eax,4), %ecx  /* ecx   = hold & mask[op] */
        addl    %ecx, dist_r            /* dist += hold & mask[op] */
        jmp     .L_check_window

#endif

.align 16,0x90

.L_check_dist_one:
        cmpl    $1, dist_r
        jne     .L_check_window
        cmpl    out_r, beg(%esp)
        je      .L_check_window

        decl    out_r
#if ! defined( USE_MMX )
        movl    len(%esp), %ecx
#else
        movl    len_r, %ecx
#endif
        movb    (out_r), %al
        subl    $3, %ecx

        movb    %al, 1(out_r)
        movb    %al, 2(out_r)
        movb    %al, 3(out_r)
        addl    $4, out_r
        rep     stosb

#if defined( USE_MMX )
        movl    lcode(%esp), %ebx       /* move lcode back to %ebx, toss dist */
#endif
        jmp     .L_while_test

.align 16,0x90

.L_check_window:
        /* regs: %esi = from, %ebp = hold, %bl = bits, %edi = out, %edx = dist
         *       %ecx = nbytes
         *
         * nbytes = out - beg;
         * if (dist <= nbytes) {
         *   from = out - dist;
         *   do {
         *     PUP(out) = PUP(from);
         *   } while (--len > 0) {
         * }
         */

        movl    in_r, in(%esp)          /* save in so from can use it's reg */
        movl    out_r, %eax
        subl    beg(%esp), %eax         /* nbytes = out - beg */

        cmpl    dist_r, %eax
        jb      .L_clip_window          /* if (dist > nbytes) 4.2% */

#if ! defined( USE_MMX )
        movl    len(%esp), %ecx
#else
        movl    len_r, %ecx
#endif
        movl    out_r, from_r
        subl    dist_r, from_r          /* from = out - dist */

        subl    $3, %ecx
        movb    (from_r), %al
        movb    %al, (out_r)
        movb    1(from_r), %al
        movb    2(from_r), %dl
        addl    $3, from_r
        movb    %al, 1(out_r)
        movb    %dl, 2(out_r)
        addl    $3, out_r
        rep     movsb

        movl    in(%esp), in_r          /* move in back to %esi, toss from */
#if defined( USE_MMX )
        movl    lcode(%esp), %ebx       /* move lcode back to %ebx, toss dist */
#endif
        jmp     .L_while_test

.align 16,0x90

#if ! defined( USE_MMX )

.L_test_for_second_level_length:
        /* else if ((op & 64) == 0) {
         *   this = lcode[this.val + (hold & mask[op])];
         * }
         */
        testb   $64, %al
        jnz     .L_test_for_end_of_block  /* if ((op & 64) != 0) */

        movl    $1, %eax
        shll    %cl, %eax
        decl    %eax
        andl    hold_r, %eax            /* eax &= hold */
        addl    %edx, %eax              /* eax += this.val */
        movl    lcode(%esp), %edx       /* edx = lcode */
        movl    (%edx,%eax,4), %eax     /* eax = lcode[val + (hold&mask[op])] */
        jmp     .L_dolen

#else /* USE_MMX */

.L_test_for_second_level_length: 
        testb   $64, %al
        jnz     .L_test_for_end_of_block  /* if ((op & 64) != 0) */

        andl    $15, %eax
        psrlq   used_mm, hold_mm        /* hold_mm >>= last bit length */
        movd    hold_mm, %ecx
        andl    .L_mask(,%eax,4), %ecx
        addl    len_r, %ecx
        movl    (%ebx,%ecx,4), %eax     /* eax = lcode[hold & lmask] */
        jmp     .L_dolen

#endif

.align 16,0x90

#if ! defined( USE_MMX )

.L_test_for_second_level_dist:
        /* else if ((op & 64) == 0) {
         *   this = dcode[this.val + (hold & mask[op])];
         * }
         */
        testb   $64, %al
        jnz     .L_invalid_distance_code  /* if ((op & 64) != 0) */

        movl    $1, %eax
        shll    %cl, %eax
        decl    %eax
        andl    hold_r, %eax            /* eax &= hold */
        addl    %edx, %eax              /* eax += this.val */
        movl    dcode(%esp), %edx       /* edx = dcode */
        movl    (%edx,%eax,4), %eax     /* eax = dcode[val + (hold&mask[op])] */
        jmp     .L_dodist

#else /* USE_MMX */

.L_test_for_second_level_dist:
        testb   $64, %al
        jnz     .L_invalid_distance_code  /* if ((op & 64) != 0) */

        andl    $15, %eax
        psrlq   used_mm, hold_mm        /* hold_mm >>= last bit length */
        movd    hold_mm, %ecx
        andl    .L_mask(,%eax,4), %ecx
        movl    dcode(%esp), %eax       /* ecx = dcode */
        addl    dist_r, %ecx
        movl    (%eax,%ecx,4), %eax     /* eax = lcode[hold & lmask] */
        jmp     .L_dodist

#endif

.align 16,0x90
.L_clip_window:
        /* regs: %esi = from, %ebp = hold, %bl = bits, %edi = out, %edx = dist
         *       %ecx = nbytes
         *
         * else {
         *   if (dist > wsize) {
         *     invalid distance
         *   }
         *   from = window;
         *   nbytes = dist - nbytes;
         *   if (write == 0) {
         *     from += wsize - nbytes;
         */
#define nbytes_r %ecx

        movl    %eax, nbytes_r
        movl    wsize(%esp), %eax       /* prepare for dist compare */
        negl    nbytes_r                /* nbytes = -nbytes */
        movl    window(%esp), from_r    /* from = window */

        cmpl    dist_r, %eax
        jb      .L_invalid_distance_too_far /* if (dist > wsize) */

        addl    dist_r, nbytes_r        /* nbytes = dist - nbytes */
        cmpl    $0, write(%esp)
        jne     .L_wrap_around_window   /* if (write != 0) */

        subl    nbytes_r, %eax
        addl    %eax, from_r            /* from += wsize - nbytes */

        /* regs: %esi = from, %ebp = hold, %bl = bits, %edi = out, %edx = dist
         *       %ecx = nbytes, %eax = len
         *
         *     if (nbytes < len) {
         *       len -= nbytes;
         *       do {
         *         PUP(out) = PUP(from);
         *       } while (--nbytes);
         *       from = out - dist;
         *     }
         *   }
         */

#if ! defined( USE_MMX )
#define len_r %eax
        movl    len(%esp), len_r
#endif
        cmpl    nbytes_r, len_r
        jbe     .L_do_copy1             /* if (nbytes >= len) */

        subl    nbytes_r, len_r         /* len -= nbytes */
        rep     movsb
        movl    out_r, from_r
        subl    dist_r, from_r          /* from = out - dist */
        jmp     .L_do_copy1

        cmpl    nbytes_r, len_r
        jbe     .L_do_copy1             /* if (nbytes >= len) */

        subl    nbytes_r, len_r         /* len -= nbytes */
        rep     movsb
        movl    out_r, from_r
        subl    dist_r, from_r          /* from = out - dist */
        jmp     .L_do_copy1

.L_wrap_around_window:
        /* regs: %esi = from, %ebp = hold, %bl = bits, %edi = out, %edx = dist
         *       %ecx = nbytes, %eax = write, %eax = len
         *
         *   else if (write < nbytes) {
         *     from += wsize + write - nbytes;
         *     nbytes -= write;
         *     if (nbytes < len) {
         *       len -= nbytes;
         *       do {
         *         PUP(out) = PUP(from);
         *       } while (--nbytes);
         *       from = window;
         *       nbytes = write;
         *       if (nbytes < len) {
         *         len -= nbytes;
         *         do {
         *           PUP(out) = PUP(from);
         *         } while(--nbytes);
         *         from = out - dist;
         *       }
         *     }
         *   }
         */
#define write_r %eax

        movl    write(%esp), write_r
        cmpl    write_r, nbytes_r
        jbe     .L_contiguous_in_window /* if (write >= nbytes) */

        addl    wsize(%esp), from_r
        addl    write_r, from_r
        subl    nbytes_r, from_r        /* from += wsize + write - nbytes */
        subl    write_r, nbytes_r       /* nbytes -= write */
#undef write_r

#if ! defined( USE_MMX )
        movl    len(%esp), len_r
#endif
        cmpl    nbytes_r, len_r
        jbe     .L_do_copy1             /* if (nbytes >= len) */

        subl    nbytes_r, len_r         /* len -= nbytes */
        rep     movsb
        movl    window(%esp), from_r    /* from = window */
        movl    write(%esp), nbytes_r   /* nbytes = write */
        cmpl    nbytes_r, len_r
        jbe     .L_do_copy1             /* if (nbytes >= len) */

        subl    nbytes_r, len_r         /* len -= nbytes */
        rep     movsb
        movl    out_r, from_r
        subl    dist_r, from_r          /* from = out - dist */
        jmp     .L_do_copy1

.L_contiguous_in_window:
        /* regs: %esi = from, %ebp = hold, %bl = bits, %edi = out, %edx = dist
         *       %ecx = nbytes, %eax = write, %eax = len
         *
         *   else {
         *     from += write - nbytes;
         *     if (nbytes < len) {
         *       len -= nbytes;
         *       do {
         *         PUP(out) = PUP(from);
         *       } while (--nbytes);
         *       from = out - dist;
         *     }
         *   }
         */
#define write_r %eax

        addl    write_r, from_r
        subl    nbytes_r, from_r        /* from += write - nbytes */
#undef write_r

#if ! defined( USE_MMX )
        movl    len(%esp), len_r
#endif
        cmpl    nbytes_r, len_r
        jbe     .L_do_copy1             /* if (nbytes >= len) */

        subl    nbytes_r, len_r         /* len -= nbytes */
        rep     movsb
        movl    out_r, from_r
        subl    dist_r, from_r          /* from = out - dist */

.L_do_copy1:
        /* regs: %esi = from, %esi = in, %ebp = hold, %bl = bits, %edi = out
         *       %eax = len
         *
         *     while (len > 0) {
         *       PUP(out) = PUP(from);
         *       len--;
         *     }
         *   }
         * } while (in < last && out < end);
         */
#undef nbytes_r
#define in_r %esi

        movl    len_r, %ecx
        rep     movsb

        movl    in(%esp), in_r          /* move in back to %esi, toss from */
#if defined( USE_MMX )
        movl    lcode(%esp), %ebx       /* move lcode back to %ebx, toss dist */
#endif
        jmp     .L_while_test

#undef len_r
#undef from_r
#undef dist_r

.L_invalid_distance_code:
        /* else {
         *   strm->msg = "invalid distance code";
         *   state->mode = BAD;
         * }
         */
        movl    $.L_invalid_distance_code_msg, %ecx
        movl    $INFLATE_MODE_BAD, %edx
        jmp     .L_update_stream_state

.L_test_for_end_of_block:
        /* else if (op & 32) {
         *   state->mode = TYPE;
         *   break;
         * }
         */
        testb   $32, %al
        jz      .L_invalid_literal_length_code  /* if ((op & 32) == 0) */

        movl    $0, %ecx
        movl    $INFLATE_MODE_TYPE, %edx
        jmp     .L_update_stream_state

.L_invalid_literal_length_code:
        /* else {
         *   strm->msg = "invalid literal/length code";
         *   state->mode = BAD;
         * }
         */
        movl    $.L_invalid_literal_length_code_msg, %ecx
        movl    $INFLATE_MODE_BAD, %edx
        jmp     .L_update_stream_state

.L_invalid_distance_too_far:
        /* strm->msg = "invalid distance too far back";
         * state->mode = BAD;
         */
        movl    in(%esp), in_r          /* from_r has in's reg, put in back */
        movl    $.L_invalid_distance_too_far_msg, %ecx
        movl    $INFLATE_MODE_BAD, %edx
        jmp     .L_update_stream_state

.L_update_stream_state:
        /* set strm->msg = %ecx, strm->state->mode = %edx */
        movl    strm_sp(%esp), %eax
        testl   %ecx, %ecx              /* if (msg != NULL) */
        jz      .L_skip_msg
        movl    %ecx, msg_strm(%eax)    /* strm->msg = msg */
.L_skip_msg:
        movl    state_strm(%eax), %eax  /* state = strm->state */
        movl    %edx, mode_state(%eax)  /* state->mode = edx (BAD | TYPE) */

.L_break_loop:

#define strm_r  %eax
#define state_r %edx

        /* len = bits >> 3;
         * in -= len;
         * bits -= len << 3;
         * hold &= (1U << bits) - 1;
         * state->hold = hold;
         * state->bits = bits;
         * strm->next_in = in;
         * strm->next_out = out;
         */
        movl    strm_sp(%esp), strm_r
        movl    bitslong_r, %ecx
        movl    state_strm(strm_r), state_r
        shrl    $3, %ecx
        subl    %ecx, in_r
        shll    $3, %ecx
        subl    %ecx, bitslong_r
        movl    out_r, next_out_strm(strm_r)
        movl    in_r, next_in_strm(strm_r)
        movl    bitslong_r, bits_state(state_r)

        movl    bitslong_r, %ecx
        movl    $1, %ebx                /* overwrites bitslong_r, %bl */
        shll    %cl, %ebx
        decl    %ebx

#undef bits_r
#undef bitslong_r

#if ! defined( USE_MMX )

        andl    %ebx, hold_r
        movl    hold_r, hold_state(state_r)

#else /* USE_MMX */

        psrlq   used_mm, hold_mm        /* hold_mm >>= last bit length */
        movd    hold_mm, %ecx
        andl    %ebx, %ecx
        movl    %ecx, hold_state(state_r)

#endif

#define last_r %ebx

        /* strm->avail_in = in < last ? 5 + (last - in) : 5 - (in - last) */
        movl    last(%esp), last_r
        cmpl    in_r, last_r
        jbe     .L_last_is_smaller     /* if (in >= last) */

        subl    in_r, last_r           /* last -= in */
        addl    $5, last_r             /* last += 5 */
        movl    last_r, avail_in_strm(strm_r)
        jmp     .L_fixup_out
.L_last_is_smaller:
        subl    last_r, in_r           /* in -= last */
        negl    in_r                   /* in = -in */
        addl    $5, in_r               /* in += 5 */
        movl    in_r, avail_in_strm(strm_r)

#undef last_r
#define end_r %ebx

.L_fixup_out:
        /* strm->avail_out = out < end ? 257 + (end - out) : 257 - (out - end)*/
        movl    end(%esp), end_r
        cmpl    out_r, end_r
        jbe     .L_end_is_smaller      /* if (out >= end) */

        subl    out_r, end_r           /* end -= out */
        addl    $257, end_r            /* end += 257 */
        movl    end_r, avail_out_strm(strm_r)
        jmp     .L_done
.L_end_is_smaller:
        subl    end_r, out_r           /* out -= end */
        negl    out_r                  /* out = -out */
        addl    $257, out_r            /* out += 257 */
        movl    out_r, avail_out_strm(strm_r)

#undef end_r

.L_done:
#if defined( USE_MMX )
        emms
#endif
        addl    $local_var_size, %esp
        popf
        popl    %ebx
        popl    %ebp
        popl    %esi
        popl    %edi
        ret

.type inflate_fast,@function
.size inflate_fast,.-inflate_fast
