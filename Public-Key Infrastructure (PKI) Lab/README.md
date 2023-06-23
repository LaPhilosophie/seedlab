# Lab Environment

下载docker文件、解压、运行docker

```shell
$ curl https://seedsecuritylabs.org/Labs_20.04/Files/Crypto_PKI/Labsetup.zip --output pki.zip 

$ unzip pki.zip

$ dcbuild # docker-compose build

$ dcup # docker-compose up
```

这里docker可能没有速度，把/etc/docker/daemon.json文件写入下面的源即可

```
# cat  /etc/docker/daemon.json
{
  "registry-mirrors": [
    "https://hub-mirror.c.163.com",
    "https://mirror.baidubce.com"
  ]
}

```

SEED VM已经提前做好了命令替换 ，可以看到.bashrc有如下代码

```shell
# Commands for for docker 
alias dcbuild='docker-compose build'
alias dcup='docker-compose up'
alias dcdown='docker-compose down'
alias dockps='docker ps --format "{{.ID}}  {{.Names}}"'
docksh() { docker exec -it $1 /bin/bash; }
```

给/etc/hosts文件增加映射

```
10.9.0.80 www.gls23.com
```



# Task 1: Becoming a Certificate Authority (CA)

> 证书颁发机构(CA)是颁发数字证书的可信实体。数字证书证明证书的指定主体对公钥的所有权。许多商业 CA 被视为根 CA; VeriSign 是编写本文时最大的 CA。希望获得商业 CA 颁发的数字证书的用户需要向这些 CA 支付费用。
>
> 在这个实验室，我们需要创建数字证书。我们自己将成为一个根 CA，并为这个 CA 生成一个证书。然后使用这个 CA 为其他 CA (例如服务器)颁发证书。
>
> 与通常由另一个 CA 签名的其他证书不同，根 CA 的证书是自签名的。Root CA 的证书通常预先加载到大多数操作系统、 Web 浏览器和其他依赖 PKI 的软件中。根 CA 的证书是无条件受信任的

##  配置文件 openssl.conf

> 为了使用 OpenSSL 创建证书，必须有一个配置文件。
>
> 配置文件通常有一个扩展名.Cnf。它由三个 OpenSSL 命令使用: ca、 req 和 x509

默认情况下，OpenSSL 使用来自/usr/lib/ssl/OpenSSL.cnf 的配置文件。因为我们需要对这个文件进行更改，所以我们将把它复制到我们的工作目录中，并指示 OpenSSL 使用这个副本。配置文件的[ CA default ]部分显示了我们需要准备的默认设置。

在使用 OpenSSL 生成证书时，可以使用 `-config` 参数来指定要使用的 `openssl.cnf` 配置文件。该参数允许你提供自定义的配置文件路径。

以下是使用 OpenSSL 命令生成证书时指定 `openssl.cnf` 文件的示例命令：

```shell 
openssl req -new -config /path/to/openssl.cnf -keyout private.key -out csr.csr
```

进入/usr/lib/ssl看一下

```shell
[06/21/23]seed@VM:.../ssl$ ll
total 4
lrwxrwxrwx 1 root root   14 Nov 24  2020 certs -> /etc/ssl/certs
drwxr-xr-x 2 root root 4096 Jul 31  2020 misc
lrwxrwxrwx 1 root root   20 Nov 24  2020 openssl.cnf -> /etc/ssl/openssl.cnf
lrwxrwxrwx 1 root root   16 Nov 24  2020 private -> /etc/ssl/private
```

查看一下openssl.cnf文件的内容，很长，我们关注一下配置文件的[ CA default ]部分，它显示了我们需要准备的默认设置

```
####################################################################
[ CA_default ]

dir             = ./demoCA              # Where everything is kept
certs           = $dir/certs            # Where the issued certs are kept
crl_dir         = $dir/crl              # Where the issued crl are kept
database        = $dir/index.txt        # database index file.
unique_subject  = no                    # Set to 'no' to allow creation of
                                        # several certs with same subject.
new_certs_dir   = $dir/newcerts         # default place for new certs.

certificate     = $dir/cacert.pem       # The CA certificate
serial          = $dir/serial           # The current serial number
crlnumber       = $dir/crlnumber        # the current crl number
                                        # must be commented out to leave a V1 CRL
crl             = $dir/crl.pem          # The current CRL
private_key     = $dir/private/cakey.pem# The private key

x509_extensions = usr_cert              # The extensions to add to the cert

# Comment out the following two lines for the "traditional"
# (and highly broken) format.
name_opt        = ca_default            # Subject Name options
cert_opt        = ca_default            # Certificate field options

# Extension copying option: use with caution.
# copy_extensions = copy


```

| 设置            | 值                     | 解释                               |
| --------------- | ---------------------- | ---------------------------------- |
| dir             | ./demoCA               | 所有文件存放的根目录               |
| certs           | $dir/certs             | 存储已颁发证书的目录               |
| crl_dir         | $dir/crl               | 存储已颁发CRL的目录                |
| database        | $dir/index.txt         | 数据库索引文件                     |
| unique_subject  | no                     | 是否允许创建具有相同主题的多个证书 |
| new_certs_dir   | $dir/newcerts          | 存储新创建证书的默认目录           |
| certificate     | $dir/cacert.pem        | CA证书文件                         |
| serial          | $dir/serial            | 当前序列号文件                     |
| crlnumber       | $dir/crlnumber         | 当前CRL号码文件                    |
| crl             | $dir/crl.pem           | 当前CRL文件                        |
| private_key     | $dir/private/cakey.pem | 私钥文件                           |
| x509_extensions | usr_cert               | 添加到证书的扩展                   |
| name_opt        | ca_default             | 主题名称选项                       |
| cert_opt        | ca_default             | 证书字段选项                       |
| copy_extensions | copy                   | 复制扩展选项（当前被注释掉）       |

在家目录下创建工作目录CA，将openssl.cnf复制到此处，删除文件中copy_extensions = copy和unique_subject  = no的注释

将该文件复制到工作目录之后，需要按照配置文件中指定的方式创建几个子目录。我们需要创建几个子目录`mkdir certs crl newcerts`



> 对于 index.txt 文件，只需创建一个空文件。对于serial文件，在文件中放置一个字符串格式的数字(例如1000)。一旦设置了配置文件 openssl.cnf，就可以创建和颁发证书。




创建几个必须的文件

```
[06/21/23]seed@VM:~/demoCA$  mkdir certs crl newcerts
[06/21/23]seed@VM:~/demoCA$ ls
certs  crl  newcerts
[06/21/23]seed@VM:~/demoCA$ echo 1000 > serial
[06/21/23]seed@VM:~/demoCA$ echo '' > index.txt
[06/21/23]seed@VM:~/demoCA$ ls
certs  crl  index.txt  newcerts  serial
[06/21/23]seed@VM:~/demoCA$ ls
certs  crl  index.txt  newcerts  serial

```

如前所述，我们需要为 CA 生成一个自签名证书。这意味着这个 CA 是完全可信的，它的证书将充当根证书。可以运行以下命令为 CA 生成自签名证书:

在命令行中指定主题信息和密码，这样就不会提示您输入任何其他信息。在下面的命令中，我们使用-subj 来设置主题信息，使用-passout pass: dees 来设置 dees 的密码。

```
openssl req -x509 -newkey rsa:4096 -sha256 -days 3650 \
-keyout ca.key -out ca.crt \
-subj "/CN=www.modelCA.com/O=Model CA LTD./C=US" \
-passout pass:dees \
-config ./my_openssl.cnf
```

执行该命令后，将生成一个名为 `ca.key` 的私钥文件和一个名为 `ca.crt` 的自签名根证书文件，可以将该根证书用于签发其他证书

- `-x509`：指定生成自签名的证书，而无需先生成证书请求（CSR）
- `-newkey rsa:4096`：生成一个新的 RSA 4096 位的密钥对。
- `-sha256`：使用 SHA-256 算法进行证书哈希。
- `-days 3650`：证书的有效期为 3650 天（约为 10 年）。
- `-keyout ca.key`：将生成的私钥保存到 `ca.key` 文件。
- `-out ca.crt`：将生成的证书保存到 `ca.crt` 文件。
- `-subj "/CN=www.modelCA.com/O=Model CA LTD./C=US"`：指定证书的主题信息，包括通用名称（Common Name）、组织（Organization）和国家代码（Country Code）。
- `-passout pass:dees`：指定私钥的密码为 "dees"。
- /path/to/your/openssl.cnf 配置文件的路径

```shell
[06/22/23]seed@VM:~/demoCA$ openssl req -x509 -newkey rsa:4096 -sha256 -days 3650 \
> -keyout ca.key -out ca.crt \
> -subj "/CN=www.modelCA.com/O=Model CA LTD./C=US" \
> -passout pass:dees \
> -config 
demoCA/          tmp_openssl.cnf  
> -config ./tmp_openssl.cnf 
Generating a RSA private key
............................................................++++
................++++
writing new private key to 'ca.key'
-----

```

我们可以使用以下命令查看 X509证书的解码内容和 RSA 密钥(- text 意味着将内容解码为纯文本;-noout 意味着不打印编码版本)

```
openssl x509 -in ca.crt -text -noout
openssl rsa -in ca.key -text -noout
```

```shell
[06/21/23]seed@VM:~/.../demoCA$ openssl x509 -in ca.crt -text -noout
Certificate:
    Data:
        Version: 3 (0x2)
        Serial Number:
            36:b9:8a:24:a2:ea:a6:49:b8:a0:53:22:31:68:75:f1:0f:31:34:69
        Signature Algorithm: sha256WithRSAEncryption
        Issuer: CN = www.modelCA.com, O = Model CA LTD., C = US
        Validity
            Not Before: Jun 22 03:45:53 2023 GMT
            Not After : Jun 19 03:45:53 2033 GMT
        Subject: CN = www.modelCA.com, O = Model CA LTD., C = US
        Subject Public Key Info:
            Public Key Algorithm: rsaEncryption
                RSA Public-Key: (4096 bit)
                Modulus:
                    00:eb:fb:ab:8d:c2:3b:65:a7:cd:9a:34:ae:6d:b9:
                    01:49:ff:cb:c5:7d:8f:b5:19:34:41:a7:3f:df:44:
                    56:25:65:ba:92:af:21:16:4b:1f:71:a3:27:1d:04:
                    8a:fc:b0:f3:01:47:e9:26:d2:ab:e7:62:36:ff:82:
                    6b:1a:a8:b3:03:1f:2c:97:a0:bc:d1:17:75:eb:b2:
                    12:f6:2b:94:a1:30:17:45:ab:40:e1:32:31:20:92:
                    27:f9:de:22:bf:32:8f:f0:bf:cb:93:b2:18:a7:75:
                    43:85:ca:cf:ff:75:48:79:e1:9a:19:43:e0:75:1a:
                    10:50:0b:52:6d:a7:0d:bf:a1:db:60:f9:50:82:78:
                    35:23:09:a7:12:ad:6b:78:78:e1:f8:d0:d6:ba:19:
                    f2:ee:fd:a5:5a:16:49:41:e6:f8:68:e5:84:6b:54:
                    ab:a9:ec:a9:22:2e:87:29:0d:62:51:a4:f6:80:49:
                    ca:36:4d:ab:ef:22:83:51:df:d7:5b:9c:14:40:4c:
                    75:42:85:92:ae:9a:4f:93:75:47:7b:e1:a7:09:35:
                    bb:b7:ea:08:2d:0f:e5:df:0d:23:86:9d:33:ef:f5:
                    f5:f4:52:60:4a:16:f0:34:80:bd:66:7a:46:6c:e2:
                    c5:10:11:ee:5e:44:14:00:74:ff:1c:57:70:bd:69:
                    f6:9f:ff:46:8c:48:4e:ea:5e:2c:33:67:fc:e4:ad:
                    c8:57:26:b1:ce:27:ed:4f:72:d3:d0:f7:12:6c:69:
                    ff:af:af:76:57:cb:2b:7e:96:88:12:34:8d:5e:f7:
                    2e:58:a3:96:4e:d5:98:13:ee:91:0c:12:45:97:a8:
                    e8:50:61:2a:c4:b0:ca:29:d5:5c:0c:4f:de:67:99:
                    c9:7c:2d:77:84:f4:41:44:61:5e:99:69:21:fe:7e:
                    e8:0f:06:2a:24:e7:ed:e4:17:37:40:d1:2f:53:99:
                    c3:65:b6:91:a4:3d:86:d9:b3:9c:4b:af:b3:b8:56:
                    c4:d8:83:8c:5b:16:c9:c2:89:de:55:8f:4e:6c:eb:
                    f2:8e:6d:2e:00:7f:6c:9e:f9:6b:63:cc:58:fe:af:
                    6d:33:31:02:8c:5f:6b:d1:1f:2f:f5:79:67:8d:d2:
                    20:33:fc:8e:f7:1c:c5:67:49:bc:bd:c7:e3:c6:e3:
                    ec:52:0c:75:34:f0:1f:2b:15:14:21:8c:91:87:1a:
                    0f:1a:90:f8:16:9b:ed:8e:da:49:ba:33:03:82:6e:
                    ff:c1:cd:6d:c0:b5:4b:42:99:e9:e2:4f:fb:e3:55:
                    30:6e:0d:6e:7d:9c:cb:94:3e:8e:65:60:76:9a:01:
                    24:81:b1:72:61:b3:bc:3e:f2:3b:52:d5:24:29:e3:
                    9f:47:1b
                Exponent: 65537 (0x10001)
        X509v3 extensions:
            X509v3 Subject Key Identifier: 
                AA:E0:4C:38:F1:86:1C:61:27:D8:D4:35:85:21:CB:AF:5F:01:07:71
            X509v3 Authority Key Identifier: 
                keyid:AA:E0:4C:38:F1:86:1C:61:27:D8:D4:35:85:21:CB:AF:5F:01:07:71

            X509v3 Basic Constraints: 
                CA:TRUE
    Signature Algorithm: sha256WithRSAEncryption
         03:c1:de:ef:e1:a4:22:3d:53:5f:5c:9d:ca:4f:80:14:7e:f3:
         e4:11:83:57:85:03:13:86:87:e0:20:39:d8:67:5f:c2:56:3c:
         a3:b0:4a:86:ab:34:21:df:71:8d:32:93:aa:a4:9e:99:36:64:
         ea:d7:13:c7:a4:da:2e:80:31:de:be:ff:64:c4:a0:7a:50:ec:
         7d:79:2b:07:ea:33:a0:f8:96:c4:c9:d7:cc:02:53:ac:43:22:
         ec:7d:33:eb:0d:86:4d:eb:17:c5:d4:cb:65:23:38:ef:6f:a7:
         dd:a2:2f:28:22:b6:2f:a3:56:28:18:69:1b:04:07:0b:06:2a:
         58:6c:96:ba:66:d0:0b:b6:8c:58:7b:8d:82:a6:82:fd:9c:ff:
         a3:08:1a:50:53:d0:4e:f8:5d:18:8f:af:0c:b7:7f:78:34:e9:
         7f:79:4c:8b:62:9d:86:b3:a2:4f:3c:f4:aa:47:b5:72:9d:7c:
         14:fc:be:28:97:3e:b7:cb:7e:19:55:8f:ea:d9:53:b1:e5:67:
         70:09:a7:51:3e:56:e7:d7:43:98:ae:c5:8f:81:92:e5:46:49:
         a9:95:7b:dd:56:5a:f6:97:ed:41:bb:5b:85:9d:56:06:3a:ac:
         47:1b:16:c6:c3:fc:5d:c5:b4:ef:74:bf:68:90:17:11:34:07:
         6d:f8:d8:63:f1:7e:3e:63:03:4d:59:86:5a:3b:32:1b:aa:58:
         df:9d:ee:81:3b:a0:d0:2b:d5:6b:65:0e:05:26:e8:4b:a8:c5:
         f8:57:64:d7:36:5c:20:43:aa:21:1d:f1:2e:cb:e8:d1:f4:8a:
         2c:51:53:1f:3e:3e:29:ca:bc:f7:dd:cd:86:ff:72:93:73:22:
         e9:48:7e:ca:00:92:85:20:e4:e3:06:3d:cb:42:b4:eb:be:85:
         80:d9:d0:ec:59:6c:1e:c7:0a:d0:00:a5:71:07:5e:e7:c6:9b:
         48:42:af:0d:21:bb:1e:87:50:89:ba:5f:cc:3b:a5:59:f1:49:
         ec:64:85:ce:42:f9:1a:f4:52:ff:e9:92:83:64:24:c7:57:4a:
         85:bc:b9:71:1d:4a:9b:3c:c9:b9:29:01:3f:eb:4c:9f:c1:4b:
         d7:8b:da:a1:1b:ba:80:dd:19:62:1b:fb:4f:6d:4f:2a:20:ff:
         ee:f6:11:4d:11:c1:1b:3f:af:82:d3:19:93:82:d8:c6:5c:9c:
         d3:38:58:5a:3f:4d:8d:c5:23:3c:87:fc:bc:b0:86:8c:d5:5f:
         ed:3d:be:26:22:b7:a9:6e:e5:d8:00:49:7c:35:5d:1d:96:b2:
         7e:59:bd:61:17:24:ff:fa:3c:f5:78:bd:1f:a0:7f:b5:4a:02:
         2c:d8:44:37:4c:6d:4b:b0
```



```shell
[06/21/23]seed@VM:~/.../demoCA$ openssl rsa -in ca.key -text -noout
Enter pass phrase for ca.key:
RSA Private-Key: (4096 bit, 2 primes)
modulus:
    00:eb:fb:ab:8d:c2:3b:65:a7:cd:9a:34:ae:6d:b9:
    01:49:ff:cb:c5:7d:8f:b5:19:34:41:a7:3f:df:44:
    56:25:65:ba:92:af:21:16:4b:1f:71:a3:27:1d:04:
    8a:fc:b0:f3:01:47:e9:26:d2:ab:e7:62:36:ff:82:
    6b:1a:a8:b3:03:1f:2c:97:a0:bc:d1:17:75:eb:b2:
    12:f6:2b:94:a1:30:17:45:ab:40:e1:32:31:20:92:
    27:f9:de:22:bf:32:8f:f0:bf:cb:93:b2:18:a7:75:
    43:85:ca:cf:ff:75:48:79:e1:9a:19:43:e0:75:1a:
    10:50:0b:52:6d:a7:0d:bf:a1:db:60:f9:50:82:78:
    35:23:09:a7:12:ad:6b:78:78:e1:f8:d0:d6:ba:19:
    f2:ee:fd:a5:5a:16:49:41:e6:f8:68:e5:84:6b:54:
    ab:a9:ec:a9:22:2e:87:29:0d:62:51:a4:f6:80:49:
    ca:36:4d:ab:ef:22:83:51:df:d7:5b:9c:14:40:4c:
    75:42:85:92:ae:9a:4f:93:75:47:7b:e1:a7:09:35:
    bb:b7:ea:08:2d:0f:e5:df:0d:23:86:9d:33:ef:f5:
    f5:f4:52:60:4a:16:f0:34:80:bd:66:7a:46:6c:e2:
    c5:10:11:ee:5e:44:14:00:74:ff:1c:57:70:bd:69:
    f6:9f:ff:46:8c:48:4e:ea:5e:2c:33:67:fc:e4:ad:
    c8:57:26:b1:ce:27:ed:4f:72:d3:d0:f7:12:6c:69:
    ff:af:af:76:57:cb:2b:7e:96:88:12:34:8d:5e:f7:
    2e:58:a3:96:4e:d5:98:13:ee:91:0c:12:45:97:a8:
    e8:50:61:2a:c4:b0:ca:29:d5:5c:0c:4f:de:67:99:
    c9:7c:2d:77:84:f4:41:44:61:5e:99:69:21:fe:7e:
    e8:0f:06:2a:24:e7:ed:e4:17:37:40:d1:2f:53:99:
    c3:65:b6:91:a4:3d:86:d9:b3:9c:4b:af:b3:b8:56:
    c4:d8:83:8c:5b:16:c9:c2:89:de:55:8f:4e:6c:eb:
    f2:8e:6d:2e:00:7f:6c:9e:f9:6b:63:cc:58:fe:af:
    6d:33:31:02:8c:5f:6b:d1:1f:2f:f5:79:67:8d:d2:
    20:33:fc:8e:f7:1c:c5:67:49:bc:bd:c7:e3:c6:e3:
    ec:52:0c:75:34:f0:1f:2b:15:14:21:8c:91:87:1a:
    0f:1a:90:f8:16:9b:ed:8e:da:49:ba:33:03:82:6e:
    ff:c1:cd:6d:c0:b5:4b:42:99:e9:e2:4f:fb:e3:55:
    30:6e:0d:6e:7d:9c:cb:94:3e:8e:65:60:76:9a:01:
    24:81:b1:72:61:b3:bc:3e:f2:3b:52:d5:24:29:e3:
    9f:47:1b
publicExponent: 65537 (0x10001)
privateExponent:
    5d:f2:8c:c2:db:ff:df:a1:a5:85:ed:d1:3f:97:76:
    be:ea:1a:4a:de:89:16:d5:18:eb:c6:54:f4:62:f5:
    54:e0:22:1e:01:a0:cf:8a:4a:d3:67:db:cb:7e:a2:
    82:a5:43:a9:4f:e2:af:75:11:c1:05:65:d5:e5:2b:
    14:aa:f2:d1:9c:58:99:69:01:a2:d0:8f:3e:ad:5f:
    45:27:e6:7d:21:73:32:66:52:67:15:1f:5f:d3:30:
    1d:16:e5:88:6e:ed:c5:2f:e6:31:3f:a6:f7:0c:05:
    3c:bf:98:7d:20:49:21:54:c2:8f:aa:69:32:d5:94:
    86:f9:6a:f0:82:a0:43:99:81:88:22:d9:7d:87:b3:
    c7:e6:30:e0:8b:b0:0c:7f:3b:9f:5e:2d:0e:5c:04:
    4e:47:26:cc:2d:b1:2e:8e:70:78:fa:5e:f4:87:f9:
    eb:a5:6f:54:4f:67:b9:dd:3d:36:39:d1:75:13:6b:
    70:a1:0d:81:1f:a1:5e:38:1a:39:bb:72:88:82:a7:
    f0:3a:d3:41:b5:e7:56:52:4b:8a:33:34:d7:c2:cf:
    a0:11:88:fd:bf:a5:89:5e:66:b5:51:e2:7a:76:d6:
    5b:55:6c:46:32:c9:a0:6d:5c:79:ee:d6:18:c5:53:
    24:e9:ae:97:52:cd:0f:bd:84:4e:d9:34:e6:03:c8:
    8d:f8:31:0c:52:b8:38:73:f3:62:74:e2:38:72:cb:
    fe:00:7a:47:8e:a0:6a:8e:1a:57:1d:d6:40:2b:54:
    2f:e8:d3:2d:be:d0:e4:3e:f4:ed:c8:2a:df:af:b7:
    a0:47:75:fe:8d:19:13:4f:30:d9:23:d1:86:9a:9f:
    d5:db:45:43:9f:fb:fa:c6:f3:2b:44:02:98:f6:e8:
    a8:12:af:c1:c0:9d:24:53:72:c6:29:ac:b0:4f:9c:
    a6:37:2e:9f:5d:f9:d9:1a:6c:7b:b6:b0:13:35:63:
    a7:f0:d3:ac:be:da:ab:c6:19:49:d5:eb:c3:6c:42:
    f8:5e:46:96:a7:7e:6f:33:2f:c4:60:20:f2:18:c4:
    cc:c6:24:da:bc:6c:19:1c:c7:bf:07:e9:b0:8a:44:
    a0:4d:6b:3d:42:84:b6:b6:3b:bc:16:24:0c:79:e8:
    0c:0b:77:29:7f:90:0c:e1:e6:2d:a6:d1:3a:41:8e:
    f6:f2:b9:74:91:aa:ea:fc:d2:45:2e:ed:46:21:8c:
    1c:3b:22:73:75:c7:d1:b6:f1:9f:0d:80:c0:30:af:
    58:4d:4d:43:38:5c:36:ff:ba:0a:bc:70:be:12:66:
    81:09:68:b2:53:92:f6:3f:1c:25:19:2b:b9:f5:e2:
    0e:a8:54:d3:90:73:50:52:6c:d0:96:f3:ce:30:7a:
    dd:19
prime1:
    00:ff:1d:5f:ca:d9:83:4a:b1:26:b2:82:69:0d:77:
    2c:c1:ed:99:84:40:22:f0:63:33:e0:ff:94:7f:c0:
    2c:aa:41:2f:a9:b7:25:df:06:e2:1d:30:f1:c1:8c:
    02:12:0d:38:61:94:f3:de:c8:20:56:0f:a9:ea:61:
    82:9b:06:25:8c:38:33:e1:fa:e0:b9:54:76:12:2d:
    27:c5:6b:8c:29:c9:ee:06:ed:1f:6b:d0:8a:b2:56:
    3f:27:ec:13:bf:58:c7:2c:57:80:56:d7:9c:e2:a2:
    e1:e9:73:1b:3e:95:e0:35:7f:c5:4a:20:e7:53:59:
    49:b9:85:fe:9e:05:fa:89:f0:5d:35:5a:d1:c3:5a:
    1d:d9:6b:a5:3b:2f:11:0b:4b:71:42:6c:7e:df:8b:
    4c:d1:38:37:87:b4:99:88:7b:e2:3c:c5:18:f1:7d:
    6c:01:b7:9f:39:ab:b0:14:93:fe:4a:d2:a1:37:f5:
    11:f4:a7:68:24:a6:1e:96:b6:ed:5d:85:1f:bc:df:
    eb:11:a3:1a:8b:98:a9:dc:4a:ab:19:a1:4f:12:02:
    cc:35:58:92:ca:5b:e3:e9:f3:7d:cf:25:ac:7a:cb:
    ab:46:32:4d:e6:75:01:03:5f:63:cb:1f:96:2d:4e:
    ad:a7:48:52:03:4d:83:80:b2:84:58:30:df:bd:59:
    22:0d
prime2:
    00:ec:cd:4c:fd:33:fc:4f:e4:c2:13:df:0a:79:eb:
    4a:a1:70:9e:52:c4:bd:74:4f:d9:8d:ff:f2:20:62:
    dc:86:09:2f:37:01:39:54:8f:84:02:bf:f3:01:e4:
    b4:8e:14:3d:1c:5d:f0:7b:48:11:a1:b6:54:ec:7d:
    98:79:a6:23:7b:6d:9e:c0:fa:10:cc:cf:2f:89:11:
    08:16:37:c9:7e:8b:1b:d4:71:3e:ae:ba:1d:76:46:
    6b:72:25:b5:6e:8c:63:b5:00:5d:6f:8f:c0:00:a8:
    76:ce:40:5c:53:ca:2e:ac:c6:9a:84:cc:72:9a:96:
    3e:27:3c:cc:3f:41:c2:13:bd:6c:f2:37:cb:c4:e3:
    8b:3a:03:a2:bb:ab:e4:8f:53:0f:1a:b3:2a:16:2e:
    b3:7f:de:a2:9d:0a:93:77:8e:6c:90:97:cb:7c:9e:
    71:e0:4c:56:f7:ae:24:f6:8c:a5:7f:41:96:5c:cd:
    4c:c5:dc:8b:54:46:30:10:61:85:5f:e5:a0:7d:78:
    fd:fe:90:9a:ec:b1:3f:8c:37:1a:a5:7d:07:75:fe:
    41:30:0f:ea:44:db:24:ea:7d:51:f1:c4:1b:40:46:
    18:3a:ed:c5:e1:1f:ea:4f:70:f0:84:7c:24:3d:5e:
    f6:72:27:ab:10:fb:63:03:1e:e8:55:ad:fa:44:cc:
    4b:c7
exponent1:
    00:ec:dd:a8:20:5e:7e:91:6e:13:e0:f0:46:7b:d3:
    28:02:53:0a:13:89:bd:26:f6:e4:a7:46:85:e9:6b:
    53:cd:2c:43:05:cf:df:e0:c8:b2:4e:aa:2f:fd:25:
    72:92:b2:25:a4:2c:b9:95:22:b9:2b:4e:d5:d3:a1:
    7f:b3:52:2c:b0:99:4a:4a:ca:35:b6:bd:9d:f6:d8:
    68:31:db:de:42:ba:93:3f:69:10:a0:78:fb:1e:04:
    08:15:98:12:e9:b9:93:0c:2f:9e:20:83:86:cd:c2:
    b0:00:a1:f8:2c:ce:d9:62:b2:e4:4a:24:6c:c3:ad:
    86:4f:34:03:29:53:a1:c0:4b:25:2f:b1:c8:4b:1a:
    33:d6:b8:24:ac:e3:d1:6e:6c:38:97:94:c6:e3:e5:
    a1:88:2c:2b:1a:db:eb:25:96:e8:82:c5:f9:97:d6:
    7c:de:c7:4f:96:2b:3b:8c:8f:b0:2e:66:8c:7b:b9:
    16:57:d2:cb:56:23:cb:08:e2:85:57:2c:90:40:3c:
    a3:34:37:fd:20:99:b9:34:a9:3b:5d:cb:b0:ef:a7:
    1b:55:78:8c:aa:48:51:3f:d9:ec:f8:d5:20:e4:ce:
    8f:92:d8:88:0d:ae:9b:27:37:7d:1f:8e:8f:50:37:
    d9:f2:14:aa:d9:18:32:3d:df:02:14:24:24:c8:d8:
    a6:4d
exponent2:
    00:c3:db:46:2b:42:9d:14:83:73:56:36:2b:17:0d:
    da:2b:4e:d7:54:43:ef:22:cd:8c:76:1b:54:6b:1e:
    f9:a0:4e:e6:63:4b:3a:dc:ca:da:f7:df:45:21:b2:
    c4:f7:a2:9b:ac:e3:b1:ac:75:be:47:8f:64:0c:3a:
    11:2b:c4:93:22:5a:57:6c:eb:27:8c:0e:6d:15:a4:
    25:99:22:c9:20:45:f4:5d:b0:d0:94:79:d1:36:6b:
    26:21:42:39:1e:d7:34:fc:96:f1:b0:fd:27:64:23:
    f2:27:c3:29:da:0f:a6:ad:36:92:c4:f5:c8:70:3d:
    85:e8:b4:2b:86:c2:5f:c0:2d:f3:77:1f:59:05:5e:
    e2:5f:b8:74:17:5f:23:ea:bb:5b:09:cd:58:29:02:
    b5:6a:34:7d:31:00:77:59:f2:4d:af:06:2d:c2:c3:
    d6:12:1b:71:ee:e7:75:21:0a:d1:33:40:cf:19:b0:
    a0:28:22:b2:86:a0:8a:ce:71:aa:7b:d7:93:f7:53:
    64:58:f1:c7:81:af:54:8d:27:62:7d:af:bc:c5:05:
    e7:6a:d6:2f:00:86:74:b1:11:b7:fe:0c:22:31:f6:
    07:c2:6d:b9:35:eb:4c:c4:29:f8:74:cb:ac:b9:a9:
    da:92:2e:67:19:e3:a2:50:09:77:46:ae:60:0a:19:
    23:2f
coefficient:
    18:01:82:8e:84:28:56:0f:0a:ce:25:4c:06:70:53:
    3c:ea:55:b3:13:f4:e3:73:21:b0:5d:e5:e4:81:1d:
    09:e2:c8:fe:f8:b1:54:75:a0:79:d7:d6:df:2f:86:
    98:f0:c7:2d:38:aa:e9:a8:8a:8b:cf:ad:60:bf:b7:
    5e:d7:18:96:be:d6:00:cd:8d:06:62:a0:ec:50:1e:
    25:26:44:14:33:b5:28:4a:4e:3b:61:c7:86:05:17:
    4c:a4:07:70:88:f0:ee:07:79:c1:0a:5e:a8:3c:4d:
    41:4b:8b:98:26:d8:7f:12:75:e2:7c:d8:12:97:c6:
    22:d6:e1:39:ec:c7:0a:72:17:ca:6e:75:69:b7:79:
    4f:ad:65:1b:8b:b6:5d:c3:65:d3:6b:44:9c:cd:37:
    c5:79:c4:a8:6f:75:56:b4:92:c9:db:2b:1f:6a:b5:
    34:d8:68:6f:cc:c5:b6:96:6b:30:2c:fa:da:34:26:
    62:d3:9c:ee:76:e2:f4:2c:ac:c0:33:76:11:01:cc:
    10:c9:d3:af:5c:39:03:2a:66:ca:cc:4a:37:63:05:
    e6:45:9a:7e:9e:61:b4:b3:9d:90:2b:a4:fa:ef:f1:
    64:aa:f0:8a:de:f6:4c:ca:e8:82:29:02:6a:d5:1b:
    d3:5e:95:1a:5c:40:0b:03:1a:3e:12:a4:ea:82:9a:
    18

```





## Q:

What part of the certificate indicates this is a CA’s certificate? 

在证书中，判断是否为CA证书可以查看"Basic Constraints"扩展字段。在这个证书中，存在并标记为关键的"X509v3 Basic Constraints"扩展字段。其取值为"CA:TRUE"，表示这是一张CA证书。该扩展字段用于识别证书是否具有签发其他证书的权限。

What part of the certificate indicates this is a self-signed certificate? 

要判断证书是否为自签名证书，可以比较主题字段和颁发者字段。在这个证书中，主题和颁发者字段相同："CN = [www.modelCA.com](http://www.modelca.com/), O = Model CA LTD., C = US"。当主题和颁发者字段相同的时候，表明证书是自签名的。

In the RSA algorithm, we have a public exponent e, a private exponent d , a modulus n , and two secret numbers p  and q , such that n = p * q . Please identify the values for these elements in your certificate and key files.

- modulus：模数，由两个素数相乘得到
- publicExponent：公开指数，表示用于加密的公钥指数，这里是65537。
- privateExponent：私有指数，表示用于解密的私钥指数。
- prime1和prime2：两个素数，用于生成模数。



# Task 2: Generating a Certificate Request for Your Web Server

一个名为bank32.com（请将其替换为您自己的Web服务器名称）的公司希望从我们的CA获取公钥证书。首先，它需要生成一个证书签名请求（CSR），其中包括公司的公钥和身份信息。CSR将被发送给CA，CA将验证请求中的身份信息，然后生成一个证书。 生成CSR的命令与我们用于创建CA的自签名证书的命令非常相似，唯一的区别是使用了"-x509"选项。没有该选项，该命令将生成一个请求；而使用该选项，该命令将生成一个自签名证书。

```
openssl req -newkey rsa:2048 -sha256 \
-keyout server.key -out server.csr \
-subj "/CN=www.gls23.com/O=gls23 Inc./C=US" \
-passout pass:dees
```

- `req`：指定使用 OpenSSL 的 req 命令。
- `-newkey rsa:2048`：生成一个新的 RSA 密钥对，密钥长度为 2048 位。
- `-sha256`：使用 SHA-256 算法进行摘要计算。
- `-keyout server.key`：将生成的私钥保存到名为 server.key 的文件中。
- `-out server.csr`：将生成的 CSR 保存到名为 server.csr 的文件中。
- `-subj "/CN=www.chenyang2022.com/O=Chenyang2022 Inc./C=US"`：设置 CSR 的主题信息，包括常用名称 (Common Name)、组织名称 (Organization) 和国家/地区 (Country)。
- `-passout pass:dees`：设置私钥文件的加密密码为 "dees"。

## Adding Alternative names

浏览器实施的主机名匹配策略要求证书中的通用名称必须与服务器的主机名匹配，否则浏览器将拒绝与服务器通信。为了允许一个证书具有多个名称，X.509 规范定义了附加到证书上的扩展。这个扩展称为主体备用名称（Subject Alternative Name，SAN）。使用 SAN 扩展，可以在证书的 subjectAltName 字段中指定多个主机名。为了生成带有这样一个字段的证书签名请求，我们可以将所有必要的信息放在一个配置文件中或者通过命令行提供。在这个任务中，我们将使用命令行方法（配置文件方法在另一个 SEED 实验室，即 TLS 实验室中使用）。我们可以向 "openssl req" 命令添加以下选项。需要注意的是，subjectAltName 扩展字段还必须包括来自通用名称字段的内容；否则，通用名称将不被接受为有效名称。

```
openssl req -newkey rsa:2048 -sha256 \
-keyout server.key -out server.csr \
-subj "/CN=www.gls23.com/O=gls23 Inc./C=US" \
-passout pass:dees \
-addext "subjectAltName = DNS:www.gls2023.com, \
DNS:www.gls2024.com"
```

```
[06/21/23]seed@VM:~/demoCA$ openssl req -newkey rsa:2048 -sha256 \
> -keyout server.key -out server.csr \
> -subj "/CN=www.gls23.com/O=Bank32 Inc./C=US" \
> -passout pass:dees \
> -addext "subjectAltName = DNS:www.gls2023.com, \
> DNS:www.gls2024.com"
Generating a RSA private key
...................+++++
..........................................................................................................................+++++
writing new private key to 'server.key'

[06/21/23]seed@VM:~/demoCA$ openssl req -in server.csr -text -noout
Certificate Request:
    Data:
        Version: 1 (0x0)
        Subject: CN = www.gls23.com, O = Bank32 Inc., C = US
        Subject Public Key Info:
            Public Key Algorithm: rsaEncryption
                RSA Public-Key: (2048 bit)
                Modulus:
                    00:a4:10:d3:4d:cf:98:01:14:4e:5f:6c:c1:2b:ba:
                    9e:64:cb:6c:3a:e2:35:27:0c:03:c5:c4:f0:2b:53:
                    f0:5b:11:40:e6:ab:df:e1:56:d5:a7:4c:56:25:a8:
                    7c:5c:79:6b:e7:16:31:98:a9:96:26:e0:e2:c5:93:
                    ec:d9:b8:cc:53:46:e7:13:79:27:cd:be:33:b0:d1:
                    d0:8e:b6:59:91:5d:48:d8:a8:66:57:0a:2d:a1:be:
                    82:61:bb:b5:c4:4f:7e:78:d5:6b:56:51:43:78:95:
                    83:02:54:91:7a:0e:66:fd:9b:d4:26:fe:97:46:e7:
                    57:03:10:4a:0b:45:bd:b7:96:23:8f:be:14:57:84:
                    c5:52:4c:4a:06:0f:55:9b:19:52:30:1a:26:b2:fe:
                    00:45:e0:02:0b:a5:0d:7a:80:df:73:55:65:84:28:
                    63:93:d1:00:80:5e:52:7c:b5:24:1b:e9:a0:7f:20:
                    43:55:e2:8d:5e:03:02:82:aa:72:54:97:83:cb:5e:
                    8b:b5:f9:b8:41:11:d7:b9:9c:fd:5c:9b:51:6d:73:
                    18:e4:48:4f:c1:54:4a:7d:d0:a0:e7:dc:1a:57:fd:
                    4c:7d:48:2d:b2:0a:74:bc:68:3a:8b:3b:97:ec:bf:
                    12:af:36:63:50:9f:4b:6e:e8:31:02:d8:e5:d1:e5:
                    2f:ed
                Exponent: 65537 (0x10001)
        Attributes:
        Requested Extensions:
            X509v3 Subject Alternative Name: 
                DNS:www.gls2023.com, DNS:www.gls2024.com
    Signature Algorithm: sha256WithRSAEncryption
         4c:f5:d9:b2:f7:4c:8d:19:f1:57:7e:6c:57:3d:d6:a9:89:ad:
         cb:cb:37:c5:44:00:c2:31:cc:64:a8:2b:51:ca:9d:9b:04:d0:
         b0:07:c9:c8:f6:fc:56:ba:0c:c7:b2:bb:aa:3c:98:15:e8:3f:
         8f:4d:bc:4e:d5:32:41:69:d5:83:6f:8c:68:bd:c1:22:35:9a:
         fa:3a:71:22:c1:71:5a:39:5d:0b:13:4c:d4:bb:d5:b4:d2:eb:
         be:e7:53:c4:50:da:77:00:23:14:86:a6:98:cd:80:9d:4b:b7:
         51:1b:f1:81:48:e5:1c:3c:dc:f2:5a:b7:a4:43:a3:5b:ad:bf:
         7f:66:59:cc:12:ca:23:c0:a1:da:38:ff:c1:e8:78:cd:b5:5f:
         43:e8:82:4f:d3:76:a9:3e:e2:54:ad:85:66:18:da:19:bd:c4:
         5b:f7:f2:53:13:ea:44:73:be:bb:8a:cb:7c:20:18:b2:65:96:
         24:22:43:ab:5f:11:62:18:4f:7f:3a:b6:47:e1:f2:70:9d:95:
         3c:eb:aa:d5:0a:5c:d9:0d:83:7f:d0:bd:ae:b4:76:a3:a8:a6:
         7f:94:0f:4f:8a:41:7e:e7:fe:9c:bc:ee:06:67:10:60:57:58:
         c1:78:c7:65:f3:5e:52:1c:02:5e:f8:98:9c:05:58:73:b0:c0:
         56:82:52:34

```

# Task 3: Generating a Certificate for your server

CSR 文件需要有 CA 的签名才能形成证书。在现实世界中，CSR 文件通常被发送到受信任的 CA 以获得签名。在这个lab中，我们将使用我们自己的可信 CA 来生成证书。下面的命令使用 CA 的 CA.crt 和 CA.key 将证书签名请求(server.csr)转换为 X509证书(server.crt) :

注意我的配置文件在./my_openssl.cnf

```shell
[06/22/23]seed@VM:~/CA$ openssl ca -config ./my_openssl.cnf -policy policy_anything \
> -md sha256 -days 3650 \
> -in server.csr -out server.crt -batch \
> -cert ./ca.crt -keyfile ./ca.key
Using configuration from ./my_openssl.cnf
Enter pass phrase for ./ca.key:
Check that the request matches the signature
Signature ok
Certificate Details:
        Serial Number: 4096 (0x1000)
        Validity
            Not Before: Jun 22 14:34:08 2023 GMT
            Not After : Jun 19 14:34:08 2033 GMT
        Subject:
            countryName               = US
            organizationName          = gls23 Inc.
            commonName                = www.gls23.com
        X509v3 extensions:
            X509v3 Basic Constraints: 
                CA:FALSE
            Netscape Comment: 
                OpenSSL Generated Certificate
            X509v3 Subject Key Identifier: 
                53:6B:EA:44:75:F5:0D:AE:C1:B7:01:67:E5:E5:0C:EF:42:0E:9F:57
            X509v3 Authority Key Identifier: 
                keyid:85:43:B9:78:0E:00:2C:30:DB:49:AC:E4:00:03:EF:B3:E0:D8:AD:49

            X509v3 Subject Alternative Name: 
                DNS:www.gls2023.com, DNS:www.gls2024.com
Certificate is to be certified until Jun 19 14:34:08 2033 GMT (3650 days)

Write out database with 1 new entries
Data Base Updated

```

执行之后，文件出现了变化

```shell
[06/22/23]seed@VM:~/CA$ tree .
.
├── ca.crt
├── ca.key
├── demoCA
│   ├── certs
│   ├── crl
│   ├── index.txt
│   ├── index.txt.attr
│   ├── index.txt.attr.old
│   ├── index.txt.old
│   ├── newcerts
│   │   ├── 1000.pem
│   │   └── 1001.pem
│   ├── serial
│   └── serial.old
├── my_openssl.cnf
├── server.crt
├── server.csr
└── server.key

4 directories, 14 files

```

查看一下server.crt

```
[06/22/23]seed@VM:~/CA$ openssl x509 -in server.crt -text -noout
Certificate:
    Data:
        Version: 3 (0x2)
        Serial Number: 4097 (0x1001)
        Signature Algorithm: sha256WithRSAEncryption
        Issuer: CN = www.modelCA.com, O = Model CA LTD., C = US
        Validity
            Not Before: Jun 22 14:54:43 2023 GMT
            Not After : Jun 19 14:54:43 2033 GMT
        Subject: C = US, O = gls23 Inc., CN = www.gls23.com
        Subject Public Key Info:
            Public Key Algorithm: rsaEncryption
                RSA Public-Key: (2048 bit)
                Modulus:
                    00:95:72:09:22:45:48:33:58:7a:a3:9b:27:d5:d8:
                    a0:e2:3d:5f:87:86:7a:10:d7:40:a2:94:6c:c6:0e:
                    84:ff:e4:61:10:9c:fe:f4:a9:90:09:c1:a1:d3:7a:
                    59:10:9a:2e:eb:e2:29:9c:04:cc:48:17:e4:1b:06:
                    b2:28:d7:d3:f5:88:0c:08:ea:c8:ad:6d:e5:eb:20:
                    f5:9d:2b:33:bc:ce:36:7a:c8:7c:21:10:97:2e:27:
                    ea:24:5b:5b:07:f6:d9:90:1d:8f:bf:b6:00:3a:c9:
                    0e:37:58:ca:0c:30:18:b3:20:0f:45:01:99:78:ce:
                    81:b8:56:93:29:58:3c:ea:e9:7d:a7:cf:d9:f4:dc:
                    37:58:75:34:a6:a5:9b:a5:1f:64:a0:70:ca:b7:24:
                    1a:50:f9:6f:64:3a:30:80:89:a1:dc:fc:72:ce:24:
                    49:eb:c8:b6:5d:b4:e5:bd:f5:45:8d:c4:a7:c3:e8:
                    b2:28:c2:b4:e3:7e:8a:48:75:b3:81:ab:bc:0f:6d:
                    43:3c:d8:9e:15:17:ee:5c:61:1d:e7:76:27:31:38:
                    66:47:68:85:e7:b2:80:fd:e1:98:36:70:12:8e:ad:
                    85:00:fc:45:1b:fe:72:6b:e9:2e:3f:e6:6f:b1:45:
                    95:af:6c:21:14:1b:64:3a:f8:87:34:28:b1:91:2a:
                    2f:3d
                Exponent: 65537 (0x10001)
        X509v3 extensions:
            X509v3 Basic Constraints: 
                CA:FALSE
            Netscape Comment: 
                OpenSSL Generated Certificate
            X509v3 Subject Key Identifier: 
                53:6B:EA:44:75:F5:0D:AE:C1:B7:01:67:E5:E5:0C:EF:42:0E:9F:57
            X509v3 Authority Key Identifier: 
                keyid:85:43:B9:78:0E:00:2C:30:DB:49:AC:E4:00:03:EF:B3:E0:D8:AD:49

            X509v3 Subject Alternative Name: 
                DNS:www.gls2023.com, DNS:www.gls2024.com
    Signature Algorithm: sha256WithRSAEncryption
         62:b0:a6:8a:15:35:b4:8f:5f:1a:18:89:d2:32:6e:10:6f:19:
         85:39:c1:ca:49:0e:d4:f0:a6:85:b9:ac:0b:7a:8c:11:19:66:
         7f:9d:af:1d:1c:86:60:e6:f5:ac:70:5a:ba:f7:1e:e1:65:21:
         31:4d:fe:33:74:64:16:66:9a:2b:02:74:cf:e3:93:af:1f:86:
         05:db:84:34:7f:73:69:3c:d1:be:f3:f5:0f:8e:e8:98:bf:5b:
         5c:55:73:33:09:0a:2a:41:db:58:d3:59:e1:2b:e3:92:96:d4:
         1d:d3:b3:39:27:50:8f:06:4b:0c:68:97:9f:a0:fe:05:1a:0f:
         95:95:24:3d:4f:70:f2:69:6a:0e:eb:b8:00:a3:dc:29:43:28:
         51:df:f5:8d:e6:89:79:2c:ef:50:22:b1:9e:17:09:76:75:30:
         c3:32:10:f0:73:34:22:26:13:64:47:3b:39:ac:c1:3f:34:ff:
         61:e8:f6:02:bf:15:17:33:30:95:e2:71:14:f1:c8:be:42:b4:
         f3:83:eb:de:52:ce:d4:c2:4e:ed:bb:cf:91:3f:09:30:ce:68:
         d4:50:39:4a:37:81:c6:fb:32:ca:fb:2e:ef:78:06:8a:d7:a1:
         69:a8:1f:6b:e3:14:59:df:95:99:fc:a5:12:0d:54:d4:03:d6:
         cf:1f:a0:e1:1d:ae:66:cd:03:db:78:65:60:f7:55:c9:79:19:
         fd:d2:db:45:b6:c2:68:f0:04:80:65:f7:40:87:c3:dc:6e:18:
         9f:82:7d:db:16:e9:02:c9:58:7b:b5:ed:62:6a:d9:2f:3a:ca:
         e0:c4:b2:d4:a1:35:2d:3e:ae:9f:be:e6:dd:fb:a9:ac:10:db:
         41:4a:e0:e8:6a:32:2b:44:8a:3e:a5:7c:c4:88:6c:ce:ef:93:
         48:de:27:21:fe:df:a7:19:f2:01:b7:d3:7e:5a:24:fd:15:3e:
         7e:07:46:05:b4:4c:b8:84:25:28:bc:55:e6:8d:17:33:f4:3d:
         36:84:5b:fe:56:61:54:7c:da:47:b9:57:bd:0f:b9:d8:86:82:
         65:70:95:6e:38:5b:98:17:c6:01:ac:a0:4a:01:5d:a8:82:92:
         ca:38:00:54:3e:38:1e:1d:3e:17:40:c5:59:7e:ff:39:04:2f:
         99:bb:66:07:39:bb:14:a8:e6:09:09:ea:54:e7:3e:18:3a:5d:
         72:44:b0:5b:48:92:69:81:05:ec:94:fb:6f:56:4e:5f:88:4f:
         fe:5f:ac:55:33:3e:e1:27:e0:c2:66:5c:70:ea:b6:d1:0e:71:
         7b:92:f7:3d:8b:a4:1b:38:84:d3:c9:ec:db:f1:63:89:5a:5a:
         6e:0c:27:08:d3:16:4a:59

```

# Task 4: Deploying Certificate in an Apache-Based HTTPS Website

编辑我们自己网站的apache ssl.conf文件

```
$ cat apache_ssl.conf 
<VirtualHost *:443> 
    DocumentRoot /var/www/gls23
    ServerName www.gls23.com
    ServerAlias www.gls2023.com
    ServerAlias www.gls2024.com
    DirectoryIndex index.html
    SSLEngine On 
    SSLCertificateFile /certs/server.crt
    SSLCertificateKeyFile /certs/server.key
</VirtualHost>
[06/22/23]seed@VM:~/.../image_www$ 

```

可以看到第九行、第十行需要文件。因此把server.crt和server.key复制到容器的certs文件夹

编辑一下容器的Dockerfile，如下

```
FROM dockerproxy.com/handsonsecurity/seed-server:apache-php
  
ARG WWWDIR=/var/www/gls23

COPY ./index.html ./index_red.html $WWWDIR/
COPY ./apache_ssl.conf /etc/apache2/sites-available
COPY ./certs/server.crt ./certs/server.key  /certs/

RUN  chmod 400 /certs/server.key \
     && chmod 644 $WWWDIR/index.html \
     && chmod 644 $WWWDIR/index_red.html \
     && a2enmod ssl \
     && a2ensite apache_ssl 
     

CMD  tail -f /dev/null

```

下面构建容器镜像、运行容器、进入容器内部、启动Apache服务

```
$ dcbuild

$ dcup

$ docker image ls 

$ docker ps 

$ docksh # 进入容器内部

$ service apache2 start 
```

![](https://img.gls.show/img/image-20230623155309100.png)



在Firefox中输入about:preferences#privacy，在Authorities tab导入我们的CA并选择Trust this CA to identify web sites，最后访问https://www.example.com  即可

# Task 5: Launching a Man-In-The-Middle Attack

> 中间人攻击通常发生在安全通信的建立阶段，例如使用SSL/TLS加密的HTTPS连接。攻击者会冒充服务器与客户端建立连接，同时与服务器和客户端分别建立独立的连接。攻击者可以生成自己的伪造证书，与客户端建立安全连接并将自己的伪造证书发送给客户端，同时与服务器建立另一个安全连接并将客户端的请求发送给服务器。这样，攻击者就能够在客户端和服务器之间的通信中拦截、查看和修改数据。
>
> 为了防止中间人攻击，通常使用证书颁发机构（CA）签发的可信证书来验证服务器的身份。客户端会对服务器的证书进行验证，包括验证证书的有效性、合法性和所属的颁发机构等。

几种方法可以让用户的 HTTPS 请求到达我们的 Web 服务器。

- 一种方法是攻击路由，这样用户的 HTTPS 请求就被路由到我们的 Web 服务器。
- 另一种方法是攻击 DNS，所以当受害者的机器试图找出目标网络服务器的 IP 地址，它得到我们的网络服务器的 IP 地址。

在这个任务中，我们模拟了攻击-DNS 的方法。我们只需修改受害者机器的/etc/hosts 文件，通过将主机名 www.example.com 映射到我们的恶意网络服务器，来模拟 DNS 缓存定位攻击的结果，而不是启动一个实际的 DNS 域名服务器缓存污染。

task4中，已经建立了一个 HTTPS 网站，这个任务中使用相同的 Apache 服务器来模拟 www.example.com 。方法类似按照 Task 4中的指令向 Apache 的 SSL 配置文件添加一个 VirtualHost 条目: ServerName 应该是 www.example.com

举个例子，可以将baidu.com映射到我们自己的服务器上，让受害者以为自己访问了百度，但是其实是访问了我们的恶意服务器

我们可以使用之前生成的自签名CA给百度签一个证书，如果浏览器加载了我们的CA，那么受害者访问百度的时候，就会显示https正常，通过了证书核验



# Task 6: Launching a Man-In-The-Middle Attack with a Compromised CA

> 在这个任务中，我们假设在 Task 1中创建的根 CA 受到攻击者的攻击，并且它的私钥被盗取了。因此，攻击者可以使用此 CA 的私钥生成任意证书。在这项任务中，我们将看到这种妥协的后果。请设计一个实验来证明攻击者可以成功地在任何 HTTPS 网站上启动 MITM 攻击。您可以使用在 Task 5中创建的相同设置，但是这一次，您需要证明 MITM 攻击是成功的，也就是说，当受害者试图访问一个网站但是登录到 MITM 攻击者的假网站时，浏览器不会引起任何怀疑。

在task5中的叙述描述了我们使用自己的**自签名CA导入到浏览器**的情况，但是如果**根CA私钥**被盗取了会有更严重的后果：

1. 伪造证书：攻击者可以使用根证书的私钥签发伪造的证书，模拟合法的网站或服务，使其看起来具有合法的身份和可信的安全性。这将导致用户误认为与真实网站或服务进行通信，从而暴露他们的敏感信息。
2. 中间人攻击：通过拦截通信并使用伪造的证书，攻击者可以进行中间人攻击，监视和修改通信内容，窃取敏感信息或注入恶意内容。这对用户、网站和系统的安全性构成了严重威胁。
3. 篡改和劫持：攻击者可以篡改通过伪造证书进行加密的通信内容，例如修改下载文件、注入恶意代码或劫持用户的会话。
4. 信任破裂：如果根证书的私钥被盗取，信任链中的所有证书都将受到威胁，导致整个系统的信任破裂。这将对证书基础设施的安全性和可信度产生长期的负面影响。

实验步骤与之前类似，使用根CA签名证书，伪造证书在浏览器进行访问时不许任何设置都会被信任