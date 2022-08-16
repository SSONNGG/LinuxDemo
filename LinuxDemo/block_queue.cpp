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

	//��ն���
	void clear()
	{

	}

	~block_queue()
	{

	}

	//�ж϶����Ƿ�����
	bool isFull()
	{

	}

	//�ж϶����Ƿ�Ϊ��
	bool isEmpty()
	{

	}

	//��ȡ����Ԫ��
	bool front()
	{

	}

	//��ȡ��βԪ��
	bool back()
	{

	}

	//��ȡ���д�С
	int size()
	{

	}

	//��ȡ�������ֵ
	int max_size()
	{

	}

	//���������Ԫ�أ��Ȼ�����Ҫʹ�ö��е��߳�
	//�����ǰû���̵߳ȴ���������������������
	bool push()
	{

	}

	//�Ӷ����е���Ԫ�أ��������û��Ԫ�أ����ȴ���������
	bool pop(T& item)
	{

	}

	//����Ԫ��ʱ���ӳ�ʱ����
	bool pop(T& item, int ms_timeout)
	{

	}

};
#endif // !BLOCK_QUEUE_H
