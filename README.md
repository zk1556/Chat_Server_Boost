## Chat_Server_Boost
 在Linux环境下实现的基于Boost库的聊天服务器，主要实现以下功能：

- **用户注册和登录**：通过Mysql数据库保存注册的用户和密码。
- **用户状态记录**：采用Redis生成5分钟的SessionID记录客户端登录的状态，在5分钟内不需要重复验证。
- **布隆过滤器**：用于快速判断登录用户是否不在数据库中，防止进一步访问数据库。
- **连接池**：实现了Mysql和Redis连接池，避免频繁连接和断开。
- **并发模型**：采用boost线程池实现并发

## 运行配置及使用

- mysql配置

```
create database user_chat;

use user_chat;

create table user(
    name char(50) null,
     password char(50) null
    );
```

- redis服务端启动

```
reids-server
```

* 生成可执行程序

```
 mkdir build
 cd build
 cmake .. //生成Makefile文件

 make
```

* 运行可执行程序

```
 服务器： ./bin/server
 客户端： ./bin/client
```

## 项目文件介绍

___

​	项目采用模块化设计，主要包括客户端、服务端、连接池、封装锁等模块。

以下是项目的目录结构及文件说明：

```
src
├── HandleClient.cpp 			   #客户端处理逻辑实现文件
├── HandleClient.h				   #客户端处理逻辑头文件
├── HandleServer.cpp			   #服务端处理逻辑实现文件
├── HandleServer.h				   #服务端处理逻辑头文件
├── bitset.h             		   #布隆过滤器数据结构定义
├── client.cpp					   #客户端运行主文件	
├── lock
│   └── locker.h        		   #线程同步机制包装类
├── mysql_pool
│   ├── sql_connection_pool.cpp    #mysql连接池实现文件
│   └── sql_connection_pool.h      #mysql连接池头文件
├── redis_pool
│   ├── nosql_connection_pool.cpp  #redis连接池实现文件
│   └── nosql_connection_pool.h    #redis连接池头文件
└── server.cpp             		   #服务器运行主文件
```





## 运行效果

___

![image-20240324171418362](img\image-20240324171418362.png)

## 参考

___

1. https://github.com/qinguoyi/TinyWebServer?tab=readme-ov-file ：学习连接池设计
2. [chat-project-based-on-ubuntu](https://github.com/CopyDragon/chat-project-based-on-ubuntu)：学习聊天服务器逻辑

