#include "log.h"

Log::~Log()
{
}

bool Log::init(const char* file_name, int close_log, int log_buf_size, int split_lines, int max_queue_size)
{
    return false;
}

void Log::write_log(int level, const char* format, ...)
{
}

void Log::flush(void)
{
}