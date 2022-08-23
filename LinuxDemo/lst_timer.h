#ifndef LST_TIMER
#define LST_TIMER

#include"log.h"
#include <netinet/in.h>

//定时器，用于处理非活动连接，提升服务器性能
class util_timer;

struct client_data
{
	sockaddr_in address;
	int sockfd;
	util_timer* timer;
};

class util_timer
{
public:
	util_timer() : prev(NULL), next(NULL) {}

public:
	time_t expire;

	void(*cb_func)(client_data*);
	client_data* user_data;
	util_timer* prev;
	util_timer* next;
};

//定时器链表：一个升序的双向链表，且带有头结点和尾结点
class sort_timer_lst
{
public:
	sort_timer_lst();
	~sort_timer_lst();

	void add_timer(util_timer* timer);
	void adjust_timer(util_timer* timer);
	void del_timer(util_timer* timer);
	void tick();

private:
	void add_timer(util_timer* timer, util_timer* lst_head);

	util_timer* head;
	util_timer* tail;

};

class Utils
{
public:
	Utils() {}
	~Utils() {}

	void init(int timeslot);
	//对文件描述符设置非阻塞
	int setnonblocking(int fd);
	//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
	void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);
	//信号处理函数
	static void signal_handler(int sig);
	//设置信号函数
	void addsignal(int sig, void(handler)(int), bool restart = true);
	//定时处理任务，重新定时用来不断触发SIGALRM信号
	void timer_handler();

	void show_error(int connfd, const char* info);

public:
	static int* u_pipefd;
	sort_timer_lst m_timer_lst;
	static int u_epollfd;
	int m_TIMESLOT;
};

void cb_func(client_data* user_data);



#endif // !LST_TIMER


