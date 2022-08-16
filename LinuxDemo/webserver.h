#ifndef WEBSERVER_H
#define WEBSERVER_H

#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<unistd.h>
#include<errno.h>
#include<fcntl.h>
#include<stdlib.h>
#include<cassert>
#include<sys/epoll.h>

const int MAX_FD = 65536;			//最大文件描述符
const int MAX_EVENT_NUMBE = 10000;	//最大事件数
const int TIMESLOT = 5;				//最小超时单位

class WebServer
{
public:
	WebServer();
	~WebServer();

	void init(int port,)

};

#endif