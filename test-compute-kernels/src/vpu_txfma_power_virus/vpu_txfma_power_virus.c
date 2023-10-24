#include <etsoc/isa/hart.h>
#include "kernel.h"

typedef struct {
    uint64_t loop_size;
} Parameters;

int entry_point(const Parameters*);

int entry_point(const Parameters* const kernel_params_ptr) {
    int ret = 0;

    if (get_hart_id() == 0)
    {
        et_printf("vpu_txfma_power_virus..\n");
    }

    ret = vpu_power_virus(kernel_params_ptr->loop_size);

    if (ret != 0)
    {
        et_printf("vpu_txfma_power_virus: failed! status code: %d\n", ret);
    }

    return ret;
}

