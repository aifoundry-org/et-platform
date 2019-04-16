#include "shire.h"
#include "worker.h"

#include <stdint.h>

void __attribute__((noreturn)) main(void)
{
    uint64_t temp;

    // Setup supervisor trap vector and sscratch (supervisor stack pointer)
    asm volatile (
        "la    %0, trap_handler \n"
        "csrw  stvec, %0        \n"
        "csrw  sscratch, sp     \n" // initial saved stack pointer
        : "=&r" (temp)
    );

    // Enable supervisor software interrupts
    asm volatile (
        "csrsi sie, 0x2     \n" // Enable supervisor software interrupts
        "csrsi sstatus, 0x2 \n" // Enable interrupts
    );

    WORKER_thread();
}
