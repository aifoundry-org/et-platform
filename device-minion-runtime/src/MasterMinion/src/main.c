#include "master.h"
#include "shire.h"

#include <stdint.h>

// Select PU peripherals for initial master minion use
#define PU_PLIC_BASE_ADDRESS  0x0010000000ULL
#define PU_TIMER_BASE_ADDRESS 0x0012005000ULL

void __attribute__((noreturn)) main(void)
{
    uint64_t temp;

    *(volatile uint64_t*)(0x12002010) = 0;

    // Configure supervisor trap vector and sscratch (supervisor stack pointer)
    asm volatile (
        "la    %0, trap_handler \n"
        "csrw  stvec, %0        \n" // supervisor trap vector
        "csrw  sscratch, sp     \n" // initial saved stack pointer
        : "=&r" (temp)
    );

    // Enable supervisor external and software interrupts
    asm volatile (
        "li    %0, 0x202    \n"
        "csrs  sie, %0      \n" // Enable supervisor external and software interrupts
        "csrsi sstatus, 0x2 \n" // Enable interrupts
        : "=&r" (temp)
    );

    MASTER_thread();
}
