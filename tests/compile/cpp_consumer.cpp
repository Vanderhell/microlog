extern "C" {
#include "mlog.h"
}

void cpp_consumer_compile(mlog_t *log)
{
    mlog_log(log, MLOG_INFO, "CPP", "consumer");
}
