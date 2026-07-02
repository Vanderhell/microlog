#include "mlog.h"

int main(void)
{
    mlog_t log;
    mlog_init(&log);
    mlog_log(&log, MLOG_INFO, "C", "consumer");
    return 0;
}
