#include <stdint.h>
#include "hart.h"
#include "macros.h"
#include "vpu.h"
#include "cacheops.h"
#include "tensor.h"
#include "common.h"
#include "fcc.h"
#include "kernel_params.h"
#include "esr_defines.h"

// TBD: Should these go to esr_define.s or to some other header?
#define ALL_BANKS_MASK 0xFUL;
#define OPCODE_FLUSH_CB 0x0A01UL;

// This test does tensor stores to a 1MB buffer and then does tensor loads from the same buffer.
// Each minion stores on a random 1KB block of the 1MB buffer and loads from another random block.
// The mapping are automatically generated using the gen_minion_maps.py script

int main(const kernel_params_t* const kernel_params_ptr)
{
    uint64_t hart_id = get_hart_id();
    uint64_t minion_id = hart_id >> 1;
    uint64_t INDEX_INIT_ADDR;
    uint64_t INDEX_STORE, INDEX_LOAD, PAIR;
    uint64_t INDEX_REG_STRIDE, INDEX_START_REG, INDEX_ROW_SIZE, INDEX_NUM_ROWS;
    
    // The 1MB data section is separated in 1024 1KB blocks.
    // INDEX_STORE, INDEX_LOAD, PAIR are specified in the included header file
    // INDEX_STORE is the block number where this minion does its tensor store
    // INDEX LOAD is the block number where this minion does its tensor load.
    // PAIR is the minion that loads from the block this minion stores
    // load_store_mappings is automatically generated
    // TBD: Bring in script to generate header automatically

    #include "load_store_mappings.h"
    
    if ((kernel_params_ptr == NULL) ||
        ((uint64_t*)kernel_params_ptr->tensor_a == NULL) ||
        (kernel_params_ptr->tensor_b == 0))
    {
        // Bad arguments
        return -1;
    }

    volatile uint64_t* data_ptr = (uint64_t*)kernel_params_ptr->tensor_a;

    if (hart_id & 1) {
	// Uncomment this to run once in sys_emu
	// C_TEST_PASS;	
	return 0;
    }
   
    setM0MaskFF();

    if (INDEX_INIT_ADDR > 0xffffffff) return 0;

    setFRegs_2(minion_id);
    uint64_t data = (uint64_t) data_ptr;
    
    uint64_t reg_stride = INDEX_REG_STRIDE;
    uint64_t start_reg = INDEX_START_REG;
    uint64_t row_size = INDEX_ROW_SIZE;
    uint64_t num_rows = INDEX_NUM_ROWS;
    uint64_t coop_minions = 0;    
    uint64_t store_addr = (uint64_t) data + 0x400 * INDEX_STORE;    
    uint64_t load_addr = (uint64_t) data + 0x400 * INDEX_LOAD; 

    uint64_t stride = 0x40;

    tensor_store(reg_stride, start_reg, row_size, num_rows, store_addr, coop_minions, stride);
    
    tensorWait(TENSOR_STORE_WAIT);  
    
    // Flush out SC coalescing buffer after store so that dependents can get the data
    //cb_flush_addr should be 0x010031E100UL
    uint64_t shire_id = (hart_id >> 6) & 0x3F;
    uint64_t sc_bank_mask = ALL_BANKS_MASK;
    uint64_t flush_cb_opcode = OPCODE_FLUSH_CB;
    volatile uint64_t *cb_flush_addr = ESR_CACHE(PRV_U, shire_id, sc_bank_mask, IDX_COP_SM_CTL_USER);
    store((uint64_t) cb_flush_addr, flush_cb_opcode);
   
    // Do not allow the load consumer to access the buffer until the store is done
    uint64_t target_minion_id = PAIR;
    uint64_t target_shire_id = (target_minion_id >> 5) & 0x1F;
    uint64_t target_minion_mask = 1ULL << (target_minion_id & 0x1F); 
    volatile uint64_t *fcc0_addr = ESR_SHIRE(PRV_U, target_shire_id, FCC0);
    store((uint64_t) fcc0_addr, target_minion_mask);

    // Wait for the producer of the buffer to be done
    wait_fcc(0);  
    
    uint64_t use_tmask = 0;
    uint64_t use_coop = 0;
    uint64_t load_trans = 0;
    uint64_t scp_start = 0;
    uint64_t use_tenb = 0;
    uint64_t offset = 0;
    uint64_t id = 0;
    uint64_t tl_num_rows = 16;
       
    tensor_load(use_tmask, use_coop, scp_start, load_trans, use_tenb, load_addr, offset, tl_num_rows, stride, id);
        
    tensorWait(TENSOR_LOAD_WAIT_0);

    unsigned long functional_error = get_tensor_error();
    
    if (functional_error != 0) {
        // Uncomment this to run once in sys_emu	
	// C_TEST_FAIL;
	return -1;
    }
  
    // Uncomment this to run once in sys_emu
    // C_TEST_PASS;
    return 0;
}
