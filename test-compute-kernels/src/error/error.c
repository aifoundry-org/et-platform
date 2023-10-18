#include <stdint.h>
#include <etsoc/isa/hart.h>

/* User error - useful for testing kernel user error handling of firmware */
int64_t entry_point(void)
{   
    /* Only even threads of shire 0 generate user error */
    if((get_shire_id() == 0) && (get_hart_id() % 2 == 0)) 
    {   
        return -10;
    }

    return 0;
}
