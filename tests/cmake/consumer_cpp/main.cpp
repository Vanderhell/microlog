extern "C" {
#include "mlog.h"
}

int main()
{
    mlog_t log;
    mlog_init(&log);
    mlog_log(&log, MLOG_INFO, "CPP", "consumer");
    return 0;
}
