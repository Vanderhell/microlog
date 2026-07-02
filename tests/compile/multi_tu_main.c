#include "mlog.h"

void multi_tu_a(mlog_t *log);
void multi_tu_b(mlog_t *log);

int main(void)
{
    mlog_t log;
    mlog_init(&log);
    multi_tu_a(&log);
    multi_tu_b(&log);
    return 0;
}
