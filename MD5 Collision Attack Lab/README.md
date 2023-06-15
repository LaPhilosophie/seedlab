# Task 1: Generating Two Different Files with the Same MD5 Hash

在这个任务中，我们将生成两个具有相同 MD5哈希值的不同文件。这两个文件的开头部分必须是相同的，也就是说，它们共享相同的前缀。我们可以使用 md5colgen 程序实现这一点，该程序允许我们提供具有任意内容的前缀文件

该程序的工作方式如图所示

![image-20230614012709811](https://img.gls.show/img/image-20230614012709811.png)

下面的命令为给定的前缀文件 prefix.txt 生成两个输出文件 out1.bin 和 out2.bin:

```shell
$ echo zara > prefix.txt
$ md5collgen -p prefix.txt -o out1.bin out2.bin
```

这样就创建了两个文件out1.bin out2.bin

发现两个文件的md5确实一样

```
[06/13/23]seed@VM:~$ md5sum out1.bin out2.bin 
b1eb0da929692656edc11a9d93733d39  out1.bin
b1eb0da929692656edc11a9d93733d39  out2.bin
```

> 由于 out1.bin 和 out2.bin 是二进制的，我们不能使用文本查看器程序(如 cat 或更多)查看它们; 我们需要使用二进制编辑器来查看(和编辑)它们

使用xxd -p分别取出两个文件的十六进制序列，用vimdiff比对一下：

![image-20230614012253064](https://img.gls.show/img/image-20230614012253064.png)



Question：

- 如果prefix不到64那么会补0直到64（包含0A）。之后会生成128长度的内容，总长度为64+128=192
- 如果prefix正好是64（63个A+一个0A），那么不会补0，总长度为64+128=192
  - 使用`echo "$(python3 -c 'print("A"*63)')" > prefix.txt`命令，可以使得该文件有63个A和一个0A，正好是64个
- 如果prefix是192个，那么不会补0，总长度为192+128
- 如果prefix是200个，那么会补0直到256，总长度为256+128

```shell
[06/14/23]seed@VM:~$ echo "$(python3 -c 'print("A"*256)')" > prefix.txt 
[06/14/23]seed@VM:~$ md5collgen -p prefix.txt -o p1.bin p2.bin 
MD5 collision generator v1.5
by Marc Stevens (http://www.win.tue.nl/hashclash/)

Using output filenames: 'p1.bin' and 'p2.bin'
Using prefixfile: 'prefix.txt'
Using initial value: 74ff661088a332eb0b5d16ad326e89c5

Generating first block: ............
Generating second block: S11.....................................
Running time: 9.41273 s
[06/14/23]seed@VM:~$ bless p1.bin 
Gtk-Message: 10:37:59.970: Failed to load module "canberra-gtk-module"
Could not find a part of the path '/home/seed/.config/bless/plugins'.
Could not find a part of the path '/home/seed/.config/bless/plugins'.
Could not find a part of the path '/home/seed/.config/bless/plugins'.
Could not find file "/home/seed/.config/bless/export_patterns"
[06/14/23]seed@VM:~$ md5sum p1.bin p2.bin 
fc4106d2ee477d8c0f1935bdd94122ac  p1.bin
fc4106d2ee477d8c0f1935bdd94122ac  p2.bin

```

# Task 2: Understanding MD5’s Property

MD5算法的如下性质: 给定两个输入 M 和 N，如果 MD5(M) = MD5(N) ，即 M 和 N 的 MD5散列是相同的，那么对于任何输入 T，MD5(M ‖ T) = MD5(N ‖ T) ，其中‖表示级联

验证性质：如果输入 M 和 N 具有相同的哈希值，将相同的后缀 T 添加到它们将导致具有相同哈希值的两个输出。此属性不仅适用于 MD5哈希算法，还适用于许多其他哈希算法。在这个任务中的工作是设计一个实验来证明这个属性适用于 MD5

```shell
[06/14/23]seed@VM:~$ md5sum p1.bin p2.bin 
69c4760a3459c3ec8f93d3022a7c6654  p1.bin
69c4760a3459c3ec8f93d3022a7c6654  p2.bin
[06/14/23]seed@VM:~$ echo nm >> p1.bin 
[06/14/23]seed@VM:~$ echo nm >> p2.bin 
[06/14/23]seed@VM:~$ md5sum p1.bin p2.bin 
6b4afe7b7301613ae99ce848080ffa96  p1.bin
6b4afe7b7301613ae99ce848080ffa96  p2.bin
```

# Task 3: Generating Two Executable Files with the Same MD5 Hash

创建两个可执行程序，使其 xyz 数组的内容不同，但可执行文件的哈希值是相同的。这个还有意思，你可以把一个程序的内容改变，但是在md5的层面上，它们是没有区别的，从而绕过完整性验证

```c
#include <stdio.h>
unsigned char xyz[200] = {
    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41
};
int main()
{
    int i;
    for (i=0; i<200; i++){
  	 	 printf("%x", xyz[i]);
    }
    printf("\n");
}
```

生成xyz数组

```shell
echo "$(python3 -c 'print("0x41,"*199)')"
```

会用到的命令

```shell
$ head -c 3200 a.out > prefix
$ tail -c 100 a.out > suffix
$ tail -c +3300 a.out > suffix
```

- 第一个命令将 a.out 的前3200字节保存为prefix
- 第二个命令将文件的最后100字节保存为suffix
- 第三个命令将文件的第3300个字节到末尾的数据保存到suffix

一个基本原理：若$MD5 (prefix ‖P) = MD5 (prefix ‖Q)$成立，那么$MD5 (prefix ‖P ‖suffix) = MD5 (prefix ‖Q ‖suffix)成立$

换句话说，如果一个文件和另一个文件的md5值已经相等，那么在它们的后面附加相同的内容，仍然相等

**回到这个任务的要求上，我们找到二进制文件十六进制先显示的prefix+AAAA处，执行md5col，可以得到md5col（prefix+AAAA），之后再附加suffix，即可完成目标所需的两个可执行文件**

看一下AAAA的位置

![image-20230615004528181](https://img.gls.show/img/image-20230615004528181.png)

$12320/64=192.5$

$12352/64=193$

因此，12353~末尾为suffix

下面过程比较简单就不描述了

```shell
[06/14/23]seed@VM:~$ head -c 12324 a.out > prefix
[06/14/23]seed@VM:~$ tail -c 12353 a.out > suffix
[06/14/23]seed@VM:~$ md5collgen -p prefix -o p1.bin p2.bin 
MD5 collision generator v1.5
by Marc Stevens (http://www.win.tue.nl/hashclash/)

Using output filenames: 'p1.bin' and 'p2.bin'
Using prefixfile: 'prefix'
Using initial value: 6a415b5a106e730948ad2d1abe26cfd3

Generating first block: ...
Generating second block: W.....................
Running time: 1.83808 s
[06/14/23]seed@VM:~$ cat suffix >> p1.bin 
[06/14/23]seed@VM:~$ cat suffix >> p2.bin 
[06/14/23]seed@VM:~$ sudo chmod +x p1.bin p2.bin 
[06/14/23]seed@VM:~$ ./p1.bin 
41414141000000000000000000000000000020f253f1bb456b3ce9be3d814456444d94d868c31d7c42867c924cae99a6fdb9cadf6aadc120e7634a1bfec34f10d8ffe44eced2944d8a99ae965727c48fbcad2e235596e0eb6c4c961fcfa440ce86b2ba4825b44d22393e17cba614ce856f3656b790be8be81c1a99840595dc448dc9bc0d27393e6244377e0bf614a89e741ff14df4883c314839dd75ea4883c485b5d415c415d415e415fc366662ef1f840000
[06/14/23]seed@VM:~$ ./p2.bin 
41414141000000000000000000000000000020f253f1bb456b3ce9be3d814456444d945868c31d7c42867c924cae99a6fdb9cadf6aadc120e7634a1bfe435010d8ffe44eced2944d8a99ae165727c48fbcad2e235596e0eb6c4c961fcfa440ce86b2bac825b44d22393e17cba614ce856f3656b790be8be81c1a998c0585dc448dc9bc0d27393e62443f7e0bf614a89e741ff14df4883c314839dd75ea4883c485b5d415c415d415e415fc366662ef1f840000
[06/14/23]seed@VM:~$ md5sum p1.bin p2.bin 
0a0c4456dec1a3c45c7b382613cc71ad  p1.bin
0a0c4456dec1a3c45c7b382613cc71ad  p2.bin

```

这里创建了两个具有相同 MD5散列的程序，它们之间的区别仅仅在于它们打印出来的数据

![image-20230615005533457](https://img.gls.show/img/image-20230615005533457.png)

# Task 4: Making the Two Programs Behave Differently

创建两个共享相同 MD5散列的程序。一个程序将始终执行正常指令，而另一个程序将执行恶意指令

```c
#include <stdio.h>

unsigned char x[200] = {
	0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41

};


unsigned char y[200] = {
	    0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
    0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
    0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
    0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42
};

int main() {
  int i;
  for (i=0; i<200; i++){
    if(x[i] != y[i]){ break; }
  }

  if(i == 200){ printf("%s", "benign code"); } /* x = y */
  else{ printf("%s", "WARNING: malicious code"); } /* x != y */

  printf("\n");
}
```

将上面源码编译为a.out文件

12320偏移处出现了AAAA，说明这里是x数组开始处

将前12324个bytes作为prefix，生成md5碰撞的p1 p2 

```shell
$ head -c 12324 a.out > prefix

$ md5collgen -p prefix -o p1 p2
MD5 collision generator v1.5
by Marc Stevens (http://www.win.tue.nl/hashclash/)

Using output filenames: 'p1' and 'p2'
Using prefixfile: 'prefix'
Using initial value: 378d43f1d8999353cc974181bbf31423

Generating first block: ...
Generating second block: S11.......................
Running time: 2.85953 s
```

![image-20230616022632664](https://img.gls.show/img/image-20230616022632664.png)

我们可以看出，a.out的A的数据区有0xe0个数据

打开p1的bless，将A的数据区填充到0xe0个数据（发现需要64个0，这里我直接在zero文件中写入63个0，加上0A正好是64）

![](https://img.gls.show/img/image-20230616022901794.png)

```shell
echo "$(python3 -c 'print("A"*63)')" > zero

[06/15/23]seed@VM:~$ cat zero >> p1 
[06/15/23]seed@VM:~$ cat zero >> p2 
```

之后将a.out中B的数据区之后的内容接到p1、p2的末尾即可

![](https://img.gls.show/img/image-20230616023556027.png)

发现p1、p2的内容md5值相同，但是运行结果不同