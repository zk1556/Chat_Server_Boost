#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

#include <stdio.h>
#include <list>
#include <error.h>
#include <string.h>
#include <iostream>
#include <string>
#include "../lock/locker.h"
#include <hiredis/hiredis.h>

using namespace std;

//连接池
class nosql_connection_pool
{
public:
	redisContext *GetConnection();				 //获取数据库连接
	bool ReleaseConnection(redisContext *conn); //释放连接
	int GetFreeConn();					 //获取连接
	void DestroyPool();					 //销毁所有连接

	//单例模式
	static nosql_connection_pool *GetInstance();

	void init(string ip,string port,int MaxConn); 

private:
	nosql_connection_pool();
	~nosql_connection_pool();

	int m_MaxConn;  //最大连接数
	int m_CurConn;  //当前已使用的连接数
	int m_FreeConn; //当前空闲的连接数
	locker lock;
	list<redisContext *> connList; //连接池
	sem reserve;

public:
	string m_ip;			 //主机地址
	string m_port;		 //数据库端口号
};

class nosql_connectionRAII{

public:
	nosql_connectionRAII(redisContext **con, nosql_connection_pool *connPool);
	~nosql_connectionRAII();
	
private:
	redisContext *conRAII;
	nosql_connection_pool *poolRAII;
};

#endif
