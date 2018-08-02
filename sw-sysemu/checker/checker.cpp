// Local
#include "checker.h"
#include "emu_casts.h"

// Global
#include <cmath>

//
#define TICKETER_REGION 0xFFF00000
#define TBOX_REGION_START 0xFFF80000
#define TBOX_REGION_END (TBOX_REGION_START + 512)

bool fp_1ulp_check(uint32_t gold, uint32_t rtl)
{
    int exp_gold = (gold >> 23) & 0xFF;

    if((exp_gold > 0) && (exp_gold < 253)) // just check regular cases (skip special denom, NaN, Inf)
    {
        uint32_t gold_clean = gold & 0x7F800000; // clean mantissa and sign from gold
        float err_1ulp = cast_uint32_to_float32(gold_clean);
        err_1ulp = err_1ulp / float (1 << 23); // put '1' in the unit of less precision

        float goldf = cast_uint32_to_float32(gold);
        float rtlf  = cast_uint32_to_float32(rtl);
        float diff = fabsf(goldf - rtlf);
        //printf("Gold: %.12e, RTL: %.12e, Diff: %.12e, Max: %.12e\n", goldf, rtlf, diff, err_1ulp);
        //printf("Hex Gold: %08X, Hex RTL: %08X\n", gold, rtl);
        return (diff <= err_1ulp);
    }
    else if ((gold & 0x7FFFFFFF) == 0) // allow sign (+/-) mismatch in case of zero
    {
        return ((rtl & 0x7FFFFFFF) == 0);
    }
    else // regular full check for special cases
    {
        return (gold == rtl);
    }

}

// Used to generate which store data bits to check for different store data sizes
uint64_t mem_mask(int32_t size)
{
    uint64_t mask;
    switch(size)
    {
        case 1:  mask = 0x00000000000000FFULL; break;
        case 2:  mask = 0x000000000000FFFFULL; break;
        case 4:  mask = 0x00000000FFFFFFFFULL; break;
        default: mask = 0xFFFFFFFFFFFFFFFFULL;
    }
    return mask;
}

// Singleton class
main_memory * memory_instance = NULL;
checker* checker_instance = NULL; // this is used when enabling the second thread from the emu, to have an object to handle the call
                                  // if there is more than 1 checker instance (e.g. one per shire), this will have to be an array

// These functions are called by emu. We should clean this to a nicer way...
uint8_t checker_memread8(uint64_t addr)
{
    uint8_t ret;
    memory_instance->read(addr, 1, &ret);
    return ret;
}

uint16_t checker_memread16(uint64_t addr)
{
    uint16_t ret;
    memory_instance->read(addr, 2, &ret);
    return ret;
}

uint32_t checker_memread32(uint64_t addr)
{
    uint32_t ret;
    memory_instance->read(addr, 4, &ret);
    return ret;
}

uint64_t checker_memread64(uint64_t addr)
{
    uint64_t ret;
    memory_instance->read(addr, 8, &ret);
    return ret;
}

void checker_memwrite8(uint64_t addr, uint8_t data)
{
    memory_instance->write(addr, 1, &data);
}

void checker_memwrite16(uint64_t addr, uint16_t data)
{
    memory_instance->write(addr, 2, &data);
}

void checker_memwrite32(uint64_t addr, uint32_t data)
{
    memory_instance->write(addr, 4, &data);
}

void checker_memwrite64(uint64_t addr, uint64_t data)
{
    memory_instance->write(addr, 8, &data);
}

void checker_thread1_enabled ( unsigned minionId, uint64_t en, uint64_t pc) {
  checker_instance -> thread1_enabled( minionId, en, pc);
}

// Creates a new checker
checker::checker(main_memory * memory_)
    : log("checker", LOG_DEBUG)
{
    for(uint32_t i = 0; i < EMU_NUM_THREADS; i++)
    {
        current_pc[i] = 0;
        reduce_state_array[i>>1] = Reduce_Idle;
    }
    memory = memory_;
    inst_cache = new instruction_cache(memory);

    set_memory_funcs((void *) checker_memread8,
                     (void *) checker_memread16,
                     (void *) checker_memread32,
                     (void *) checker_memread64,
                     (void *) checker_memwrite8,
                     (void *) checker_memwrite16,
                     (void *) checker_memwrite32,
                     (void *) checker_memwrite64);

    memory->setGetThread(get_thread);

    // Inits X0 to 0
    for(int i = 0; i < EMU_NUM_THREADS; i++)
    {
        set_thread(i);
        init(x0, 0);
        initcsr(i);
        threadEnabled[i] = true;
    }

    texrec_func_ptr = nullptr;
    checker_instance = this;
    memory_instance = memory;
#ifdef EMU_DEBUG
    init_emu(true, false);
#endif
}

// Destroys the checker
checker::~checker()
{
}

// Sets the PC
void checker::start_pc(uint32_t thread, uint64_t pc)
{
    if(thread >= EMU_NUM_THREADS)
        log << LOG_FTL << "start pc with thread invalid (" << thread << ")" << endm;
    current_pc[thread] = pc;
}

// Sets the PC due IPI
void checker::ipi_pc(uint32_t thread, uint64_t pc)
{
    if(thread >= EMU_NUM_THREADS)
        log << LOG_FTL << "IPI pc with thread invalid (" << thread << ")" << endm;
    current_pc[thread] = pc;
}

checker_result checker::do_reduce(uint32_t thread, instruction * inst, uint32_t * wake_minion)
{
    uint64_t other_min, action;
    // Gets the source used for the reduce
    uint64_t src1 = (xreg) inst->get_param(2);
    uint64_t value = xget(src1);

    get_reduce_info(value, &other_min, &action);

    // Sender
    if(action == 0)
    {
        // Moves to ready to send
        reduce_state_array[thread>>1] = Reduce_Ready_To_Send;
        reduce_pair_array[thread>>1]  = other_min;
        // If the other minion hasn't arrived yet, wait
        if((reduce_state_array[other_min] == Reduce_Idle) || (reduce_pair_array[other_min] != (thread>>1)))
        {
            return CHECKER_WAIT;
        }
        // If it has consumed the data, move both threads to Idle
        else if(reduce_state_array[other_min] == Reduce_Data_Consumed)
        {
            reduce_state_array[thread>>1] = Reduce_Idle;
            reduce_state_array[other_min] = Reduce_Idle;
            // Wakes up the thread0 of the other minion to guarantee it checks the reduce instruction
            // If this is not done, the thread might not get any other event from the minion monitor to
            // wake it up
            * wake_minion = other_min;
        }
        else
        {
            log << LOG_FTL << "Reduce error: Minion: " << (thread >> 1) << " found pairing receiver minion: " << other_min << " in Reduce_Ready_To_Send!!" << endm;
        }
    }
    // Receiver
    else if(action == 1)
    {
        // If receiver hasn't change the data consumed state, wait because the previous
        // reduce is not done
        if(reduce_state_array[thread>>1] == Reduce_Data_Consumed)
        {
            return CHECKER_WAIT;
        }

        // Previous reduce done, set with which minion it is doing the reduce
        reduce_pair_array[thread>>1] = other_min;
        // If the sender minion other minion hasn't arrived yet, wait
        if((reduce_state_array[other_min] == Reduce_Idle) || (reduce_pair_array[other_min] != (thread>>1)))
        {
            return CHECKER_WAIT;
        }
        // If pairing minion is in ready to send, consume the data
        else if(reduce_state_array[other_min] == Reduce_Ready_To_Send)
        {
            reduce_state_array[thread>>1] = Reduce_Data_Consumed;
            // Wakes up the thread0 of the other minion to guarantee it checks the reduce instruction
            // If this is not done, the thread might not get any other event from the minion monitor to
            // wake it up
            * wake_minion = other_min;
        }
        else
        {
            log << LOG_FTL << "Reduce error: Minion: " << (thread >> 1) << " found pairing sender minion: " << other_min << " in Reduce_Data_Consumed!!" << endm;
        }
    }
    return CHECKER_OK;
}

// Emulates next instruction in the flow and compares state changes against the changes
// passed as a parameter
checker_result checker::emu_inst(uint32_t thread, inst_state_change * changes, uint32_t * wake_minion)
{
    if(thread >= EMU_NUM_THREADS)
        log << LOG_FTL << "emu_inst with thread invalid (" << thread << ")" << endm;

    if ( ! threadEnabled[thread] )
      log << LOG_ERR << "emu_inst called for thread "<<thread<<", which is disabled"<<endm;

    log << LOG_DEBUG << "emu_inst called for thread "<<thread<<endm;

    set_thread(thread);
    log << LOG_DEBUG << "after set_thread()"<<endm;
    instruction * inst = inst_cache->get_instruction(virt_to_phys(current_pc[thread], Mem_Access_Fetch));
    log << LOG_DEBUG << "after get_instruction()"<<endm;
    setlogstate(&emu_state_change); // This is done every time just in case we have several checkers
    log << LOG_DEBUG << "after setlogstate()"<<endm;
    clearlogstate();
    log << LOG_DEBUG << "after clearlogstate()"<<endm;
    emu_state_change.pc = current_pc[thread];
    set_pc(current_pc[thread]);
    log << LOG_DEBUG << "after set_pc()"<<endm;
    update_msg_port_data();    // write any pending port data before executing the next instruction
    log << LOG_DEBUG << "after update_msg_port_data()"<<endm;

    // In case that the instruction is a reduce:
    //   - The thread that is the sender has to wait until the receiver has copied the reduce data,
    //     otherwise the sender thread could advance and update the VRF contents
    //   - The thread that is the receiver has to wait until the sender is also in the reduce
    //     operation
    if(inst->get_is_reduce())
    {
        checker_result res = do_reduce(thread, inst, wake_minion);
        if(res == CHECKER_WAIT) return CHECKER_WAIT;
    }

    // Now the instruction can be executed
    inst->exec();

    // As trapped instructions do not retire in the minion, we need to execute
    // the next instruction as well
    if(emu_state_change.exec_trap)
    {
        // Go to target
        current_pc[thread] = emu_state_change.pc;

        inst = inst_cache->get_instruction(virt_to_phys(current_pc[thread], Mem_Access_Fetch));
        clearlogstate();
        emu_state_change.pc = current_pc[thread];
        set_pc(current_pc[thread]);
        inst->exec();
    }

    // Checks modified fields
    if(changes != NULL)
    {
        // PC
        std::ostringstream stream;
        stream << "Checker Mismatch @ PC 0x" << std::hex << current_pc[thread] << std::dec << " (" << inst->get_mnemonic() << ") -> ";
        if(changes->pc != current_pc[thread])
        {
            stream << "PC error. Expected PC is 0x" << std::hex << current_pc[thread] << " but provided is 0x" << changes->pc << std::dec << std::endl;
            error_msg = stream.str();
            return CHECKER_ERROR;
        }

        // Changing integer register
        if(changes->int_reg_mod != emu_state_change.int_reg_mod)
        {
            stream << "Int Register write error. Expected write is " << emu_state_change.int_reg_mod << " but provided is " << changes->int_reg_mod;
            error_msg = stream.str();
            return CHECKER_ERROR;
        }
        if(emu_state_change.int_reg_mod)
        {
            if(changes->int_reg_rd != emu_state_change.int_reg_rd)
            {
                stream << "Int Register dest error. Expected dest is x" << emu_state_change.int_reg_rd << " but provided is x" << changes->int_reg_rd;
                error_msg = stream.str();
                return CHECKER_ERROR;
            }

            // Check if it is an AMO (special case where RTL drives value)
            if(inst->get_is_amo() && (emu_state_change.int_reg_rd != 0))
            {
                log << LOG_INFO << "AMO value (" << inst->get_mnemonic() << ")" << endm;
                // Set EMU state to what RTL says
                emu_state_change.int_reg_data = changes->int_reg_data;
                init((xreg) inst->get_param(0), emu_state_change.int_reg_data);
            }

            // Check if it is an Fast Local Barrier (special case where RTL drives value)
            if(inst->get_is_flb() && (emu_state_change.int_reg_rd != 0))
            {
                log << LOG_INFO << "FastLocalBarrier value (" << inst->get_mnemonic() << ")" << endm;
                // Set EMU state to what RTL says
                emu_state_change.int_reg_data = changes->int_reg_data;
                init((xreg) inst->get_param(0), emu_state_change.int_reg_data);
            }

            if(inst->get_is_load() && (virt_to_phys(emu_state_change.mem_addr[0], Mem_Access_Load) >= TBOX_REGION_START && virt_to_phys(emu_state_change.mem_addr[0], Mem_Access_Load) < TBOX_REGION_END))
            {
                log << LOG_INFO << "Access to tbox (" << inst->get_mnemonic() << ")" << endm;
                // Set EMU state to what RTL says
                emu_state_change.int_reg_data = changes->int_reg_data;
                init((xreg) inst->get_param(0), emu_state_change.int_reg_data);
            }


            // Writes to X0/Zero are ignored
            if((changes->int_reg_data != emu_state_change.int_reg_data) && (emu_state_change.int_reg_rd != 0))
            {
                stream << "Int Register data error. Expected data is 0x" << std::hex << emu_state_change.int_reg_data << " but provided is 0x" << changes->int_reg_data << std::dec;
                error_msg = stream.str();
                return CHECKER_ERROR;
            }
        }

        // Changing floating register
        if(changes->fp_reg_mod != emu_state_change.fp_reg_mod)
        {
            stream << "FP Register write error. Expected write is " << emu_state_change.fp_reg_mod << " but provided is " << changes->fp_reg_mod;
            error_msg = stream.str();
            return CHECKER_ERROR;
        }
        if(emu_state_change.fp_reg_mod)
        {
            if(changes->fp_reg_rd != emu_state_change.fp_reg_rd)
            {
                stream << "FP Register dest error. Expected dest is f" << emu_state_change.fp_reg_rd << " but provided is f" << changes->fp_reg_rd;
                error_msg = stream.str();
                return CHECKER_ERROR;
            }

            if( inst->get_is_texrcv() && get_mask(0) )
            {
              log << LOG_INFO << "Access to tbox (" << inst->get_mnemonic() << ")" << endm;
              // send the data to the tbox monitor, to check this is actually the data sent from the tbox
              unsigned wordIdx = inst->get_param(1);
              texrec(thread >> 1, thread &1,(const uint8_t*) emu_state_change.fp_reg_data, wordIdx,  get_mask(0));
            }

            for(int i = 0; i < (VL/2); i++)
            {
              if(inst->get_is_1ulp())
              {
                bool errlo = !fp_1ulp_check(emu_state_change.fp_reg_data[i] & 0xFFFFFFFF, changes->fp_reg_data[i] & 0xFFFFFFFF);
                bool errhi = !fp_1ulp_check(emu_state_change.fp_reg_data[i] >> 32, changes->fp_reg_data[i] >> 32);
                if (errlo || errhi)
                {
                    stream << "FP Register data error (";
                    if (errlo && errhi)
                        stream << (2*i+1) << ", " << (2*i);
                    else if (errlo)
                        stream << (2*i);
                    else
                        stream << (2*i+1);
                    stream << "). Expected data is 0x" << std::hex << emu_state_change.fp_reg_data[i] << " but provided is 0x" << changes->fp_reg_data[i] << std::dec;
                    error_msg = stream.str();
                    return CHECKER_ERROR;
                }
              }
              else
              {
                if( changes->fp_reg_data[i] != emu_state_change.fp_reg_data[i])
                {
                    bool errlo = (emu_state_change.fp_reg_data[i] & 0xFFFFFFFF) != (changes->fp_reg_data[i] & 0xFFFFFFFF);
                    bool errhi = (emu_state_change.fp_reg_data[i] >> 32) != (changes->fp_reg_data[i] >> 32);
                    stream << "FP Register data error (";
                    if (errlo && errhi)
                        stream << (2*i+1) << ", " << (2*i);
                    else if (errlo)
                        stream << (2*i);
                    else
                        stream << (2*i+1);
                    stream << "). Expected data is 0x" << std::hex << emu_state_change.fp_reg_data[i] << " but provided is 0x" << changes->fp_reg_data[i] << std::dec;
                    error_msg = stream.str();
                    return CHECKER_ERROR;
                }
              }
            }
        }

        // Changing mask register
        for(int m = 0; m < 8; m++)
        {
            if(changes->m_reg_mod[m] != emu_state_change.m_reg_mod[m])
            {
                stream << "Mask Register write error for entry " << m << ". Expected write is " << emu_state_change.m_reg_mod[m] << " but provided is " << changes->m_reg_mod[m];
                error_msg = stream.str();
                return CHECKER_ERROR;
            }
            if(emu_state_change.m_reg_mod[m])
            {
                for(int i = 0; i < VL; i++)
                {
                    if(changes->m_reg_data[m][i] != emu_state_change.m_reg_data[m][i])
                    {
                        stream << "Mask Register data error for entry " << m << " at bit " << i << ". Expected data is " << std::hex << (uint32_t) emu_state_change.m_reg_data[m][i] << " but provided is " << (uint32_t) changes->m_reg_data[m][i] << std::dec;
                        error_msg = stream.str();
                        return CHECKER_ERROR;
                    }
                }
            }
        }

        // Memory changes
        for(int i = 0; i < VL; i++)
        {
            if(changes->mem_mod[i] != emu_state_change.mem_mod[i])
            {
                stream << "Memory write error (" << i << "). Expected write is " << emu_state_change.mem_mod[i] << " but provided is " << changes->mem_mod[i];
                error_msg = stream.str();
                return CHECKER_ERROR;
            }
            if(emu_state_change.mem_mod[i])
            {
                if(changes->mem_size[i] != emu_state_change.mem_size[i])
                {
                    stream << "Memory write size error (" << i << "). Expected size is " << emu_state_change.mem_size[i] << " but provided is " << changes->mem_size[i];
                    error_msg = stream.str();
                    return CHECKER_ERROR;
                }
                if(changes->mem_addr[i] != emu_state_change.mem_addr[i])
                {
                    stream << "Memory write address error (" << i << "). Expected addr is 0x" << std::hex << emu_state_change.mem_addr[i] << " but provided is 0x" << changes->mem_addr[i] << std::dec;
                    error_msg = stream.str();
                    return CHECKER_ERROR;
                }
                uint64_t rtl_mem_data = changes->mem_data[i] & mem_mask(changes->mem_size[i]);
                uint64_t emu_mem_data = emu_state_change.mem_data[i] & mem_mask(changes->mem_size[i]);
                // Atomic instructions are not checked currently
                if((rtl_mem_data != emu_mem_data) && !inst->get_is_amo())
                {
                    stream << "Memory write data error (" << i << "). Expected data is 0x" << std::hex << emu_mem_data << " but provided is 0x" << rtl_mem_data << std::dec;
                    error_msg = stream.str();
                    return CHECKER_ERROR;
                }
            }
        }

        // TensorLoad
        if(inst->get_is_tensor_load())
        {
            int entry;
            int size;
            uint64_t data;
            data = get_scratchpad_value(0, 0, &entry, &size);
            std::list<bool> conv_list;
            get_scratchpad_conv_list(&conv_list);
            auto conv_list_it = conv_list.begin();

            // For all the written entries
            for(int i = 0; i < size; i++)
            {
                // Load was skipped due conv CSR, ignore check
                if(* conv_list_it == 1)
                {
                    conv_list_it++;
                    continue;
                }
                conv_list_it++;

                // Looks for the 1st entry in the list of RTL written lines with same destination
                auto it = spd_entry_list[thread].begin();
                while(it != spd_entry_list[thread].end())
                {
                    if(it->entry == (entry + i)) { break; }
                    it++;
                }

                // Checks that an entry was actually found
                if(it == spd_entry_list[thread].end())
                {
                    stream << "Couldn't find scratchpad destination " << entry + i << " in the RTL scratchpad list!!";
                    error_msg = stream.str();
                    return CHECKER_ERROR;
                }

                // Compares the data
                for(int j = 0; j < 8; j++)
                {
                    data = get_scratchpad_value(entry + i, j, &entry, &size);
                    if(data != it->data[j])
                    {
                        stream << "TensorLoad write data error for cacheline " << i << " written in entry " << entry + i << " data lane " << j << ". Expected data is 0x" << std::hex << data << " but provided is 0x" << it->data[j] << std::dec;
                        error_msg = stream.str();
                        spd_entry_list[thread].erase(it);
                        return CHECKER_ERROR;
                    }
                }
                spd_entry_list[thread].erase(it);
            }
        }

        // TensorFMA
        if(inst->get_is_tensor_fma())
        {
            int size;
            int passes;
            bool conv_skip[4];
            uint32_t data;
            data = get_tensorfma_value(0, 0, 0, &size, &passes, &conv_skip[0]);
            // For all the passes
            for(int pass = 0; pass < passes; pass++)
            {
                // For all the written entries
                for(int entry = 0; entry < size; entry++)
                {
                    // Move to next entry if this pass for this entry was skipped due conv CSR
                    for(int lane = 0; lane < 4; lane++)
                    {             
                        get_tensorfma_value(entry, pass, lane, &size, &passes, &conv_skip[lane]);
                    }
                    if (conv_skip[0] && conv_skip[1] && conv_skip[2] && conv_skip[3]) continue;
                    
                    // Looks for the 1st entry in the list of RTL written lines with same destination
                    auto it = tensorfma_list[thread].begin();
                    while(it != tensorfma_list[thread].end())
                    {
                        if(it->entry == entry) { break; }
                        it++;
                    }
                    // Checks that an entry was actually found
                    if(it == tensorfma_list[thread].end())
                    {
                        stream << "Couldn't find TensorFMA destination " << entry << " in the RTL TensorFMA list for pass " << pass << "!!";
                        error_msg = stream.str();
                        return CHECKER_ERROR;
                    }

                    // Compares the data for all the lanes (4 x 32b lanes)
                    for(int lane = 0; lane < 4; lane++)
                    {
                        if(conv_skip[lane] == 1) continue;
                        data = get_tensorfma_value(entry, pass, lane, &size, &passes, &conv_skip[lane]);
#ifdef USE_REAL_TXFMA
                        if(data != it->data[lane])
#else
                        if(!fp_1ulp_check(data, it->data[lane]))
#endif
                        {
                            stream << "TensorFMA write data error for register " << entry << " lane " << lane << " pass " << pass << ". Expected data is 0x" << std::hex << data << " but provided is 0x" << it->data[lane] << std::dec;
                            error_msg = stream.str();
                            tensorfma_list[thread].erase(it);
                            return CHECKER_ERROR;
                        }
                    }
                    tensorfma_list[thread].erase(it);
                }
            }
        }

        // Reduce
        if(inst->get_is_reduce())
        {
            int size;
            int start_entry;
            uint32_t data;
            data = get_reduce_value(0, 0, &size, &start_entry);

            // For all the written entries
            for(int entry = start_entry; entry < (start_entry + size); entry++)
            {
                // Looks for the 1st entry in the list of RTL written lines with same destination
                auto it = reduce_list[thread].begin();
                while(it != reduce_list[thread].end())
                {
                    if(it->entry == entry) { break; }
                    it++;
                }

                // Checks that an entry was actually found
                if(it == reduce_list[thread].end())
                {
                    stream << "Couldn't find Reduce destination " << entry << " in the RTL Reduce list!!";
                    error_msg = stream.str();
                    return CHECKER_ERROR;
                }

                // Compares the data for all the lanes (4 x 32b lanes)
                for(int lane = 0; lane < 4; lane++)
                {
                    data = get_reduce_value(entry, lane, &size, &start_entry);
                    if(data != it->data[lane])
                    {
                        stream << "Reduce write data error for register " << entry << " lane " << lane << ". Expected data is 0x" << std::hex << data << " but provided is 0x" << it->data[lane] << std::dec;
                        error_msg = stream.str();
                        reduce_list[thread].erase(it);
                        return CHECKER_ERROR;
                    }
                }
                reduce_list[thread].erase(it);
            }
        }
    }

    // PC update
    if(emu_state_change.pc_mod)
       current_pc[thread] = emu_state_change.pc;
    else
       current_pc[thread] += inst->get_size();

    return CHECKER_OK;
}

// Return the last error message
std::string checker::get_error_msg()
{
    return error_msg;
}

// Returns the mnemonic for a PC
std::string checker::get_mnemonic(uint64_t pc)
{
    instruction * inst = inst_cache->get_instruction(virt_to_phys(pc, Mem_Access_Fetch));
    return inst->get_mnemonic();
}

// enables or disables the 2nd thread
void checker::thread1_enabled ( unsigned minionId, uint64_t en, uint64_t pc)
{
  unsigned thread = minionId | 1;
  if (en != threadEnabled[thread] ) {
    threadEnabled[thread] = en;
    if (en) current_pc[thread] = pc;
  }
}

// Scratchpad write
void checker::tensorload_write(uint32_t thread, uint32_t entry, uint64_t * data)
{
    scratchpad_entry scp_entry;

    scp_entry.entry = entry;
    for(int i = 0; i < 8; i++)
    {
        scp_entry.data[i] = data[i];
    }
    spd_entry_list[thread].push_back(scp_entry);
}

// TensorFMA write
void checker::tensorfma_write(uint32_t thread, uint32_t entry, uint32_t * data, uint32_t tensorfma_regfile_wmask)
{
    tensorfma_entry tensorfma;

    tensorfma.entry = entry;
    for(int i = 0; i < 4; i++)
    {
        tensorfma.data[i] = data[i];
    }
    tensorfma.tensorfma_regfile_wmask = tensorfma_regfile_wmask;

    tensorfma_list[thread].push_back(tensorfma);
}

// Reduce write
void checker::reduce_write(uint32_t thread, uint32_t entry, uint32_t * data)
{
    tensorfma_entry reduce;

    reduce.entry = entry;
    for(int i = 0; i < 4; i++)
    {
        reduce.data[i] = data[i];
    }
    reduce_list[thread].push_back(reduce);
}

// Virtual to physical
uint64_t checker::virt_to_phys(uint64_t addr, mem_access_type macc)
{
    return virt_to_phys_emu(addr, macc);
}

void checker::set_texrec_func(func_texrec_t func_ptr)
{
    texrec_func_ptr = func_ptr;
}

void checker::texrec(unsigned minionId, unsigned thread_id, const uint8_t *data, unsigned wordIdx, uint32_t mask)
{
    if (!texrec_func_ptr)
    {
        log << LOG_ERR << "tbox::texrec is unimplemented."<<endm;
        return;
    }
    texrec_func_ptr(minionId, thread_id, data, wordIdx, mask);
}
