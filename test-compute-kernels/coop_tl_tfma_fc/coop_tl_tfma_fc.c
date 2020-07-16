
#include <stdint.h>
#include "hart.h"
#include "macros.h"
#include "vpu.h"
#include "cacheops.h"
#include "tensor.h"
#include "common.h"
#include "fcc.h"
#include "flb.h"
#include "kernel_params.h"
#include "esr_defines.h"
#include "crc32.h"
#include "log.h"

#define ALL_BANKS_MASK 0xFUL;
#define OPCODE_FLUSH_CB 0x0A01UL;
#define TSTORE_FLB 0
#define CRC_FLB 1
#define TL_COOP_FLB 2

// Coop Tensor Load NOC / L3 / DDR stress test
// Random tensor load 0 (SCP) and  and 1 (TenB) followed by FMA
// On every iteration we load next set of constrained random parameters
// for tensor loads and FMAs. Every new coop load uses a different coop id
// When we are close to using all the available id's we add a barrier so
// that we make sure all coop loads are done and the ids can get recycled.
// We use only 5 shires and 4 iterations for the VCS test.
// For Zebu we run on all shires, and run many iterations
// At the end we store data to memory so that we can run a CRC check on Zebu.
// CRC check is not used in VCS because it takes a long time and it is not needed 
// since we have cosim/BEMU.

// The randmozed parameters are passed using arrays.
// There are array entried for every iteration of the loop
// and within each iteration parameters for each minion.


#define TL_COOP_CSR_IDX 0
#define TL_IS_COOP_IDX 1
#define TL_TMASK_IDX 2
#define TL_CODE_IDX 3
#define TL_SCP_START_LINE_IDX 4
#define TL_TENB_IDX 5 
#define TL_ADDR_IDX 6 
#define TL_OFFSET_IDX 7
#define TL_NUM_LINES_IDX 8
#define TL_STRIDE_IDX 9
#define TL_PARAMS 10

#define TFMA_TMASK 0 
#define TFMA_BCOLS 1
#define TFMA_AROWS 2
#define TFMA_ACOLS 3
#define TFMA_ASTART_COL 4 
#define TFMA_TENB 5
#define TFMA_SCP_START_LINEA 6
#define TFMA_SCP_START_LINEB 7 
#define TFMA_CLEAR_RF 8
#define TFMA_TYPE 9
#define TFMA_USE_TENC 10
#define TFMA_UNSIGNEDA 11
#define TFMA_UNSIGNEDB 12
#define TFMA_PARAMS 13

#define TOTAL_MINIONS 1024
#define NUM_ITER 100
#define NUM_RANDOM_SAMPLES 10
#define NUM_ITER_FOR_BARRIER 4

#include "tl0_configs.h"
#include "tl1_configs.h"
#include "tfma_configs.h"

int64_t main(const kernel_params_t* const kernel_params_ptr)
{
    if ((kernel_params_ptr == NULL) ||
        ((uint64_t*)kernel_params_ptr->tensor_a == NULL) ||
        (kernel_params_ptr->tensor_b == 0) ||
        ((uint64_t*)kernel_params_ptr->tensor_c == NULL) ||
        (kernel_params_ptr->tensor_d == 0))
    {
	// Bad arguments
	return -1;
    }

    uint64_t hart_id = get_hart_id();
    uint64_t minion_id = hart_id >> 1;
    uint64_t shire_id = (hart_id >> 6) & 0x3f;       
  
    if (hart_id & 1) {
        //C_TEST_PASS; 
	return 0;
    }
    setM0MaskFF();
    uint64_t tl_minion_idx = minion_id * TL_PARAMS;
    uint64_t tfma_minion_idx = minion_id * TFMA_PARAMS;

    uint64_t tl0_coop_csr = tl0_configs[tl_minion_idx + TL_COOP_CSR_IDX];
    uint64_t tl0_is_coop = tl0_configs[tl_minion_idx + TL_IS_COOP_IDX];
    uint64_t tl0_code = tl0_configs[tl_minion_idx + TL_CODE_IDX];
    uint64_t tl0_scp_start_line = tl0_configs[tl_minion_idx + TL_SCP_START_LINE_IDX];
    uint64_t tl0_tenb = tl0_configs[tl_minion_idx + TL_TENB_IDX];
    uint64_t tl0_addr = tl0_configs[tl_minion_idx + TL_ADDR_IDX];
    uint64_t tl0_offset = tl0_configs[tl_minion_idx + TL_OFFSET_IDX];
    uint64_t tl0_num_lines = tl0_configs[tl_minion_idx + TL_NUM_LINES_IDX];
    uint64_t tl0_stride = tl0_configs[tl_minion_idx + TL_STRIDE_IDX];   
    
    // Set out_data far from in_data so that no overwriting happens
    volatile uint64_t *in_data = (uint64_t*)kernel_params_ptr->tensor_a;
    volatile uint64_t *out_data = (uint64_t*)kernel_params_ptr->tensor_c;

    volatile uint64_t base_src_addr = (uint64_t) in_data;
       
    // Fill up the SCP to avoid X's in VCS
    tensor_load(0, 0, 0 /*start line*/, 0, 0, // regular tl0 on scp
	base_src_addr, 0, 15 /*num lines*/, 0x40, 0);
    
    tensor_wait(TENSOR_LOAD_WAIT_0);
    
    tensor_load(0, 0, 16 /*start line*/, 0, 0, // regular tl0 on scp
	base_src_addr, 0, 15 /*num lines*/, 0x40, 0);
	
    tensor_wait(TENSOR_LOAD_WAIT_0);
    
    tensor_load(0, 0, 32 /*start line*/, 0, 0, // regular tl0 on scp
	base_src_addr, 0, 15 /*num lines*/, 0x40, 0);
	
    // Do not use masks (to maximize number of cache line accesses
    // Write always to the RF and do not add TenC
    tensor_wait(TENSOR_LOAD_WAIT_0);
    
    for (uint64_t iter=0; iter < NUM_ITER; iter++) {		

	// === Actual kernel body:
	// Index in arrays is Iteration idx + Minion Idx + Param Idx:
	// Using smaller types will reduce cache misses
	uint64_t tl_iter_idx = (iter % NUM_RANDOM_SAMPLES) * TL_PARAMS * TOTAL_MINIONS;
	uint64_t tfma_iter_idx = (iter % NUM_RANDOM_SAMPLES) * TFMA_PARAMS * TOTAL_MINIONS;
	uint64_t tl_next_iter_idx = ((iter + 1)% NUM_RANDOM_SAMPLES) * TL_PARAMS * TOTAL_MINIONS;

	tensor_coop(tl0_coop_csr);

	// Tensor Load 1 -- Tenb is 0
	// Set mask to 0 and ID to 0
	tensor_load(0, tl0_is_coop, tl0_scp_start_line, tl0_code, tl0_tenb,
	            base_src_addr + tl0_addr, tl0_offset, tl0_num_lines, tl0_stride, 0);
	
	if (iter < NUM_ITER) {
	    tl0_coop_csr = tl0_configs[tl_minion_idx  + TL_COOP_CSR_IDX + tl_next_iter_idx]; // was (iter + 1) * TL_PARAMS * TOTAL_MINIONS];
	    tl0_is_coop = tl0_configs[tl_minion_idx  + TL_IS_COOP_IDX+ tl_next_iter_idx];
	    tl0_code = tl0_configs[tl_minion_idx  + TL_CODE_IDX+ tl_next_iter_idx];
	    // Since start lines are random make sure that the next TL0 does not overwrite the lines read by current FMA
	    tl0_scp_start_line = ((tfma_configs[tfma_minion_idx  + TFMA_SCP_START_LINEA + tfma_iter_idx] % 48) + 16) % 48; //tfma_scp_start_linea + 16;
	    tl0_tenb = tl0_configs[tl_minion_idx  + TL_TENB_IDX + tl_next_iter_idx];
	    tl0_addr = tl0_configs[tl_minion_idx  + TL_ADDR_IDX + tl_next_iter_idx];
	    tl0_offset = tl0_configs[tl_minion_idx  + TL_OFFSET_IDX + tl_next_iter_idx];
	    tl0_num_lines = tl0_configs[tl_minion_idx  + TL_NUM_LINES_IDX + tl_next_iter_idx];
	    tl0_stride = tl0_configs[tl_minion_idx  + TL_STRIDE_IDX + tl_next_iter_idx];
	}

	// Tensor Load 1 -- Tenb is 1
	// Set mask to 0 and ID to 1
	tensor_coop(tl1_configs[tl_minion_idx  + TL_COOP_CSR_IDX + tl_iter_idx]);
	tensor_load(0, 
		    tl1_configs[tl_minion_idx + TL_IS_COOP_IDX+ tl_iter_idx],
		    tl1_configs[tl_minion_idx  + TL_SCP_START_LINE_IDX + tl_iter_idx], 
		    tl1_configs[tl_minion_idx  + TL_CODE_IDX+ tl_iter_idx], 
		    tl1_configs[tl_minion_idx  + TL_TENB_IDX + tl_iter_idx],
	            base_src_addr + tl1_configs[tl_minion_idx  + TL_ADDR_IDX + tl_iter_idx], 
		    tl1_configs[tl_minion_idx  + TL_OFFSET_IDX + tl_iter_idx], 
		    tl1_configs[tl_minion_idx  + TL_NUM_LINES_IDX + tl_iter_idx],
		    tl1_configs[tl_minion_idx  + TL_STRIDE_IDX + tl_iter_idx], 
		    1);

	tensor_wait(TENSOR_LOAD_WAIT_0);
	
	// Tensor FMA    
	tensor_fma(0, 
		   tfma_configs[tfma_minion_idx + TFMA_BCOLS + tfma_iter_idx], // tfma_bcols, 
		   tfma_configs[tfma_minion_idx + TFMA_AROWS + tfma_iter_idx], // tfma_arows, 
		   tfma_configs[tfma_minion_idx + TFMA_ACOLS + tfma_iter_idx], // tfma_acols, 
		   tfma_configs[tfma_minion_idx + TFMA_ASTART_COL + tfma_iter_idx], // tfma_astart_col, 
		   tfma_configs[tfma_minion_idx + TFMA_TENB + tfma_iter_idx], // tfma_tenb,
		   tfma_configs[tfma_minion_idx + TFMA_UNSIGNEDA + tfma_iter_idx], // tfma_unsigneda, 
		   tfma_configs[tfma_minion_idx + TFMA_UNSIGNEDB + tfma_iter_idx], // tfma_unsignedb, 
		   1, // tfma_use_tenc, 
		   tfma_configs[tfma_minion_idx + TFMA_SCP_START_LINEB + tfma_iter_idx], // tfma_scp_start_lineb, 
		   tfma_configs[tfma_minion_idx + TFMA_SCP_START_LINEA + tfma_iter_idx], // tfma_scp_start_linea,
		   tfma_configs[tfma_minion_idx + TFMA_TYPE + tfma_iter_idx], //tfma_type, 
		   1); // tfma_clear_rf);
		
	// Add a barrier when we may run out of Coop load id's.
	// This happens when reach the min number of iterations needed to run out of coop ids (NUM_ITER_FOR_BARRIER).
	// This number depends on gen_random_params.py, and whether we use coops for both TL0 and TL1.
	// It can also happen when we use NUM_RANDOM_SAMPLES iterations of randomly generated variables
	// and NUM_RANDOM_SAMPLES < NUM_ITER, in which case we wrap arounf to the 1st sample and this can have a coop id conflict.
	if ((((iter+1) % NUM_ITER_FOR_BARRIER) == 0) || ((iter % NUM_RANDOM_SAMPLES) == NUM_RANDOM_SAMPLES-1)) {
	    uint64_t tl_barrier;
	    WAIT_FLB(32, TL_COOP_FLB, tl_barrier);
	    if (tl_barrier == 1) {		
		uint64_t target_min_mask = 0xFFFFFFFFUL;
		target_min_mask = target_min_mask & (~(1ULL << (minion_id & 0x1f)));
		SEND_FCC(shire_id, 0, 0, target_min_mask);
	    } else {
		WAIT_FCC(0);
	    }
	}
    }

    // Store the data with tensor stores -- this is not needed for VCS, but since we want to run on Zebu, use it here as well
    volatile uint64_t base_dst_addr = (uint64_t) out_data;
    
    uint64_t ts_reg_stride = 0;
    uint64_t ts_start_reg = 0;
    uint64_t ts_num_cols = 3;
    uint64_t ts_addr = base_dst_addr + (minion_id * 1024);
    uint64_t ts_num_rows = 15;
    uint64_t ts_coop = 0;
    uint64_t ts_stride = 0x40;

    tensor_store(ts_reg_stride, ts_start_reg, ts_num_cols, ts_num_rows, ts_addr, ts_coop, ts_stride);

    tensor_wait(TENSOR_STORE_WAIT);

    // Synchronization for all minions in a shire.
    uint64_t barrier_result;
    WAIT_FLB(32, TSTORE_FLB, barrier_result);
    if (barrier_result == 1) {

	// The last minion to reach this barrier flushes the CB and sends a credit to all others to continue
	// Having more than one minions flush the CB at the same time may end up in having one of the flushes
	// dropped by the shire cache

	uint64_t sc_bank_mask = ALL_BANKS_MASK;
        uint64_t flush_cb_opcode = OPCODE_FLUSH_CB;
        volatile uint64_t *cb_flush_addr = (volatile uint64_t *)ESR_CACHE(shire_id, sc_bank_mask, SC_IDX_COP_SM_CTL_USER);
        store((uint64_t) cb_flush_addr, flush_cb_opcode);

	__asm__ __volatile__ ("fence\n");
	// You will need to poll each bank separately
        uint64_t cb_busy = 0;
        while (cb_busy != 0x4) {              
            uint64_t cb_busy_bank[4];               
            for (uint64_t b=0; b < 4; b++) {             
                cb_flush_addr = (volatile uint64_t *)ESR_CACHE(shire_id, b, SC_IDX_COP_SM_CTL_USER);
                cb_busy_bank[b] = ((*cb_flush_addr) >> 24) & 0x4;             
            }
            cb_busy = cb_busy_bank[0] | cb_busy_bank[1] | cb_busy_bank[2] | cb_busy_bank[3];
        }
      	
	uint64_t target_min_mask = 0xFFFFFFFFUL;
	target_min_mask = target_min_mask & (~(1ULL << (minion_id & 0x1f)));
	SEND_FCC(shire_id, 0, 0, target_min_mask);
    } else {
	WAIT_FCC(0);
    }

    __asm__ __volatile__ ("fence\n");

    // Need to evict ts_addr to memory with evict va.
    evict_va(0, 3, ts_addr, 15, 0x40, 0, 0);
    WAIT_CACHEOPS;

    unsigned long functional_error = get_tensor_error();
    
    if (functional_error != 0) {
        log_write(LOG_LEVEL_CRITICAL, "Tensor error, shire %lu, minion %lu , error value: %x\n", shire_id, minion_id, functional_error);
        return -1;
    }

    uint64_t crc_barrier_result;
    WAIT_FLB(32, CRC_FLB, crc_barrier_result);
     
    if (crc_barrier_result == 1) {
 
        uint32_t crc = 0;

        if (ts_addr % 4) {
            return -2;
        }

        // Total number of bytes:
        // 16 lines / minion * 64 bytes / line * 32 minions = 32768
        crc = crc32_8bytes((void *) (base_dst_addr + shire_id * 32768), 32768, crc);
        uint32_t *crc_ptr = (uint32_t*)(base_dst_addr + 1048576 + shire_id * 64);
	*crc_ptr = crc;
	//if (shire_id == 0) {
        //    log_write(LOG_LEVEL_CRITICAL, "Shire %lu, CRC value %x\n", shire_id, crc);
	//}
    }

    return 0;
}
