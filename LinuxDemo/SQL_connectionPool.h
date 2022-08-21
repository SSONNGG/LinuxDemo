#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

#include<stdio.h>
#include<list>
#include<mysql/mysql.h>
#include<error.h>
#include<string.h>
#include<iostream>
#include<string>
#include"locker.h"
#include"log.h"

//数据库连接池
class connection_pool
{
private:
	connection_pool();
	~connection_pool();

	int m_MaxConn;			//最大连接数
	int m_CurConn;			//当前已使用的连接数
	int m_FreeConn;			//当前空闲的连接数
	locker lock;
	list<MYSQL*> connList;	//数据库连接池
	sem reserve;

public:
	string m_Url;			//主机地址	
	string m_Port;			//数据库端口号
	string m_User;			//数据库用户名
	string m_Password;		//数据库密码
	string m_Database;		//数据库名
	int m_close_log;		//日志开关

public:
	MYSQL* GetConnection();					//获取数据库连接
	bool ReleaseConnection(MYSQL* conn);	//释放连接
	int GetFreeConn();						//获取连接
	void DestoryPool();						//销毁所有连接

	//使用单例模式
	static connection_pool* GetInstance();
	//初始化
	void init(string url, string user, string password, string database, int port, int maxConn, int closeLog);
};

class connection_pool_RAII
{
private:
	MYSQL* conRAII;
	connection_pool* poolRAII;
public:
	//双指针对MYSQL *conRAII进行修改
	connection_pool_RAII(MYSQL** con, connection_pool* coonPool);
	~connection_pool_RAII();
};

#endif // !SQL_Conn_Pool

