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

const int MAX_FD = 65536;			//����ļ�������
const int MAX_EVENT_NUMBE = 10000;	//����¼���
const int TIMESLOT = 5;				//��С��ʱ��λ

class WebServer
{
public:
	WebServer();
	~WebServer();

	void init(int port,)

};

#endif