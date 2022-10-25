#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "locker.h"
#include "SQL_connectionPool.h"

//�̳߳ض���
template<typename T>
class threadpool
{
public:
	/// <summary>
	/// �̳߳س�ʼ�����캯��
	/// </summary>
	/// <param name="actor_model">ģ��</param>
	/// <param name="connPool">���ݿ����ӳ�</param>
	/// <param name="thread_number">�̳߳����̵߳�����</param>
	/// <param name="max_request">����������������ġ��ȴ���������������</param>
	threadpool(int actor_model, connection_pool* connPool, int thread_number = 8, int max_request = 10000);
	~threadpool();
	bool append(T* request, int state);
	bool append_p(T* request);
private:
	static void* worker(void* arg);	//�����߳����еĺ��������ϴӹ���������ȡ������ִ��
	void run();

private:
	int m_thread_number;		//�̳߳��е��߳���
	int m_max_requests;			//���������������������
	pthread_t* m_threads;		//�����̳߳ص����飬��СΪm_thread_number
	std::list<T*> m_workqueue;	//�������
	locker m_queuelocker;		//����������еĻ�����
	sem m_queuestat;			//�Ƿ���������Ҫ����
	connection_pool* m_connPool;	//���ݿ����ӳ�
	int m_actor_model;			//ģ���л�
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
