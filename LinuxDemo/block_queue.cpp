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
	int m_front;	//front标记
	int m_back;		//back标记
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

	//清空队列，恢复队列到初始状态
	void clear()
	{
		m_mutex.lock();
		m_size = 0;
		m_front = -1;
		m_back = -1;
		m_mutex.unlock();
	}

	~block_queue()
	{
		m_mutex.lock();
		if (m_array != NULL)
		{
			delete[]m_array;
		}
		m_mutex.unlock();

	}

	//判断队列是否填满
	bool isFull()
	{
		m_mutex.lock();
		if (m_size >= m_max_size) {
			m_mutex.unlock();
			return true;
		}
		m_mutex.unlock();
		return false;
	}

	//判断队列是否为空
	bool isEmpty()
	{
		m_mutex.lock();
		if (0 == m_size) {
			m_mutex.unlock();
			return true;
		}
		m_mutex.unlock();
		return false;

	}

	//获取队首元素
	bool front(T &value)
	{
		m_mutex.lock();
		if (0 == m_size)	//当前队列没有空间，直接返回
		{
			m_mutex.unlock();
			return false;
		}
		value = m_array[m_front];
		m_mutex.unlock();
		return true;
	}

	//获取队尾元素
	bool back(T& value)
	{
		m_mutex.lock();
		if (0 == m_size)	//当前队列没有空间，直接返回
		{
			m_mutex.unlock();
			return false;
		}
		value = m_array[m_back];
		m_mutex.unlock();
		return true;
	}

	//获取队列大小
	int size()
	{
		int temp = 0;
		m_mutex.lock();
		temp = m_size;
		m_mutex.unlock();
		return temp;
	}

	//获取队列最大值
	int max_size()
	{
		int temp = 0;
		m_mutex.lock();
		temp = m_max_size;
		m_mutex.unlock();
		return temp;
	}

	//往队列添加元素，先唤醒需要使用队列的线程
	//如果当前没有线程等待条件变量，则唤醒无意义
	bool push(const T &item)
	{
		m_mutex.lock();
		if (m_size >= m_max_size)
		{
			m_cond.broadcast();
			m_mutex.unlock();
			return false;
		}
		m_back = (m_back + 1) % m_max_size;
		m_array[m_back] = item;
		m_size++;

		m_cond.broadcast();
		m_mutex.unlock();
		return true;
	}

	//从队列中弹出元素，如果队列没有元素，则会等待条件变量
	bool pop(T& item)
	{
		m_mutex.lock();
		while (m_size<=0)
		{
			if (!m_cond.wait(m_mutex.get()))
			{
				m_mutex.unlock();
				return false;
			}
		}
		m_front=(m_front+1)%m_max_size;
		item=m_array[m_front];
		m_size--;
		m_mutex.unlock();
		return true;
	}

	//弹出元素时增加超时处理
	bool pop(T& item, int ms_timeout)
	{
		struct timespec t = { 0,0 };
		struct timeval now = { 0,0 };

		gettimeofday(&now, NULL);
		m_mutex.lock();
		if (m_size <= 0)
		{
			t.tv_sec = now.tv_sec + ms_timeout / 1000;
			t.tv_nsec = (ms_timeout % 1000) * 1000;
			if (!m_cond.timewait(m_mutex.get(), t))
			{
				m_mutex.unlock();
				return false;
			}
		}
		if (m_size <= 0)
		{
			m_mutex.unlock();
			return false;
		}
		m_front = (m_front + 1) % m_max_size;
		item = m_array[m_front];
		m_size--;
		m_mutex.unlock();
		return true;

	}

};
#endif // !BLOCK_QUEUE_H
