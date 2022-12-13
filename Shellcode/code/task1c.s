section .text
  global _start
    _start:
      ; Store the argument string on stack
      xor  eax, eax
      push eax          ; Use 0 to terminate the string
      push "//sh"
      push "/bin"
      mov  ebx, esp     ; Get the string address

      ; Construct the argument array argv[]
        push eax

        mov eax,"##al"
        shr eax,16
        push eax

        mov eax,"ls -"
        push eax
        mov  ecx,esp ;store ls -al into ecx

        xor eax,eax
        push eax
        mov eax,"##-c"
        shr eax,16
        push eax
        mov edx,esp ;store -c into edx

        xor eax,eax
        push eax ; 0 terminate
        push ecx ; ls -al
        push edx ; -c 
        push ebx ; /bin/sh
        mov ecx,esp

      ; For environment variable 
      xor  edx, edx     ; No env variables 

      ; Invoke execve()
      xor  eax, eax     ; eax = 0x00000000
      mov   al, 0x0b    ; eax = 0x0000000b
      int 0x80
