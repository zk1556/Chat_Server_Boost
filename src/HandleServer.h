#ifndef _HANDLE_SERVER_H
#define _HANDLE_SERVER_H

#include<iostream>
#include<unordered_map>
#include<set>
#include "./lock/locker.h"
#include "./mysql_pool/sql_connection_pool.h"
#include "./redis_pool/nosql_connection_pool.h"
#include<unistd.h>
#include<string.h>
#include<vector>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<mysql/mysql.h>
#include<pthread.h>
#include<fcntl.h>
#include<boost/bind.hpp>
#include<boost/asio.hpp>
#include "bitset.h"
using namespace std;

class HandleServer{
public:
    HandleServer(string port, string user, string passWord, string databaseName,int sql_num,
    string nosql_ip,string nosql_port,int nosql_num){
        //Msyql
        m_port=port;
        m_user=user;
        m_passWord=passWord;
        m_databaseName=databaseName;
        m_sql_num=sql_num;
        
        //Redis
        m_nosql_ip=nosql_ip;
        m_nosql_port=nosql_port;
        m_nosql_num=nosql_num;
    }
    ~HandleServer(){
       
    }
    void run(); //运行服务器

    void thread_pool();

    void sql_pool(); //初始化sql连接池

    void handle_all_request(int arg);

    void login_user(string str,int cfd,bool &if_login,string &login_name); //登录用户

    void register_user(string str,int cfd); //注册用户

    void test_redis(); //测试redis

    void exits_user(string str, int cfd); //redis判断用户是否存在

    size_t hash_func(string name);  //哈希函数

    void bloom_init(); //布隆过滤器初始化

private:
    locker mutx;//互斥锁，锁住需要修改name_sock_map的临界区

    locker group_mutx;//互斥锁，锁住修改group_map的临界区
 
    unordered_map<string,int> name_sock_map;//名字和套接字描述符

    unordered_map<int,set<int>> group_map;//记录群号和套接字描述符集合

     //sql
    string m_port;

    sql_connection_pool *m_sql_connPool;

    string m_user;         //登陆数据库用户名

    string m_passWord;     //登陆数据库密码

    string m_databaseName; //使用数据库名

    int m_sql_num;

    //nosql
    nosql_connection_pool *m_nosql_connPool;

    string m_nosql_ip;

    string m_nosql_port;

    int m_nosql_num;

    //布隆过滤器
    bitset<UINT_MAX> bloom_filter;
};



#endif
