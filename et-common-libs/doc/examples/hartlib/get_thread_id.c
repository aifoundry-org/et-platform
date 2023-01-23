/*
  Examples of using get_thread_id api
  Below is an example which demonstrates how to print thread id for only thread 0.
*/

/* Include api specific header */
#include <etsoc/isa/hart.h>

int main(void)
{
    /* Only execute on thread 0 */
    if (get_thread_id() == 0)
    {
        /* Print thread ID */
        et_printf("Thread ID %d\n", get_thread_id());
    }

    return 0;
}