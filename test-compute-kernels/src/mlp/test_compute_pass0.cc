// Local
#include "test_common.h"
#include "test_compute_pass0.h"

void 
#ifdef __clang__
__attribute__ ((noinline))
#endif
test_compute_pass0(uint32_t shire_id, uint32_t minion_id)
{
    __asm__ __volatile__ (
        "test_compute_pass0_start_point:\n"
        ".global test_compute_pass0_start_point\n"
    );

    #include "test_compute_pass.h"

    __asm__ __volatile__ (
        "test_compute_pass0_end_point:\n"
        ".global test_compute_pass0_end_point\n"
    );
}

