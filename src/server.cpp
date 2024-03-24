/*************************************************************************
	> File Name: server.cpp
	> Author: fuyinglong
	> Mail: 838106527@qq.com
	> Created Time: Sun Oct 18 16:06:56 2020
 ************************************************************************/


#include<iostream>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<fcntl.h>
#include<unistd.h>
#include<queue>
#include<chrono>
#include<boost/bind.hpp>
#include<boost/asio.hpp>
#include "HandleServer.h"
using namespace std;



int main(){
   
     //需要修改的数据库信息,登录名,密码,库名 
    string port = "3306";
    string user = "root";
    string passwd = "123456";
    string databasename = "test_connect";
    int sql_num = 8;
    
    string nosql_port = "6379";
    string nosql_ip = "127.0.0.1";
    int nosql_num = 8;

    HandleServer server(port,user,passwd,databasename,sql_num,nosql_ip,nosql_port,nosql_num);
    
    server.run();


  

}
