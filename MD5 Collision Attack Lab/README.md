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

- 如果prefix不到64那么会补0直到64（包含0A）
- 使用`echo "$(python3 -c 'print("A"*63)')" > prefix.txt`命令，可以使得该文件有63个A和一个0A，正好是64个



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

生成xyz

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
- 第二个命令将最后100字节的 a.out 保存为suffix
- 第三个命令将文件第3300个字节到末尾的数据保存到suffix

一个基本原理：若$MD5 (prefix ‖P) = MD5 (prefix ‖Q)$成立，那么$MD5 (prefix ‖P ‖suffix) = MD5 (prefix ‖Q ‖suffix)成立$

换句话说，如果一个文件和另一个文件的md5值已经相等，那么在它们的后面附加相同的内容，仍然相等

**回到这个任务的要求上，我们找到二进制文件十六进制先显示的prefix+AAAA处，执行md5col，可以得到md5col（prefix+AAAA），之后再附加suffix，即可完成目标所需的两个可执行文件**

看一下AAAA的位置

![image-20230615004528181](https://img.gls.show/img/image-20230615004528181.png)

$12320/64=192.5$

$12352/64=193$

因此，12353~末尾为suffix

下面过程比较简单就不描述了

![image-20230615005533457](https://img.gls.show/img/image-20230615005533457.png)

# Task 4: Making the Two Programs Behave Differently