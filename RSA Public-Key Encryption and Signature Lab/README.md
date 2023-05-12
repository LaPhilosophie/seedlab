![](https://gls.show/image/seed/rsaflow.jpg)

# 写在lab之前

## RSA算法的数学解释

-   随机选择两个不相同的素数 $p, q$ 。
-   $p, q$ 将相乘, 为 $n=p \times q$ 。
-   计算 $\mathrm{n}$ 的欧拉函数 $\varphi(\mathrm{n})$, 欧拉函数证明, 当 $p, q$ 为不相同的素数时,$\varphi(n)=(p-1)(q-1) 。$
-   随机选择一个整数 $\mathrm{e}$, 满足两个条件: $\varphi(n)$ 与 $\mathrm{e}$ 质,且 $1<e<\varphi(n)$ 。
-   计算 $\mathrm{e}$ 对于 $p(n)$ 的逆元 $\mathrm{d}$, 也就是说找到一个 $\mathrm{d}$ 满足 $e d=1 \bmod \varphi(n)$ 。
-   这个式子等价于 $e d-1=k \varphi(n)$, 实际上就是对于方程 $e d-k \varphi(n)=1$求 $(d, k)$ 的整数解。这个方程可以用扩展欧几里得算法求解。
-   最终把 $(e, n)$ 封装成公钥， $(d, n)$ 封装成私钥。
-   **公钥加密，私钥解密**
-   **私钥签名，公钥验证**

对于大数运算，不能使用C语言基本的数据类型，而是要使用 openssl 提供的 big num library，我们将把每个大数定义为一个 BIGNUM 类型，然后使用库提供的 API 进行各种操作，如加法、乘法、求幂、模块化操作等，big number APIs可以在 https://linux.die.net/man/3/bn 找到

`BN_CTX` 是一个结构体，用于存储大数运算时用到的临时变量。通过动态分配内存方式创建` BIGNUM `的开销比较大，所以使用 `BN_CTX `来提高效率。

代码的具体实现：

```c
//动态分配并初始化一个 BIGNUM 结构体
BIGNUM *BN_new(void);

//ossl_typ.h
struct bignum_st {
         BN_ULONG *d;
         int top;
         int dmax;
         int neg;
         int flags;
};

// bn_lcl.h 
typedef struct bignum_st BIGNUM;
```

-   编译命令

```c
gcc cr.c -lcrypto
```

-   使用`BIGNUM`库进行很多位的大数操作

    -   模意义下的幂运算

        ```c
        BN_mod_exp(res, a, c, n, ctx)
        ```

    -   两个大数相乘

        ```c
        //Compute res = a ∗ b. It should be noted that a BN CTX structure is need in this API
        BN_mul(res, a, b, ctx)
        ```

    -   求逆元

        ```c
        //Compute modular inverse, i.e., given a, find b, such that a ∗ b mod n = 1. The value b is called the inverse of a, with respect to modular n
        BN_mod_inverse(b, a, n, ctx)
        ```
在labPDF中，有着更多的使用范例

# 3.1 Task 1: Deriving the Private Key
设 p，q，e 是三个质数。设 n = p * q。我们将使用(e，n)作为公钥，请计算私钥 d。p、q 和 e 的十六进制值如下所示

```
p = F7E75FDC469067FFDC4E847C51F452DF
q = E85CED54AF57E53E092113E62F436F4F
e = 0D88C3
```
原理可见前面**RSA算法的数学解释**

```c
#include <stdio.h>
#include <openssl/bn.h>

#define NBITS 256

int main ()
{
    BN_CTX *ctx = BN_CTX_new();
    BIGNUM *p = BN_new();
    BIGNUM *p_1 = BN_new();
    BIGNUM *q = BN_new();
    BIGNUM *q_1 = BN_new();
    BIGNUM *e = BN_new();
    BIGNUM *d = BN_new();
    BIGNUM *n = BN_new();
    BIGNUM *phi_n = BN_new();
    BIGNUM *res = BN_new();
    BIGNUM *one = BN_new();
    //对p、q、e进行赋值
    BN_hex2bn(&p, "F7E75FDC469067FFDC4E847C51F452DF");
    BN_hex2bn(&q, "E85CED54AF57E53E092113E62F436F4F");
    BN_hex2bn(&e, "0D88C3");
    BN_hex2bn(&one, "1");
    //计算得出p*q=n
    BN_mul(n, p, q, ctx);
    //phi_n=(p-1)*(q-1)
    BN_sub(p_1, p, one);
    BN_sub(q_1, q, one);
    BN_mul(phi_n,p_1, q_1, ctx);
    BN_mod_inverse(d, e, phi_n, ctx);

    //print function
    char * number_str = BN_bn2hex(n);
    char * number_str1 = BN_bn2hex(d);
    printf("p * q =  %s\n , d = %s \n", number_str,number_str1);
    
    //free 
    OPENSSL_free(number_str);
    OPENSSL_free(number_str1);
    
    BN_clear_free(p);
    BN_clear_free(q);
    BN_clear_free(e);
    BN_clear_free(d);
    BN_clear_free(n);
    BN_clear_free(p_1);
    BN_clear_free(q_1);
    BN_clear_free(phi_n);
    BN_clear_free(res);
    BN_clear_free(one);
    
    return 0;
}

```

# Task 2: Encrypting a Message

- (e, n)是公钥
- 加密消息"A top secret!"
- ASCII转化为hex字符串，再通过BN_hex2bn() API将字符串再转化为BIGNUM

python 命令将十六进制字符串转换回纯 ASCII 字符串:
```shell
$ python -c ’print("4120746f702073656372657421".decode("hex"))’

A top secret!
```

python 命令将纯 ASCII 字符串转换回十六进制字符串:
```shell
$ python -c ’print("A top secret!".encode("hex"))’

4120746f702073656372657421
```

给出的参数：
```
n = DCBFFE3E51F62E09CE7032E2677A78946A849DC4CDDE3A4D0CB81629242FB1A5
e = 010001 (this hex value equals to decimal 65537)
M = A top secret!
d = 74D806F9F3A62BAE331FFE3F0A68AFE35B3D2E4794148AACBC26AA381CD7D30D
```

**思路**

-   ASCII 字符串转换为十六进制字符，再转换为`BIGNUM`类型

-   使用d可以验证结果正确

**代码**

```c
#include <stdio.h>
#include <openssl/bn.h>

#define NBITS 256

int main ()
{
    BN_CTX *ctx = BN_CTX_new();
    BIGNUM *e = BN_new();
    BIGNUM *d = BN_new();
    BIGNUM *n = BN_new();
    BIGNUM *M = BN_new();
    BIGNUM *C = BN_new();
    BIGNUM *res = BN_new();
    
    //对p、q、e M进行赋值
    BN_hex2bn(&n, "DCBFFE3E51F62E09CE7032E2677A78946A849DC4CDDE3A4D0CB81629242FB1A5");
    BN_hex2bn(&e, "010001");
    BN_hex2bn(&d, "74D806F9F3A62BAE331FFE3F0A68AFE35B3D2E4794148AACBC26AA381CD7D30D");
    BN_hex2bn(&M, "4120746f702073656372657421");
	
    //C = M^e mod n
    BN_mod_exp(C, M, e, n, ctx);
    BN_mod_exp(res, C, d, n, ctx);
    
    //print function
    char * number_str = BN_bn2hex(C);
    printf("C = %s \n", number_str);
    
    char * number_str1 = BN_bn2hex(res);
    printf("C^d = %s \n", number_str1);
    
    printf("      4120746f702073656372657421 == \"A top secret \"\n");

    //free 
    OPENSSL_free(number_str);
    OPENSSL_free(number_str1);
    
    BN_clear_free(e);
    BN_clear_free(d);
    BN_clear_free(n);
    BN_clear_free(M);
    BN_clear_free(C);
    BN_clear_free(res);
    return 0;
}
```

# Task 3: Decrypting a Message
使用的公钥/私钥与 Task 2中使用的公钥/私钥相同,解密以下密文 C，并将其转换回纯 ASCII 字符串
```
C = 8C0F971DF2F3672B28811407E2DABBE1DA0FEBBBDFC7DCB67396567EA1E2493F
```


**思路**

-   RSA运算之后使用python解码得出信息

**代码**

```c
#include <stdio.h>
#include <openssl/bn.h>

#define NBITS 256

int main ()
{
    BN_CTX *ctx = BN_CTX_new();
    BIGNUM *e = BN_new();
    BIGNUM *d = BN_new();
    BIGNUM *n = BN_new();
    BIGNUM *M = BN_new();
    BIGNUM *C = BN_new();
    BIGNUM *res = BN_new();
    
    //对p、q、e M进行赋值
    BN_hex2bn(&n, "DCBFFE3E51F62E09CE7032E2677A78946A849DC4CDDE3A4D0CB81629242FB1A5");
    BN_hex2bn(&e, "010001");
    BN_hex2bn(&d, "74D806F9F3A62BAE331FFE3F0A68AFE35B3D2E4794148AACBC26AA381CD7D30D");
    BN_hex2bn(&M, "4120746f702073656372657421");
    BN_hex2bn(&C, "8C0F971DF2F3672B28811407E2DABBE1DA0FEBBBDFC7DCB67396567EA1E2493F");
    
    //M = C^d mod n
    BN_mod_exp(res, C, d, n, ctx);
    
    //print function
    char * number_str = BN_bn2hex(res);
    printf("M = %s \n", number_str);
    
    //free 
    OPENSSL_free(number_str);
    
    BN_clear_free(e);
    BN_clear_free(d);
    BN_clear_free(n);
    BN_clear_free(M);
    BN_clear_free(C);
    BN_clear_free(res);
    return 0;
}
```


# Task 4: Signing a Message

使用的公钥/私钥与 Task 2中使用的公钥/私钥相同

字符串的hex编码是`49206f776520796f75202432303030`


-   按照上述RSA法则进行编程
-   M1 = I owe you $2000，对应结果为`80A55421D72345AC199836F60D51DC9594E2BDB4AE20C804823FB71660DE7B82 `

-   M2 = I owe you $3000，对应结果为` 04FC9C53ED7BBE4ED4BE2C24B0BDF7184B96290B4ED4E3959F58E94B1ECEA2EB`
-   虽然明文只是改变一个字符，但是签名结果迥然不同

# Task 5: Verifying a Signature

S 是 M 的签名，Alice 的公钥是(e，n)，验证签名是否确实是爱丽丝的


```
M = Launch a missile.
S = 643D6F34902D9C7EC90CB0B2BCA36C47FA37165C0005CAB026C0542CBDB6802F
e = 010001 (this hex value equals to decimal 65537)
n = AE1CD4DC432798D933779FBD46C6E1247F0CF1233595113AA51B450F18116115
```


转码得到对应hex为`4c61756e63682061206d697373696c65`

-   **签名过程：私钥签名，公钥验证**

-   与运行截图比对，$S^e$的结果与明文相同，因此签名有效

# Task 6: Manually Verifying an X.509 Certificate

使用程序手动验证 X.509证书。X.509包含有关公钥的数据和数据上的发行者签名。我们将从 Web 服务器下载一个真正的 X.509证书，获取其发行者的公钥，然后使用这个公钥验证证书上的签名

证书生成过程：
- 首先 CA 会把持有者的公钥、用途、颁发者、有效时间等信息打成一个包，然后对这些信息进行 Hash 计算，得到一个 Hash 值；
- 然后 CA 会使用自己的私钥将该 Hash 值加密，生成 Certificate Signature，也就是 CA 对证书做了签名；
- 最后将 Certificate Signature 添加在文件证书上，形成数字证书；


## Step 1: Download a certificate from a real web server.
显示证书
```shell
$ openssl s_client -connect www.example.org:443 -showcerts
```
打印出来的"Begin CERTIFICATE" 和 "END CERTIFICATE"之间的就是证书，将其手动保存在`.pem`文件中
- 如果有两个证书，第一个是网站的证书，保存为c0.pem，第二个是中间CA的证书，保存为c1.pem
- 如果只有一个证书，那么说明网站的证书是根 CA 签名的

- 使用 -modulus 选项显示 n
```shell
For modulus (n):
$ openssl x509 -in c1.pem -noout -modulus
```

```c
Modulus=C70E6C3F23937FCC70A59D20C30E533F7EC04EC29849CA47D523EF03348574C8A3022E465C0B7DC9889D4F8BF0F89C6C8C5535DBBFF2B3EAFBE356E74A46D91322CA36D59BC1A8E3964393F20CBCE6F9E6E899C86348787F5736691A191D5AD1D47DC29CD47FE18012AE7AEA88EA57D8CA0A0A3A1249A262197A0D24F737EBB473927B05239B12B5CEEB29DFA41402B901A5D4A69C436488DEF87EFEE3F51EE5FEDCA3A8E46631D94C25E918B9895909AEE99D1C6D370F4A1E352028E2AFD4218B01C445AD6E2B63AB926B610A4D20ED73BA7CCEFE16B5DB9F80F0D68B6CD908794A4F7865DA92BCBE35F9B3C4F927804EFF9652E60220E10773E95D2BBDB2F1
```

## Step 2: Extract the public key (e, n) from the issuer’s certificate. 求出公钥 （e,n）

## Step 3: Extract the signature from the server’s certificate.从服务器的证书中提取签名
- 没有特别的命令打印出 e 和 签名信息，可以打印出证书所有的信息然后在文本中查找
- 签名中会有空格和`:`，可以使用脚本cat signature | tr -d ’[:space:]:’除去，得到纯十六进制字符串
```
Print out all the fields, find the exponent (e) and signature:
$ openssl x509 -in c1.pem -text -noout
```


## Step 4: 提取服务器证书的正文

```c
$ openssl asn1parse -i -in c0.pem -strparse 4 -out c0_body.bin -noout
```

计算sha值

```c
$ sha256sum c0_body.bin
```

## Step 5:Verify the signature. 

可以看出两段`sha`值相等，都是`837D0252E9D5F45DEC8294152267D7097B387AA178549B65CD6FB2F307013E1C`，因此可以验证，证书有效

**代码**

-   求出$S^e$

```c
#include <stdio.h>
#include <openssl/bn.h>

#define NBITS 256

int main ()
{
    BN_CTX *ctx = BN_CTX_new();
    BIGNUM *e = BN_new();
    BIGNUM *n = BN_new();
    BIGNUM *S = BN_new();
    BIGNUM *res = BN_new();
    //赋值
    BN_hex2bn(&n,	      "C70E6C3F23937FCC70A59D20C30E533F7EC04EC29849CA47D523EF03348574C8A3022E465C0B7DC9889D4F8BF0F89C6C8C5535DBBFF2B3EAFBE356E74A46D91322CA36D59BC1A8E3964393F20CBCE6F9E6E899C86348787F5736691A191D5AD1D47DC29CD47FE18012AE7AEA88EA57D8CA0A0A3A1249A262197A0D24F737EBB473927B05239B12B5CEEB29DFA41402B901A5D4A69C436488DEF87EFEE3F51EE5FEDCA3A8E46631D94C25E918B9895909AEE99D1C6D370F4A1E352028E2AFD4218B01C445AD6E2B63AB926B610A4D20ED73BA7CCEFE16B5DB9F80F0D68B6CD908794A4F7865DA92BCBE35F9B3C4F927804EFF9652E60220E10773E95D2BBDB2F1");
    BN_hex2bn(&S, "398a004992481658de3e9cce83391bb1ac9a95f956ff7c2d82d3a8365be6cf7dfd4a987248f2b7f652d40b092ca25c3347e29a9b3e97bdd8ba0009c9ae1eb3bcdee81ee1ded905f8a9b03dedb97ab2a93c934e078cf05ecc8bf375336e5582e599429f9c8154fbadce280c3842609568e14d5f832da43276d8511c1d66b79cad1f19f940e47c744ddb2abbeaf3244de9387721523cfd1e811e900084aec866fce3817891d04378992aa485313c9f6bef489e1e394d5107b7534dffe213abe3ca6d7c21f6e2fa2273f465717577da088ef72d5be601c9f7960c5f2da8d73e4c5ec29278e41b4e9b28369f1877f2bbf56a6471780fead5687f1157b4ff0fb0e473");
    BN_hex2bn(&e, "10001");
    
    
    BN_mod_exp(res, S, e, n, ctx);
     
    //print function
    char * number_str = BN_bn2hex(res);
    printf("S^e = %s \n", number_str);
    
    //free 
    OPENSSL_free(number_str);
    
    BN_clear_free(e);
    BN_clear_free(n);
    BN_clear_free(res);
    BN_clear_free(S);
    return 0;
}

```



