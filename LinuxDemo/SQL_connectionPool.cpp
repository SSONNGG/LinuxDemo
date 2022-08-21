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

//��ʼ��
void connection_pool::init(string url, string user, string password, string database, int port, int maxConn, int closeLog)
{
	//��ʼ�����ݿ���Ϣ
	m_Url = url;
	m_User = user;
	m_Password = password;
	m_Database = database;
	m_Port = port;
	m_close_log = closeLog;
	//����maxConn�����ݿ�����
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
		//�������ݿ����ӳغͿ���������
		connList.push_back(con);
		++m_FreeConn;
	}
	//���ź�����ʼ��Ϊ������Ӵ���
	reserve = sem(m_FreeConn);
	m_MaxConn = m_FreeConn;
}

//�������ݿ���������ʱ�������ݿ����ӳط���һ���������ӣ�����ʹ�úͿ���������
MYSQL* connection_pool::GetConnection()
{
	MYSQL* conn = NULL;

	if (0 == connList.size())
	{
		return NULL;
	}
	//ȡ�����ӣ��ź���ԭ��-1������ź���ԭ��Ϊ0���������ȴ�
	reserve.wait();

	lock.lock();

	conn = connList.front();
	connList.pop_front();

	--m_FreeConn;
	++m_CurConn;

	lock.unlock();
	return conn;
}

//�ͷŵ�ǰʹ�õ�����
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
	//�ͷź��ź���ԭ��+1
	reserve.post();
	return true;
}

//�������ݿ����ӳ�
void connection_pool::DestoryPool()
{
	lock.lock();
	if (connList.size() > 0)
	{
		//ʹ�õ������������ر����ݿ�����
		list<MYSQL*>::iterator it;
		for (it = connList.begin(); it != connList.end(); ++it)
		{
			MYSQL* conn = *it;
			mysql_close(conn);
		}
		m_CurConn = 0;
		m_FreeConn = 0;
		//���List
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
