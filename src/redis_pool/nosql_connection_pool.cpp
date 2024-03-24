
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <pthread.h>
#include <iostream>
#include "nosql_connection_pool.h"

using namespace std;

nosql_connection_pool::nosql_connection_pool()
{
	m_CurConn = 0;
	m_FreeConn = 0;
}

nosql_connection_pool *nosql_connection_pool::GetInstance()
{
	static nosql_connection_pool connPool;
	return &connPool;
}

//构造初始化
void nosql_connection_pool::init(string ip,string port, int MaxConn)
{
	m_ip = ip;
	m_port = port;

	for (int i = 0; i < MaxConn; i++)
	{	
		redisContext *con = redisConnect("127.0.0.1", 6379);
		if (con->err)
		{
			redisFree(con);
			cout << "连接redis失败" << endl;
		}

		connList.push_back(con);
		++m_FreeConn;
	}

	reserve = sem(m_FreeConn);

	m_MaxConn = m_FreeConn;
}


//当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
redisContext *nosql_connection_pool::GetConnection()
{
	redisContext *con = NULL;

	if (0 == connList.size())
		return NULL;

	reserve.wait();
	
	lock.lock();

	con = connList.front();
	connList.pop_front();

	--m_FreeConn;
	++m_CurConn;

	lock.unlock();
	return con;
}

//释放当前使用的连接
bool nosql_connection_pool::ReleaseConnection(redisContext *con)
{
	if (NULL == con)
		return false;

	lock.lock();

	connList.push_back(con);
	++m_FreeConn;
	--m_CurConn;

	lock.unlock();

	reserve.post();
	return true;
}

//销毁数据库连接池
void nosql_connection_pool::DestroyPool()
{
	lock.lock();
	if (connList.size() > 0)
	{
		list<redisContext *>::iterator it;
		for (it = connList.begin(); it != connList.end(); ++it)
		{
			redisContext *con = *it;
			redisFree(con);
		}
		m_CurConn = 0;
		m_FreeConn = 0;
		connList.clear();
	}

	lock.unlock();
}

//当前空闲的连接数
int nosql_connection_pool::GetFreeConn()
{
	return this->m_FreeConn;
}

nosql_connection_pool::~nosql_connection_pool()
{
	DestroyPool();
}

nosql_connectionRAII::nosql_connectionRAII(redisContext **SQL, nosql_connection_pool *connPool){
	*SQL = connPool->GetConnection();
	
	conRAII = *SQL;
	poolRAII = connPool;
}

nosql_connectionRAII::~nosql_connectionRAII(){
	poolRAII->ReleaseConnection(conRAII);
}