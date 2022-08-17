#ifndef LOG_H_
#define LOG_H_
#include<stdio.h>
#include<iostream>
#include<string>
#include<stdarg.h>
#include<pthread.h>
#include"block_queue.h"

using namespace std;

class Log
{
private:
	char dir_name[128];					//·����
	char log_name[128];					//log�ļ���
	int m_split_lines;					//��־�������
	int m_log_buf_size;					//��־��������С
	long long m_count;					//��־������¼
	int m_today;						//����ʱ��
	FILE* m_fp;							//�ļ�ָ��
	char* m_buf;
	block_queue<string>* m_log_queue;	//��������
	bool m_is_async;					//ͬ����־λ
	locker m_mutex;						//��
	int m_close_log;					//�ر���־

public:
	static Log* get_instance()
	{
		static Log instance;
		return &instance;
	}

	static void* flush_log_thread(void* args)
	{
		Log::get_instance()->async_write_log();
	}
	
	bool init(const char* file_name, int close_log, int log_buf_size = 8192, int split_lines = 5000000, int max_queue_size = 0);
	void write_log(int level,const char *format,...);
	void flush(void);

private:
	Log();
	virtual ~Log();
	void* async_write_log()
	{
		string single_log;
		//������������ȡ����־string��д���ļ�
		while (m_log_queue->pop(single_log))
		{
			m_mutex.lock();
			fputs(single_log.c_str(), m_fp);
			m_mutex.unlock();
		}
	}
};
#endif

