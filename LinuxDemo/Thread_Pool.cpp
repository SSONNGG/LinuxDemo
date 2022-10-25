#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "locker.h"
#include "SQL_connectionPool.h"

//线程池定义
template<typename T>
class threadpool
{
public:
	/// <summary>
	/// 线程池初始化构造函数
	/// </summary>
	/// <param name="actor_model">模型</param>
	/// <param name="connPool">数据库连接池</param>
	/// <param name="thread_number">线程池中线程的数量</param>
	/// <param name="max_request">请求队列中最多允许的、等待处理的请求的数量</param>
	threadpool(int actor_model, connection_pool* connPool, int thread_number = 8, int max_request = 10000);
	~threadpool();
	bool append(T* request, int state);
	bool append_p(T* request);
private:
	static void* worker(void* arg);	//工作线程运行的函数，不断从工作队列中取出任务并执行
	void run();

private:
	int m_thread_number;		//线程池中的线程数
	int m_max_requests;			//请求队列中允许的最大请求
	pthread_t* m_threads;		//描述线程池的数组，大小为m_thread_number
	std::list<T*> m_workqueue;	//请求队列
	locker m_queuelocker;		//保护请求队列的互斥锁
	sem m_queuestat;			//是否有任务需要处理
	connection_pool* m_connPool;	//数据库连接池
	int m_actor_model;			//模型切换
};


template<typename T>
threadpool<T>::threadpool(int actor_model, connection_pool* connPool, int thread_number, int max_requests)
	:m_actor_model(actor_model)
	, m_thread_number(thread_number)
	, m_max_requests(max_requests)
	, m_threads(NULL)
	, m_connPool(connPool)
{
	if (thread_number <= 0 || max_requests <= 0)
	{
		throw std::exception();
	}
	m_threads = new pthread_t[m_thread_number];

	if (!m_threads)
		throw std::exception();
	for (int i = 0; i < thread_number; ++i)
	{
		if (pthread_create(m_threads + i, NULL, worker, this) != 0)
		{
			delete[]m_threads;
			throw std::exception();
		}

		if (pthread_detach(m_threads[i]))
		{
			delete[] m_threads;
			throw std::exception();
		}
	}
}

template<typename T>
threadpool<T>::~threadpool()
{
	delete[]m_threads;
}

template<typename T>
bool threadpool<T>::append(T* request, int state)
{
	m_queuelocker.lock();
	if (m_workqueue.size() >= m_max_requests)
	{
		m_queuelocker.unlock();
		return false;
	}
	request->m_state = state;
	m_workqueue.push_back(request);
	m_queuelocker.unlock();
	m_queuestat.post();
	return true;
}

template<typename T>
bool threadpool<T>::append_p(T* request)
{
	m_queuelocker.lock();
	if (m_workqueue.size() >= m_max_requests)
	{
		m_queuelocker.unlock();
		return false;
	}
	m_workqueue.push_back(request);
	m_queuelocker.unlock();
	m_queuestat.post();
	return true;
}

template<typename T>
void* threadpool<T>::worker(void* arg)
{
	threadpool* pool = (threadpool*)arg;
	pool->run();
	return pool;
}

template<typename T>
void threadpool<T>::run()
{
	while (true)
	{
		m_queuestat.wait();
		m_queuelocker.lock();
		if (m_workqueue.empty())
		{
			m_queuelocker.unlock();
			continue;
		}

		T* request = m_workqueue.front();
		m_workqueue.pop_front();
		m_queuelocker.unlock();

		if (!request)
		{
			continue;
		}
		if (1 == m_actor_model)
		{
			if (0 == request->m_state)
			{
				if (request->read_once())
				{
					request->improv = 1;
					connection_pool_RAII mysqlcon(&request->mysql, m_connPool);
					request->process()
				}
				else
				{
					request->improv = 1;
					request->timer_flag = 1;
				}
			}
			else
			{
				if (request->write())
				{
					request->improv = 1;
				}
				else
				{
					request->improv = 1;
					request->timer_flag = 1;
				}
			}
		}
		else
		{
			connection_pool_RAII mysqlcon(&request->mysql, m_connPool);
			request->process();
		}
	}
}


#endif
