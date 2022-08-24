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

//循环读取用户数据，直到无数据可读或对方关闭连接
//非阻塞ET工作模式下，需要一次性将数据读完
bool http_conn::read_once()
{
	if (m_read_idx >= READ_BUFFER_SIZE)
	{
		return false;
	}
	int bytes_read = 0;

	//LT读数据
	if (0 == m_TRIGMODE)
	{
		bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
		m_read_idx += bytes_read;

		if (bytes_read <= 0)
		{
			return false;
		}
		return true;
	}
	else //ET读数据
	{
		while (true)
		{
			bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
			if (bytes_read == -1)
			{
				if (errno == EAGAIN || errno == EWOULDBLOCK)
				{
					break;
				}
				return false;
			}
			else if (bytes_read == 0)
			{
				return false;
			}
			m_read_idx += bytes_read;
		}
		return true;
	}
}

//解析http请求行，获得请求方法，目标url以及http版本号
http_conn::HTTP_CODE http_conn::parse_request_line(char* text)
{
	m_url = strpbrk(text, " \t");
	if (!m_url)
	{
		return BAD_REQUEST;
	}
	*m_url++ = '\0';
	char* method = text;
	if (strcasecmp(method, "GET") == 0)
	{
		m_method == GET;
	}
	else if(strcasecmp(method,"POST")==0)
	{
		m_method = POST;
		cgi = 1;
	}
	else
	{
		return BAD_REQUEST;
	}
	m_url += strspn(m_url, " \t");
	m_version = strpbrk(m_url, " \t");
	
	if (!m_version)
	{
		return BAD_REQUEST;
	}
	*m_version++ = '\0';
	m_version += strspn(m_version, " \t");
	if (strcasecmp(m_version, "HTTP/1.1") != 0)
	{
		return BAD_REQUEST;
	}
	if (strncasecmp(m_url, "http://", 7) == 0)
	{
		m_url += 7;
		m_url = strchr(m_url, '/');
	}

	if (strncasecmp(m_url, "https://", 8) == 0)
	{
		m_url += 8;
		m_url = strchr(m_url, '/');
	}

	if (!m_url || m_url[0] != '/')
	{
		return BAD_REQUEST;
	}
	//当url显示为/时，显示判断界面
	if (strlen(m_url) == 1)
	{
		strcat(m_url, "judge.html");
	}
	m_check_state = CHECK_STATE_HEADER;
	return NO_REQUEST;
}

//解析http请求头
http_conn::HTTP_CODE http_conn::parse_headers(char* text)
{
	if (text[0] == '\0')
	{
		if (m_content_length != 0)
		{
			m_check_state = CHECK_STATE_CONTENT;
			return NO_REQUEST;
		}
		return GET_REQUEST;
	}
	else if (strncasecmp(text, "Connection", 11) == 0)
	{
		text += 11;
		text += strspn(text, " \t");
		if (strcasecmp(text, "keep-alive") == 0)
		{
			m_linger = true;
		}
	}
	else if (strncasecmp(text, "Content-length", 15) == 0)
	{
		text += 15;
		text += strspn(text, " \t");
		m_content_length = atol(text);
	}
	else if (strncasecmp(text, "Host:", 5))
	{
		text += 5;
		text += strspn(text, " \t");
		m_host = text;
	}
	else
	{
		LOG_INFO("UNKNOW HEADER!PLEASE RETRY:%s", text);
	}
	return NO_REQUEST;

}

//判断http请求是否被完整读入
http_conn::HTTP_CODE http_conn::parse_content(char* text)
{
	if (m_read_idx >= (m_content_length + m_checked_idx))
	{
		text[m_content_length] = '\0';
		//POST请求中最后为输入的用户名和密码
		m_string = text;
		return GET_REQUEST;
	}
	return NO_REQUEST;
}

http_conn::HTTP_CODE http_conn::do_request()
{
	strcpy(m_real_file, doc_root);
	int len = strlen(doc_root);
	const char* p = strrchr(m_url, '/');

	//处理CGI
	if (cgi == 1 && (*(p + 1) == '2' || *(p + 1) == '3'))
	{
		//根据标志判断是登录检测还是注册检测
		char flag = m_url[1];
		char* m_url_real = (char*)malloc(sizeof(char) * 200);
		strcpy(m_url_real, "/");
		strcpy(m_url_real, m_url + 2);
		strncpy(m_real_file + len, m_url_real, FILENAME_LEN - len - 1);
		free(m_url_real);

		//提取用户名和密码
		char name[100], password[100];
		int i;
		for (i = 5; m_string[i] != '&'; ++i)
		{
			name[i - 5] = m_string[i];
		}
		name[i - 5] = '\0';

		int j = 0;
		for (i = i + 10; m_string[i] != '\0'; ++i, ++j) {
			password[j] = m_string[i];
		}
		password[j] = '\0';

		if (*(p + 1) == '3')	//注册，先检测数据库中是否有重名的用户
		{
			char* sql_insert = (char*)malloc(sizeof(char) * 200);
			strcpy(sql_insert, "INSERT INTO user(username,pwd) Values(");
			strcat(sql_insert, "'");
			strcat(sql_insert, name);
			strcat(sql_insert, "' , '");
			strcat(sql_insert, password);
			strcat(sql_insert, "')");

			if (users.find(name) == users.end())
			{
				m_lock.lock();
				int res = mysql_query(mysql, sql_insert);
				users.insert(pair<string, string>(name, password));
				m_lock.unlock();

				if (!res)
				{
					strcpy(m_url, "/log.html");
				}
				else
				{
					strcpy(m_url, "/registerError.html");
				}
			}
			else
			{
				strcpy(m_url, "/registerError.html");
			}
		}
		else if (*(p + 1) == '2')	//如果是登录，直接判断
		{
			if (users.find(name) != users.end() && users[name] == password)
			{
				strcpy(m_url, "/welcome.html");
			}
			else
			{
				strcpy(m_url, "/logError.html");
			}
		}
	}
	if (*(p + 1) == '0')
	{
		char* m_url_real = (char*)malloc(sizeof(char) * 200);
		strcpy(m_url_real, "/register.html");
		strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

		free(m_url_real);
	}
	else if (*(p + 1) == '1')
	{
		char* m_url_real = (char*)malloc(sizeof(char) * 200);
		strcpy(m_url_real, "/log.html");
		strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

		free(m_url_real);
	}
	else if (*(p + 1) == '5')
	{
		char* m_url_real = (char*)malloc(sizeof(char) * 200);
		strcpy(m_url_real, "/picture.html");
		strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

		free(m_url_real);
	}
	else if (*(p + 1) == '6')
	{
		char* m_url_real = (char*)malloc(sizeof(char) * 200);
		strcpy(m_url_real, "/video.html");
		strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

		free(m_url_real);
	}
	else if (*(p + 1) == '7')
	{
		char* m_url_real = (char*)malloc(sizeof(char) * 200);
		strcpy(m_url_real, "/fans.html");
		strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

		free(m_url_real);
	}
	else
	{
		strncpy(m_real_file + len, m_url, FILENAME_LEN - len - 1);
	}
	
	if (stat(m_real_file, &m_file_stat) < 0)
	{
		return NO_RESOURCE;
	}

	if (!(m_file_stat.st_mode & S_IROTH))
	{
		return FORBIDDEN_REQUEST;
	}

	if (S_ISDIR(m_file_stat.st_mode))
	{
		return BAD_REQUEST;
	}

	int fd = open(m_real_file, O_RDONLY);
	m_file_address = (char*)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	return FILE_REQUEST;
}


void http_conn::unmap()
{
	if (m_file_address)
	{
		munmap(m_file_address, m_file_stat.st_size);
		m_file_address = 0;
	}
}

bool http_conn::write()
{
	int temp = 0;

	if (bytes_to_send == 0)
	{
		modfd(m_epollfd, m_sockfd, EPOLLIN, m_TRIGMODE);
		init();
		return true;
	}

	while (1)
	{
		temp = writev(m_sockfd, m_iv, m_iv_count);

		if (temp < 0)
		{
			if (errno == EAGAIN)
			{
				modfd(m_epollfd, m_sockfd, EPOLLOUT, m_TRIGMODE);
				return true;
			}
			unmap();
			return false;
		}

		bytes_have_send += temp;
		bytes_to_send -= temp;
		if (bytes_have_send >= m_iv[0].iov_len)
		{
			m_iv[0].iov_len = 0;
			m_iv[1].iov_base = m_file_address + (bytes_have_send - m_write_idx);
			m_iv[1].iov_len = bytes_to_send;
		}
		else
		{
			m_iv[0].iov_base = m_write_buf + bytes_have_send;
			m_iv[0].iov_len = m_iv[0].iov_len - bytes_have_send;
		}

		if (bytes_to_send <= 0)
		{
			unmap();
			modfd(m_epollfd, m_sockfd, EPOLLIN, m_TRIGMODE);

			if (m_linger)
			{
				init();
				return true;
			}
			else
			{
				return false;
			}
		}
	}
}

bool http_conn::add_response(const char* format, ...)
{
	if (m_write_idx >= WRITE_BUFFER_SIZE)
	{
		return false;
	}
	va_list arg_list;
	va_start(arg_list, format);
	int len = vsnprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - 1 - m_write_idx, format, arg_list);
	if (len >= (WRITE_BUFFER_SIZE - 1 - m_write_idx))
	{
		va_end(arg_list);
		return false;
	}
	m_write_idx += len;
	va_end(arg_list);

	LOG_INFO("request:%s", m_write_buf);

	return true;
}