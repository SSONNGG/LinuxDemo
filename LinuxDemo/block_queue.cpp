//采用循环数组实现阻塞队列
#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H
#include<iostream>
#include<stdlib.h>
#include<pthread.h>
#include<sys/time.h>
#include "locker.h"

using namespace std;

template<class T>	//定义函数模板
class block_queue
{
private:
	locker m_mutex;
	cond m_cond;

	T* m_array;
	int m_size;
	int m_max_size;
	int m_front;
	int m_back;
public:
	block_queue(int max_size = 1000)
	{
		if (max_size <= 0)
		{
			exit(-1);
		}
		m_max_size = max_size;
		m_array = new T[max_size];
		m_size = 0;
		m_front = -1;
		m_back = -1;
	}

	//清空队列
	void clear()
	{

	}

	~block_queue()
	{

	}

	//判断队列是否填满
	bool isFull()
	{

	}

	//判断队列是否为空
	bool isEmpty()
	{

	}

	//获取队首元素
	bool front()
	{

	}

	//获取队尾元素
	bool back()
	{

	}

	//获取队列大小
	int size()
	{

	}

	//获取队列最大值
	int max_size()
	{

	}

	//往队列添加元素，先唤醒需要使用队列的线程
	//如果当前没有线程等待条件变量，则唤醒无意义
	bool push()
	{

	}

	//从队列中弹出元素，如果队列没有元素，则会等待条件变量
	bool pop(T& item)
	{

	}

	//弹出元素时增加超时处理
	bool pop(T& item, int ms_timeout)
	{

	}

};
#endif // !BLOCK_QUEUE_H
