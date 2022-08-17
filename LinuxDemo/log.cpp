#include<string.h>
#include<time.h>
#include<sys/time.h>
#include<stdarg.h>
#include<pthread.h>
#include "log.h"

using namespace std;

//构造和析构
Log::Log()
{
    m_count = 0;
    m_is_async = false;
}

Log::~Log()
{
    if (m_fp != NULL)
    {
        fclose(m_fp);
    }
}

bool Log::init(const char* file_name, int close_log, int log_buf_size, int split_lines, int max_queue_size)
{
    //如果设置了max_queue_size，则设置为异步
    if (max_queue_size >= 1)
    {
        m_is_async = true;
        m_log_queue = new block_queue<string>(max_queue_size);
        pthread_t tid;
        //flush_log_thread为回调函数，表示创建线程异步写日志
    }
    return false;
}

void Log::write_log(int level, const char* format, ...)
{
}

void Log::flush(void)
{
}