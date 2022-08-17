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
}

//≥ı ºªØ
void connection_pool::init(string url, string user, string password, string database, int port, int maxConn, int closeLog)
{
	m_Url = url;
	m_User = user;
	m_Password = password;
	m_Database = database;
	m_Port = port;
	m_close_log = closeLog;

	for (int i = 0; i < maxConn; i++)
	{
		MYSQL* con = NULL;
		con = mysql_init(con);

		if (con == NULL)
		{
			
		}
	}
}

MYSQL* connection_pool::GetConnection()
{
	return nullptr;
}

bool connection_pool::ReleaseConnection(MYSQL* conn)
{
	return false;
}

int connection_pool::GetFreeConn()
{
	return 0;
}

void connection_pool::DestoryPool()
{
}

connection_pool_RAII::connection_pool_RAII(MYSQL** con, connection_pool* coonPool)
{
}

connection_pool_RAII::~connection_pool_RAII()
{
}
