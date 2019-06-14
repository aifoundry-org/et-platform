#ifndef _SYS_EMU_H_
#define _SYS_EMU_H_

#include "api_communicate.h"
#include "emu_defines.h"
#include "net_emulator.h"
#include "rvtimer.h"

#include <cstdint>
#include <list>
#include <memory>
#include <tuple>

////////////////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////////////////

#define RESET_PC    0x8000001000ULL
#define SP_RESET_PC 0x0040000000ULL
#define FCC_T0_ADDR 0x01003400C0ULL
#define FCC_T1_ADDR 0x01003400D0ULL
#define FLB_ADDR    0x0100340100ULL

////////////////////////////////////////////////////////////////////////////////
/// \bried Struct holding the values of the parsed command line arguments
////////////////////////////////////////////////////////////////////////////////
struct sys_emu_cmd_options {
    char * elf_file            = nullptr;
    char * mem_desc_file       = nullptr;
    char * net_desc_file       = nullptr;
    char * api_comm_path       = nullptr;
    bool elf                   = false;
    bool mem_desc              = false;
    bool net_desc              = false;
    bool api_comm              = false;
    bool master_min            = false;
    bool minions               = false;
    bool second_thread         = true;
    bool shires                = false;
    bool log_en                = false;
    bool create_mem_at_runtime = false;
    int  log_min               = -1;
    char * dump_file           = nullptr;
#ifdef SYSEMU_PROFILING
    char * dump_prof_file      = nullptr;
    int dump_prof              = 0;
#endif
    int dump                   = 0;
    uint64_t dump_addr         = 0;
    uint64_t dump_size         = 0;
    char *   dump_mem          = nullptr;
    uint64_t reset_pc          = RESET_PC;
    uint64_t sp_reset_pc       = SP_RESET_PC;
    bool reset_pc_flag         = false;
    bool sp_reset_pc_flag      = false;
    bool debug                 = false;
    uint64_t max_cycles        = 10000000;
    bool max_cycle             = false;
    bool mins_dis              = false;
};

/// Function used for parsing the command line arguments
std::tuple<bool, struct sys_emu_cmd_options> parse_command_line_arguments(int argc, char* argv[]);



// Reduce state
enum reduce_state
{
    Reduce_Idle,
    Reduce_Ready_To_Send,
    Reduce_Data_Consumed
};


class sys_emu
{
public:
    sys_emu() = default;
    virtual ~sys_emu() = default;

    void init_simulator(const sys_emu_cmd_options& cmd_options);

    static void fcc_to_threads(unsigned shire_id, unsigned thread_dest,
                               uint64_t thread_mask, unsigned cnt_dest);
    static void msg_to_thread(int thread_id);
    static void send_ipi_redirect_to_threads(unsigned shire_id, uint64_t thread_mask);
    static void raise_timer_interrupt(uint64_t shire_mask);
    static void clear_timer_interrupt(uint64_t shire_mask);
    static void raise_software_interrupt(unsigned shire_id, uint64_t thread_mask);
    static void clear_software_interrupt(unsigned shire_id, uint64_t thread_mask);
    static void raise_external_interrupt(unsigned shire_id);
    static void clear_external_interrupt(unsigned shire_id);
    static void raise_external_supervisor_interrupt(unsigned shire_id);
    static void clear_external_supervisor_interrupt(unsigned shire_id);
    static void evl_dv_handle_irq_inj(bool raise, uint64_t subopcode, uint64_t shire_mask);
    int main_internal(int argc, char * argv[]);

    static uint64_t get_emu_cycle()  { return emu_cycle; }
    static RVTimer& get_pu_rvtimer() { return pu_rvtimer; }

protected:

    // Function to be overwritten by subclass to allocate custom
    virtual std::unique_ptr<api_communicate> allocate_api_listener(bemu::MainMemory* memory) {
        return std::unique_ptr<api_communicate>(new api_communicate(memory));
    }

private:

#ifdef SYSEMU_DEBUG
    bool process_dbg_cmd(std::string cmd);
    bool get_pc_break(uint64_t &pc, int &thread);
#endif

    static uint64_t        emu_cycle;
    static std::list<int>  enabled_threads; // List of enabled threads
    static std::list<int>  fcc_wait_threads[2]; // List of threads waiting for an FCC
    static std::list<int>  port_wait_threads; // List of threads waiting for a port write
    static uint16_t        pending_fcc[EMU_NUM_THREADS][EMU_NUM_FCC_COUNTERS_PER_THREAD]; // Pending FastCreditCounter list
    static uint64_t        current_pc[EMU_NUM_THREADS]; // PC for each thread
    static reduce_state    reduce_state_array[EMU_NUM_MINIONS]; // Reduce state
    static uint32_t        reduce_pair_array[EMU_NUM_MINIONS]; // Reduce pairing minion
    static int             global_log_min;
    static RVTimer         pu_rvtimer;

    net_emulator net_emu;
    std::unique_ptr<api_communicate> api_listener;
};


#endif// _SYS_EMU_H_
