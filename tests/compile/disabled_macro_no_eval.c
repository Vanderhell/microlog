#define MLOG_LEVEL_MIN MLOG_NONE
#include "mlog.h"

static int side_effect(void)
{
    return 1;
}

void disabled_macro_no_eval(void)
{
    MLOG_DEBUG("SIDE", "value=%d", side_effect());
}
