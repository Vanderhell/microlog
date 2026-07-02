#define MLOG_BUF_SIZE 2
#include "mlog.h"

void min_buf_size_consumer(void)
{
    mlog_t log;
    mlog_init(&log);
    mlog_log(&log, MLOG_INFO, "", "x");
}
