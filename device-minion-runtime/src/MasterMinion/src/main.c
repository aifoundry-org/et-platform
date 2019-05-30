#include "master.h"
#include "shire.h"

#include <stdint.h>

void __attribute__((noreturn)) main(void)
{
    uint64_t temp;

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
