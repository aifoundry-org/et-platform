#include <stdlib.h>

#include "PLIC.h"

#include "atomic_region.h"
#include "cacheops.h"
#include "macros.h"
#include "shire.h"

#define DRAM_SPACE_BASE_ADDRESS 0x8100000000ULL
#define DRAM_SPACE_SIZE 0x7EFFFFFFFFULL

// Select PU peripherals for initial master minion use
#define PU_PLIC_BASE_ADDRESS  0x10000000ULL
#define PU_UART_BASE_ADDRESS  0x14002000ULL
#define PU_TIMER_BASE_ADDRESS 0x14005000ULL
#define PU_SRAM_BASE_ADDRESS  0x14008000ULL
#define PU_SRAM_SIZE          0x40000UL // 256KB
#define PU_PLL_BASE_ADDRESS   0x1A000000ULL

// Select SPIO peripherals for initial SP use
#define SPIO_NOC_SPIO_REGBUS_BASE_ADDRESS    0x40100000ULL
#define SPIO_NOC_PU_MAIN_REGBUS_BASE_ADDRESS 0x40200000ULL
#define SPIO_NOC_PSHIRE_REGBUS_BASE_ADDRESS  0x40300000ULL
#define SPIO_SRAM_BASE_ADDRESS               0x40400000ULL
#define SPIO_SRAM_SIZE                       0x100000UL // 1MB
#define SPIO_MAIN_NOC_REGBUS_BASE_ADDRESS    0x42000000ULL
#define SPIO_PLIC_BASE_ADDRESS               0x50000000ULL
#define SPIO_UART0_BASE_ADDRESS              0x54002000ULL

static void master_code(void);

int main(void)
{
    // enable shadow registers for hartid and sleep txfma
    __asm__ __volatile__ (
        "csrwi 0x7d2, 0x3\n"
        : 
        : 
        : "t0"
    );
    
    // Gets the minion id
    unsigned int minion_id = get_minion_id();

    // Master shire, go to master code. Run in m-mode to allow port configuration
    if (minion_id >= 1024)
    {
        master_code();
    }

    // configure trap vector and move to user mode
    __asm__ __volatile__ 
    ( 
       "la t0, mtrap_vector\n"
       "csrw mtvec, t0\n"
       "li t0, 0x1800\n"
       "csrrc x0, mstatus, t0\n"  // clear mstatus.mpp
       "la t0, 1f\n"
       "csrw mepc, t0\n"          // set mepc to user-mode entry point
       "mret\n"                   // go to user mode
       "1:\n"                     // label to referr to user-mode
       : 
       : 
       : "t0"
    );
    
    minion_id = minion_id & 0x1F;

    // If thread0
    if (get_thread_id() == 0)
    {
        // If minion0 within shire, initialize barriers
        if (minion_id == 0)
        {
            // Sets the global variables
            uint64_t barrier_t0 = ATOMIC_REGION;

            // Resets the barriers
            __asm__ __volatile__ (
                // Set 
                "add       x31, %[barrier_t0], zero\n"
                "addi      x30, x0, 32\n"
                "init_loop:\n"
                "sd        zero, 0(x31)\n"
                "addi      x31, x31, 8\n"
                "addi      x30, x30, -1\n"
                "bne       x30, x0, init_loop\n"
                "fence"
              :
              : [barrier_t0] "r" (barrier_t0)
              : "x30", "x31"
            );

            // Wake up the other threads 0 but current one
            uint64_t ipi_t0_mask = ~0x1ULL;
            uint64_t ipi_t0_addr = IPI_THREAD0;

            __asm__ __volatile__ (
                "sd %[ipi_t0_mask], 0(%[ipi_t0_addr])\n"
              :
              : [ipi_t0_mask] "r" (ipi_t0_mask),
                [ipi_t0_addr] "r" (ipi_t0_addr)
              :
            );
        }
        else
        {
            // Other minions go to sleep and wait for initialization
            __asm__ __volatile__ (
                "csrw 0x821, x0\n"
                :
                :
                :
            );
        }

        // Enables 4 elements of FPU
        // Sets icache control to do cooperative fetch (move data to buffer)
        __asm__ __volatile__ (
            "mov.m.x m0, zero, 0x0f\n"
            :
            :
            : "a1"
        );

        // Enable the scratchpad and allocate ways 0, 1 and 2 for all sets
        //volatile char __attribute__((aligned(4096))) scp[4096];
        uint64_t scp = 0x20000000;
        
        __asm__ __volatile__ (
            // Lock S/W function (0x0)
            "li a1, 0x0\n"
            "add  a1, a1, %[scp]\n"
            "addi a3, zero, 16\n"
            
            "li   a4, 0x080000000001000\n"
            "li   a5, 0x100000000002000\n"
            "loop_lock:\n"
            // Lock way 0
            "csrw 0x8dF, a1\n"
            "add  a1, a1, a4\n"
            // Lock way 1
            "csrw 0x8dF, a1\n"
            "add  a1, a1, a4\n"
            // Lock way 2
            "csrw 0x8dF, a1\n"
            // Move to next CL, way 0
            "addi a1, a1, 64\n"
            "addi a3, a3, -1\n"
            "xor  a1, a1, a5\n"
            "bne  a3, zero, loop_lock\n"
            :
            : [scp] "r" (scp)
            : "a1", "a3", "a4", "a5"
        );
    }

    //Rounding mode

    // Set RM to 0 (round near even)
    // Go to sleep and wait for someone to provide a new PC
    __asm__ __volatile__ (
        "csrrwi x0, frm, 0\n"
        "wfi_loop:\n"
        "csrw 0x822, x0\n"
        //"csrwi 0x821, 1\n"
        "beq  zero, zero, wfi_loop\n"
    );
    return 0;
}

// // Let the slave shires finish the set up
#define IPI_CYCLES 8000

// Struct that stores information for a layer
typedef struct{
  uint64_t compute_pc;
  uint64_t helper_pc;
  uint64_t shire_mask;
  uint64_t minion_mask;
  uint64_t id;
  uint64_t tensor_a;
  uint64_t tensor_b;
  uint64_t tensor_c;
  uint64_t tensor_d;
  uint64_t tensor_e;
  uint64_t compute_size;
  uint64_t helper_size;
  uint64_t info_pointer;
} net_desc_t;

// Function to delay a fixed amount of cycles
static void delay_cycles(uint32_t cycles) {
    for (uint32_t i = 0; i < cycles / 8; i++) {} // coarse approximation, each iteration of the loop takes 8 cycles
}

// only to be executed by the control shire
static void master_code(void)
{
    uint64_t mid, tid;

    // Get minion and thread
    mid = get_minion_id();
    tid = get_thread_id();

    // Only minion 0 thread 0 of master shire doing something
    if ((mid != 1024) || tid) {
        WFI;
    }

    // Get data from net description
    uint64_t    count    = (* (uint64_t *) 0x8200000000ULL);
    net_desc_t *net_desc = (net_desc_t*) 0x8200000040ULL;

    // Enable port to receive done messages
    // TODO: there could be an overflow... move to two ports of 16 entries each of 8 bytes
    char port_buf[16*4]; 

    //      use_tmask way  addr                         num_lines stride id
    //      --------- ---  ----                         --------- ------ --
    lock_va(0x0,      0x0, (uint64_t) (void*) port_buf, 1,        0x0,   0x0); //  lock set0, way0 in dcache

    intptr_t scp_conf = (intptr_t) port_buf;
    uint32_t scp_set = (scp_conf >> 6) & 0xF;

    // open port 0
    __asm__ __volatile__
    (
        /* TODO STEP 3
            --> This thread changes to supervisor mode (it is in machine mode)
            --> This thread opens a port to listen for shire responses
            --> Enable       = 1'b1
            --> out of band  = 1'b1
            --> reserved     = 2'b00
            --> user mode    = 1'b1
            --> log msg size = 3'b010
            --> max msgs     = 4'b1111
            --> reserved     = 4'b0000
            --> SCP set      = 8'h[scp_set]
            --> SCP way      = 8'h00
            --> reserved     = 32'h0000_0000
        */
        "li a3,0x00000F53\n"
        "slli a2,%[scp_set],16\n"
        "or a3, a3, a2\n"
        "csrw 0x9cc,a3\n" //Open port 0   
        :
        : [scp_set] "r" (scp_set)
        : "a2","a3"
    );
  
    // Wait for other shires to finish their set up
    uint32_t delay = IPI_CYCLES;
    delay_cycles(delay);

    // Write the IPI_REDIRECT_FILTER for all compute shires to all 1s
    for (uint32_t shire = 0 ; shire < 32; shire++) {
        __asm__ __volatile__
        ( 
        "slli x3, %[shire], 22\n" // shire_id to be added to ESR address
        "li   x4, 0x01C0340088\n" // shire_id = 0, neigh_id = 4'hF --> broadcast, PP = 2'b00
        "add  x4, x4, x3\n"       // add shire_id
        "addi x5, x0, -1\n"       // all 1s
        "sd   x5, 0(x4)\n"        // IPI_REDIRECT_FILTER write
        :
        : [shire] "r" (shire)
        : "x3",  "x4", "x5"
        );
    }

    FENCE;

  // Iterate over all the kernels
    for (uint32_t iter = 0; iter < count; iter++) {
        // Write parameters
        __asm__ __volatile__
        (
            // write data
            "sd %[tensor_a], 0(%[info_pointer])\n"
            "sd %[tensor_b], 8(%[info_pointer])\n"
            "sd %[tensor_c], 16(%[info_pointer])\n"
            "sd %[tensor_d], 24(%[info_pointer])\n"
            "sd %[tensor_e], 32(%[info_pointer])\n"
            "sd %[compute_pc], 40(%[info_pointer])\n"
            "sd %[helper_pc], 48(%[info_pointer])\n"
            "sd %[compute_size], 56(%[info_pointer])\n"
            "sd %[helper_size], 60(%[info_pointer])\n"
            :
            :
            [compute_pc]   "r" (net_desc[iter].compute_pc),
            [helper_pc]    "r" (net_desc[iter].helper_pc),
            [tensor_a]     "r" (net_desc[iter].tensor_a),
            [tensor_b]     "r" (net_desc[iter].tensor_b),
            [tensor_c]     "r" (net_desc[iter].tensor_c),
            [tensor_d]     "r" (net_desc[iter].tensor_d),
            [tensor_e]     "r" (net_desc[iter].tensor_e),
            [compute_size] "r" (net_desc[iter].compute_size),
            [helper_size]  "r" (net_desc[iter].helper_size),
            [info_pointer] "r" (net_desc[iter].info_pointer)
            :
        );

        // evict Line with Configuration data to L3
        uint64_t addr = (uint64_t)(void*) net_desc[iter].info_pointer ; 

        //       use_tmask dst  start addr  num_lines stride id
        //       --------- ---- ----- ----- --------- ------ --
        evict_va( 0,       0x2, 0x0,  addr, 1,        0x0,   0x0); 

        FENCE;

        // For all the shires write the helper PC to the IPI_REDIRECT_PC
        for (uint32_t shire = 0 ; shire < 32; shire++) {
            __asm__ __volatile__
            ( 
                "slli x3, %[shire], 22\n"    // shire_id to be added to ESR address
                "li   x4, 0x01001F0040\n"    // shire_id = 0, neigh_id = 4'hF --> broadcast, PP = 2'b00
                "add  x4, x4, x3\n"          // add shire_id
                "sd   %[helper_pc], 0(x4)\n" // IPI_REDIRECT_PC write
                :
                : [shire] "r" (shire),
                [helper_pc] "r" (net_desc[iter].helper_pc)
                : "x3",  "x4", "x5"
            );
        }

    FENCE;

    // For all the shires trigger redirect to thread1
    for (uint32_t shire = 0 ; shire < 32; shire++) {
        __asm__ __volatile__
        ( 
            "slli x3, %[shire], 22\n"       // shire_id to be added to ESR address
            "li   x4, 0x0100340080\n"       //
            "add  x4, x4, x3\n"             // add shire_id
            "li   x5, 0xAAAAAAAAAAAAAAAA\n" // All thread1 are the destinations
            "sd   x5, 0(x4)\n"              // IPI_REDIRECT_TRIGGER write
            :
            : [shire] "r" (shire)
            : "x3",  "x4", "x5"
        );
    }

    // For all the shires write the compute PC to the IPI_REDIRECT_PC
    for (uint32_t shire = 0 ; shire < 32; shire++) {
        __asm__ __volatile__
        ( 
            "slli x3, %[shire], 22\n"     // shire_id to be added to ESR address
            "li   x4, 0x01001F0040\n"     //shire_id = 0, neigh_id = 4'hF --> broadcast, PP = 2'b00
            "add  x4, x4, x3\n"           // add shire_id
            "sd   %[compute_pc], 0(x4)\n" // IPI_REDIRECT_PC write
            :
            : [shire] "r" (shire),
            [compute_pc] "r" (net_desc[iter].compute_pc)
            : "x3",  "x4", "x5"
        );
    }

    FENCE;

    // For all the shires trigger thread0
    for (uint32_t shire = 0 ; shire < 32; shire++) {
        __asm__ __volatile__
        ( 
            "slli x3, %[shire], 22\n"       // shire_id to be added to ESR address
            "li   x4, 0x0100340080\n"       //
            "add  x4, x4, x3\n"             // add shire_id
            "li   x5, 0x5555555555555555\n" // All thread0 are the destinations
            "sd   x5, 0(x4)\n"              // IPI_REDIRECT_TRIGGER write
            :
            : [shire] "r" (shire)
            : "x3",  "x4", "x5"
        );
    }

    // Wait for all the shires to finish
    for (uint32_t shire = 0 ; shire < 32; shire++) {
        // read head from port
        __asm__ __volatile__ ( "csrr x0,0xcc8\n"); //blocking read of port 0
        }
  }

  C_TEST_PASS;
}
