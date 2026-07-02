#define MLOG_LEVEL_MIN MLOG_NONE
#include "mlog.h"

void level_min_none_consumer(void)
{
    MLOG_TRACE("TRACE", "suppressed");
    MLOG_DEBUG("DEBUG", "suppressed");
    MLOG_INFO("INFO", "suppressed");
    MLOG_WARN("WARN", "suppressed");
    MLOG_ERROR("ERROR", "suppressed");
}
