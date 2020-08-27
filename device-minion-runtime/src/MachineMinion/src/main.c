#include "esr_defines.h"
#include "fcc.h"
#include "flb.h"
#include "hart.h"
#include "layout.h"
#include "message.h"

#include <stdint.h>

void __attribute__((noreturn)) main(void)
{
    uint64_t temp;

    // "Upon reset, a hart's privilege mode is set to M. The mstatus fields MIE and MPRV are reset to 0.
    // The pc is set to an implementation-defined reset vector. The mcause register is set to a value
    // indicating the cause of the reset. All other hart state is undefined."

    asm volatile(
        "la    %0, trap_handler     \n" // Setup machine mode trap handler
        "csrw  mtvec, %0            \n"
        "li    %0, 0x333            \n" // Delegate supervisor and user software, timer and external interrupts to supervisor mode
        "csrs  mideleg, %0          \n"
        "li    %0, 0x100            \n" // Delegate user environment calls to supervisor mode
        "csrs  medeleg, %0          \n"
        "csrwi menable_shadows, 0x3 \n" // Enable shadow registers for hartid and sleep txfma
        "csrsi mie, 0x8             \n" // Enable machine software interrupts
        "csrsi mstatus, 0x8         \n" // Enable interrupts
        : "=&r"(temp));

    asm volatile(
        "li    %0, 0x1020  \n" // Setup for mret into S-mode: bitmask for mstatus MPP[1] and SPIE
        "csrc  mstatus, %0 \n" // clear mstatus MPP[1] = supervisor mode, SPIE = interrupts disabled
        "li    %0, 0x800   \n" // bitmask for mstatus MPP[0]
        "csrs  mstatus, %0 \n" // set mstatus MPP[0] = supervisor mode
        : "=&r"(temp));

    // Enable counter number 3 for trace logging
    asm volatile (
        "csrsi mcounteren, 8  \n"
        "csrsi scounteren, 8  \n"
    );

    // Enable counter in 1st core of the neighbourhood only. Enabling it for all
    // cores will accumulate the cycles from all the cores and won't give us any
    // advantage
    if (get_hart_id() % 16 == 0 || get_hart_id() % 16 == 1)
    {
        // Enable mhpmevent3 for each neighborhood to count cycles
        asm volatile (
            "csrwi mhpmevent3, 1  \n"
        );
    }

    if (get_hart_id() % 64 == 0) // First HART every shire, master or worker
    {
        // Block user-level PC redirection
        volatile uint64_t *const ipi_redirect_filter_ptr =
            (volatile uint64_t *)ESR_SHIRE(THIS_SHIRE, IPI_REDIRECT_FILTER);
        *ipi_redirect_filter_ptr = 0;
    }

    if ((get_shire_id() == 32) &&
        ((get_minion_id() & 0x1F) < 16)) // Master shire non-sync minions (lower 16)
    {
        const uint64_t *const master_entry = (uint64_t *)FW_MASTER_SMODE_ENTRY;
        const uint32_t minion_mask = 0xFFFFU;
        bool result;

        // First HART in each neighborhood
        if (get_hart_id() % 16 == 0) {
            const uint64_t neighborhood_id = get_neighborhood_id();

            volatile uint64_t *const mprot_ptr =
                (volatile uint64_t *)ESR_NEIGH(THIS_SHIRE, neighborhood_id, MPROT);
            uint64_t mprot = *mprot_ptr;

            if (neighborhood_id == 0) {
                // Set MPROT for neighborhood 0 in master shire to allow acess to OS, PCI-E and IO regions
                mprot &= ~0x7ULL; // clear disable_pcie_access and set io_access_mode = b00 (user)
                mprot |= 0x40; // set enable_secure_memory to use M/S RX/RW regions
                *mprot_ptr = mprot;
            } else {
                // Set MPROT for neighborhoods 1-3 in master shire to disable access to OS, PCI-E and IO regions and enable secure memory permissions
                mprot |=
                    0x4E; // set enable_secure_memory, disable_osbox_access, disable_pcie_access and io_access_mode = b10 (disabled)
                *mprot_ptr = mprot;
            }

            // Wait for MPROT config on all 2 non-sync neighborhoods to complete before any thread can continue.
            WAIT_FLB(2, 31, result);

            if (result) {
                // Last neighborhood to configure MPROT sends FCC0 to all HARTs (other than sync-minions) in this shire
                // minion thread1s aren't enabled yet, so send FCC0 to 16 thread0s.
                SEND_FCC(THIS_SHIRE, THREAD_0, FCC_0, minion_mask);
            }
        }

        // Only thread0s participate in the initial MRPOT config rendezvous
        // thread1s boot up later, long after MPROT has been configured per neighborhood
        if (get_thread_id() == 0) {
            WAIT_FCC(0);
        }

        // Jump to master firmware in supervisor mode
        asm volatile("csrw  mepc, %0 \n" // write return address
                     "mret           \n" // return in S-mode
                     :
                     : "r"(master_entry));
    } else // Worker shire and Master shire sync-minions (upper 16)
    {
        const uint64_t *const worker_entry = (uint64_t *)FW_WORKER_SMODE_ENTRY;
        const uint32_t minion_mask = (get_shire_id() == MASTER_SHIRE) ? 0xFFFF0000U : 0xFFFFFFFFU;

        // First HART in each neighborhood
        if (get_hart_id() % 16 == 0) {
            const uint64_t neighborhood_id = get_neighborhood_id();

            // Set MPROT for all neighborhoods in worker shire to disable access to OS, PCI-E and IO regions and enable secure memory permissions
            volatile uint64_t *const mprot_ptr =
                (volatile uint64_t *)ESR_NEIGH(THIS_SHIRE, neighborhood_id, MPROT);
            uint64_t mprot = *mprot_ptr;
            mprot |=
                0x46; // set enable_secure_memory, disable_pcie_access and io_access_mode = b10 (disabled)
            *mprot_ptr = mprot;

            // minion thread1s aren't enabled yet, so send FCC0 to all thread0s
            SEND_FCC(THIS_SHIRE, THREAD_0, FCC_0, minion_mask);
        }

        // Only thread0s participate in the initial MRPOT config rendezvous
        // thread1s boot up later, long after MPROT has been configured per neighborhood
        if (get_thread_id() == 0) {
            WAIT_FCC(0);
        }

        // Jump to worker firmware in supervisor mode
        asm volatile("csrw  mepc, %0 \n" // write return address
                     "mret           \n" // return in S-mode
                     :
                     : "r"(worker_entry));
    }

    while (1) {
        // Should never get here
    };
}
