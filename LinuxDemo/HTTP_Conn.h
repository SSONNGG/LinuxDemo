/*
	HTTP 连接处理类，通过主从状态机封存Http连接类。
	主状态机在内部调用从状态机，从状态机将处理状态和数据传给主状态机。
*/
#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <map>

#include "locker.h"
#include "SQL_connectionPool.h"
#include "lst_timer.h"
#include "log.h"

class http_conn
{
public:
	static const int FILENAME_LEN = 200;
	static const int READ_BUFFER_SIZE = 2048;
	static const int WRITE_BUFFER_SIZE = 1024;
	enum METHOD
	{
		GET = 0,
		POST,
		HEAD,
		PUT,
		DELETE,
		TRACE,
		OPTIONS,
		CONNECT,
		PATH
	};
	enum CHECK_STATE //主状态机，用于标识解析的位置
	{
		CHECK_STATE_REQUESTLINE = 0, //解析请求行
		CHECK_STATE_HEADER,			 //解析请求头
		CHECK_STATE_CONTENT			 //解析消息体，用于解析POST请求
	};
	enum HTTP_CODE	//标识HTTP请求的处理结果
	{
		NO_REQUEST,			//请求不完整，需要继续读取请求报文数据
		GET_REQUEST,		//获得了完整的HTTP请求
		BAD_REQUEST,		//HTTP请求出现语法错误
		NO_RESOURCE,
		FORBIDDEN_REQUEST,
		FILE_REQUEST,
		INTERNAL_ERROR,		//服务器内部错误
		CLOSED_CONNECTION
	};
	enum LINE_STATUS //从状态机，用于标识解析一行的读取状态
	{
		LINE_OK = 0, //完整读取完一行
		LINE_BAD,	 //报文语法出现错误
		LINE_OPEN	 //读取的行不完整
	};

public:
	http_conn() {}
	~http_conn() {}

public:
	void init(int sockfd, const sockaddr_in &addr, char *, int, int, string user, string password, string database);
	void close_conn(bool real_close = true);
	void process();
	bool read_once();
	bool write();
	sockaddr_in *get_address()
	{
		return &m_address;
	}
	void initmysql_result(connection_pool *connPool);
	int timer_flag;
	int improv;

public:
	static int m_epollfd;
	static int m_user_count;
	MYSQL *mysql;
	int m_state; //读为0，写为1

private:
	void init();
	HTTP_CODE process_read();
	bool process_write(HTTP_CODE ret);
	HTTP_CODE parse_request_line(char *text);
	HTTP_CODE parse_headers(char *text);
	HTTP_CODE parse_content(char *text);
	HTTP_CODE do_request();
	char *get_line() { return m_read_buf + m_start_line; };
	LINE_STATUS parse_line();
	void unmap();
	bool add_response(const char *format, ...);
	bool add_content(const char *content);
	bool add_status_line(int status, const char *title);
	bool add_headers(int content_length);
	bool add_content_type();
	bool add_content_length(int content_length);
	bool add_linger();
	bool add_blank_line();

private:
	int m_sockfd;
	sockaddr_in m_address;
	char m_read_buf[READ_BUFFER_SIZE];
	int m_read_idx;
	int m_checked_idx;
	int m_start_line;
	char m_write_buf[WRITE_BUFFER_SIZE];
	int m_write_idx;
	CHECK_STATE m_check_state;
	METHOD m_method;
	char m_real_file[FILENAME_LEN];
	char *m_url;
	char *m_version;
	char *m_host;
	int m_content_length;
	bool m_linger;
	char *m_file_address;
	struct stat m_file_stat;
	struct iovec m_iv[2];
	int m_iv_count;
	int cgi;
	char *m_string;
	int bytes_to_send;
	int bytes_have_send;
	char *doc_root;

	map<string, string> user;
	int m_TRIGMODE;
	int m_close_log;

	char sql_user[100];
	char sql_password[100];
	char sql_database[100];
};

#endif