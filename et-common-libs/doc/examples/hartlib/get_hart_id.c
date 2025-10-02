/*
  Example of using get_hart_id api.
  Below is an example which demonstrates how to launch a specific message handler on a 
  specific hart id. Hart id can be retrieved using get_hart_id api and then a handler
  can be launched on it.
*/

/* Include api specific header */
#include <etsoc/isa/hart.h>
#include "utils.h"

/* Stub implementation */
void launch_msg_dispatcher()
{
    /* Process messages. */
}

int main(void)
{
    /* Print hart ID */
    et_printf("Hart ID %d\n", get_hart_id());

    /* Launch specific handler on specific hart */
    if (get_hart_id() == 1)
    {
        /* Launch a specific handler */
        launch_msg_handler();
    }

    return 0;
}