;
; gvmat32.asm -- Asm portion of the optimized longest_match for 32 bits x86
; Copyright (C) 1995-1996 Jean-loup Gailly and Gilles Vollant.
; File written by Gilles Vollant, by modifiying the longest_match
;  from Jean-loup Gailly in deflate.c
; It need wmask == 0x7fff
;     (assembly code is faster with a fixed wmask)
;
; For Visual C++ 4.2 and ML 6.11c (version in directory \MASM611C of Win95 DDK)
;   I compile with : "ml /coff /Zi /c gvmat32.asm"
;
; uInt longest_match_gvasm(IPos cur_match,int* match_start_ptr,uInt scan_end,
;                          uInt scan_start,ush* prev,uch* window,int best_len,
;                          IPos limit,uInt chain_length,uch* scanrp,
;                          uInt nice_match);

;uInt longest_match(s, cur_match)
;    deflate_state *s;
;    IPos cur_match;                             /* current match */

	NbStack		equ	76
	cur_match	equ	dword ptr[esp+NbStack-0]
	str_s		equ	dword ptr[esp+NbStack-4]
	; 5 dword on top (ret,ebp,esi,edi,ebx)
	adrret		equ	dword ptr[esp+NbStack-8]
	pushebp		equ	dword ptr[esp+NbStack-12]
	pushedi		equ	dword ptr[esp+NbStack-16]
	pushesi		equ	dword ptr[esp+NbStack-20]
	pushebx		equ	dword ptr[esp+NbStack-24]

	chain_length    equ dword ptr [esp+NbStack-28]
	limit           equ dword ptr [esp+NbStack-32]
	best_len        equ dword ptr [esp+NbStack-36]
	window          equ dword ptr [esp+NbStack-40]
	prev            equ dword ptr [esp+NbStack-44]
	scan_start      equ  word ptr [esp+NbStack-48]
	scan_end        equ  word ptr [esp+NbStack-52]
	match_start_ptr equ dword ptr [esp+NbStack-56]
	nice_match      equ dword ptr [esp+NbStack-60]
	scanrp          equ dword ptr [esp+NbStack-64]

	windowlen       equ dword ptr [esp+NbStack-68]
	match_start     equ dword ptr [esp+NbStack-72]
	strend			equ dword ptr [esp+NbStack-76]
	NbStackAdd		equ	(76-24)

    .386p

    name    gvmatch
    .MODEL  FLAT


@lmtype				TYPEDEF         PROTO C :PTR , :SDWORD
longest_match_c		PROTO           @lmtype

    ;  all the +4 offsets are due to the addition of pending_buf_size
    ;  in the deflate_state structure since the asm code was first written
    dep_chain_length    equ     70h+4
    dep_window          equ     2ch+4
    dep_strstart        equ     60h+4
    dep_prev_length     equ     6ch+4
    dep_nice_match      equ     84h+4
    dep_w_size          equ     20h+4
    dep_prev            equ     34h+4
    dep_w_mask          equ     28h+4
    dep_good_match      equ     80h+4
    dep_match_start     equ     64h+4
    dep_lookahead       equ     68h+4


_TEXT   segment
    public  _longest_match_asm7fff

    MAX_MATCH		equ 258
	MIN_MATCH		equ 3
	MIN_LOOKAHEAD equ (MAX_MATCH+MIN_MATCH+1)

; initialize or check the variables used in match.asm.


; -----------------------------------------------------------------------
; Set match_start to the longest match starting at the given string and
; return its length. Matches shorter or equal to prev_length are discarded,
; in which case the result is equal to prev_length and match_start is
; garbage.
; IN assertions: cur_match is the head of the hash chain for the current
;   string (strstart) and its distance is <= MAX_DIST, and prev_length >= 1

; int longest_match(cur_match)

_longest_match_asm7fff proc near



	    ; return address

		mov		eax,[esp+4]
		mov		bx,[eax+dep_w_mask]
		cmp		bx,7fffh
		jnz		longest_match_c

	    push    ebp             
	    push    edi             
	    push    esi             
	    push    ebx             

	    sub     esp,NbStackAdd

		;//mov		ebp,str_s
		mov		ebp,eax

		mov		eax,[ebp+dep_max_chain_length]
		mov		ebx,[ebp+dep_prev_length]
		cmp		[ebp+dep_good_match],ebx	; if prev_length>=good_match chain_length >>= 2
		ja		noshr		
		shr		eax,2
noshr:
		mov		edi,[ebp+dep_nice_match]
		mov		chain_length,eax
		mov		edx,[ebp+dep_lookahead]
		cmp		edx,edi
;if ((uInt)nice_match > s->lookahead) nice_match = s->lookahead;
		jae		nolookaheadnicematch
		mov		edi,edx
nolookaheadnicematch:
		mov		best_len,ebx


		mov		esi,[ebp+dep_window]
		mov		ecx,[ebp+dep_strstart]
		mov		window,esi
		
		mov		nice_match,edi
		add		esi,ecx
		mov		scanrp,esi
		mov		ax,word ptr [esi]
		mov		bx,word ptr [esi+ebx-1]
		add		esi,MAX_MATCH-1
		mov		scan_start,ax
		mov		strend,esi
		mov		scan_end,bx

;    IPos limit = s->strstart > (IPos)MAX_DIST(s) ?
;        s->strstart - (IPos)MAX_DIST(s) : NIL;

		mov		esi,[ebp+dep_w_size]
		sub		esi,MIN_LOOKAHEAD
		; here esi = MAX_DIST(s)
		sub		ecx,esi
		ja		nodist
		xor		ecx,ecx
nodist:
		mov		limit,ecx




		mov		eax,[ebp+dep_prev]
		mov		prev,eax

	    mov     ebx,dword ptr [ebp+dep_match_start]
	    mov     bp,scan_start
	    mov     edx,cur_match
	    mov     match_start,ebx

	    mov     bx,scan_end
	    mov     eax,window
		mov		edi,eax
	    add     edi,best_len
	    mov     esi,prev
		dec     edi
	    mov     windowlen,edi
	    
	    jmp     beginloop2
	    align   4

; here, in the loop
;;;;       eax = chain_length
;       edx = dx = cur_match
;       ecx = limit
;        bx = scan_end
;        bp = scan_start
;       edi = windowlen (window + best_len)
;       esi = prev


;// here; eax <=16
normalbeg0add16: 
		add		chain_length,16
		jz		exitloop
normalbeg0: 
	    cmp     word ptr[edi+edx-0],bx
	    je      normalbeg2
	    and     edx,7fffh
	    mov     dx,word ptr[esi+edx*2]
	    cmp     ecx,edx
	    jnb     exitloop
	    dec     chain_length
		jnz		normalbeg0
;jnbexitloopshort1:
		jmp     exitloop

contloop3:
	    mov     edi,windowlen

; cur_match = prev[cur_match & wmask]
	    and		edx,7fffh
	    mov     dx,word ptr[esi+edx*2]
; if cur_match > limit, go to exitloop
	    cmp     ecx,edx
jnbexitloopshort1:
	    jnb     exitloop
; if --chain_length != 0, go to exitloop

beginloop2:
		sub		chain_length,16+1
		jna     normalbeg0add16

do16:
	    cmp     word ptr[edi+edx],bx
	    je      normalbeg2dc0

maccn	MACRO	lab
	    and     edx,7fffh
	    mov     dx,word ptr[esi+edx*2]
	    cmp     ecx,edx
	    jnb     exitloop
	    cmp     word ptr[edi+edx-0],bx
	    je      lab
		ENDM

rcontloop0:
		maccn normalbeg2dc1

rcontloop1:
		maccn normalbeg2dc2

rcontloop2:
		maccn normalbeg2dc3

rcontloop3:
		maccn normalbeg2dc4

rcontloop4:
		maccn normalbeg2dc5

rcontloop5:
		maccn normalbeg2dc6

rcontloop6:
		maccn normalbeg2dc7

rcontloop7:
		maccn normalbeg2dc8

rcontloop8:
		maccn normalbeg2dc9

rcontloop9:
		maccn normalbeg2dc10

rcontloop10:
		maccn normalbeg2dc11

rcontloop11:
		maccn short normalbeg2dc12

rcontloop12:
		maccn short normalbeg2dc13

rcontloop13:
		maccn short normalbeg2dc14

rcontloop14:
		maccn short normalbeg2dc15

rcontloop15:
	    and     edx,7fffh
	    mov     dx,word ptr[esi+edx*2]
	    cmp     ecx,edx
	    jnb     short exitloopshort

		sub		chain_length,16
		ja		do16
		jmp		normalbeg0add16

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

exitloopshort:
		jmp		exitloop                  

normbeg	MACRO	rcontlab,valsub
	    cmp     bp,word ptr[eax+edx]
		jne		rcontlab
		add     chain_length,16-valsub
		jmp		iseq
		ENDM

normalbeg2dc12:
		normbeg	rcontloop12,12

normalbeg2dc13:
		normbeg	rcontloop13,13

normalbeg2dc14:
		normbeg	rcontloop14,14

normalbeg2dc15:
		normbeg	rcontloop15,15

normalbeg2dc11:
		normbeg	rcontloop11,11

normalbeg2dc10:
		normbeg	rcontloop10,10


normalbeg2dc9:
		normbeg	rcontloop9,9

normalbeg2dc8:
		normbeg	rcontloop8,8

normalbeg2dc7:
		normbeg	rcontloop7,7

normalbeg2dc5:
		normbeg	rcontloop5,5





normalbeg2dc6:
		normbeg	rcontloop6,6

normalbeg2dc4:
		normbeg	rcontloop4,4

normalbeg2dc3:
		normbeg	rcontloop3,3

normalbeg2dc2:
		normbeg	rcontloop2,2

normalbeg2dc1:
		normbeg	rcontloop1,1

normalbeg2dc0:
		normbeg	rcontloop0,0


; we go in normalbeg2 because *(ushf*)(match+best_len-1) == scan_end

normalbeg2:

			 ; 10 nop here take 10% time
		mov		edi,window
		  ;mov     chain_length,eax    ; now, we need eax...
					
	    cmp     bp,word ptr[edi+edx]
	    jne     contloop3                   ; if *(ushf*)match != scan_start, continue

iseq:

		mov		edi,eax
	    mov     esi,scanrp                  ; esi = scan    
	    add     edi,edx				; edi = window + cur_match = match

		       
	    mov     eax,[esi+3]					; compare manually dword at match+3
	    xor     eax,[edi+3]                 ;       and scan +3

	    jz      begincompare                ; if equal, go to long compare

			; we will determine the unmatch byte and calculate len (in esi)
		or		al,al
		je		eq1rr
	    mov     esi,3
	    jmp     trfinval
eq1rr:
	    or      ax,ax                               
	    je      eq1

	    mov     esi,4
	    jmp     trfinval
eq1:
	    shl     eax,8
	    jz      eq11
	    mov     esi,5
	    jmp     trfinval
eq11:
	    mov     esi,6
	    jmp     trfinval

begincompare:
		; here we now scan and match begin same
	    add     edi,6
	    add     esi,6
	    mov     ecx,(MAX_MATCH-(2+4))/4     ;//; scan for at most MAX_MATCH bytes
	    repe    cmpsd                                               ;//; loop until mismatch

	    je      trfin                                               ; go to trfin if not unmatch
			; we determine the unmatch byte
	    sub     esi,4
	    mov     eax,[edi-4]
	    xor     eax,[esi]
	    or      al,al

	    jnz     trfin
	    inc     esi

	    or      ax,ax
	    jnz     trfin
	    inc     esi

	    shl     eax,8           
	    jnz     trfin
	    inc     esi

trfin:      
	    sub     esi,scanrp          ; esi = len
trfinval:
	    cmp     esi,best_len        ; if len <= best_len, go contloop2
	    jbe     contloop2

	    mov     best_len,esi        ; len become best_len

	    mov     match_start,edx
	    cmp     esi,nice_match ;//; if esi >= nice_match, exit
	    mov     ecx,scanrp
	    jae     exitloop
	    add     esi,window
	    add     ecx,best_len
			dec             esi
	    mov     windowlen,esi
	    mov     bx,[ecx-1]


; now we restore eax, ecx and esi, for the big loop :
contloop2:
	    mov     esi,prev
	    mov     ecx,limit
	    ;mov     eax,chain_length
		mov		eax,window
	    jmp     contloop3

exitloop:        
	    mov     ebx,match_start
		mov		ebp,str_s
	    mov     dword ptr [ebp+dep_match_start],ebx
	    mov     eax,best_len
	    add     esp,NbStackAdd


	    pop     ebx
	    pop     esi
	    pop     edi
	    pop     ebp 
	    ret

_longest_match_asm7fff endp

_TEXT   ends
end
