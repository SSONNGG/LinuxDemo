#include <mysql/mysql.h>
#include <fstream>

#include "HTTP_Conn.h"

//����http��Ӧ״̬
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

//��ʼ�����ݿ�����
void http_conn::initmysql_result(connection_pool* connPool)
{
	//�����ӳ���ȡ������
	MYSQL* mysql = NULL;
	connection_pool_RAII mysqlconn(&mysql, connPool);

	//�������ݿ��
	if (mysql_query(mysql, "SELECT username,pwd from user"))
	{
		LOG_ERROR("SELECT ERROR:%s\n", mysql_error(mysql));
	}
	//�����ݿ���м����������Ľ����
	MYSQL_RES* result = mysql_store_result(mysql);
	//���ؽ�����е�����
	int num_fields = mysql_num_fields(result);
	//�ӽ�����л�ȡ��һ�У�����Ӧ���û������������map��
	while (MYSQL_ROW row=mysql_fetch_row(result))
	{
		string temp1(row[0]);
		string temp2(row[1]);
		users[temp1] = temp2;
	}
}

//���ļ����������÷�����
int setnonblocking(int fd)
{
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_option);
	return old_option;
}

//���ں��¼���ע����¼���ETģʽ��ѡ����
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

//���ں��¼���ɾ��������
void removefd(int epollfd, int fd)
{
	epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
	close(fd);
}

//���¼�����ΪEPOLLONESHOT
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

//�ر�һ�����ӣ��û�����-1
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

//��ʼ�����ӣ��ⲿ���ó�ʼ���׽���
void http_conn::init(int sockfd, const sockaddr_in& addr, char* root, int TRIGMode, int close_log, string user, string password, string database)
{
	m_sockfd = sockfd;
	m_address = addr;

	addfd(m_epollfd, sockfd, true, m_TRIGMODE);
	m_user_count++;

	//�������������������ʱ�������������£�
	//1.��վ��Ŀ¼����
	//2.http��Ӧ��ʽ����
	//3.���ʵ��ļ���������ȫΪ��
	doc_root = root;
	m_TRIGMODE = TRIGMode;
	m_close_log = close_log;

	strcpy(sql_user, user.c_str());
	strcpy(sql_password, password.c_str());
	strcpy(sql_database, database.c_str());

	init();
}

//��ʼ���½��ܵ�����
//check_state Ĭ��Ϊ����������״̬
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

//��״̬�������ڷ�����һ������
//����ֵΪ�еĶ�ȡ״̬����LINE_OK,LINE_BAD,LINE_OPEN
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

//ѭ����ȡ�û����ݣ�ֱ�������ݿɶ���Է��ر�����
//������ET����ģʽ�£���Ҫһ���Խ����ݶ���
bool http_conn::read_once()
{
	if (m_read_idx >= READ_BUFFER_SIZE)
	{
		return false;
	}
	int bytes_read = 0;

	//LT������
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
	else //ET������
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

//����http�����У�������󷽷���Ŀ��url�Լ�http�汾��
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
	//��url��ʾΪ/ʱ����ʾ�жϽ���
	if (strlen(m_url) == 1)
	{
		strcat(m_url, "judge.html");
	}
	m_check_state = CHECK_STATE_HEADER;
	return NO_REQUEST;
}