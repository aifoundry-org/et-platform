#include <etsoc/isa/hart.h>

#define bool uint64_t
#define false 0x0ULL
#define true 0x1ULL

#include "barrier.h"

#include "utils.h"
#include "kernel.h"

typedef struct {
    uint64_t loop_size;
} Parameters;

int entry_point(const Parameters* const kernel_params_ptr) {
    uint64_t sid = get_shire_id();
    /* Limit SCW test to only Shire0..Shire31 */
    if (sid < 32) {
        uint64_t ret = noc_power_virus(kernel_params_ptr->loop_size);
        if (ret == 0) return 0;
        else return -1;
    }
    return 0;
}
