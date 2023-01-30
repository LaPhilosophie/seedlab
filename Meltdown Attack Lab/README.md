# Lab Environment

- 熔断漏洞是英特尔 CPU 内部的一个缺陷，对 AMD 机器无效
- 熔断和幽灵攻击都使用 CPU 缓存作为侧通道窃取受保护的信息
- seed预先构建的虚拟机提供了环境，因此我们需要在 Ubuntu 16.04 VM以完成实验，注意不要更新虚拟机的操作系统
- 使用`gcc -march=native -o myprog myprog.c`命令去编译程序，这里-march=native表示启用本地机器支持的所有指令子集

# Task 1: Reading from Cache versus from Memory

多次运行，观察发现每次`[3*4096]`和`[7*4096]`的CPU cycles都是最少的

![](https://cdn.jsdelivr.net/gh/LaPhilosophie/image/img/20230124101340.png)

# Task 2: Using Cache as a Side Channel

- secret是一个一字节的秘密值，我们通过cache的侧信道攻击方式猜出该值
- 一字节一共有256种可能，我们可以构造出大小为256\*4096+1024的数组，由于访问过[secret*4096 + 1024]之后，该值会被cache，因此可以通过循环的方式依次访问[i]\*4096+1024，使用时间最短的i就是被cache的secret值
  - 之所以使用4096，是因为它比典型的缓存块大(64字节) ，缓存是在块级别完成的，而不是在字节级别，因此定义一个包含256个元素的数组(即 array [256])是行不通的
  - 之所以使用1024，是因为[0 * 4096]可能与相邻内存中的变量位于相同的缓存块中，为了避免与变量cache到相同的块中，[i*4096]需要加上1024的偏移

![](https://cdn.jsdelivr.net/gh/LaPhilosophie/image/img/20230124151051.png)

# Task 3: Place Secret Data in Kernel Space

MeltdownKernel.c使用一个内核模块来存储秘密数据

编译、安装内核模块，使用 dmesg 命令从内核消息缓冲区中查找秘密数据的地址secret data address：0xf9dcd000

![](https://cdn.jsdelivr.net/gh/LaPhilosophie/image/img/20230128174009.png)

用户级程序调用内核模块中的函数。此函数将访问机密数据，而不会将其泄露给用户级程序。这种访问的副作用是秘密数据现在位于 CPU 缓存中

# Task 4: Access Kernel Memory from User Space

由于用户空间程序无法直接访问内核地址的数据，因此会导致segment fault

> 访问被禁止的内存位置将发出 SIGSEGV 信号; 如果程序不自己处理这个异常，操作系统将处理它并终止程序。这就是程序崩溃的原因

![](https://cdn.jsdelivr.net/gh/LaPhilosophie/image/img/20230128194724.png)

# Task 5: Handle Error/Exceptions in C

为了避免task4中的segment fault，我们可以在程序中定义我们自己的信号处理程序来捕获由错误事件引发的异常，从而防止程序因错误事件而崩溃。访问被禁止的内存位置将发出 SIGSEGV 信号

与 C + + 或其他高级语言不同，C 不直接支持错误处理(也称为异常处理) ，例如 try/catch 子句。但是，我们可以使用 sigsetjmp ()和 siglongjmp()来模拟 try/catch 子句

关于代码的几点注释：

- **setjmp**() saves the stack context/environment in *env*，**sigsetjmp**() is similar to **setjmp**(). If, and only if, *savesigs* is nonzero, the process's current signal mask is saved in *env* and will be restored if a **[siglongjmp](https://linux.die.net/man/3/siglongjmp)**(3) is later performed with this *env*。如果希望可移植地保存和恢复信号掩码，使用 sigsetjmp ()和 siglongjmp (3)
- 设置信号处理：signal(SIGSEGV, catch_segv)注册一个 SIGSEGV 信号处理程序，这样当一个 SIGSEGV 信号被引发时，处理程序函数 catch Segv ()将被调用
- 设置检查点：在信号处理程序完成对异常的处理之后，它需要让程序从特定的检查点继续执行。因此，我们需要首先定义一个检查点。使用 sigsetjmp ()实现的: sigsetjmp (jbuf，1)将堆栈上下文/环境保存在 jbuf 中，以供 siglongjmp ()稍后使用。参考：https://pubs.opengroup.org/onlinepubs/7908799/xsh/sigsetjmp.html
- roll back到一个检查点: 当调用 siglongjmp (jbuf，1)时，保存在 jbuf 变量中的状态被复制回处理器，计算从 sigsetjmp ()函数的返回点开始，但是 sigsetjmp ()函数的返回值是 siglongjmp ()函数的第二个参数，在我们的例子中是1。因此，在异常处理之后，程序继续从 else 分支执行
- 触发异常: `char kernel_data = *(char*)kernel_data_addr; `处的代码将触发一个 SIGSEGV 信号，这是由于内存访问冲突(用户级程序不能访问内核内存)

```
#include <setjmp.h>

int setjmp(jmp_buf env); //参数env为用来保存目前堆栈环境

int sigsetjmp(sigjmp_buf env, int savesigs); 


```

```c
#include <stdio.h>
#include <setjmp.h>
#include <signal.h>

static sigjmp_buf jbuf;

static void catch_segv()
{
  // Roll back to the checkpoint set by sigsetjmp().
  siglongjmp(jbuf, 1);                         
}

int main()
{ 
  // The address of our secret data
  unsigned long kernel_data_addr = 0xfb61b000;

  // Register a signal handler
  signal(SIGSEGV, catch_segv);                     

  //setjmp() and sigsetjmp() return 0 if returning directly
  if (sigsetjmp(jbuf, 1) == 0) {	               
     // A SIGSEGV signal will be raised. 
     char kernel_data = *(char*)kernel_data_addr; 

     // The following statement will not be executed.
     printf("Kernel data at address %lu is: %c\n", 
                    kernel_data_addr, kernel_data);
  }
  else {
     printf("Memory access violation!\n");
  }

  printf("Program continues to execute.\n");
  return 0;
}
```

运行后输出：

```
Memory access violation!
Program continues to execute.
```

# Task 6: Out-of-Order Execution by CPU

这个任务是为了验证乱序执行的存在

```c
1 number = 0;
2 kernel_address = (char*)0xfb61b000;
3 kernel_data = *kernel_address;	//涉及两个操作: 加载数据(通常加载到寄存器中) ，以及检查是否允许数据访问
4 number = number + kernel_data;
```

对于如上语句，第3行将引发一个异常，因为地址为0xfb61b000的内存属于内核。因此，执行将在第3行被中断，而第4行将永远不会被执行，所以数字变量的值仍然是0

然而，现代高性能 CPU 不再严格按照指令的原始顺序执行指令，而是允许乱序执行耗尽所有的执行单元。第3行涉及两个操作: 加载数据(通常加载到寄存器中) ，以及检查是否允许数据访问。**如果数据已经在 CPU 缓存中，那么第一个操作将非常快，而第二个操作可能需要一段时间。为了避免等待，CPU 将继续执行第4行和后续指令，同时并行执行访问检查**。这是乱序执行。在访问检查完成之前，不会提交执行结果。由于访问了内核数据，检查失败了，因此由乱序执行引起的所有结果都将被丢弃，就像从未发生过一样。这就是为什么从外面我们看不到第四行被执行的原因

`meltdown(0xfb61b000); `语句调用了meltdown函数，尽管该函数内的非法内存访问会导致异常，但是下一条语句，也即是`array[7 * 4096 + DELTA] += 1;`由于CPU乱序执行机制仍然会运行，从而将7纳入cache，之后我们调用reloadSideChannel可以得出秘密值是7

![](https://cdn.jsdelivr.net/gh/LaPhilosophie/image/img/20230129200746.png)

# Task 7: The Basic Meltdown Attack

## Task 7.1: A Naive Approach

显示 Memory access violation

## Task 7.2: Improve the Attack by Getting the Secret Data Cached

熔断漏洞是一个竞争条件的漏洞，其中包括竞争条件之间的乱序执行和访问检查。乱序执行越快，我们能执行的指令就越多，我们就越有可能创造出可观察到的效果，帮助我们得到秘密数据

在我们的代码中，乱序执行的第一步是将内核数据加载到一个寄存器中。同时，对这样的访问执行安全检查。如果数据加载慢于安全检查，也就是说，当安全检查完成时，因为访问检查失败，内核数据仍然在从内存到寄存器的路上，乱序执行会立即中断并丢弃。我们的攻击也会失败

在这里可以先访问/proc/secret_data，使其在CPU cache中，这样可以加快数据加载的速度，从而利用竞态漏洞，在安全检查结束之前获得信息

在flushsidechannel后添加：

```c
// Open the /proc/secret_data virtual file.
int fd = open("/proc/secret_data", O_RDONLY);
if (fd < 0) {
    perror("open");
    return -1;
}
int ret = pread(fd, NULL, 0, 0); // Cause the secret data to be cached.
```

依旧显示 Memory access violation

## Task 7.3: Using Assembly Code to Trigger Meltdown

通过在内核内存访问之前添加几行汇编指令再做一次改进，关于汇编的写法，这里有一个参考https://meltdownattack.com/meltdown.pdf

![](https://cdn.jsdelivr.net/gh/LaPhilosophie/image/img/20230130204216.png)

# Task 8: Make the Attack More Practical