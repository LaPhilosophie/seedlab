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