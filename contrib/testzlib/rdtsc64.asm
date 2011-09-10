; rdtsc64.asm
;
; unsigned _int64 myrdtsc();
;
; return the performance rdtsc value, on AMD64/Intel EM64T
;
; compile with :
;    ml64.exe" /Flrdtsc64   /c /Zi rdtsc64.asm
;
.code
myrdtsc PROC
          rdtsc
          shl rdx,32
          or rax,rdx
          ret
myrdtsc ENDP

END
