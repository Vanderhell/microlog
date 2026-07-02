extern "C" {
#include "mlog.h"
}

static void cpp_consumer_compile(mlog_t *log)
{
    mlog_log(log, MLOG_INFO, "CPP", "consumer");
}

int main()
{
    mlog_t log;
    mlog_init(&log);
    cpp_consumer_compile(&log);
    return 0;
}
