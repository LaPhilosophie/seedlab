section .text
  global _start
    _start:
        BITS 32
        jmp short two
    one:
        pop ebx
        xor eax, eax

        ;the next 4 lines converse # into 0
        mov [ebx+12], al
        mov [ebx+15], al
        mov [ebx+20], al
        mov [ebx+25], al

        mov [ebx+26],ebx ;put address of "/usr/bin/env\0" to where AAAA is

        lea eax,[ebx+13]
        mov [ebx+30],eax ;put address of "-i\0" to where BBBB is 

        lea eax,[ebx+16]
        mov [ebx+34],eax ;put address of "a=11\0" to where CCCC is

        lea eax,[ebx+21]
        mov [ebx+38],eax ;put address of "b=22\0" to where DDDD is

        xor eax,eax
        mov [ebx+42],eax ;0 terminate

        ; now ebx point to "/usr/bin/env\0"     

        lea ecx, [ebx+26] ;put address of "/usr/bin/env -i a=11 b=22" to ecx 

        xor edx,edx ; edx = 0 

        mov al,  0x0b
        int 0x80
     two:
        call one
        db '/usr/bin/env#-i#a=11#b=22#AAAABBBBCCCCDDDDEEEE'
           ;012345678901234567890123456789012345678901234567890
           ;          1         2         3         4    