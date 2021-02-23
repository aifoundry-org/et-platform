#include <unwind.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"

static _Unwind_Reason_Code trace_func(struct _Unwind_Context *ctx, void *d);

static _Unwind_Reason_Code trace_func(struct _Unwind_Context *ctx, void *d)
{
    int *depth = (int *)d;
    //Log_Write(LOG_LEVEL_CRITICAL, "\t#%d: program counter at %08x\n", *depth, _Unwind_GetIP(ctx));
    (*depth)++;
    return _URC_NO_REASON;
}

void print_backtrace(void)
{
    int depth = 0;
    _Unwind_Reason_Code code = _Unwind_Backtrace(&trace_func, &depth);
}

#pragma GCC diagnostic pop
