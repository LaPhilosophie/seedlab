# Environment Setup

- 使用-m32标志编译为32位的程序
- 关闭地址随机化：`sudo sysctl -w kernel.randomize_va_space=0`
- 使用`-fno-stack-protected` 选项在编译期间禁用StackGuard保护，比如`gcc -m32 -fno-stack-protector example.c`

- 程序(和共享库)的二进制映像必须声明它们是否需要可执行堆栈。通过在程序头中标记一个字段来决定这个正在运行的程序的堆栈成是可执行的/不可执行的。最近版本的Ubuntu默认程序堆栈是不可执行的，可以通过`-z execstack`选项来使得堆栈可执行
- 为了简化攻击难度，我们需要将/bin/sh指向/bin/zsh。在 Ubuntu 20.04中，/bin/sh 符号链接指向/bin/dash shell，此shell具有保护机制：如果dash是在 Set-UID 进程中执行的，那么它会立即将有效用户 ID 更改为实际用户 ID，也即是会放弃其特权。`sudo ln -sf /bin/zsh /bin/sh`

综上：

```shell
sudo sysctl -w kernel.randomize_va_space=0

sudo ln -sf /bin/zsh /bin/sh

gcc -m32  -fno-stack-protector -z noexecstack -o retlib retlib.c

sudo chown root retlib

sudo chmod 4755 retlib

touch badfile 

./retlib
```



# Task 1: Finding out the Addresses of libc Functions

在 Linux 中，当一个程序运行时，libc 库将被加载到内存中。当内存地址随机化被关闭时，对于同一个程序，库总是加载在相同的内存地址中(对于不同的程序，libc 库的内存地址可能是不同的)。因此，我们可以使用 gdb 这样的调试工具轻松找到 system ()的地址

可以将gdb的批处理命令写在gdb_command.txt中，然后输入

```shell
gdb -q -batch -x gdb_command.txt ./retlib
```

gdb_command.txt：

```
break main
run
p system
p exit
quit
```

system地址：0xf7e11420

exit地址：0xf7e03f80

# Task 2: Putting the shell string in the memory

我们的目的是触发system()函数，且参数是`/bin/sh`，这样就可以得到shell。因此，我们必须先知道字符串`/bin/sh`的地址

可以使用环境变量的方式实现上述目标

```shell
$ export MYSHELL=/bin/sh
$ env | grep MYSHELL
MYSHELL=/bin/sh
```

编写程序./prtenv打印出`/bin/sh`字符串所在地址，注意到文件名和retlib必须都是相同字符数目

```c
#include<stdio.h>

void main(){
    char* shell = getenv("MYSHELL");
    if (shell)
    	printf("%x\n", (unsigned int)shell);
}
```

得到地址为`ffffd45d`





# Task 4: Defeat Shell’s countermeasure

回忆一下防御机制：如果dash是在 Set-UID 进程中执行的，那么它会立即将有效用户 ID 更改为实际用户 ID，也即是会放弃其特权

之前我们手动将sh指向zsh，这个任务中我们需要















