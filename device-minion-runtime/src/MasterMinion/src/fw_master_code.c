#include <stdio.h>

// Auxiliar functions
#include "shire.h"
#include "macros.h"
//#include "et_test_common.h"
#include "cacheops.h"
//#include "tensors.h"
#include "fw_master_code.h"

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

// only to be executed by the control shire
void fw_master_code()
{
	uint64_t mid, tid;
	uint64_t num_shires = 32;

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

	//      use_tmask addr                         num_lines stride id   warl
	//      --------- ----                         --------- ------ --   ----
	lock_va(0x0,      (uint64_t) (void*) port_buf, 0,        0x0,   0x0, 0); //  lock set0, way0 in dcache

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

	// Receives an FCC1 from all shires when all minions are done
	for (uint32_t shire = 0 ; shire < num_shires; shire++) {
        // Other minions go to sleep and wait for initialization
        __asm__ __volatile__ (
            "csrwi 0x821, 1\n"
        );
	}

	// Write the IPI_REDIRECT_FILTER for all compute shires to all 1s
	for (uint32_t shire = 0 ; shire < num_shires; shire++) {
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
        __asm__ __volatile__ (
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

        //       use_tmask dst    addr  num_lines stride id   warl
        //       --------- ----   ----- --------- ------ --   ----
        evict_va(0,        to_L3, addr, 0,        0x0,   0x0, 0);

        FENCE;

        // For all the shires write the helper PC to the IPI_REDIRECT_PC
        for (uint32_t shire = 0 ; shire < num_shires; shire++) {
            __asm__ __volatile__ (
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
        for (uint32_t shire = 0 ; shire < num_shires; shire++) {
            __asm__ __volatile__ (
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
        for (uint32_t shire = 0 ; shire < num_shires; shire++) {
            __asm__ __volatile__ (
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
        for (uint32_t shire = 0 ; shire < num_shires; shire++) {
            __asm__ __volatile__ (
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
        for (uint32_t shire = 0 ; shire < num_shires; shire++) {
            // Consumes FCC0
            __asm__ __volatile__ (
                "csrwi 0x821, 0\n"
            );
        }
    }

    C_TEST_PASS;
}
