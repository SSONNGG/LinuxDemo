#include "lst_timer.h"

sort_timer_lst::sort_timer_lst()
{
	
}

sort_timer_lst::~sort_timer_lst()
{

}

void sort_timer_lst::add_timer(util_timer* timer)
{
}

void sort_timer_lst::adjust_timer(util_timer* timer)
{
}

void sort_timer_lst::del_timer(util_timer* timer)
{
}

void sort_timer_lst::tick()
{
}

void sort_timer_lst::add_timer(util_timer* timer, util_timer* lst_head)
{
}

void Utils::init(int timeslot)
{
}

int Utils::setnonblocking(int fd)
{
	return 0;
}

void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode)
{
}

void Utils::sig_handler(int sig)
{
}

void Utils::addsig(int sig, void(handler)(int), bool restart)
{
}

void Utils::timer_handler()
{
}

void Utils::show_error(int connfd, const char* info)
{
}

class Utils;
void cb_func(client_data* user_data)
{
}
