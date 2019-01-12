#include <cmath>
#include <exception>

#include "checker.h"
#include "emu_casts.h"
#include "emu.h"
#include "insn.h"
#include "common/riscv_disasm.h"

#define TBOX_REGION_START 0xFFF80000
#define TBOX_REGION_END (TBOX_REGION_START + 512)

#ifdef DEBUG_STATE_CHANGES
// Used for debugging the checker
std::ostringstream& operator<< (std::ostringstream& os, const inst_state_change& state)
{
    std::ios_base::fmtflags ff = os.flags();
    os.setf(std::ios_base::showbase);

    os  << "{pc_mod=" << (state.pc_mod ? "Y" : "N")
        << std::hex
        << ", pc=" << state.pc
        << ", inst_bits=" << state.inst_bits
        << std::dec;
    if (state.int_reg_mod)
    {
        os << ", x" << state.int_reg_rd << "=" << std::hex << state.int_reg_rd << std::dec;
    }
    if (state.fp_reg_mod)
    {
        os << ", f" << state.fp_reg_rd << "=";
        for (int i = 0; i < (VL/2); ++i)
        {
            os  << (i==0 ? "[" : ", ")
                << std::hex
                << uint32_t(state.fp_reg_data[i] & 0xffffffff)
                << ", "
                << uint32_t((state.fp_reg_data[i] >> 32) & 0xffffffff)
                << std::dec;
        }
        os << "]";
    }
    for (int m = 0; m < 8; ++m)
    {
        if (!state.m_reg_mod[m]) continue;
        uint32_t mval = 0;
        for (int i = 0; i < VL; ++i)
            mval |= ((state.m_reg_data[m][i] != 0) << i);
        os << ", m" << m << "=" << std::hex << mval << std::dec;
    }
    for (int i = 0; i < VL; ++i)
    {
        if (!state.mem_mod[i]) continue;
        os  << ", MEM["
            << std::hex
            << state.mem_addr[i] << "..." << (state.mem_addr[i] + state.mem_size[i] - 1)
            << "]=" << state.mem_data[i]
            << std::dec;
    }
    os << "}";
    os.flags(ff);
    return os;
}
#endif // DEBUG_STATE_CHANGES

bool fp_1ulp_check(uint32_t gold, uint32_t rtl)
{
    int exp_gold = (gold >> 23) & 0xFF;

    if((exp_gold > 0) && (exp_gold < 253)) // just check regular cases (skip special denom, NaN, Inf)
    {
        uint32_t gold_clean = gold & 0x7F800000; // clean mantissa and sign from gold
        float err_1ulp = cast_uint32_to_float(gold_clean);
        err_1ulp = err_1ulp / float (1 << 23); // put '1' in the unit of less precision

        float goldf = cast_uint32_to_float(gold);
        float rtlf  = cast_uint32_to_float(rtl);
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
checker::checker(main_memory * memory_, enum logLevel emu_log_level)
    : log("checker", emu_log_level)
{
    for(uint32_t i = 0; i < EMU_NUM_THREADS; i++)
    {
        current_pc[i] = 0;
        reduce_state_array[i>>1] = Reduce_Idle;
    }
    memory = memory_;

    waived_csrs.push_back(csr_validation0);
    waived_csrs.push_back(csr_validation1);
    waived_csrs.push_back(csr_validation2);
    waived_csrs.push_back(csr_validation3);

    set_memory_funcs((void *) checker_memread8,
                     (void *) checker_memread16,
                     (void *) checker_memread32,
                     (void *) checker_memread64,
                     (void *) checker_memwrite8,
                     (void *) checker_memwrite16,
                     (void *) checker_memwrite32,
                     (void *) checker_memwrite64);

    memory->setGetThread(get_thread);

    set_delayed_msg_port_write(true);

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
    init_emu(emu_log_level);
}

// Destroys the checker
checker::~checker()
{
}

// Sets the core type
void checker::set_et_core(int core_type)
{
   set_core_type((et_core_t)core_type);
}

// Sets the PC
void checker::start_pc(uint32_t thread, uint64_t pc)
{
    if(thread >= EMU_NUM_THREADS)
        log << LOG_FTL << "start pc with thread invalid (" << std::hex << thread << ")" << endm;
    current_pc[thread] = pc;
}

// Sets the PC due IPI
void checker::ipi_pc(uint32_t thread, uint64_t pc)
{
    if(thread >= EMU_NUM_THREADS)
        log << LOG_FTL << "IPI pc with thread invalid (" << std::hex << thread << ")" << endm;
    current_pc[thread] = pc;
}

checker_result checker::do_reduce(uint32_t thread, uint64_t value, int * wake_minion)
{
    uint64_t other_min, action;

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
            * wake_minion = (int) other_min;
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

void checker::emu_disasm(char* str, size_t size, uint32_t bits)
{
   riscv_disasm(str, size, bits);
}

// Emulates next instruction in the flow and compares state changes against the changes
// passed as a parameter
checker_result checker::emu_inst(uint32_t thread, inst_state_change * changes, int * wake_minion)
{
    checker_result check_res = CHECKER_OK;
    if (thread >= EMU_NUM_THREADS)
       log << LOG_FTL << "emu_inst with thread invalid (" << thread << ")" << endm;

    if (!threadEnabled[thread])
       log << LOG_ERR << "emu_inst called for thread " << thread << ", which is disabled" << endm;

    log << LOG_DEBUG << "emu_inst called for thread " << thread << endm;

    set_thread(thread);
    setlogstate(&emu_state_change); // This is done every time just in case we have several checkers

    insn_t inst;

    // As trapped instructions do not retire in the minion, we need to keep
    // executing until the first non-trapping instruction.  We do this by
    // setting retry to true until the first instruction that does not trap.
    bool retry = false;
    int retry_count = 10;
    do
    {
        try
        {
            // Initialize emu_state_change
            clearlogstate();
            // Fetch new instruction (may trap)
            set_pc(current_pc[thread]);
            inst.fetch_and_decode(current_pc[thread]);

            // In case that the instruction is a reduce:
            //   - The thread that is the sender has to wait until the receiver has copied the reduce data,
            //     otherwise the sender thread could advance and update the VRF contents
            //   - The thread that is the receiver has to wait until the sender is also in the reduce
            //     operation
            if(inst.is_reduce())
            {
                // Gets the source used for the reduce
                uint64_t value = xget(inst.rs1());
                checker_result res = do_reduce(thread, value, wake_minion);
                if(res == CHECKER_WAIT) return CHECKER_WAIT;
            }

            // Execute the instruction (may trap)
            inst.execute();
            retry = false;
        }
        catch (const trap_t& t)
        {
            take_trap(t);
            current_pc[thread] = emu_state_change.pc; // Go to target
            retry = true;
            retry_count--;
            // If the trap takes us to the current instruction, most likely the trap vector was not defined
            // and we are about to loop forever because {m,s}tvec point to an illegal instruction
            // Loop a few times for the sake of it to make it clear in the log.
            if ((current_pc[thread] == emu_state_change.pc) && (retry_count < 0)) {
               log << LOG_FTL << "Sad, looks like we are stuck in an infinite trap recursion. Giving up." << endm;
               retry = false;
            }
        }
        catch (const std::exception& e)
        {
            log << LOG_FTL << e.what() << endm;
        }
    }
    while (retry);

    // Checks modified fields
    if(changes != NULL)
    {
#ifdef DEBUG_STATE_CHANGES
        inst_state_change tmp_state_change = emu_state_change;
        // to avoid false diffs
        if (!emu_state_change.pc_mod)
            tmp_state_change.pc = current_pc[thread];
        tmp_state_change.inst_bits = changes->inst_bits;
        log << LOG_DEBUG << "EMU changes: " << tmp_state_change << endm;
        log << LOG_DEBUG << "RTL changes: " << (*changes) << endm;
#endif
        char insn_disasm[128];
        riscv_disasm(insn_disasm, 128, inst.bits);
        std::ostringstream stream;
        stream << "Checker Mismatch @ PC 0x" << std::hex << current_pc[thread] << std::dec << " (" << insn_disasm << ") -> ";

        error_msg = "";
        // Check PC
        if(changes->pc != current_pc[thread])
        {
            stream << "PC error. Expected PC is 0x" << std::hex << current_pc[thread] << " but provided is 0x" << changes->pc << std::dec << std::endl;
            error_msg += stream.str();
            if(emu_state_change.pc_mod)
               current_pc[thread] = emu_state_change.pc;
            else
               current_pc[thread] += inst.size();
            // don't check anything else when PC mismatches... everything would mismatch
            return CHECKER_ERROR;
        }

        // Instruction bits -- the RTL monitor shows the uncompressed version of the instruction always,
        // while bemu shows the original instruction bits, so we cannot really compare them here
        /*if(changes->inst_bits != inst.bits)
        {
            stream << "Inst error. Expected inst is 0x" << std::hex << inst.bits << " but provided is 0x" << changes->inst_bits << std::dec << std::endl;
            error_msg += stream.str();
            check_res = CHECKER_ERROR;
        }*/

        // Changing integer register
        if(changes->int_reg_mod != emu_state_change.int_reg_mod)
        {
            stream << "Int Register write error. Expected write is " << emu_state_change.int_reg_mod << " but provided is " << changes->int_reg_mod << std::endl;
            error_msg += stream.str();
            check_res = CHECKER_ERROR;
        }
        if(emu_state_change.int_reg_mod)
        {
            if(changes->int_reg_rd != emu_state_change.int_reg_rd)
            {
                stream << "Int Register dest error. Expected dest is x" << emu_state_change.int_reg_rd << " but provided is x" << changes->int_reg_rd << std::endl;
                error_msg += stream.str();
                check_res = CHECKER_ERROR;
            }

            // Check if load comes from an unpredictible memory region, accept RTL value if so.
            if (inst.is_load() && address_is_in_ignored_region(emu_state_change.mem_addr[0]))
            {
               log << LOG_INFO << "Access to an ignored memory region (" << insn_disasm << ")" << endm;
               emu_state_change.int_reg_data = changes->int_reg_data;
               init(inst.rd(), emu_state_change.int_reg_data);
            }


            // Check if we read a CSR that we want to waive checking for
            if (inst.is_csr_read() && (std::find(waived_csrs.begin(), waived_csrs.end(), get_csr_enum(inst.csrimm())) != waived_csrs.end()))
            {
                log << LOG_INFO << "Waived CSR value (" << insn_disasm << ")" << endm;
                emu_state_change.int_reg_data = changes->int_reg_data;
                init(inst.rd(), emu_state_change.int_reg_data);
            }

            // Check if we just read a cycle register, in which case the RTL drives value
            if (inst.is_csr_read() && ((get_csr_enum(inst.csrimm()) == csr_cycle  )||
                                       (get_csr_enum(inst.csrimm()) == csr_mcycle )))
            {
                log << LOG_INFO << "CYCLE CSR value (" << insn_disasm << ")" << endm;
                emu_state_change.int_reg_data = changes->int_reg_data;
                init(inst.rd(), emu_state_change.int_reg_data);
            }

            // Check if it is an AMO (special case where RTL drives value)
            if(inst.is_amo() && (emu_state_change.int_reg_rd != 0))
            {
                log << LOG_INFO << "AMO value (" << insn_disasm << ")" << endm;
                // Set EMU state to what RTL says
                emu_state_change.int_reg_data = changes->int_reg_data;
                init(inst.rd(), emu_state_change.int_reg_data);
            }

            // Check if it is an Fast Local Barrier (special case where RTL drives value)
            if(inst.is_flb() && (emu_state_change.int_reg_rd != 0))
            {
                log << LOG_INFO << "FastLocalBarrier value (" << insn_disasm << ")" << endm;
                // Set EMU state to what RTL says
                emu_state_change.int_reg_data = changes->int_reg_data;
                init(inst.rd(), emu_state_change.int_reg_data);
            }

            // NB: the BEMU memread functions put the *physical* address used by the load into mem_addr without setting mem_mod so we can
            // check for TBOX accesses here.
            if(inst.is_load() && (emu_state_change.mem_addr[0] >= TBOX_REGION_START) && (emu_state_change.mem_addr[0] < TBOX_REGION_END))
            {
                log << LOG_INFO << "Access to tbox (" << insn_disasm << ")" << endm;
                // Set EMU state to what RTL says
                emu_state_change.int_reg_data = changes->int_reg_data;
                init(inst.rd(), emu_state_change.int_reg_data);
            }

            // Writes to X0/Zero are ignored
            if((changes->int_reg_data != emu_state_change.int_reg_data) && (emu_state_change.int_reg_rd != 0))
            {
                stream << "Int Register data error. Expected data is 0x" << std::hex << emu_state_change.int_reg_data << " but provided is 0x" << changes->int_reg_data << std::dec << std::endl;
                error_msg += stream.str();
                check_res = CHECKER_ERROR;
            }
        }

        // Changing floating register
        if(changes->fp_reg_mod != emu_state_change.fp_reg_mod)
        {
            stream << "FP Register write error. Expected write is " << emu_state_change.fp_reg_mod << " but provided is " << changes->fp_reg_mod << std::endl;
            error_msg += stream.str();
            check_res = CHECKER_ERROR;
        }

	// Changing fflags
	if ( changes->fflags_mod != emu_state_change.fflags_mod )
	{
	  // Someone changed the flags
	  std::string changer =  emu_state_change.fflags_mod ? "EMU" : "RTL";
	  stream << "fflags changed by " << changer << ". Expected new flags: " << std::hex <<  emu_state_change.fflags_value << " but provided are " << changes->fflags_value << std::dec << std::endl;
	  error_msg += stream.str();
	  check_res = CHECKER_WARNING;
	}

	if ( emu_state_change.fflags_mod)
	{
	  if ( changes->fflags_value != emu_state_change.fflags_value )
	  {
	    stream << "fflags values change. Expected new flags: " << std::hex << emu_state_change.fflags_value << " but provided are " << changes->fflags_value << std::dec << std::endl;
	    error_msg += stream.str();
	    check_res = CHECKER_WARNING;
	  }
	}



        if(emu_state_change.fp_reg_mod)
        {
#if 0
            log << LOG_DEBUG << "\tm0 = " << get_mask(0) << endm;
#endif
            if(changes->fp_reg_rd != emu_state_change.fp_reg_rd)
            {
                stream << "FP Register dest error. Expected dest is f" << emu_state_change.fp_reg_rd << " but provided is f" << changes->fp_reg_rd << std::endl;
                error_msg += stream.str();
                check_res = CHECKER_ERROR;
            }

            // Check if load comes from an unpredictible memory region, accept RTL value if so.
            if (inst.is_load() && address_is_in_ignored_region(emu_state_change.mem_addr[0]))
            {
               log << LOG_INFO << "Access to an ignored memory region (" << insn_disasm << ")" << endm;
               for (int i = 0; i < (VL/2); i++) {
                  emu_state_change.fp_reg_data[i] = changes->fp_reg_data[i];
               }
               fpinit(inst.fd(), emu_state_change.fp_reg_data);
            }

            for(int i = 0; i < (VL/2); i++)
            {
              if(inst.is_1ulp())
              {
                bool errlo = !fp_1ulp_check(emu_state_change.fp_reg_data[i] & 0xFFFFFFFF, changes->fp_reg_data[i] & 0xFFFFFFFF);
                bool errhi = !fp_1ulp_check(emu_state_change.fp_reg_data[i] >> 32, changes->fp_reg_data[i] >> 32);
                if (errlo || errhi)
                {
                    uint32_t emu_datalo = emu_state_change.fp_reg_data[i] & 0xFFFFFFFF;
                    uint32_t rtl_datalo = changes->fp_reg_data[i] & 0xFFFFFFFF;
                    uint32_t emu_datahi = emu_state_change.fp_reg_data[i] >> 32;
                    uint32_t rtl_datahi = changes->fp_reg_data[i] >> 32;
                    stream << "FP Register data error f" << emu_state_change.fp_reg_rd;
                    if (errlo && errhi)
                        stream << "[" << (2*i+1) << ", " << (2*i) << "]";
                    else
                        stream << "[" << (errlo ? (2*i) : (2*i+1)) << "]";
                    stream << ". Expected data is " << std::hex;
                    if (errlo && errhi)
                        stream << "{0x" << std::hex << emu_datahi << ", 0x" << emu_datalo << "}";
                    else
                        stream << "0x" << std::hex << (errlo ? emu_datalo : emu_datahi);
                    stream << " but provided is ";
                    if (errlo && errhi)
                        stream << "{0x" << std::hex << rtl_datahi << ", 0x" << rtl_datalo << "}";
                    else
                        stream << "0x" << std::hex << (errlo ? rtl_datalo : rtl_datahi);
                    stream << " Current mask: 0x" << get_mask(0) << std::dec << std::endl;
                    error_msg += stream.str();
                    check_res = CHECKER_ERROR;
                }
#if 0
                if( changes->fp_reg_data[i] != emu_state_change.fp_reg_data[i])
                {
                    // Checker and RTL do not match exactly; force the RTL value
                    // into the checker to avoid errors in future instructions
                    fpinit((freg)changes->fp_reg_rd, changes->fp_reg_data);
                    //log << LOG_INFO << "Forcing f" << changes->fp_reg_rd << "[  to {" << changes->fp_reg_dataendm;
                }
#endif
              }
              else
              {
                if( changes->fp_reg_data[i] != emu_state_change.fp_reg_data[i])
                {
                    bool errlo = (emu_state_change.fp_reg_data[i] & 0xFFFFFFFF) != (changes->fp_reg_data[i] & 0xFFFFFFFF);
                    bool errhi = (emu_state_change.fp_reg_data[i] >> 32) != (changes->fp_reg_data[i] >> 32);
                    if (errlo || errhi)
                    {
                        uint32_t emu_datalo = emu_state_change.fp_reg_data[i] & 0xFFFFFFFF;
                        uint32_t rtl_datalo = changes->fp_reg_data[i] & 0xFFFFFFFF;
                        uint32_t emu_datahi = emu_state_change.fp_reg_data[i] >> 32;
                        uint32_t rtl_datahi = changes->fp_reg_data[i] >> 32;
                        stream << "FP Register data error f" << emu_state_change.fp_reg_rd;
                        if (errlo && errhi)
                            stream << "[" << (2*i+1) << ", " << (2*i) << "]";
                        else
                            stream << "[" << (errlo ? (2*i) : (2*i+1)) << "]";
                        stream << ". Expected data is " << std::hex;
                        if (errlo && errhi)
                            stream << "{0x" << std::hex << emu_datahi << ", 0x" << emu_datalo << "}";
                        else
                            stream << "0x" << std::hex << (errlo ? emu_datalo : emu_datahi);
                        stream << " but provided is ";
                        if (errlo && errhi)
                            stream << "(0x" << std::hex << rtl_datahi << ", 0x" << rtl_datalo << ")";
                        else
                            stream << "0x" << std::hex << (errlo ? rtl_datalo : rtl_datahi);
                        stream << " Current mask: 0x" << get_mask(0) << std::dec << std::endl;
                        error_msg += stream.str();
                        check_res = CHECKER_ERROR;
                    }
                }
              }


            }
        }

        // Changing mask register
        for(int m = 0; m < 8; m++)
        {
            if(changes->m_reg_mod[m] != emu_state_change.m_reg_mod[m])
            {
                stream << "Mask Register write error for entry " << m << ". Expected write is " << emu_state_change.m_reg_mod[m] << " but provided is " << changes->m_reg_mod[m] << std::endl;
                error_msg += stream.str();
                check_res = CHECKER_ERROR;
            }
            if(emu_state_change.m_reg_mod[m])
            {
                for(int i = 0; i < VL; i++)
                {
                    if(changes->m_reg_data[m][i] != emu_state_change.m_reg_data[m][i])
                    {
                        stream << "Mask Register data error m" << m << "[" << i << "]. Expected data is " << std::hex << (uint32_t) emu_state_change.m_reg_data[m][i] << " but provided is " << (uint32_t) changes->m_reg_data[m][i] << std::dec << std::endl;
                        error_msg += stream.str();
                        check_res = CHECKER_ERROR;
                    }
                }
            }
        }

        // Memory changes
        for(int i = 0; i < VL; i++)
        {
            if(changes->mem_mod[i] != emu_state_change.mem_mod[i])
            {
                stream << "Memory write error (" << i << "). Expected write is " << emu_state_change.mem_mod[i] << " but provided is " << changes->mem_mod[i] << std::endl;
                error_msg += stream.str();
                check_res = CHECKER_ERROR;
            }
            if(emu_state_change.mem_mod[i])
            {
                if(changes->mem_size[i] != emu_state_change.mem_size[i])
                {
                    stream << "Memory write size error (" << i << "). Expected size is " << emu_state_change.mem_size[i] << " but provided is " << changes->mem_size[i] << std::endl;
                    error_msg += stream.str();
                    check_res = CHECKER_ERROR;
                }
                if(changes->mem_addr[i] != emu_state_change.mem_addr[i])
                {
                    stream << "Memory write address error (" << i << "). Expected addr is 0x" << std::hex << emu_state_change.mem_addr[i] << " but provided is 0x" << changes->mem_addr[i] << std::dec << std::endl;
                    error_msg += stream.str();
                    check_res = CHECKER_ERROR;
                }
                uint64_t rtl_mem_data = changes->mem_data[i] & mem_mask(changes->mem_size[i]);
                uint64_t emu_mem_data = emu_state_change.mem_data[i] & mem_mask(changes->mem_size[i]);
                // Atomic instructions are not checked currently
                if((rtl_mem_data != emu_mem_data) && !inst.is_amo())
                {
                    stream << "Memory write data error (" << i << "). Expected data is 0x" << std::hex << emu_mem_data << " but provided is 0x" << rtl_mem_data << std::dec << std::endl;
                    error_msg += stream.str();
                    check_res = CHECKER_ERROR;
                }
            }
        }

        // TensorLoad
        if(inst.is_tensor_load())
        {
            int entry;
            int size;
            uint64_t data;
            data = get_scratchpad_value(0, 0, &entry, &size);
            std::list<bool> conv_list;
            get_scratchpad_conv_list(&conv_list);
            auto conv_list_it = conv_list.begin();

            // For all the written entries
            for(int i = 0; i < size && emu_state_change.tensor_mod; i++)
            {
                // Load was skipped due conv CSR, ignore check
                if(* conv_list_it == 1)
                {
                    conv_list_it++;
                    continue;
                }
                conv_list_it++;

                // Looks for the 1st entry in the list of RTL written lines with same destination
                auto it = scp_entry_list[thread].begin();
                while(it != scp_entry_list[thread].end())
                {
                    if(it->entry == (entry + i)) { break; }
                    it++;
                }

                // Checks that an entry was actually found
                if(it == scp_entry_list[thread].end())
                {
                    stream << "Couldn't find scratchpad destination " << entry + i << " in the RTL scratchpad list!!" << std::endl;
                    error_msg += stream.str();
                    return CHECKER_ERROR;
                }

                // Compares the data
                for(int j = 0; j < 8; j++)
                {
                    data = get_scratchpad_value(entry + i, j, &entry, &size);
                    if(data != it->data[j])
                    {
                        stream << "TensorLoad write data error for cacheline " << i << " written in entry " << entry + i << " data lane " << j << ". Expected data is 0x" << std::hex << data << " but provided is 0x" << it->data[j] << std::dec << std::endl;
                        error_msg += stream.str();
                        scp_entry_list[thread].erase(it);
                        return CHECKER_ERROR;
                    }
                }
                scp_entry_list[thread].erase(it);
            }
        }

        // TensorFMA
        if(inst.is_tensor_fma())
        {
            int size;
            int passes;
            bool conv_skip[VL];
            uint32_t data;
            data = get_tensorfma_value(0, 0, 0, &size, &passes, &conv_skip[0]);
            // For all the passes
            for(int pass = 0; pass < passes; pass++)
            {
                // For all the written entries
                for(int entry = 0; entry < size; entry++)
                {
                    // Move to next entry if this pass for this entry was skipped due conv CSR
                    bool skip = true;
                    for(int lane = 0; lane < VL; lane++)
                    {
                        get_tensorfma_value(entry, pass, lane, &size, &passes, &conv_skip[lane]);
                        skip = skip && conv_skip[lane];
                    }

                    if (skip) continue;

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
                        stream << "Couldn't find TensorFMA destination " << entry << " in the RTL TensorFMA list for pass " << pass << "!!" << std::endl;
                        error_msg += stream.str();
                        check_res = CHECKER_ERROR;
                    }
                    else
                    {
                        // Compares the data for all the lanes (8 x 32b lanes)
                        for(int lane = 0; lane < VL; lane++)
                        {
                            if(conv_skip[lane] == 1) continue;
                            data = get_tensorfma_value(entry, pass, lane, &size, &passes, &conv_skip[lane]);
                            if(data != it->data[lane])
                            {
                                stream << "TensorFMA write data error for register f" << entry << "[" << lane << "] pass " << pass << ". Expected data is 0x" << std::hex << data << " but provided is 0x" << it->data[lane] << std::dec << std::endl;
                                error_msg += stream.str();
                                tensorfma_list[thread].erase(it);
                                return CHECKER_ERROR;
                            }
                        }
                        tensorfma_list[thread].erase(it);
                    }
                }
            }
        }

        // TensorQuant
        if(inst.is_tensor_quant())
        {
            int size;
            int transforms;
            bool is_pack;
            uint32_t data;
            data = get_tensorquant_value(0, 0, 0, &size, &transforms, &is_pack);
            // For all the transforms
            for(int trans = 0; trans < transforms; trans++)
            {
                // For all the written entries
                for(int entry = 0; entry < size; entry++)
                {
                    // Looks for the 1st entry in the list of RTL written lines with same destination
                    auto it = tensorquant_list[thread].begin();

                    // Pack 128b writes two times same destination (even) and doesn't write odd
                    data = get_tensorquant_value(entry, trans, 0, &size, &transforms, &is_pack);
                    // Ignore odd entries
                    if(is_pack && (entry & 1)) continue;

                    // Check next entry for regular operations, ignore 1 write for packs
                    int entries_checked = is_pack ? 2 : 1;
                    for(int i = 0; i < entries_checked; i++)
                    {
                        it = tensorquant_list[thread].begin();
                        while(it != tensorquant_list[thread].end())
                        {
                            if(it->entry == entry) { break; }
                            it++;
                        }
                        // Checks that an entry was actually found
                        if(it == tensorquant_list[thread].end())
                        {
                            stream << "Couldn't find TensorQuant destination " << entry << " in the RTL TensorQuant list for trans " << trans << "!!" << std::endl;
                            error_msg += stream.str();
                            return CHECKER_ERROR;
                        }
                        // Ignore first write for pack 128b
                        if(is_pack && (i == 0))
                        {
                            tensorquant_list[thread].erase(it);
                        }
                    }

                    // Compares the data for all the lanes (8 x 32b lanes)
                    for(int lane = 0; lane < VL; lane++)
                    {
                        data = get_tensorquant_value(entry, trans, lane, &size, &transforms, &is_pack);
                        if(data != it->data[lane])
                        {
                            stream << "TensorQuant write data error for register f" << entry << "[" << lane << "] trans " << trans << ". Expected data is 0x" << std::hex << data << " but provided is 0x" << it->data[lane] << std::dec << std::endl;
                            error_msg += stream.str();
                            tensorquant_list[thread].erase(it);
                            return CHECKER_ERROR;
                        }
                    }
                    tensorquant_list[thread].erase(it);
                }
            }
        }

        // Reduce
        if(inst.is_reduce())
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
                    stream << "Couldn't find Reduce destination " << entry << " in the RTL Reduce list!!" << std::endl;
                    error_msg += stream.str();
                    check_res = CHECKER_ERROR;
                }
                else
                {
                    // Compares the data for all the lanes (4 x 32b lanes)
                    for(int lane = 0; lane < 4; lane++)
                    {
                        data = get_reduce_value(entry, lane, &size, &start_entry);
                        if(data != it->data[lane])
                        {
                            stream << "Reduce write data error for register " << entry << " lane " << lane << ". Expected data is 0x" << std::hex << data << " but provided is 0x" << it->data[lane] << std::dec << std::endl;
                            error_msg += stream.str();
                            reduce_list[thread].erase(it);
                            return CHECKER_ERROR;
                        }
                    }
                    reduce_list[thread].erase(it);
                }
            }
        }
    }

    // PC update
    if(emu_state_change.pc_mod)
       current_pc[thread] = emu_state_change.pc;
    else
       current_pc[thread] += inst.size();

    return check_res;
}

// Return the last error message
std::string checker::get_error_msg()
{
    return error_msg;
}

// enables or disables the 2nd thread
void checker::thread1_enabled(unsigned minionId, uint64_t en, uint64_t pc)
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
    scp_entry_list[thread].push_back(scp_entry);
}

// TensorFMA write
void checker::tensorfma_write(uint32_t thread, uint32_t entry, uint32_t * data, uint32_t tensorfma_regfile_wmask)
{
    tensorfma_entry tensorfma;

    tensorfma.entry = entry;
    for(int i = 0; i < VL; i++)
    {
        tensorfma.data[i] = data[i];
    }
    tensorfma.tensorfma_regfile_wmask = tensorfma_regfile_wmask;

    tensorfma_list[thread].push_back(tensorfma);
}

// TensorQuant write
void checker::tensorquant_write(uint32_t thread, uint32_t entry, uint32_t * data, uint32_t tensorquant_regfile_wmask)
{
    tensorfma_entry tensorquant;

    tensorquant.entry = entry;
    for(int i = 0; i < VL; i++)
    {
        tensorquant.data[i] = data[i];
    }
    tensorquant.tensorfma_regfile_wmask = tensorquant_regfile_wmask;

    tensorquant_list[thread].push_back(tensorquant);
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

bool checker::address_is_in_ignored_region(uint64_t addr)
{
   auto it = ignored_mem_regions.begin();
   while (it != ignored_mem_regions.end())
   {
      if ((addr >= it->base) && (addr <= it->top)) return true;
      it++;
   }
   return false;
}

void checker::add_ignored_mem_region(uint64_t base, uint64_t top)
{
   ignored_mem_region region;
   region.base = base;
   region.top  = top;
   ignored_mem_regions.push_back(region);
}
