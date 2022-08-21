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

//���ݿ����ӳ�
class connection_pool
{
private:
	connection_pool();
	~connection_pool();

	int m_MaxConn;			//���������
	int m_CurConn;			//��ǰ��ʹ�õ�������
	int m_FreeConn;			//��ǰ���е�������
	locker lock;
	list<MYSQL*> connList;	//���ݿ����ӳ�
	sem reserve;

public:
	string m_Url;			//������ַ	
	string m_Port;			//���ݿ�˿ں�
	string m_User;			//���ݿ��û���
	string m_Password;		//���ݿ�����
	string m_Database;		//���ݿ���
	int m_close_log;		//��־����

public:
	MYSQL* GetConnection();					//��ȡ���ݿ�����
	bool ReleaseConnection(MYSQL* conn);	//�ͷ�����
	int GetFreeConn();						//��ȡ����
	void DestoryPool();						//������������

	//ʹ�õ���ģʽ
	static connection_pool* GetInstance();
	//��ʼ��
	void init(string url, string user, string password, string database, int port, int maxConn, int closeLog);
};

class connection_pool_RAII
{
private:
	MYSQL* conRAII;
	connection_pool* poolRAII;
public:
	//˫ָ���MYSQL *conRAII�����޸�
	connection_pool_RAII(MYSQL** con, connection_pool* coonPool);
	~connection_pool_RAII();
};

#endif // !SQL_Conn_Pool

