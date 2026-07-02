#define MLOG_MAX_BACKENDS 1
#include "mlog.h"

int max_backends_one_consumer(void)
{
    mlog_t log;
    mlog_init(&log);
    return (int)(sizeof(log.backends) / sizeof(log.backends[0]));
}
