#include <stdint.h>
#include <stddef.h>
#include <etsoc/common/utils.h>
#include <trace/trace_umode.h>

int64_t main(void)
{
    char str[] = "Message";

    et_printf("Hello from CM UMode.\n\r");
    et_printf("Now you can Debug me using Trace (~_^).\n\r");
    et_printf("Test: String \"%s\" has length %d.\n\r", str, et_strlen(str));

    /* Get some data bytes from U-mode range address */
    et_trace_memory((uint8_t*)0x8102000000, 128);

    /* Dump the GPRs state */
    et_trace_register();

    return 0;
}
