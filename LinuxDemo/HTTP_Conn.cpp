#include <mysql/mysql.h>
#include <fstream>

#include "HTTP_Conn.h"

//定义http响应状态
const char* ok_200_title = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form = "You don't have permission to get file from this server.\n";
const char* error_404_title = "Not Found";
const char* error_404_form = "The requested file wasn't found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unusual problem serving the request file.\n";

locker m_lock;
map<string, string>users;

//初始化数据库连接
void http_conn::initmysql_result(connection_pool* connPool)
{
	//从连接池中取出连接
	MYSQL* mysql = NULL;
	connection_pool_RAII mysqlconn(&mysql, connPool);

	//检索数据库表
	if (mysql_query(mysql, "SELECT username,pwd from user"))
	{
		LOG_ERROR("SELECT ERROR:%s\n", mysql_error(mysql));
	}
	//从数据库表中检索出完整的结果集
	MYSQL_RES* result = mysql_store_result(mysql);
	//返回结果集中的列数
	int num_fields = mysql_num_fields(result);
	//从结果集中获取下一行，将对应的用户名和密码存入map中
	while (MYSQL_ROW row=mysql_fetch_row(result))
	{
		string temp1(row[0]);
		string temp2(row[1]);
		users[temp1] = temp2;
	}
}

//对文件描述符设置非阻塞
int setnonblocking(int fd)
{
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_option);
	return old_option;
}

//将内核事件表注册读事件，ET模式，选择开启
void addfd(int epollfd, int fd, bool one_shot, int TRIGMode)
{
	epoll_event event;
	event.data.fd = fd;

	if (1 == TRIGMode)
	{
		event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
	}
	else
	{
		event.events = EPOLLIN | EPOLLRDHUP;
	}

	if (one_shot)
	{
		event.events |= EPOLLONESHOT;
	}
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
	setnonblocking(fd);
}

//从内核事件表删除描述符
void removefd(int epollfd, int fd)
{
	epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
	close(fd);
}

//将事件重置为EPOLLONESHOT
void modfd(int epollfd, int fd, int ev, int TRIGMode)
{
	epoll_event event;
	event.data.fd = fd;

	if (1 == TRIGMode)
	{
		event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
	}
	else
	{
		event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
	}
	epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

int http_conn::m_user_count = 0;
int http_conn::m_epollfd = -1;

//关闭一个连接，用户总量-1
void http_conn::close_conn(bool real_close)
{
	if (real_close && (m_sockfd != -1))
	{
		cout << "close " << m_sockfd << endl;
		removefd(m_epollfd, m_sockfd);
		m_sockfd = -1;
		m_user_count--;
	}
}

//初始化连接，外部调用初始化套接字
void http_conn::init(int sockfd, const sockaddr_in& addr, char* root, int TRIGMode, int close_log, string user, string password, string database)
{
	m_sockfd = sockfd;
	m_address = addr;

	addfd(m_epollfd, sockfd, true, m_TRIGMODE);
	m_user_count++;

	//当浏览器出现连接重置时，可能问题如下：
	//1.网站根目录出错
	//2.http响应格式出错
	//3.访问的文件中内容完全为空
	doc_root = root;
	m_TRIGMODE = TRIGMode;
	m_close_log = close_log;

	strcpy(sql_user, user.c_str());
	strcpy(sql_password, password.c_str());
	strcpy(sql_database, database.c_str());

	init();
}

//初始化新接受的连接
//check_state 默认为分析请求行状态
void http_conn::init()
{
	mysql = NULL;
	bytes_to_send = 0;
	bytes_have_send = 0;
	m_check_state = CHECK_STATE_REQUESTLINE;
	m_linger = false;
	m_method = GET;
	m_url = 0;
	m_version = 0;
	m_content_length = 0;
	m_host = 0;
	m_start_line = 0;
	m_checked_idx = 0;
	m_read_idx = 0;
	m_write_idx = 0;
	cgi = 0;
	m_state = 0;
	timer_flag = 0;
	improv = 0;

	memset(m_read_buf, '\0', READ_BUFFER_SIZE);
	memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
	memset(m_real_file, '\0', FILENAME_LEN);
}

//从状态机，用于分析出一行内容
//返回值为行的读取状态，有LINE_OK,LINE_BAD,LINE_OPEN
http_conn::LINE_STATUS http_conn::parse_line()
{
	char temp;
	for (; m_checked_idx < m_read_idx; ++m_checked_idx)
	{
		temp = m_read_buf[m_checked_idx];
		if (temp == '\r')
		{
			if ((m_checked_idx + 1) == m_read_idx)
			{
				return LINE_OPEN;
			}
			else if (m_read_buf[m_checked_idx+1]=='\n')
			{
				m_read_buf[m_checked_idx++] = '\0';
				m_read_buf[m_checked_idx++] = '\0';
				return LINE_OK;
			}
			return LINE_BAD;
		}
		else if (temp=='\n')
		{
			if (m_checked_idx > 1 && m_read_buf[m_checked_idx - 1] == '\r')
			{
				m_read_buf[m_checked_idx - 1] = '\0';
				m_read_buf[m_checked_idx++] = '\0';
				return LINE_OK;
			}
			return LINE_BAD;
		}
	}
	return LINE_OPEN;
}

