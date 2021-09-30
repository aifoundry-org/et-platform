// Local
#include "test_helper_code.h"
#include "test_helper_code_pass1.h"

void test_helper_code_pass1(uint32_t shire_id, uint32_t minion_id)
{
    (void) shire_id;
    (void) minion_id;
    __asm__ __volatile__ (
        "test_helper_code_pass1_start_point:\n"
        ".global test_helper_code_pass1_start_point\n"
    );

    // Helper code
    uint64_t start_pc = 0, end_pc = 0;
    __asm__ __volatile__ (
        "la %[start_pc], test_helper_code_pass1_start_point\n"
        "la %[end_pc],   test_helper_code_pass1_end_point\n"
      : [start_pc] "+&r" (start_pc),
        [end_pc] "+&r" (end_pc)
    );

    prefetch_code_l2(start_pc, end_pc);

    // Activation prefetch code
    __asm__ __volatile__ (
        "la %[start_pc], test_helper_activation_pass1_start_point\n"
        "la %[end_pc],   test_helper_activation_pass1_end_point\n"
      : [start_pc] "+&r" (start_pc),
        [end_pc] "+&r" (end_pc)
    );

    prefetch_code_l2(start_pc, end_pc);

    // Compute code
    __asm__ __volatile__ (
        "la %[start_pc], test_compute_pass1_start_point\n"
        "la %[end_pc],   test_compute_pass1_end_point\n"
      : [start_pc] "+&r" (start_pc),
        [end_pc] "+&r" (end_pc)
    );

    prefetch_code_l2(start_pc, end_pc);

    // Drain code
    __asm__ __volatile__ (
        "la %[start_pc], test_helper_drain_pass1_start_point\n"
        "la %[end_pc],   test_helper_drain_pass1_end_point\n"
      : [start_pc] "+&r" (start_pc),
        [end_pc] "+&r" (end_pc)
    );

    prefetch_code_l2(start_pc, end_pc);

    ////////////////////////////////////////////////
    // Prefetch to L1
    ////////////////////////////////////////////////

    // Helper code
    __asm__ __volatile__ (
        "la %[start_pc], test_helper_code_pass1_start_point\n"
        "la %[end_pc],   test_helper_code_pass1_end_point\n"
      : [start_pc] "+&r" (start_pc),
        [end_pc] "+&r" (end_pc)
    );

    prefetch_code_l1_icache(start_pc, end_pc, 8); // Only for minion 31

    // Activation prefetch code
    __asm__ __volatile__ (
        "la %[start_pc], test_helper_activation_pass1_start_point\n"
        "la %[end_pc],   test_helper_activation_pass1_end_point\n"
      : [start_pc] "+&r" (start_pc),
        [end_pc] "+&r" (end_pc)
    );

    prefetch_code_l1_icache(start_pc, end_pc, 3); // Only for minions 0..15

    // Compute code
    __asm__ __volatile__ (
        "la %[start_pc], test_compute_pass1_start_point\n"
        "la %[end_pc],   test_compute_pass1_end_point\n"
      : [start_pc] "+&r" (start_pc),
        [end_pc] "+&r" (end_pc)
    );

    prefetch_code_l1_icache(start_pc, end_pc, 0xf); // For all minions

    // Drain code
    __asm__ __volatile__ (
        "la %[start_pc], test_helper_drain_pass1_start_point\n"
        "la %[end_pc],   test_helper_drain_pass1_end_point\n"
      : [start_pc] "+&r" (start_pc),
        [end_pc] "+&r" (end_pc)
    );

    prefetch_code_l1_icache(start_pc, end_pc, 0x8); // For neigh 3 only

    __asm__ __volatile__ (
        "test_helper_code_pass1_end_point:\n"
        ".global test_helper_code_pass1_end_point\n"
    );
}

