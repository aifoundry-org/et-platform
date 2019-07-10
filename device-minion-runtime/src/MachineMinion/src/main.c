#include "shire.h"
#include "esr_defines.h"
#include "layout.h"
#include "message.h"

#include <stdint.h>

void __attribute__((noreturn)) main(void)
{
    uint64_t temp;
    // "Upon reset, a hart's privilege mode is set to M. The mstatus fields MIE and MPRV are reset to 0.
    // The pc is set to an implementation-defined reset vector. The mcause register is set to a value
    // indicating the cause of the reset. All other hart state is undefined."

    asm volatile (
        "la    %0, trap_handler     \n" // Setup machine mode trap handler
        "csrw  mtvec, %0            \n"
        "li    %0, 0x333            \n" // Delegate supervisor and user software, timer and external interrupts to supervisor mode
        "csrs  mideleg, %0          \n"
        "li    %0, 0x100            \n" // Delegate user environment calls to supervisor mode
        "csrs  medeleg, %0          \n"
        "csrw  mscratch, sp         \n" // Initial saved stack pointer points to M-mode stack scratch region
        "csrwi menable_shadows, 0x3 \n" // Enable shadow registers for hartid and sleep txfma
        "csrsi mie, 0x8             \n" // Enable machine software interrupts
        "csrsi mstatus, 0x8         \n" // Enable interrupts
        : "=&r" (temp)
    );

    // TODO read OTP table of which minion shires are healthy and usable? Or are 0-32 logical always usable?
    // TODO file ticket to track this.

    asm volatile (
        "li    %0, 0x1020  \n" // Setup for mret into S-mode: bitmask for mstatus MPP[1] and SPIE
        "csrc  mstatus, %0 \n" // clear mstatus MPP[1] = supervisor mode, SPIE = interrupts disabled
        "li    %0, 0x800   \n" // bitmask for mstatus MPP[0]
        "csrs  mstatus, %0 \n" // set mstatus MPP[0] = supervisor mode
        : "=&r" (temp)
    );

    // Block user-level PC redirection
    volatile uint64_t* const ipi_redirect_filter_ptr = ESR_SHIRE(PRV_M, 0xFF, IPI_REDIRECT_FILTER);
    *ipi_redirect_filter_ptr = 0;

    if (get_shire_id() == 32)
    {
        const uint64_t* const master_entry = (uint64_t*)FW_MASTER_SMODE_ENTRY;

        // Set MPROT[2] for neighborhood 0 in master shire to disable access to OS region
        volatile uint64_t* mprot_ptr = ESR_NEIGH(PRV_M, 0xFF, 0, MPROT);
        *mprot_ptr = 0x4U;

        // Set MPROT[0:2] for neighborhoods 1-3 in master shire to disable access to OS, PCI-E and IO regions
        // TODO FIXME master shire only has 1 neighborhood in mini-SoC Zebu image
        // mprot_ptr = ESR_NEIGH(PRV_M, 0xFF, 1, MPROT);
        // *mprot_ptr = 0x7U;

        // mprot_ptr = ESR_NEIGH(PRV_M, 0xFF, 2, MPROT);
        // *mprot_ptr = 0x7U;

        // mprot_ptr = ESR_NEIGH(PRV_M, 0xFF, 3, MPROT);
        // *mprot_ptr = 0x7U;

        // Jump to master firmware in supervisor mode
        asm volatile (
            "csrw  mepc, %0 \n" // write return address
            "mret           \n" // return in S-mode
            :
            : "r" (master_entry)
        );
    }
    else
    {
        const uint64_t* const worker_entry = (uint64_t*)FW_WORKER_SMODE_ENTRY;

        // Set MPROT[2:0] for all neighborhoods in worker shires to disable access to OS, PCI-E and IO regions
        volatile uint64_t* const mprot_ptr = ESR_NEIGH(PRV_M, 0xFF, 0xF, MPROT);
        *mprot_ptr = 0x7U;

        // Jump to worker firmware in supervisor mode
        asm volatile (
            "csrw  mepc, %0 \n" // write return address
            "mret           \n" // return in S-mode
            :
            : "r" (worker_entry)
        );
    }

    while (1)
    {
        // Should never get here
    };
}
