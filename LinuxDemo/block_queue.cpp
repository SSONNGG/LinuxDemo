//����ѭ������ʵ����������
#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H
#include<iostream>
#include<stdlib.h>
#include<pthread.h>
#include<sys/time.h>
#include "locker.h"

using namespace std;

template<class T>	//���庯��ģ��
class block_queue
{
private:
	locker m_mutex;
	cond m_cond;

	T* m_array;
	int m_size;
	int m_max_size;
	int m_front;	//front���
	int m_back;		//back���
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

	//��ն��У��ָ����е���ʼ״̬
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

	//�ж϶����Ƿ�����
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

	//�ж϶����Ƿ�Ϊ��
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

	//��ȡ����Ԫ��
	bool front(T &value)
	{
		m_mutex.lock();
		if (0 == m_size)	//��ǰ����û�пռ䣬ֱ�ӷ���
		{
			m_mutex.unlock();
			return false;
		}
		value = m_array[m_front];
		m_mutex.unlock();
		return true;
	}

	//��ȡ��βԪ��
	bool back(T& value)
	{
		m_mutex.lock();
		if (0 == m_size)	//��ǰ����û�пռ䣬ֱ�ӷ���
		{
			m_mutex.unlock();
			return false;
		}
		value = m_array[m_back];
		m_mutex.unlock();
		return true;
	}

	//��ȡ���д�С
	int size()
	{
		int temp = 0;
		m_mutex.lock();
		temp = m_size;
		m_mutex.unlock();
		return temp;
	}

	//��ȡ�������ֵ
	int max_size()
	{
		int temp = 0;
		m_mutex.lock();
		temp = m_max_size;
		m_mutex.unlock();
		return temp;
	}

	//���������Ԫ�أ��Ȼ�����Ҫʹ�ö��е��߳�
	//�����ǰû���̵߳ȴ���������������������
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

	//�Ӷ����е���Ԫ�أ��������û��Ԫ�أ����ȴ���������
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

	//����Ԫ��ʱ���ӳ�ʱ����
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
