# Task 1: Generate Encryption Key in a Wrong Way


-    使用当前时间作为伪随机数生成器的种子，生成一个128位的加密秘钥
-   用 `time`函数来获得系统时间，它的返回值为从` 00:00:00 GMT, January 1, 1970` 到现在所持续的秒数
-   `rand()`配合`srand()`函数进行使用，当`srand()`的参数值固定的时候，`rand()`获得的数也是固定的，它返回一个范围在 0 到 `RAND_MAX `之间的伪随机数。所以一般`srand`的参数用`time(NULL)`，因为系统的时间一直在变，所以`rand()`返回的数，也就一直在变，相当于是随机数

代码：

```c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define KEYSIZE 16
void main()
{
	int i;
	char key[KEYSIZE];
	printf("%lld\n", (long long)time(NULL));
	srand(time(NULL)); 
		for (i = 0; i < KEYSIZE; i++) {
			key[i] = rand() % 256;
			printf("%.2x", (unsigned char)key[i]);
		}
	printf("\n"); 
}
```

输出：
```
$ ./a.out 
1665748464
bba0f21c1012ceb31df267e5b673d461
```

# Task 2: Guessing the Key
已知：
- 加密文件的时间戳是“2018-04-1723:08:49”
- 以时间为基础生成的秘钥可能在前两个小时内生成
- 文件头以% PDF-1.5开头
- 
```
Plaintext: 255044462d312e350a25d0d4c5d80a34
Ciphertext: d06bf9d0dab8e8ef880660d2af65aa82
IV: 09080706050403020100A2B2C2D2E2F2
```

思路：
-   计算出时间的区间，时间作为随机种子
-   使用AES算法进行枚举
-   将结果与目标字符串进行匹配即可
-   **需要着重注意的点：每次执行`AES_cbc_encrypt`后，初始向量会被更新， 所以需要保存初始向量（在这个地方被卡了一次）**

在Linux C中使用OpenSSL的一些前置知识：

```c
# define AES_ENCRYPT     1
# define AES_DECRYPT     0

//设置加密密钥，使用字符缓冲区  
int AES_set_encrypt_key(  
        const unsigned char *userKey,  
        const int bits,  
        AES_KEY *key);  

int AES_set_decrypt_key(  
        const unsigned char *userKey,  
        const int bits,  
        AES_KEY *key);  

void AES_cbc_encrypt(  
        const unsigned char *in,  
        unsigned char *out,  
        const unsigned long length,  
        const AES_KEY *key,  
        unsigned char *ivec,  
        const int enc);  
```


| 参数名称 | 含义                                                     |
| :------- | :------------------------------------------------------- |
| in       | 输入数据，长度任意                                       |
| out      | 输出数据，能够容纳下输入数据，且长度必须是16字节的倍数。 |
| length   | 输入数据的实际长度。                                     |
| key      | 使用AES_set_encrypt_key/AES_set_decrypt_key生成的Key     |
| ivec     | 可读写的一块内存。长度必须是16字节。                     |
| enc      | AES_ENCRYPT 代表加密， AES_DECRYPT代表解密               |

代码：

```c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <openssl/aes.h>
using namespace std;
#define KEYSIZE 16
int main()
{
	int i,j;
	unsigned char encryptText[17];
	const unsigned char plainText[17] = "\x25\x50\x44\x46\x2d\x31\x2e\x35\x0a\x25\xd0\xd4\xc5\xd8\x0a\x34";
	const unsigned char target_str[17] = "\xd0\x6b\xf9\xd0\xda\xb8\xe8\xef\x88\x06\x60\xd2\xaf\x65\xaa\x82";
	int target_time = 1524020929;
	for (i = target_time;i >=target_time - 60*60*2 ; i--)
	{
		srand(i);
		unsigned char key[KEYSIZE];
		unsigned char ini_vec[17] = 		 "\x09\x08\x07\x06\x05\x04\x03\x02\x01\x00\xA2\xB2\xC2\xD2\xE2\xF2";
		for (j = 0; j < KEYSIZE; j++) 
		{
			key[j] = rand() % 256;
			//printf("%.2x", (unsigned char)key[j]);
		}
		AES_KEY enc_key;
		AES_set_encrypt_key(key, 128 , &enc_key);
		AES_cbc_encrypt(plainText, encryptText, 16 , &enc_key, ini_vec, AES_ENCRYPT);     // 加密
		
		int flag=1;
		for(j=0;j<16;j++)
		{
			if(encryptText[j]!=target_str[j])
			{
				flag=0;
				break;
			}
		}
		if(flag)
		{
			cout<<"the key is : ";
			for(j=0;j<16;j++)
			{
				printf("%.2x", (unsigned char)key[j]);
			}
			cout<<endl;
			//return 0;
		}
		
	}
    return 0;
}
```

# Task 3: Measure the Entropy of Kernel

以下行为可以增加熵：

-   移动、点击鼠标
-   敲击键盘
-   切换任务，运行程序

```c
void add_keyboard_randomness(unsigned char scancode);
void add_mouse_randomness(__u32 mouse_data);
void add_interrupt_randomness(int irq);
void add_blkdev_randomness(int major);
```

使用以下命令查找内核当前的熵值（不同于信息论中熵的含义。在这里，它仅仅意味着系统当前有多少位随机数）
```c
watch -n .1 cat /proc/sys/kernel/random/entropy_avail
```

# Task 4: Get Pseudo Random Numbers from /dev/random

## 关于熵：

-   Linux内核采用熵来描述数据的随机性

-   熵是描述系统混乱无序程度的物理量，一个系统的熵越大则说明该系统的有序性越差，即不确定性越大

-   `/dev/random`是阻塞的，`/dev/urandom`是“unblocked”，非阻塞的随机数发生器

**可以使用以下指令获得读取伪随机数并用`hexdump`  打印：**

```shell
$ cat /dev/random | hexdump
```


-   如果不作任何操作，熵值缓慢稳定增加（或许是因为时间）

-   如果点击鼠标或者操作键盘等操作，熵值快速增加 

-   触发产生随机数的熵值大小为为 64，每次产生一个随机数都会降低熵值，如果低于64则不会产生随机数

-   若熵池空了，对`/dev/random`的读操作将会被阻塞，直到收集到了足够的环境噪声为止

## Q：a server uses /dev/random to generate the random session key with a client. Please escribe how you can launch a Denial-Of-Service (DOS) attack on such a server.

-   使得server无法产生序列号即可
-   `/dev/random`如果没有足够的可用熵，读取将在某些系统上阻塞
-   通过创建大量会话 id 快速从系统中降低熵，导致`/dev/random`阻塞，从而使得正常的序列号无法产生

```c
$ head -c 1M /dev/urandom > output.bin
$ ent output.bin
```


-   Entropy = 7.999845 bits per byte.

-   最佳压缩会将这个 1048576 字节文件的大小减少 0%。

-   1048576 个样本的卡方分布为 225.47，随机超过该值的概率为 90.86%。

-   数据字节的算术平均值为 127.4740（127.5 = 随机）
-   Pi 的蒙特卡罗值为 3.145969948（误差 0.14%）
-   序列相关系数为 0.000297（完全不相关 = 0.0）

-   综上，生成随机数质量是很好的


# Task 5: Get Random Numbers from /dev/urandom
Linux 提供了通过`/dev/urandom` 设备访问随机池的另一种方法，只是这个设备不会阻塞。/dev/Random 和`/dev/urandom` 都使用池中的随机数据来生成伪随机数。当熵不足时,`/dev/Random` 将暂停，而`/dev/urandom` 将继续生成新的数字

-   `/dev/urandom`的u指的是“unblocked”，不会被阻塞
-   运行指令，会不断地打印输出

代码：

```c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <openssl/aes.h>

using namespace std;

#define LEN 32 // 256 bits

int main()
{
	unsigned char* key = (unsigned char * ) malloc(sizeof(unsigned char) * LEN);
	FILE * random = fopen("/dev/urandom", "r");
	fread(key, sizeof(unsigned char) * LEN, 1, random);
	fclose(random);
	for(int i=0;i<LEN;i++)
	{
		printf("%.2x",key[i]);
	}
	cout<<endl;
    return 0;
}

```