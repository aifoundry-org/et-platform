#include <etsoc/isa/hart.h>

#define bool uint64_t
#define false 0x0ULL
#define true 0x1ULL

#include "utils.h"
#include "kernel.h"

typedef struct {
    uint64_t loop_size;
} Parameters;

int main(const Parameters* const kernel_params_ptr)
{
    int ret = SUCCESS;

    if (get_hart_id() == 0)
    {
        et_printf("vpu_tima_power_virus..\n");
    }

    if (get_thread_id() == 0)
    {
        ret = vpu_tima_power_virus(kernel_params_ptr->loop_size);

        if (ret != SUCCESS)
        {
            et_printf("vpu_tima_power_virus: failed! status code: %d\n", ret);
        }
    }

    return ret;
}
