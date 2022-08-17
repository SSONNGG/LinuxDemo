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

}

connection_pool::~connection_pool()
{
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

connection_pool* connection_pool::GetInstance()
{
	return nullptr;
}

void connection_pool::init(string url, string user, string password, string database, int port, int maxConn, int closeLog)
{
}

connection_pool_RAII::connection_pool_RAII(MYSQL** con, connection_pool* coonPool)
{
}

connection_pool_RAII::~connection_pool_RAII()
{
}
