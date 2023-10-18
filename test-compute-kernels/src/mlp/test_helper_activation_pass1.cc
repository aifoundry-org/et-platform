#include "test_helper_activation_pass1.h"

void 
#ifdef __clang__
__attribute__ ((noinline))
#endif
test_helper_activation_pass1(uint32_t shire_id, uint32_t minion_id)
{
    __asm__ __volatile__ (
        "test_helper_activation_pass1_start_point:\n"
        ".global test_helper_activation_pass1_start_point\n"
    );

    #include "test_helper_activation_pass.h"

    __asm__ __volatile__ (
        "test_helper_activation_pass1_end_point:\n"
        ".global test_helper_activation_pass1_end_point\n"
    );
}

