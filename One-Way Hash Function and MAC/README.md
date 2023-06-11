# hash简介

Hash函数也叫杂凑函数、散列函数、哈希函数，可以把消息或数据压缩成固定长度的摘要

## 性质
- 等长性：给出任意的输入，得到的输出（摘要）长度不变。比如sha-1得到的摘要固定是160位，md5为128位
- 单向性：任给x，得到$y=h(x)$是容易的，任给$y=h(x)$，得到x是困难的
- 抗弱碰撞性：已知x，找到不与x相等的y满足$h(x)=h(y)$是不可行的
- 抗强碰撞性：找到任意两个不同的输入$x，y$使得$h(x)=h(y)$是不可行的

## 哈希碰撞

![](https://gls.show/img/202306112258191.png)

- 根据抗碰撞性，不会有两个不同的输入，使得哈希运算后的输出相等
- 如果找到反例，比如上图中John和Sandra的输出相等，则称产生了哈希碰撞

## 空间分析
16个二进制位的哈希值，产生碰撞的可能性是 65536 分之一。也就是说，如果有65537个用户，就一定会产生碰撞。哈希值的长度扩大到32个二进制位，碰撞的可能性就会下降到 4,294,967,296 分之一。

## 生日悖论
在一个房间中，如果有23人，则存在两个人的生日相同的概率要大于50%。这个“悖论”并非是逻辑中的悖论，而是与直观感觉相悖，因此被称为生日悖论。当人数增加时，该概率也会增加。若人数为50，则存在两个人的生日相同的概率要大于97%。

n个人中，每个人的生日日期都不同的概率：
$$
 {\bar {p}}(n)=1\cdot \left(1-{\frac {1}{365}}\right)\cdot \left(1-{\frac {2}{365}}\right)\cdots \left(1-{\frac {n-1}{365}}\right)={\frac {365}{365}}\cdot {\frac {364}{365}}\cdot {\frac {363}{365}}\cdot {\frac {362}{365}}\cdots {\frac {365-n+1}{365}} 
$$
p(n）表示n个人中至少2人生日相同的概率

$$
 p(n)=1-{\bar {p}}(n)=1-{365! \over 365^{n}(365-n)!} 
$$
 ## 哈希碰撞


- 因此，由上可知，如果哈希值的取值空间是365，只要计算23个哈希值，就有50%的可能产生碰撞
- 为了防止哈希碰撞，需要增大哈希映射的空间

# lab setup

首先，配置和安装 openssl 库

进入openssl文件夹，执行make命令

```
# cd /home/seed/openssl-1.0.1
# make 
# make test 
# make install 
```

安装十六进制编辑器，以便于查看和修改二进制格式的文件。预制的vm中已经为我们安装了GHex

# Task 1: Generating Message Digest and MAC

给文件file.txt写入值

```shell
[06/11/2023 07:51] seed@ubuntu:~$ echo 1 > file.txt 
[06/11/2023 07:51] seed@ubuntu:~$ cat file.txt 
1
[06/11/2023 07:52] seed@ubuntu:~$ 

```

使用不同算法计算哈希值

```shell
[06/11/2023 07:52] seed@ubuntu:~$ openssl dgst -md5 file.txt 
MD5(file.txt)= b026324c6904b2a9cb4b88d6d61c81d1
[06/11/2023 07:52] seed@ubuntu:~$ openssl dgst -sha1 file.txt 
SHA1(file.txt)= e5fa44f2b31c1fb553b6021e7360d07d5d91ff5e
[06/11/2023 07:52] seed@ubuntu:~$ openssl dgst -sha256 file.txt 
SHA256(file.txt)= 4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865
```

| 哈希算法 |  文件名  |                            哈希值                            |
| :------: | :------: | :----------------------------------------------------------: |
|   MD5    | file.txt |               b026324c6904b2a9cb4b88d6d61c81d1               |
|   SHA1   | file.txt |           e5fa44f2b31c1fb553b6021e7360d07d5d91ff5e           |
|  SHA256  | file.txt | 4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865 |

# Task 2: Keyed Hash and HMAC

为一个文件生成一个含有秘钥的哈希(即 MAC)可以使用-hmac 选项(这个选项目前没有文档说明，但是 openssl 支持它)

下面的示例使用 HMAC-MD5算法为文件生成键控哈希。- hmac 选项后面的字符串是密钥

```shell
$ openssl dgst -md5 -hmac "abcdefg" filename
```

> 在 HMAC 我们一定要用固定大小的钥匙吗？如果是，密钥大小是多少？如果没有，为什么？

尝试：

```shell
[06/11/2023 07:55] seed@ubuntu:~$ openssl dgst -hmac 'aaaa' -md5 file.txt
HMAC-MD5(file.txt)= abc3af6de7e79bcf63c145b691486b3a
[06/11/2023 08:07] seed@ubuntu:~$ openssl dgst -hmac 'aaaaaaaa' -md5 file.txt
HMAC-MD5(file.txt)= 3e6883461e3b7962e28680120799b618
[06/11/2023 08:07] seed@ubuntu:~$ openssl dgst -hmac 'a' -md5 file.txt
HMAC-MD5(file.txt)= 47105645a0aafa20383f8ed98841e11d
[06/11/2023 08:08] seed@ubuntu:~$ openssl dgst -hmac '' -md5 file.txt
HMAC-MD5(file.txt)= 6f1b585dfcb5ef68c77a4dd227a6dd45
[06/11/2023 08:08] seed@ubuntu:~$ openssl dgst -hmac ' ' -md5 file.txt
HMAC-MD5(file.txt)= 6600036474432850121f816e2f570295
```

这说明不是必须具有固定的大小，密钥长度可以是任意的

为了解释这个结果，我们需要深入一下hmac的原理

hmac，也即是基于哈希函数和密钥的消息认证码（Hash-based Message Authentication Code），可以用来验证消息的完整性和真实性

HMAC的计算过程如下

1. 如果密钥K的长度大于哈希函数的分组长度，就用哈希函数对密钥K进行哈希运算，得到一个新的密钥K'，并用0填充到分组长度。
2. 如果密钥K的长度小于哈希函数的分组长度，就用0填充到分组长度，得到一个新的密钥K'。
3. 用密钥K'和一个常量ipad（00110110）进行异或运算，得到一个内部密钥Ki。
4. 用密钥K'和一个常量opad（01011100）进行异或运算，得到一个外部密钥Ko。
5. 用内部密钥Ki和消息M进行连接，然后用哈希函数对连接结果进行哈希运算，得到一个中间结果H(Ki∣M)。
6. 用外部密钥Ko和中间结果H(Ki∣M)进行连接，然后用哈希函数对连接结果进行哈希运算，得到最终的HMAC值H(Ko∣H(Ki∣M))。

HMAC可以使用不同的哈希函数，例如SHA-1、SHA-256等，生成的HMAC值的长度与哈希函数的输出长度相同

# Task 3: The Randomness of One-way Hash

现在我们有一个文件，使用ghex观察bit为`310A`

先计算md5

修改为`110A`

再次执行计算

```shell
[06/11/2023 08:16] root@ubuntu:/home/seed# openssl dgst -md5 file.txt 
MD5(file.txt)= b026324c6904b2a9cb4b88d6d61c81d1

[06/11/2023 08:17] root@ubuntu:/home/seed# openssl dgst -md5 file.txt 
MD5(file.txt)= fb053f5c08e90c03a38b470476a61991
```

写个脚本计算下

```python
def compare_strings(a, b):
  # 如果两个字符串为空，就返回0
  if a is None or b is None:
    return 0
  # 如果两个字符串长度不等，就返回0
  if len(a) != len(b):
    return 0
  # 初始化计数器
  count = 0
  # 使用for循环遍历两个字符串的每个字符
  for i in range(len(a)):
    # 如果字符相等，就把计数器加一
    if a[i] == b[i]:
      count += 1
  # 返回计数器的值
  return count

# 测试代码
a = "b026324c6904b2a9cb4b88d6d61c81d1"
b = "fb053f5c08e90c03a38b470476a61991"
print(compare_strings(a, b))

print(len(a)-compare_strings(a, b))
```

命令行

```shell
philo@DESKTOP-0MMJKEF:~$ vim 1.py
philo@DESKTOP-0MMJKEF:~$ sudo chmod +x 1.py
[sudo] password for philo:'
philo@DESKTOP-0MMJKEF:~$ python3 ./1.py
5 
27
philo@DESKTOP-0MMJKEF:~$
```

发现即使仅仅改变了一个bit，生成的哈希值也有很大的改变

# Task 4: One-Way Property versus Collision-Free Property

**散列函数的单向性和无碰撞性的蛮力破解**

散列函数是一种将任意长度的输入映射到固定长度的输出的函数，通常用于验证消息的完整性和真实性。散列函数具有两个重要的属性：

- 单向性：给定一个散列值，很难找到一个输入，使得散列函数对该输入产生该散列值。
- 无碰撞性：很难找到两个不同的输入，使得散列函数对它们产生相同的散列值。

在这个任务中，我们将使用蛮力方法来探究破坏这两个属性所需的时间。蛮力方法就是尝试所有可能的输入，直到找到一个满足条件的输入为止。

为了完成这个任务，我们需要使用 C 语言编写程序，调用 openssl 加密库中的消息摘要函数。消息摘要函数是一种常用的散列函数，例如 MD5 和 SHA-1。我们可以从 http://www.openssl.org/docs/crypto/evp_digestinit.html 中找到一个示例代码，我们需要熟悉这个示例代码，并根据我们的需求进行修改。

由于现代的散列函数都设计得很强大，对它们进行蛮力破解需要花费很长的时间。为了简化任务，我们将只使用散列值的前 24 位。也就是说，我们相当于使用了一个修改过的散列函数，它的输出长度只有 24 位。请注意，这样做会降低散列函数的安全性，并不适用于实际场景。

请设计并实现一个实验，回答以下问题：

1. 用蛮力法破坏单向性需要多少次试验？你应该多次重复你的实验，并报告你的平均试验次数。
2. 用蛮力法破坏无碰撞性需要多少次试验？同样，你应该报告你的平均试验次数。
3. 根据你的观察，使用蛮力法更容易破坏哪个属性？
4. (10个加分)你能用数学方法解释你观察到的差异吗？