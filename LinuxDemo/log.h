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
	char dir_name[128];					//路径名
	char log_name[128];					//log文件名
	int m_split_lines;					//日志最大行数
	int m_log_buf_size;					//日志缓冲区大小
	long long m_count;					//日志行数记录
	int m_today;						//当天时间
	FILE* m_fp;							//文件指针
	char* m_buf;
	block_queue<string>* m_log_queue;	//阻塞队列
	bool m_is_async;					//同步标志位
	locker m_mutex;						//锁
	int m_close_log;					//关闭日志

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
		//从阻塞队列中取出日志string，写入文件
		while (m_log_queue->pop(single_log))
		{
			m_mutex.lock();
			fputs(single_log.c_str(), m_fp);
			m_mutex.unlock();
		}
	}
};
#endif

