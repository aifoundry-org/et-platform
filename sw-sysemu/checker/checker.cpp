// Global
#include <cmath>
#include <exception>
#include <assert.h>
#include <queue>

// Local
#include "checker.h"
#include "emu_casts.h"
#include "emu.h"
#include "emu_gio.h"
#include "insn.h"
#include "common/riscv_disasm.h"

// Defines
#define TBOX_REGION_START 0xFFF80000
#define TBOX_REGION_END (TBOX_REGION_START + 512)

#define ESR_SHIRE_REGION_START 0x100340000L
#define ESR_SHIRE_REGION_END   0x1FFF5FFF8L

uint32_t tbox_id_from_thread(uint32_t current_thread);

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
bool fail_on_check = false;       // Option to still emulation instruction but dont fail test


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
checker::checker(main_memory * memory_, testLog& log_, bool checker_en)
    : log(log_)
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

    checker_instance = this;
    fail_on_check = checker_en;
    memory_instance = memory;
    init_emu();

    set_memory_funcs(checker_memread8,
                     checker_memread16,
                     checker_memread32,
                     checker_memread64,
                     checker_memwrite8,
                     checker_memwrite16,
                     checker_memwrite32,
                     checker_memwrite64);

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
        log << LOG_ERR << "start pc with thread invalid (" << thread << ")" << endm;
    current_pc[thread] = pc;
}

// Sets the PC due IPI
void checker::ipi_pc(uint32_t thread, uint64_t pc)
{
    if(thread >= EMU_NUM_THREADS)
        log << LOG_ERR << "IPI pc with thread invalid (" << thread << ")" << endm;
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
	    LOG(ERR, "\tReduce error: Minion: 0x%08x found pairing receiver minion: 0x%lu in Reduce_Ready_To_Send!!", (thread>>1),other_min);	
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
	    LOG(ERR, "\tReduce error: Minion: 0x%08x found pairing sender minion: 0x%lu in Reduce_Data_Consumed!!", (thread>>1),other_min);	
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
checker_result checker::emu_inst(uint32_t thread, inst_state_change * changes, std::queue<uint32_t> &wake_minions)
{
    checker_result check_res = CHECKER_OK;
    if (thread >= EMU_NUM_THREADS )
       log << LOG_ERR << "emu_inst with thread invalid (" << thread << ")" << endm;

    if (!threadEnabled[thread])
       log << LOG_ERR << "emu_inst called for thread " << thread << ", which is disabled" << endm;

    log << LOG_DEBUG << "emu_inst called for thread " << thread << endm;

    set_thread(thread);
    setlogstate(&emu_state_change); // This is done every time just in case we have several checkers

    insn_t inst;
    std::ostringstream stream;
    error_msg = "";

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
            check_pending_interrupts();
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
                int  wake_minion=-1;
                checker_result res = do_reduce(thread, value, &wake_minion);
                if (wake_minion >=0) wake_minions.push(wake_minion);
                if(res == CHECKER_WAIT) return CHECKER_WAIT;
            }

            // Execute the instruction (may trap)
            inst.execute();

            // check if we have to wake any minions
            std::queue<uint32_t> &minions_to_awake = get_minions_to_awake();
            while( ! minions_to_awake.empty() ) {
              wake_minions.push(minions_to_awake.front());
              minions_to_awake.pop();
            }
            
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
               log << LOG_ERR << "Sad, looks like we are stuck in an infinite trap recursion. Giving up." << endm;
               retry = false;
            }
        }
        catch (const checker_wait_t &t)
        {

          log<<LOG_INFO<<"Delaying retire because of: " << t.what() << endm;
          return CHECKER_WAIT;
        }
        catch (const std::exception& e)
        {
            log << LOG_ERR << e.what() << endm;
        }

        // Emulate the RBOXes
        for (uint32_t s = 0; s < EMU_NUM_COMPUTE_SHIRES; s++)
        {
            for (uint32_t r = 0; r < EMU_RBOXES_PER_SHIRE; r++)
                GET_RBOX(s, r).run(true);
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
        stream << "BEMU Checker mismatch @ PC 0x" << std::hex << current_pc[thread] << std::dec << " (" << insn_disasm << ") -> ";

        // Check PC
        if(changes->pc != current_pc[thread])
        {
	    LOG(ERR, "\tBEMU Checker PC error. BEMU expects PC: 0x%lx but DUT reported PC: 0x%lx ",current_pc[thread] ,changes->pc);	
            // don't check anything else when PC mismatches... everything would mismatch
            check_res = CHECKER_ERROR;
            goto finished_checking;
        }

        // Instruction bits -- the RTL monitor shows the uncompressed version of the instruction always,
        // while bemu shows the original instruction bits, so we cannot really compare them here
        /*if(changes->inst_bits != inst.bits)
        {
            stream << "BEMU Checker Inst error. BEMU expects inst is 0x" << std::hex << inst.bits << " but DUT reported 0x" << changes->inst_bits << std::dec << std::endl;
            // don't check anything else when instruction bits mismatch... everything would mismatch
            check_res = CHECKER_ERROR;
            goto finished_checking;
        }*/

        // Changing integer register
        if(changes->int_reg_mod != emu_state_change.int_reg_mod)
        {
	    stream << "BEMU Checker Int Register write error. BEMU expects register write is " << emu_state_change.int_reg_mod << " but DUT reported " << changes->int_reg_mod << std::endl;
            check_res = CHECKER_ERROR;
        }
        if(emu_state_change.int_reg_mod)
        {
            if(changes->int_reg_rd != emu_state_change.int_reg_rd)
            {
	    LOG(ERR, "\tBEMU Checker Int Register dest error. BEMU expects register dest is: %d but DUT reported: %d ",emu_state_change.int_reg_rd , changes->int_reg_rd);	
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

            //Read esr_icache_trigger status
            if(inst.is_load() && (emu_state_change.mem_addr[0] >= ESR_SHIRE_REGION_START) && (emu_state_change.mem_addr[0] < ESR_SHIRE_REGION_END))
            {
              log << LOG_INFO << "Access to SHIRE ESR(" << insn_disasm << ")" << endm;
              //Set EMU state to what RTL says
              emu_state_change.int_reg_data = changes->int_reg_data;
              init(inst.rd(), emu_state_change.int_reg_data);
            }

            // Writes to X0/Zero are ignored
            if((changes->int_reg_data != emu_state_change.int_reg_data) && (emu_state_change.int_reg_rd != 0))
            {
	    	LOG(ERR, "\tBEMU Checker Int Register data error. BEMU expects data is: 0x%lx but DUT reported: 0x%lx ",emu_state_change.int_reg_data , changes->int_reg_data);	
                check_res = CHECKER_ERROR;
                //Set EMU state to what RTL says
                init(inst.rd(), emu_state_change.int_reg_data);
            }
        }

        // Changing floating register
        if(changes->fp_reg_mod != emu_state_change.fp_reg_mod)
        {
            stream << "BEMU Checker FP Register write error. BEMU expects write is " << emu_state_change.fp_reg_mod << " but DUT reported " << changes->fp_reg_mod << std::endl;
            check_res = CHECKER_ERROR;
        }

        // Changing fflags
        if ( changes->fflags_mod != emu_state_change.fflags_mod )
        {
          // Someone changed the flags
          std::string changer =  emu_state_change.fflags_mod ? "EMU" : "RTL";
	  LOG(WARN, "\tBEMU Checker fflags changed by %s. BEMU expects new flags: 0x%lx but DUT reported: 0x%lx ",changer.c_str(), emu_state_change.fflags_value , changes->fflags_value);	
          check_res = CHECKER_WARNING;
        }
        
        if ( emu_state_change.fflags_mod)
        {
          if ( changes->fflags_value != emu_state_change.fflags_value )
          {
	  LOG(WARN, "\tBEMU Checker fflags values change. BEMU expects new flags: 0x%lx but DUT reported: 0x%lx ",emu_state_change.fflags_value , changes->fflags_value);	
            check_res = CHECKER_WARNING;
          }
        }

        if(emu_state_change.fp_reg_mod)
        {
            if(changes->fp_reg_rd != emu_state_change.fp_reg_rd)
            {
	  	LOG(ERR, "\tBEMU Checker FP Register dest error. BEMU expects dest is f%d but DUT reported f%d ",emu_state_change.fp_reg_rd , changes->fp_reg_rd);	
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

            bool fp_reg_data_mismatch = false;
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
                    stream << "BEMU Checker FP Register data_error f" << emu_state_change.fp_reg_rd;
                    if (errlo && errhi)
                        stream << "[" << (2*i+1) << ", " << (2*i) << "]";
                    else
                        stream << "[" << (errlo ? (2*i) : (2*i+1)) << "]";
                    stream << ". BEMU expects data is ";
                    if (errlo && errhi)
                        stream << "{0x" << std::hex << emu_datahi << ", 0x" << emu_datalo << "}";
                    else
                        stream << "0x" << std::hex << (errlo ? emu_datalo : emu_datahi);
                    stream << " but DUT reported ";
                    if (errlo && errhi)
                        stream << "{0x" << std::hex << rtl_datahi << ", 0x" << rtl_datalo << "}";
                    else
                        stream << "0x" << std::hex << (errlo ? rtl_datalo : rtl_datahi);
                    stream << " Current mask: 0x" << get_mask(0) << std::dec << std::endl;
                    check_res = CHECKER_ERROR;
                }
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
                        stream << "BEMU Checker FP Register data_error f" << emu_state_change.fp_reg_rd;
                        if (errlo && errhi)
                            stream << "[" << (2*i+1) << ", " << (2*i) << "]";
                        else
                            stream << "[" << (errlo ? (2*i) : (2*i+1)) << "]";
                        stream << ". BEMU expects data is ";
                        if (errlo && errhi)
                            stream << "{0x" << std::hex << emu_datahi << ", 0x" << emu_datalo << "}";
                        else
                            stream << "0x" << std::hex << (errlo ? emu_datalo : emu_datahi);
                        stream << " but DUT reported ";
                        if (errlo && errhi)
                            stream << "(0x" << std::hex << rtl_datahi << ", 0x" << rtl_datalo << ")";
                        else
                            stream << "0x" << std::hex << (errlo ? rtl_datalo : rtl_datahi);
                        stream << " Current mask: 0x" << get_mask(0) << std::dec << std::endl;
                        check_res = CHECKER_ERROR;
                    }
                }
              }
              fp_reg_data_mismatch = fp_reg_data_mismatch || (changes->fp_reg_data[i] != emu_state_change.fp_reg_data[i]);
            }

            //Set EMU state to what RTL says
            if (fp_reg_data_mismatch)
            {
                fpinit((freg)changes->fp_reg_rd, changes->fp_reg_data);
            }
        }

        // Changing mask register
        for(int m = 0; m < 8; m++)
        {
            if(changes->m_reg_mod[m] != emu_state_change.m_reg_mod[m])
            {
	  	LOG(ERR, "\tBEMU Checker Mask Register write error for entry %d. BEMU expects write is %d but DUT reported %d ",m, emu_state_change.m_reg_mod[m] , changes->m_reg_mod[m]);	
                check_res = CHECKER_ERROR;
            }
            if(emu_state_change.m_reg_mod[m])
            {
                for(int i = 0; i < VL; i++)
                {
                    if(changes->m_reg_data[m][i] != emu_state_change.m_reg_data[m][i])
                    {
	  		stream << "BEMU Checker Mask Register data_error m" << m << "[" << i << "]. BEMU expects data is " << std::hex << (uint32_t) emu_state_change.m_reg_data[m][i] << " but DUT reported " << (uint32_t) changes->m_reg_data[m][i] << std::dec << std::endl;
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
                stream << "BEMU Checker Memory write error (" << i << "). BEMU expects write is " << emu_state_change.mem_mod[i] << " but DUT reported " << changes->mem_mod[i] << std::endl;
                check_res = CHECKER_ERROR;
            }
            if(emu_state_change.mem_mod[i])
            {
                if(changes->mem_size[i] != emu_state_change.mem_size[i])
                {
                    stream << "BEMU Checker Memory write size error (" << i << "). BEMU expects size is " << emu_state_change.mem_size[i] << " but DUT reported " << changes->mem_size[i] << std::endl;
                    check_res = CHECKER_ERROR;
                }
                if( (changes->mem_addr[i] & VA_M) != (emu_state_change.mem_addr[i] & VA_M))
                {
                    stream << "BEMU Checker Memory write address error (" << i << "). BEMU expects addr is 0x" << std::hex << emu_state_change.mem_addr[i] << " & " << VA_M <<" but DUT reported 0x" << changes->mem_addr[i] <<" & "<< VA_M << std::dec << std::endl;
                    check_res = CHECKER_ERROR;
                }
                uint64_t rtl_mem_data = changes->mem_data[i] & mem_mask(changes->mem_size[i]);
                uint64_t emu_mem_data = emu_state_change.mem_data[i] & mem_mask(changes->mem_size[i]);
                // Atomic instructions are not checked currently
                if((rtl_mem_data != emu_mem_data) && !inst.is_amo())
                {
                    stream << "BEMU Checker Memory write data_error (" << i << "). BEMU expects data is 0x" << std::hex << emu_mem_data << " but DUT reported 0x" << rtl_mem_data << std::dec << std::endl;
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

            if (emu_state_change.tl_transform)
                aggregate_tl_data(thread);

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
                    if(it->entry == get_scratchpad_next_entry(entry+i)) { break; }
                    it++;
                }

                // Checks that an entry was actually found
                if(it == scp_entry_list[thread].end())
                {
                    stream << "BEMU Checker couldn't find scratchpad destination " << entry + i << " in the DUT reported scratchpad list!!" << std::endl;
                    stream << "SCP Candidates: " << std::endl;
                    for(auto e : scp_entry_list[thread]) {
                        stream << "\tEntry : " << e.entry << std::endl;
                    }
                    check_res = CHECKER_ERROR;
                }
                else
                {
                    // Compares the data
                    for(int j = 0; j < 8; j++)
                    {
                        data = get_scratchpad_value(entry + i, j, &entry, &size);
                        if(data != it->data[j])
                        {
                            stream << "BEMU Checker TensorLoad write data_error for cacheline " << i << " written in entry " << entry + i << " data lane " << j << ". BEMU expects data is 0x" << std::hex << data << " but DUT reported 0x" << it->data[j] << std::dec << std::endl;
                            check_res = CHECKER_ERROR;
                        }
                    }
                    scp_entry_list[thread].erase(it);
                }
            }
        }

        // TensorFMA
        if(inst.is_tensor_fma())
        {
            // For all the passes
            for(int pass = 0; pass < emu_state_change.tensorfma_passes; pass++)
            {
                // For all the written entries
                for(int freg = 0; freg < 32; freg++)
                {
                    if (~emu_state_change.tensorfma_mod[pass] & (1u<<freg))
                        continue;

                    // Move to next entry if this pass for this entry was skipped due conv CSR
                    bool skip = true;
                    for(int lane = 0; lane < VL; lane++)
                    {
                        skip = skip && emu_state_change.tensorfma_skip[pass][freg][lane];
                    }
                    if (skip) continue;

                    // Looks for the 1st entry in the list of RTL written lines with same destination
                    auto it = std::find_if(tensorfma_list[thread].begin(), tensorfma_list[thread].end(),
                                           [=] (const tensorfma_entry& x) { return x.entry == freg; });
                    // Checks that an entry was actually found
                    if(it == tensorfma_list[thread].end())
                    {
                        stream << "BEMU Checker couldn't find TensorFMA destination " << freg << " in the DUT reported TensorFMA list for pass " << pass << "!!" << std::endl;
                        check_res = CHECKER_ERROR;
                    }
                    else
                    {
                        // Compares the data for all the lanes (8 x 32b lanes)
                        for(int lane = 0; lane < VL; lane++)
                        {
                            if(emu_state_change.tensorfma_skip[pass][freg][lane]) continue;
                            uint32_t data = emu_state_change.tensorfma_data[pass][freg][lane];
                            if(data != it->data[lane])
                            {
                                stream << "BEMU Checker TensorFMA write data_error for register f" << freg << "[" << lane << "] pass " << pass << ". BEMU expects data is 0x" << std::hex << data << " but DUT reported 0x" << it->data[lane] << std::dec << std::endl;
                                check_res = CHECKER_ERROR;
                            }
                        }
                        tensorfma_list[thread].erase(it);
                    }
                }
            }
#if 0 /* FIXME: Re-enable this when the TIMA RTL is fixed */
            while (!tensorfma_list[thread].empty())
            {
                const auto& front = tensorfma_list[thread].front();
                if (front.tensorfma_regfile_wmask)
                {
                    stream << "BEMU Checker missing TensorFMA destination " << tensorfma_list[thread].front().entry << std::endl;
                    check_res = CHECKER_ERROR;
                }
                tensorfma_list[thread].pop_front();
            }
#endif
        }

        // TensorQuant
        if(inst.is_tensor_quant())
        {
            int freg;
            int size;
            int transforms;
            bool skip_entry;
            uint32_t data;
            data = get_tensorquant_value(0, 0, 0, &freg, &size, &transforms, &skip_entry);
            // For all the transforms
            for(int trans = 0; trans < transforms; trans++)
            {
                // For all the written entries
                for(int entry = 0; entry < size; entry++)
                {
                    data = get_tensorquant_value(entry, trans, 0, &freg, &size, &transforms, &skip_entry);
                    auto it = std::find_if(tensorquant_list[thread].begin(), tensorquant_list[thread].end(),
                                           [=] (const tensorfma_entry& x) { return x.entry == freg; });
                    if (skip_entry && it != tensorquant_list[entry].end())
                    {
                        // Pack 128b writes the destination once or twice depending on number of columns
                        tensorquant_list[thread].erase(it);
                        it = std::find_if(tensorquant_list[thread].begin(), tensorquant_list[thread].end(),
                                          [=] (const tensorfma_entry& x) { return x.entry == freg; });
                    }
                    // Checks that an entry was actually found
                    if(it == tensorquant_list[thread].end())
                    {
                        stream << "BEMU Checker couldn't find TensorQuant destination " << freg << " in the DUT reported TensorQuant list for trans " << trans << "!!" << std::endl;
                        check_res = CHECKER_ERROR;
                        goto finished_checking;
                    }
                    // Compares the data for all the lanes (VL x 32b lanes)
                    for(int lane = 0; lane < VL; lane++)
                    {
                        data = get_tensorquant_value(entry, trans, lane, &freg, &size, &transforms, &skip_entry);
                        if(data != it->data[lane])
                        {
                            stream << "BEMU Checker TensorQuant write data_error for register f" << freg << "[" << lane << "] trans " << trans << ". BEMU expects data is 0x" << std::hex << data << " but DUT reported 0x" << it->data[lane] << std::dec << std::endl;
                            check_res = CHECKER_ERROR;
                        }
                    }
                    tensorquant_list[thread].erase(it);
                }
            }
            while (!tensorquant_list[thread].empty())
            {
                const auto& front = tensorquant_list[thread].front();
                if (front.tensorfma_regfile_wmask)
                {
                    stream << "BEMU Checker missing TensorQuant destination " << front.entry << std::endl;
                    check_res = CHECKER_ERROR;
                }
                tensorquant_list[thread].pop_front();
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
                    stream << "BEMU Checker couldn't find Reduce destination " << entry << " in the DUT provided Reduce list!!" << std::endl;
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
                            stream << "BEMU Checker Reduce write data_error for register " << entry << " lane " << lane << ". BEMU expects data is 0x" << std::hex << data << " but DUT reported 0x" << it->data[lane] << std::dec << std::endl;
                            check_res = CHECKER_ERROR;
                        }
                    }
                    reduce_list[thread].erase(it);
                }
            }
        }
    }

finished_checking:
    // PC update
    if(emu_state_change.pc_mod)
       current_pc[thread] = emu_state_change.pc;
    else
       current_pc[thread] += inst.size();

    if (check_res != CHECKER_OK)
    {
        error_msg += stream.str();
        if (!fail_on_check)
            check_res = CHECKER_WARNING;
    }
    return check_res;
}

void checker::raise_interrupt(unsigned minionId, int cause) 
{
  ::raise_interrupt(minionId, cause); //note, using :: to call the one from the checker, not itself
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
void checker::tensorload_write(uint32_t thread, uint32_t entry, uint64_t * data, uint32_t banks)
{
    scratchpad_entry scp_entry;

    scp_entry.entry = entry;
    scp_entry.banks = banks;
    for(int i = 0; i < 8; i++)
    {
        scp_entry.data[i] = data[i];
    }
    scp_entry_list[thread].push_back(scp_entry);
}

void checker::aggregate_tl_data(uint32_t thread)
{
  std::list<scratchpad_entry> aggregated_scp_entries;
  
  log << LOG_DEBUG << "Aggregating partial writes to dcache. size : " << scp_entry_list[thread].size() << endm;
  log << LOG_DEBUG << "Dumping SCP entries Before aggregation " << endm;
  
  for ( auto e : scp_entry_list[thread] ) {
    log << LOG_DEBUG << "Entry: " << std::hex << e.entry << " Banks: " << e.banks << " data: " << e.data[0] << " " << e.data[1] << " " << e.data[2] << " " << e.data[3] << " " << e.data[4] << " " << e.data[5] << " " << e.data[6] << " " << e.data[7] << std::dec << endm;
  }
  
  // empty the queue, merge all that can be merged
  while ( !scp_entry_list[thread].empty() ) {
    auto it = scp_entry_list[thread].begin();
    scratchpad_entry entry;
    // Get the entry and copy it. Move data to next half-line if it's needed
    entry = *it;
    it = scp_entry_list[thread].erase(it);
    int addr_fhl = entry.entry & ~(1 << 5);
    int addr_shl = entry.entry |  (1 << 5);
    if ( entry.entry == addr_shl )
    {
      // This entry address writes to a second half-line.
      entry.entry = addr_fhl;
      for( int i = 0 ; i < 4 ; ++i )
      {
        entry.data[4+i] = entry.data[i];
      }
    }

    // Search for all mergeable entries
    log << LOG_DEBUG << "Merging Entry: " << std::hex << entry.entry << " Banks: " << entry.banks << " addr_fhl: " << addr_fhl << " addr_shl: " << addr_shl << std::dec << endm;
    auto it2 = scp_entry_list[thread].begin();
    while (it2 != scp_entry_list[thread].end())
    {
      auto entry2 = *it2;
      if ( entry2.entry == addr_fhl || entry2.entry == addr_shl )
      {
        int offset = entry2.entry == addr_fhl ? 0 : 4;
        for (int i = 0 ; i < 4 ; ++i)
        {
          uint32_t valid = entry2.banks & (uint32_t(1) << i);
          if( valid )
          {
            entry.data[offset+i] = entry2.data[i];
            entry.banks |= (valid << offset);
          }
        }
        log << LOG_DEBUG << "Merging and deleting entry " << std::hex << entry2.entry << std::dec << endm;
        it2 = scp_entry_list[thread].erase(it2);
      } else
      {
        ++it2;
      }
    }
    entry.entry>>=6; //remove entry offset
    aggregated_scp_entries.push_back(entry);
  }
  
  // Replace old list with the new merged list
  std::swap(scp_entry_list[thread], aggregated_scp_entries);

  log << LOG_DEBUG << "Dumping SCP entries After aggregation " << endm;
  
  for( auto e : scp_entry_list[thread] ) {
    log << LOG_DEBUG << "Entry: " << std::hex << e.entry << " Banks: " << e.banks << " data: " << e.data[0] << " " << e.data[1] << " " << e.data[2] << " " << e.data[3] << " " << e.data[4] << " " << e.data[5] << " " << e.data[6] << " " << e.data[7] << std::dec << endm;
  }
    
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


void checker::thread_port_write(uint32_t target_thread, uint32_t port_id, uint32_t source_thread)
{
  commit_msg_port_data(target_thread, port_id, source_thread);
}

void checker::tbox_port_write(uint32_t target_thread, uint32_t port_id, uint32_t tbox_id)
{
  commit_msg_port_data_from_tbox(target_thread, port_id, tbox_id);
}

void checker::rbox_port_write(uint32_t target_thread, uint32_t port_id, uint32_t rbox_id)
{
  commit_msg_port_data_from_rbox(target_thread, port_id, rbox_id);
}
