#include "mlog.h"

static int sink;

int c99_macro_control_flow(int x)
{
    if (x > 0)
        MLOG_INFO("FLOW", "positive");
    else
        MLOG_WARN("FLOW", "non-positive");

    for (sink = 0; sink < 1; ++sink)
        MLOG_DEBUG("FLOW", "loop");

    do MLOG_ERROR("FLOW", "line one"); while (0); do MLOG_TRACE("FLOW", "line two"); while (0);

    if (x == 7)
        goto done;

    MLOG_INFO("FLOW", "after goto check");

done:
    return x;
}
