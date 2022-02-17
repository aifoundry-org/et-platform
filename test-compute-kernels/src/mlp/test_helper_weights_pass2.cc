#include "test_helper_weights_pass2.h"

void test_helper_weights_pass2(uint32_t shire_id, uint32_t minion_id)
{
    __asm__ __volatile__ (
        "test_helper_weights_pass2_start_point:\n"
        ".global test_helper_weights_pass2_start_point\n"
    );

    #include "test_helper_weights_pass.h"

    __asm__ __volatile__ (
        "test_helper_weights_pass2_end_point:\n"
        ".global test_helper_weights_pass2_end_point\n"
    );
}

