#include<mysql/mysql.h>
#include<stdio.h>
#include<string>
#include<string.h>
#include<stdlib.h>
#include<list>
#include<pthread.h>
#include<iostream>
#include "SQL_connectionPool.h"

using namespace std;
connection_pool::connection_pool()
{
	m_CurConn = 0;
	m_FreeConn = 0;
}

connection_pool* connection_pool::GetInstance()
{
	static connection_pool connPool;
	return &connPool;
}

connection_pool::~connection_pool()
{
	DestoryPool();
}

//初始化
void connection_pool::init(string url, string user, string password, string database, int port, int maxConn, int closeLog)
{
	//初始化数据库信息
	m_Url = url;
	m_User = user;
	m_Password = password;
	m_Database = database;
	m_Port = port;
	m_close_log = closeLog;
	//创建maxConn条数据库连接
	for (int i = 0; i < maxConn; i++)
	{
		MYSQL* con = NULL;
		con = mysql_init(con);

		if (con == NULL)
		{
			LOG_ERROR("MYSQL ERROR");
			exit(1);
		}
		con = mysql_real_connect(con, url.c_str(), user.c_str(), password.c_str(), database.c_str(), port, NULL, 0);

		if (con == NULL)
		{
			LOG_ERROR("MYSQL ERROR");
			exit(1);
		}
		//更新数据库连接池和空闲连接数
		connList.push_back(con);
		++m_FreeConn;
	}
	//将信号量初始化为最大连接次数
	reserve = sem(m_FreeConn);
	m_MaxConn = m_FreeConn;
}

//当有数据库连接请求时，从数据库连接池返回一个可用连接，更新使用和空闲连接数
MYSQL* connection_pool::GetConnection()
{
	MYSQL* conn = NULL;

	if (0 == connList.size())
	{
		return NULL;
	}
	//取出连接，信号量原子-1，如果信号量原子为0，则阻塞等待
	reserve.wait();

	lock.lock();

	conn = connList.front();
	connList.pop_front();

	--m_FreeConn;
	++m_CurConn;

	lock.unlock();
	return conn;
}

//释放当前使用的连接
bool connection_pool::ReleaseConnection(MYSQL* conn)
{
	if (NULL == conn) 
	{
		return false;
	}	

	lock.lock();

	connList.push_back(conn);
	++m_FreeConn;
	--m_CurConn;

	lock.unlock();
	//释放后信号量原子+1
	reserve.post();
	return true;
}

//销毁数据库连接池
void connection_pool::DestoryPool()
{
	lock.lock();
	if (connList.size() > 0)
	{
		//使用迭代器遍历，关闭数据库连接
		list<MYSQL*>::iterator it;
		for (it = connList.begin(); it != connList.end(); ++it)
		{
			MYSQL* conn = *it;
			mysql_close(conn);
		}
		m_CurConn = 0;
		m_FreeConn = 0;
		//清空List
		connList.clear();
	}
	lock.unlock();
}


int connection_pool::GetFreeConn()
{
	return this->m_FreeConn;
}

connection_pool_RAII::connection_pool_RAII(MYSQL** SQL, connection_pool* coonPool)
{
	*SQL = coonPool->GetConnection();
	conRAII = *SQL;
	poolRAII = coonPool;
}

connection_pool_RAII::~connection_pool_RAII()
{
	poolRAII->ReleaseConnection(conRAII);
}
