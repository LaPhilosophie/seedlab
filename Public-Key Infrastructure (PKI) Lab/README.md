# Lab Environment

下载docker文件、解压、运行docker

```shell
$ curl https://seedsecuritylabs.org/Labs_20.04/Files/Crypto_PKI/Labsetup.zip --output pki.zip 

$ unzip pki.zip

$ dcbuild # docker-compose build

$ dcup # docker-compose up
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



我们需要创建几个子目录`mkdir certs crl newcerts`



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
-passout pass:dees
```

文件 CA.key 包含 CA 的私钥，而 CA.crt 包含公钥证书

我们可以使用以下命令查看 X509证书的解码内容和 RSA 密钥(- text 意味着将内容解码为纯文本;-noout 意味着不打印编码版本)

```
openssl x509 -in ca.crt -text -noout
openssl rsa -in ca.key -text -noout
```

```shell
[06/21/23]seed@VM:~/demoCA$ openssl x509 -in ca.crt -text -noout
Certificate:
    Data:
        Version: 3 (0x2)
        Serial Number:
            71:fc:12:16:27:a5:ab:14:52:cc:85:4f:4a:a9:82:6a:9e:bd:e4:54
        Signature Algorithm: sha256WithRSAEncryption
        Issuer: CN = www.modelCA.com, O = Model CA LTD., C = US
        Validity
            Not Before: Jun 21 09:55:18 2023 GMT
            Not After : Jun 18 09:55:18 2033 GMT
        Subject: CN = www.modelCA.com, O = Model CA LTD., C = US
        Subject Public Key Info:
            Public Key Algorithm: rsaEncryption
                RSA Public-Key: (4096 bit)
                Modulus:
                    00:ba:66:8f:db:4d:9b:ca:c0:63:4e:27:6b:2a:d8:
                    26:ee:56:6e:8b:b7:58:43:f5:be:07:b6:02:75:85:
                    90:bc:bd:81:39:b5:3c:be:d9:ca:e9:35:b7:cf:dc:
                    0e:7f:6d:d9:56:72:ee:45:57:92:1e:ea:96:7b:7f:
                    8a:2e:88:47:fa:c4:2b:a2:7e:bb:cd:e7:db:d2:ff:
                    c7:07:bd:03:fd:19:43:43:a8:fb:00:9b:50:58:0f:
                    59:25:dc:ab:b1:8f:d1:06:34:f8:08:f7:f2:1a:86:
                    e2:ff:4b:33:26:e7:34:35:8d:63:a0:2e:1a:d9:f6:
                    69:61:7c:5c:2d:2e:2d:cd:54:1b:ab:13:dc:c5:da:
                    7c:a0:27:ae:ae:79:20:68:3a:1b:e8:ca:5c:43:24:
                    53:e1:82:93:70:18:f9:7e:20:ad:a4:c4:0e:74:0e:
                    8f:f1:73:e3:10:8b:01:a9:fb:c6:ba:0a:af:76:7b:
                    b0:d7:07:cb:ae:df:6e:92:45:d8:bd:9f:d3:07:98:
                    07:2e:49:03:56:15:4a:41:58:94:40:51:64:7e:e3:
                    ca:dc:c7:c8:69:57:79:5b:9b:2a:08:c3:94:72:25:
                    e2:ab:aa:6c:12:a2:6c:e7:70:85:97:44:a7:98:c0:
                    0d:96:f0:d0:46:27:ef:67:40:5c:b4:af:bc:8e:bb:
                    ed:04:d2:69:ce:72:0a:31:f6:93:8d:c2:ff:79:bf:
                    d1:8e:dd:30:44:9c:4c:82:96:03:bc:cf:0e:b9:8a:
                    24:e7:2c:93:33:ab:8d:b4:61:b7:1c:60:8e:21:31:
                    8a:a5:3d:40:e3:58:bd:e7:8b:7b:a7:3a:87:62:08:
                    e0:4c:fc:f9:21:bc:98:73:cc:b2:2b:92:44:76:e0:
                    69:f7:19:c6:cd:24:23:e2:cb:b0:1c:8d:00:0c:4f:
                    9a:16:48:fc:50:f3:8b:79:1b:60:83:d3:95:f9:60:
                    2d:78:92:a9:24:61:f5:cd:11:bc:c5:6d:1b:27:04:
                    5c:29:1d:33:4d:49:f4:f8:28:fa:d7:a0:26:a3:d6:
                    87:92:a4:1b:39:b2:c1:24:78:e9:8e:61:78:19:d8:
                    7f:7d:a7:fd:3d:0c:2b:29:12:48:05:3b:8c:6d:b1:
                    04:47:2d:9b:25:6e:4a:17:02:16:6e:f1:a5:76:58:
                    92:7e:41:6d:78:28:ce:af:52:fa:fc:5b:55:ff:c9:
                    0f:e0:7f:04:3d:6d:44:20:49:a8:ff:2d:b5:6c:eb:
                    a1:38:e9:89:b2:94:64:19:d4:23:2e:47:5e:83:fb:
                    95:7b:86:be:26:f1:30:05:6c:36:3f:92:96:b7:d6:
                    74:2b:91:bf:98:47:b9:49:75:a5:26:36:fe:54:2e:
                    90:8d:65
                Exponent: 65537 (0x10001)
        X509v3 extensions:
            X509v3 Subject Key Identifier: 
                C4:48:97:26:35:58:1B:8D:AF:3B:2C:B0:52:CB:DA:68:9C:9A:72:B9
            X509v3 Authority Key Identifier: 
                keyid:C4:48:97:26:35:58:1B:8D:AF:3B:2C:B0:52:CB:DA:68:9C:9A:72:B9

            X509v3 Basic Constraints: critical
                CA:TRUE
    Signature Algorithm: sha256WithRSAEncryption
         ac:a8:6f:8c:9e:6b:74:e4:b5:7f:af:11:d4:de:14:25:3f:42:
         7f:4c:a3:61:81:69:5b:a1:86:87:ae:8e:d9:bf:76:ff:08:85:
         b2:29:9a:36:9c:04:e3:4c:1d:6c:b4:ad:5c:8c:17:d3:3d:65:
         50:e8:49:bd:b9:cf:5b:8e:54:3c:ba:43:cc:1b:f5:53:ec:c6:
         4e:52:65:41:fb:29:96:62:48:26:12:aa:22:c8:de:1c:c6:dd:
         77:4f:5e:e4:01:cc:84:b5:24:cb:6d:cb:82:77:f2:81:35:c8:
         9f:72:fe:59:d5:5d:2d:05:08:81:56:24:98:24:bf:30:35:72:
         37:7e:e6:e2:f1:a3:75:5e:4b:b9:e9:22:85:99:c5:48:dd:fa:
         4b:11:26:25:eb:ad:39:26:07:61:84:e8:20:63:4f:cf:d5:d3:
         7a:64:b5:9d:a7:f2:41:87:9c:ef:11:58:5c:de:9e:d7:5b:c8:
         82:0b:67:d1:3f:00:d3:7d:ef:8e:0b:56:04:70:17:42:92:f6:
         13:8c:45:e5:17:8c:9d:33:e2:ff:81:3a:94:70:fc:62:06:53:
         99:7d:c0:de:8d:27:cd:8b:06:99:cc:63:49:87:66:78:ad:1c:
         f3:0d:69:02:a5:af:b6:11:65:b7:fb:06:34:98:a2:0e:62:92:
         3b:c7:62:3e:a7:e6:3a:bb:af:c0:f7:ea:4f:b2:48:75:49:e3:
         f6:60:0c:c6:51:a8:2e:f5:bd:d9:76:24:5e:a0:c3:4f:ba:29:
         be:a1:b2:e1:62:a1:ea:53:f8:83:ea:17:72:36:d7:ce:4b:83:
         51:34:78:57:4f:99:5c:d1:a5:90:2e:db:66:40:b9:8c:ba:c6:
         e1:b6:12:64:11:4b:ab:c5:34:63:19:93:bc:cf:6a:36:2a:17:
         a0:8a:c8:1c:f4:f1:93:50:91:01:8a:43:fc:43:71:e5:66:30:
         50:81:63:93:a4:e6:18:47:9e:0a:af:78:4f:34:ae:2d:7c:b2:
         95:87:fa:06:ab:aa:1f:80:1c:4b:60:f9:98:3c:b2:de:70:41:
         a8:58:52:92:e5:43:9a:63:09:c9:8d:4a:dd:71:d4:8e:af:d4:
         26:15:b8:12:26:b5:59:5f:da:0c:e8:31:06:fc:6f:42:bd:63:
         4a:e7:6d:ed:0e:d2:c8:4e:e4:99:fd:83:ef:b6:2f:36:8e:c4:
         86:59:7f:1f:eb:44:df:04:23:d2:de:68:3d:e8:f0:03:a5:c9:
         c2:d2:28:00:9a:f1:eb:55:ed:ac:c7:7e:35:28:cc:7f:fd:0c:
         e8:36:a3:1c:27:4c:d7:b6:61:f2:38:fd:1b:2c:6f:1d:10:df:
         88:a7:e3:d8:6f:26:d7:37
[06/21/23]seed@VM:~/demoCA$ openssl rsa -in ca.key -text -noout
Enter pass phrase for ca.key:
RSA Private-Key: (4096 bit, 2 primes)
modulus:
    00:ba:66:8f:db:4d:9b:ca:c0:63:4e:27:6b:2a:d8:
    26:ee:56:6e:8b:b7:58:43:f5:be:07:b6:02:75:85:
    90:bc:bd:81:39:b5:3c:be:d9:ca:e9:35:b7:cf:dc:
    0e:7f:6d:d9:56:72:ee:45:57:92:1e:ea:96:7b:7f:
    8a:2e:88:47:fa:c4:2b:a2:7e:bb:cd:e7:db:d2:ff:
    c7:07:bd:03:fd:19:43:43:a8:fb:00:9b:50:58:0f:
    59:25:dc:ab:b1:8f:d1:06:34:f8:08:f7:f2:1a:86:
    e2:ff:4b:33:26:e7:34:35:8d:63:a0:2e:1a:d9:f6:
    69:61:7c:5c:2d:2e:2d:cd:54:1b:ab:13:dc:c5:da:
    7c:a0:27:ae:ae:79:20:68:3a:1b:e8:ca:5c:43:24:
    53:e1:82:93:70:18:f9:7e:20:ad:a4:c4:0e:74:0e:
    8f:f1:73:e3:10:8b:01:a9:fb:c6:ba:0a:af:76:7b:
    b0:d7:07:cb:ae:df:6e:92:45:d8:bd:9f:d3:07:98:
    07:2e:49:03:56:15:4a:41:58:94:40:51:64:7e:e3:
    ca:dc:c7:c8:69:57:79:5b:9b:2a:08:c3:94:72:25:
    e2:ab:aa:6c:12:a2:6c:e7:70:85:97:44:a7:98:c0:
    0d:96:f0:d0:46:27:ef:67:40:5c:b4:af:bc:8e:bb:
    ed:04:d2:69:ce:72:0a:31:f6:93:8d:c2:ff:79:bf:
    d1:8e:dd:30:44:9c:4c:82:96:03:bc:cf:0e:b9:8a:
    24:e7:2c:93:33:ab:8d:b4:61:b7:1c:60:8e:21:31:
    8a:a5:3d:40:e3:58:bd:e7:8b:7b:a7:3a:87:62:08:
    e0:4c:fc:f9:21:bc:98:73:cc:b2:2b:92:44:76:e0:
    69:f7:19:c6:cd:24:23:e2:cb:b0:1c:8d:00:0c:4f:
    9a:16:48:fc:50:f3:8b:79:1b:60:83:d3:95:f9:60:
    2d:78:92:a9:24:61:f5:cd:11:bc:c5:6d:1b:27:04:
    5c:29:1d:33:4d:49:f4:f8:28:fa:d7:a0:26:a3:d6:
    87:92:a4:1b:39:b2:c1:24:78:e9:8e:61:78:19:d8:
    7f:7d:a7:fd:3d:0c:2b:29:12:48:05:3b:8c:6d:b1:
    04:47:2d:9b:25:6e:4a:17:02:16:6e:f1:a5:76:58:
    92:7e:41:6d:78:28:ce:af:52:fa:fc:5b:55:ff:c9:
    0f:e0:7f:04:3d:6d:44:20:49:a8:ff:2d:b5:6c:eb:
    a1:38:e9:89:b2:94:64:19:d4:23:2e:47:5e:83:fb:
    95:7b:86:be:26:f1:30:05:6c:36:3f:92:96:b7:d6:
    74:2b:91:bf:98:47:b9:49:75:a5:26:36:fe:54:2e:
    90:8d:65
publicExponent: 65537 (0x10001)
privateExponent:
    31:78:20:bb:b2:08:23:b2:15:68:db:7b:4c:9e:9b:
    0e:6b:ef:e0:b2:a3:01:3e:49:d5:a0:0f:5d:03:3a:
    9b:6c:ab:cc:15:f6:6e:2c:3c:f6:d1:a3:db:cd:6a:
    27:95:8e:b5:ab:c0:e2:b4:4f:fa:56:85:e3:76:c0:
    c4:82:b4:9f:af:ca:68:d0:bb:a4:f4:e0:d9:49:ba:
    97:aa:29:51:d5:8f:8d:78:5e:4d:15:eb:27:c3:c3:
    04:12:61:9a:b9:31:5f:35:55:92:83:c3:44:19:02:
    4b:80:b8:ad:9d:74:b4:b5:b6:77:ff:64:6e:ee:3f:
    9f:78:b2:b9:e6:e4:8e:f6:c0:75:11:cb:68:d3:08:
    7a:34:ad:de:6e:15:14:b4:3f:4d:f3:ce:b2:9f:04:
    87:f7:f0:8b:32:85:9c:5c:ad:d8:e9:93:70:67:a0:
    fa:12:a8:73:6e:80:dd:8d:0d:7b:b8:74:42:bc:a6:
    fd:1a:7b:8b:08:8f:3e:d0:bc:a9:ee:ad:c1:f9:2e:
    06:b1:a7:ea:03:b2:76:4f:3c:e1:28:f8:c3:d8:4e:
    b1:73:70:13:cc:fc:4f:3c:89:d6:53:99:a5:05:ba:
    f7:96:3e:22:5f:eb:09:2d:4a:b7:0f:a8:6f:18:a1:
    42:aa:dc:8b:91:f3:ae:72:5d:68:32:af:97:2d:f6:
    c3:f9:df:02:4b:dd:66:58:71:6c:2f:80:59:03:75:
    1c:51:ed:69:bf:a1:6d:d3:ae:3f:98:fa:4d:f0:88:
    c6:33:54:0b:a8:f5:c7:ab:d4:d9:05:e9:85:21:4f:
    d8:a1:24:6e:01:34:32:e2:30:91:d0:ff:4a:29:6e:
    e6:bd:9f:17:08:8b:28:e5:ce:d9:90:f5:f5:ed:2c:
    4e:84:42:62:90:7b:18:49:de:16:b4:4a:f0:f9:3b:
    8c:3b:14:70:e2:54:2e:7f:1c:81:b6:02:97:3e:10:
    4b:88:8c:12:1a:bd:bf:91:33:74:4a:de:6c:26:b2:
    29:c2:8f:14:10:ac:23:7e:b9:1f:1d:c5:54:8d:3a:
    57:81:a8:a5:17:c8:f0:03:39:a4:77:9d:ae:64:3f:
    37:28:fe:49:f8:35:54:79:62:f5:8d:8e:ba:e2:fe:
    2f:22:7e:9e:76:a5:27:95:ed:08:26:db:01:7e:17:
    f1:7a:c3:53:e6:e0:c5:f8:b9:47:7b:19:71:15:88:
    6f:dd:07:e5:50:58:1a:df:6e:de:b6:0b:50:12:6a:
    9c:2f:9a:3b:31:75:95:02:cc:dd:b0:81:0b:dd:95:
    7a:0a:c0:e2:65:1d:93:06:32:7b:7b:38:55:90:81:
    9f:57:00:1f:ca:60:b8:97:b4:af:87:d8:7a:05:12:
    d2:0d
prime1:
    00:de:ca:fe:eb:ae:9a:7b:92:20:d4:ff:76:13:12:
    cc:34:68:8b:d5:82:41:e4:cf:2e:3a:09:2f:45:37:
    5f:c8:5d:39:2c:ff:ac:8f:03:cc:80:71:a4:73:68:
    1f:44:d9:77:f5:11:a1:bc:87:ed:4a:c0:92:b8:74:
    93:6f:48:e6:54:5d:99:3e:e1:98:02:19:51:1e:17:
    04:43:72:3a:02:72:59:a5:9b:a1:66:d1:50:6c:ab:
    17:37:9a:d8:7f:c9:03:c5:88:a3:ec:06:73:4c:50:
    4c:e9:d2:23:61:1c:6a:86:86:ec:25:59:b8:34:43:
    c3:c4:4b:f0:eb:50:95:bf:80:b6:e9:b1:1f:0d:22:
    50:ec:be:d4:2e:c1:52:9d:c8:68:ca:0c:f6:b9:c5:
    4f:df:ea:0b:fa:ad:ed:00:9e:e4:ba:10:2d:27:9a:
    33:4d:9d:76:fb:83:cb:cf:21:c3:21:86:a1:ea:b8:
    55:7a:38:c7:95:88:aa:fe:8f:74:c0:76:77:09:e9:
    e7:86:cf:31:77:5b:32:05:e3:6e:79:02:c6:0c:3d:
    37:6b:0c:37:cf:bd:73:f7:47:6c:f5:98:16:7b:91:
    b0:7a:73:4d:ec:84:b2:39:0d:0b:9f:57:d4:31:0b:
    18:ce:57:17:28:61:5e:d0:ea:10:ca:0b:a7:bf:5d:
    64:ff
prime2:
    00:d6:2e:f6:38:c7:db:bf:5c:ab:de:0d:4f:f6:c7:
    18:28:ac:77:bf:cf:92:f7:a5:b1:c9:1c:b1:91:f8:
    12:3e:b0:8a:47:a9:ad:6d:66:4e:7e:d4:0a:b8:70:
    6a:32:52:17:12:73:f1:86:31:07:6f:ad:eb:1b:5c:
    04:7e:22:24:e4:a4:a1:2a:b0:5d:df:64:0d:16:bb:
    47:41:2c:83:6d:cb:22:7c:c0:88:5c:5d:e7:2b:d3:
    b3:89:a6:a5:f7:22:f4:b0:d5:58:7c:72:fe:d5:73:
    06:b4:1a:fa:b8:a4:5d:37:67:0b:af:c6:a0:d3:26:
    f2:22:6b:02:5a:c8:69:ba:ab:99:68:13:7d:01:d0:
    38:6d:c7:d4:bc:cc:3b:f1:eb:3c:bd:6f:33:0b:ec:
    37:49:62:bb:39:63:06:8a:af:5e:58:79:ab:78:a2:
    58:46:e0:99:21:c8:b0:81:78:5f:29:0c:34:a8:5a:
    20:f7:87:9a:b7:16:ab:64:93:cb:f3:d1:90:44:aa:
    e0:16:9a:c6:c0:17:b7:c4:c8:a3:aa:24:f0:96:51:
    51:b1:43:58:20:0e:7a:ff:23:c7:d5:95:70:5c:64:
    0e:de:26:97:fd:da:e0:ed:a3:41:06:4f:15:34:b5:
    27:e3:d7:52:d3:3d:ac:7e:d3:8e:a3:cd:ce:38:58:
    99:9b
exponent1:
    4d:bb:a1:e9:f5:47:ad:29:b5:0e:db:fb:04:78:a0:
    8b:00:97:19:6d:93:34:14:1d:5a:de:6d:81:c6:ea:
    42:6c:54:50:a0:fc:42:01:df:4e:49:58:49:73:e0:
    c5:12:e3:49:bc:57:24:02:81:06:7d:de:33:41:69:
    57:80:a5:ec:75:a3:cc:4e:c5:cc:a5:ac:a9:c4:fa:
    15:d6:3a:eb:33:9e:97:ab:8f:4a:e2:69:1f:21:2a:
    be:99:e6:43:df:8c:d3:54:f3:6c:9c:6b:8a:0a:d2:
    6b:1a:88:81:8f:7a:93:69:6b:e7:34:40:92:eb:b9:
    d8:39:6e:db:8b:88:54:02:c2:cf:12:9f:74:e3:ae:
    3c:8b:f0:58:47:d7:5d:79:d9:25:f6:13:f7:89:22:
    6f:4b:a7:d6:54:ec:c6:34:aa:35:49:03:98:37:b3:
    02:b7:98:15:f5:4c:1c:fe:f8:a8:2f:e3:3a:94:0a:
    2e:d4:b4:be:35:ab:03:9a:65:ef:e8:e5:6b:ae:12:
    bb:f4:7a:9b:dc:53:bd:be:36:83:3e:36:7f:7a:b4:
    65:43:24:0d:f5:5a:0a:c2:fe:ff:49:8b:3e:9d:fc:
    67:1c:e7:e9:47:fb:2e:f0:a0:47:7e:4b:68:b9:28:
    74:85:ea:6c:8a:5c:9c:a3:ae:2b:f0:99:1c:a9:a7:
    5d
exponent2:
    00:d1:05:11:8b:06:18:00:df:5a:66:a0:4d:3a:42:
    bf:e3:e9:b4:0f:7f:6e:28:ba:0e:0f:7b:2e:1b:8a:
    a1:9b:86:43:dc:7d:cd:5e:e7:29:91:82:33:58:1d:
    74:b5:d9:00:e5:24:3b:3f:3f:17:c7:b8:4d:a9:ec:
    9a:01:d2:26:78:ef:e4:cc:b9:43:53:1f:db:da:24:
    37:fc:75:89:69:9b:84:f3:84:1d:4f:2e:e0:cc:17:
    94:ad:af:f1:65:86:3a:8f:5c:15:37:96:2d:f5:76:
    84:c3:3d:55:12:de:6c:8e:05:c5:14:ec:5d:c4:d2:
    cc:18:24:a0:2e:a2:48:f4:40:44:bc:99:d0:33:f5:
    d5:2b:e5:89:4a:1a:72:21:bb:f5:11:d5:b4:56:5d:
    56:f3:0a:24:36:73:da:4d:7b:a9:31:e4:c3:1f:3e:
    27:b2:3b:bb:95:89:ba:1f:0f:ae:a6:6b:5f:f4:2f:
    c8:de:67:fa:e7:d4:7a:bc:f3:b3:65:25:4d:8a:85:
    cb:9c:a3:bd:74:77:71:02:dc:df:43:cb:3a:f3:27:
    36:f7:2c:06:93:7a:2e:34:51:e1:d2:21:3d:7f:1c:
    b2:99:bf:c6:32:99:e9:4a:60:38:b4:04:77:68:71:
    a6:15:26:ec:90:74:2e:a3:b2:a8:10:59:67:77:fb:
    97:23
coefficient:
    09:6c:93:7f:fb:4a:37:30:f9:89:8b:78:aa:41:3f:
    e9:18:30:79:79:fe:f7:35:ca:7d:ee:5c:ad:8e:1b:
    b2:ab:25:cb:ec:53:4d:e0:f7:d1:a9:28:bd:8f:a9:
    d5:83:cc:b2:13:4a:d7:cc:63:1e:e3:ad:79:56:27:
    91:25:41:fb:d7:36:f0:0f:91:e4:ae:86:31:c4:07:
    64:f3:c9:3c:0d:39:f2:36:97:38:f3:bc:dc:81:9a:
    f0:d4:ed:49:66:40:9a:98:e9:20:cb:6a:27:f6:ca:
    86:f2:c0:6f:ac:6e:fc:ad:f3:5c:02:c7:ed:d0:61:
    84:6f:ed:17:76:c8:c3:4e:0b:fc:3d:8c:07:ce:97:
    89:82:4c:92:48:6e:f4:94:9a:95:51:84:1a:6a:33:
    42:62:65:f5:cc:a7:65:c0:b8:44:05:aa:31:6c:3a:
    87:cd:b2:78:1b:48:80:5a:eb:51:eb:d5:83:6d:aa:
    86:f8:e2:0a:ad:b8:40:d7:b9:34:b3:75:13:1e:5f:
    d0:f2:32:46:5d:fb:b4:33:8c:e9:55:cd:44:51:38:
    f9:32:84:30:ca:20:de:c3:54:b3:96:59:e0:1b:42:
    3d:fb:de:f1:ff:4b:ab:9f:15:fa:66:a0:a2:9b:b9:
    f5:90:fa:77:ad:d8:f6:43:e3:55:fb:25:ec:ea:21:
    51


```

Q:

What part of the certificate indicates this is a CA’s certificate? 

在证书中，判断是否为CA证书可以查看"Basic Constraints"扩展字段。在这个证书中，存在并标记为关键的"X509v3 Basic Constraints"扩展字段。其取值为"CA:TRUE"，表示这是一张CA证书。该扩展字段用于识别证书是否具有签发其他证书的权限。

What part of the certificate indicates this is a self-signed certificate? 

要判断证书是否为自签名证书，可以比较主题字段和颁发者字段。在这个证书中，主题和颁发者字段相同："CN = [www.modelCA.com](http://www.modelca.com/), O = Model CA LTD., C = US"。当主题和颁发者字段相同的时候，表明证书是自签名的。

In the RSA algorithm, we have a public exponent e, a private exponent d , a modulus n , and two secret numbers p  and q , such that n = p * q . Please identify the values for these elements in your certificate and key files.

- modulus：模数，由两个素数相乘得到
- publicExponent：公开指数，表示用于加密的公钥指数，这里是65537。
- privateExponent：私有指数，表示用于解密的私钥指数。
- prime1和prime2：两个素数，用于生成模数。

## Certificate Authority (CA)

如前所述，我们需要为 CA 生成一个自签名证书。这意味着这个 CA 是完全可信的，它的证书将充当根证书。可以运行以下命令为 CA 生成自签名证书

# Task 2: Generating a Certificate Request for Your Web Server

一个名为bank32.com（请将其替换为您自己的Web服务器名称）的公司希望从我们的CA获取公钥证书。首先，它需要生成一个证书签名请求（CSR），其中包括公司的公钥和身份信息。CSR将被发送给CA，CA将验证请求中的身份信息，然后生成一个证书。 生成CSR的命令与我们用于创建CA的自签名证书的命令非常相似，唯一的区别是使用了"-x509"选项。没有该选项，该命令将生成一个请求；而使用该选项，该命令将生成一个自签名证书。



```
openssl req -newkey rsa:2048 -sha256 \
-keyout server.key -out server.csr \
-subj "/CN=www.gls23.com/O=Bank32 Inc./C=US" \
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

