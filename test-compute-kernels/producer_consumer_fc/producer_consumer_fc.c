
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
#define FCC_FLB 2

#include "tfma_configs.h"
#include "reduce_configs.h"
#include "tload_tstore_mappings.h"

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

#define REDUCE_START_REG_IDX 0
#define REDUCE_OP_IDX 1
#define REDUCE_NUM_REGS_IDX 2
#define REDUCE_PARTNER_IDX 3
#define REDUCE_ACTION_IDX 4
#define REDUCE_PARAMS 5

#define STORE_IDX 0
#define LOAD_IDX 1
#define PAIR_IDX 2

#define MAPPINGS_PARAMS 3
#define TOTAL_MINIONS 1024
#define NUM_ITER 4


// Randomized producer-consumer test
// Part 1: A producer makes an FMA followed by a reduce
// Part 2: The receiver of the reduce does a tensor store and how becomes producer
// Part 3: The producer of part 2 wakes up the consumer with a credit inc, and the consumer does a tensor load
//         from the same location
// Betwen parts 1 and 2 there is no need for synchronization. Reduces communicate using the send/receive types
// Between parts 2 and 3 the CB is flushed by the last minion in each shire to do its tensor store (we use an FLB for that purpose)
// and then each producer minion in a shire does a cridit inc.
// The tensor-store / tensor-load buffer shifts by 1MB on every iteration. 
// Randomized paramters:
// 1. FMA parameters (FMAs load both inputs from the scratchpad). See file tfma_configs.h
// 2. Reduce parameters (Reduce pairs stay the same across iterations). See file reduce_configs.h
// 3. Tensor store / load pairs, and the location where each producer writes and each consumer reads in buffer. See file tload_tstore_mappings.h
//
// At the end of the test we do a CRC check on the data that is on each minion's SCP.
// So we do 3 tensor stores from SCP and than run a CRC check on totally 3Megs of data (96K per shire)
// TBD : To be on the safe side, we should keep the load-store pairs the same across iterations to avoid one store from iteration k
//       from sending a credit to a load from iteration k-1. See some waveforms to see if this can happen...
// TBD if time permits: Randomize number of credits to send/receive -- this will increase FCC activity in the NOC.
// TBD if time permits: Randomize Tensor load / Tensor Store fields

int64_t main(const kernel_params_t* const kernel_params_ptr)
{
    // Only one tensor is needed
    if ((kernel_params_ptr == NULL) ||
        ((uint64_t*)kernel_params_ptr->tensor_a == NULL) ||
        (kernel_params_ptr->tensor_b == 0) ||
	((uint64_t*)kernel_params_ptr->tensor_c == NULL) ||
        (kernel_params_ptr->tensor_d == 0)) {
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
   
    uint64_t tfma_minion_idx = minion_id * TFMA_PARAMS;
    uint64_t reduce_minion_idx = minion_id * REDUCE_PARAMS;
    uint64_t mappings_minion_idx = minion_id * MAPPINGS_PARAMS;
    
    uint64_t tfma_iter_idx = 0;
    uint64_t reduce_iter_idx = 0;
    uint64_t mappings_iter_idx = 0;
    uint64_t tfma_tmask = 0;
    uint64_t tfma_clear_rf = 1;
    uint64_t tfma_use_tenc = 1;
    
    uint64_t store_addr = 0;
    uint64_t load_addr = 0;
    volatile uint64_t *in_data = (uint64_t*)kernel_params_ptr->tensor_a;
    volatile uint64_t *out_data = (uint64_t*)kernel_params_ptr->tensor_c;

    volatile uint64_t base_addr = (uint64_t) in_data;
    volatile uint64_t out_addr = (uint64_t) out_data;

    // Fill up the SCP to avoid X's (SCP lines are random)
    // Read from the end of the input buffer
    tensor_load(0, 0, 0 /*start line*/, 0, 0, // regular tl0 on scp
        base_addr + 0x100000 * NUM_ITER, 0, 15 /*num lines*/, 0x40, 0);
    
    tensor_wait(TENSOR_LOAD_WAIT_0);
    
    tensor_load(0, 0, 16 /*start line*/, 0, 0, // regular tl0 on scp
        base_addr + 0x100000 * NUM_ITER, 0, 15 /*num lines*/, 0x40, 0);
        
    tensor_wait(TENSOR_LOAD_WAIT_0);
    
    tensor_load(0, 0, 32 /*start line*/, 0, 0, // regular tl0 on scp
        base_addr + 0x100000 * NUM_ITER, 0, 15 /*num lines*/, 0x40, 0);

    for (uint64_t iter=0; iter < NUM_ITER; iter++) {

	tensor_wait(TENSOR_LOAD_WAIT_0);

	tfma_iter_idx = iter * TFMA_PARAMS * TOTAL_MINIONS;
	reduce_iter_idx = iter * REDUCE_PARAMS * TOTAL_MINIONS;
	mappings_iter_idx = iter * MAPPINGS_PARAMS * TOTAL_MINIONS;

	tensor_fma(tfma_tmask,
		   tfma_configs[tfma_iter_idx + tfma_minion_idx + TFMA_BCOLS],
		   tfma_configs[tfma_iter_idx + tfma_minion_idx + TFMA_AROWS],
		   tfma_configs[tfma_iter_idx + tfma_minion_idx + TFMA_ACOLS],
		   tfma_configs[tfma_iter_idx + tfma_minion_idx + TFMA_ASTART_COL],
		   tfma_use_tenc,
		   tfma_configs[tfma_iter_idx + tfma_minion_idx + TFMA_UNSIGNEDA],
		   tfma_configs[tfma_iter_idx + tfma_minion_idx + TFMA_UNSIGNEDB],
		   tfma_configs[tfma_iter_idx + tfma_minion_idx + TFMA_TENB],
		   tfma_configs[tfma_iter_idx + tfma_minion_idx + TFMA_SCP_START_LINEB],
		   tfma_configs[tfma_iter_idx + tfma_minion_idx + TFMA_SCP_START_LINEA],
		   tfma_configs[tfma_iter_idx + tfma_minion_idx + TFMA_TYPE],
		   tfma_clear_rf);
	
	tensor_reduce(reduce_configs[reduce_iter_idx + reduce_minion_idx + REDUCE_START_REG_IDX],
		      reduce_configs[reduce_iter_idx + reduce_minion_idx + REDUCE_OP_IDX],
		      reduce_configs[reduce_iter_idx + reduce_minion_idx + REDUCE_NUM_REGS_IDX],
		      reduce_configs[reduce_iter_idx + reduce_minion_idx + REDUCE_PARTNER_IDX],
		      reduce_configs[reduce_iter_idx + reduce_minion_idx + REDUCE_ACTION_IDX]);

	// STORE_IDX is the index inside the buffer where this minion will store its Reg file
	store_addr = base_addr + iter * 0x100000 + 0x400 * tload_tstore_mappings[mappings_iter_idx + mappings_minion_idx + STORE_IDX];

	tensor_store(0, // reg_stride
		     0, // start_reg
		     3, // row_size
		     15, // num_rows
		     store_addr,
		     0, // coop minions
		     0x40); // stride
		     
	tensor_wait(TENSOR_STORE_WAIT);

	// Synchronization for all minions in a shire.
	uint64_t barrier_result;
	WAIT_FLB(32, FCC_FLB, barrier_result);
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
	    target_min_mask = target_min_mask & (~(1ULL << minion_id));
	    SEND_FCC(shire_id, 0, 0, target_min_mask);
	} else {
	    WAIT_FCC(0);
	}

	// Do not allow the load consumer to access the buffer until the store is done
	uint64_t target_minion_id = tload_tstore_mappings[mappings_iter_idx + mappings_minion_idx + PAIR_IDX];
	uint64_t target_shire_id = (target_minion_id >> 5) & 0x1F;
	uint64_t target_minion_mask = 1ULL << (target_minion_id & 0x1F); 
	SEND_FCC(target_shire_id, 0, 1, target_minion_mask);
	__asm__ __volatile__ ("fence\n");

	WAIT_FCC(1);

	// LOAD_IDX is the index inside the buffer where this minion will read from
	uint64_t tl_iter = (iter == NUM_ITER -1) ? iter : iter + 1;
	load_addr = base_addr + iter * 0x100000 + 0x400 * tload_tstore_mappings[mappings_iter_idx + mappings_minion_idx + LOAD_IDX];
	
	tensor_load(0, // use_tmask, 
		    0, // use_coop,
		    tfma_configs[tl_iter * TFMA_PARAMS * TOTAL_MINIONS + tfma_minion_idx + TFMA_SCP_START_LINEA], // scp_start_line
		    0, // load_trans, 
		    0, // use_tenb, 
		    load_addr, 
		    0, // offset, 
		    15, // tl_num_rows, 
		    0x40, // stride, 
		    0); //id)	
    }

    tensor_wait(TENSOR_LOAD_WAIT_0);
    
    // Use Tensor store from SCP to write back whatever the tensor loads (consumers) wrote back to memory
    tensor_store_scp(0, // entry_stride
		     0, // start_line
		     15, // rows
		     out_addr + 0xc00 * minion_id,
		     0x40); // stride

    tensor_store_scp(0, // entry_stride
		     16, // start_line
		     15, // rows
		     out_addr + 0xc00 * minion_id + 0x400,
		     0x40); // stride

    tensor_store_scp(0, // entry_stride
		     32, // start_line
		     15, // rows
		     out_addr + 0xc00 * minion_id + 0x800,
		     0x40); // stride

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
	target_min_mask = target_min_mask & (~(1ULL << minion_id));
	SEND_FCC(shire_id, 0, 0, target_min_mask);
    } else {
	WAIT_FCC(0);
    }

    __asm__ __volatile__ ("fence\n");

    // Need to evict ts_addr to memory with evict va.
    evict_va(0, to_Mem, out_addr + 0xc00 * minion_id, 15, 0x40, 0, 0);
    evict_va(0, to_Mem, out_addr + 0xc00 * minion_id + 0x400, 15, 0x40, 0, 0);
    evict_va(0, to_Mem, out_addr + 0xc00 * minion_id + 0x800, 15, 0x40, 0, 0);
    WAIT_CACHEOPS;

    // Check for tensor error and make the CRC check
    unsigned long functional_error = get_tensor_error();
    
    if (functional_error != 0) {
        log_write(LOG_LEVEL_CRITICAL, "Tensor error, shire %lu, minion %lu , error value: %x\n", shire_id, minion_id, functional_error);
        return -1;
    }

    uint64_t crc_barrier_result;
    WAIT_FLB(32, CRC_FLB, crc_barrier_result);
     
    if (crc_barrier_result == 1) {
 
        uint32_t crc = 0;

        if (out_addr % 4) {
            return -2;
        }

        // Total number of bytes per shire
        // 48 lines / minion * 64 bytes / line * 32 minions = 98304 (96K)
	crc = crc32_8bytes((void *) (kernel_params_ptr->tensor_c + shire_id * 98304), 98304, crc);
	uint32_t *crc_ptr = (uint32_t*)(out_addr + 98304 * 32 + shire_id * 64);
        *crc_ptr = crc;
        //if (shire_id == 0) {
	//    log_write(LOG_LEVEL_CRITICAL, "Shire %lu, CRC value %x\n", shire_id, crc);
	//}
    }

    return 0;


}
