#ifndef _HANDLECLIENT_H
#define _HANDLECLIENT_H

#include<iostream>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include <pthread.h>
#include <fcntl.h>
#include <fstream>

using namespace std;

class HandleClient{
public:
	HandleClient(){

	}
	~HandleClient(){	
   		close(cfd);
	}
	void init();
	void run();
	//线程执行此函数来发送消息
	static void *handle_send(void *arg);

//线程执行此函数来接收消息
	static  void *handle_recv(void *arg);

private:
	int cfd;
};



#endif
