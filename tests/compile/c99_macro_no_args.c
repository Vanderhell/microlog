#include "mlog.h"

void c99_macro_no_args(void)
{
    MLOG_INFO("TAG", "message");
    MLOG_ERROR("ERR", "fatal");
}
