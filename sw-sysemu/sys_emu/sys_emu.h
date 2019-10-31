#ifndef _SYS_EMU_H_
#define _SYS_EMU_H_

#include "api_communicate.h"
#include "emu_defines.h"
#include "rvtimer.h"
#include "mem_directory.h"
#include "scp_directory.h"

#include <algorithm>
#include <bitset>
#include <cstdint>
#include <list>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <vector>

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
    struct dump_info {
        uint64_t addr;
        uint64_t size;
        std::string file;
    };

    char * elf_file                   = nullptr;
    char * mem_desc_file              = nullptr;
    char * api_comm_path              = nullptr;
    bool elf                          = false;
    bool mem_desc                     = false;
    bool api_comm                     = false;
    bool master_min                   = false;
    bool minions                      = false;
    bool second_thread                = true;
    bool shires                       = false;
    bool log_en                       = false;
    bool create_mem_at_runtime        = false;
    int  log_min                      = -1;
#ifdef SYSEMU_PROFILING
    char * dump_prof_file             = nullptr;
    int dump_prof                     = 0;
#endif
    std::vector<dump_info>            dump_at_end;
    std::unordered_multimap<uint64_t, dump_info> dump_at_pc;
    char *   dump_mem                 = nullptr;
    uint64_t reset_pc                 = RESET_PC;
    uint64_t sp_reset_pc              = SP_RESET_PC;
    bool reset_pc_flag                = false;
    bool sp_reset_pc_flag             = false;
    bool coherency_check              = false;
#ifdef SYSEMU_DEBUG
    bool debug                        = false;
#endif
    uint64_t max_cycles               = 10000000;
    bool max_cycle                    = false;
    bool mins_dis                     = false;
    int  mem_reset                    = 0;
    bool mem_reset_flag               = false;
    char * pu_uart_tx_file            = nullptr;
    char * pu_uart1_tx_file           = nullptr;
    uint64_t log_at_pc                = ~0ull;
    uint64_t stop_log_at_pc           = ~0ull;
};

std::tuple<bool, struct sys_emu_cmd_options> parse_command_line_arguments(int argc, char* argv[]);


class api_communicate;

class sys_emu
{
public:
    sys_emu() = default;
    virtual ~sys_emu() = default;

    bool init_simulator(const sys_emu_cmd_options& cmd_options);

    /// Function used for parsing the command line arguments
    static std::tuple<bool, struct sys_emu_cmd_options> parse_command_line_arguments(int argc, char* argv[]);

    static void set_thread_pc(unsigned thread_id, uint64_t pc);
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
    static void shire_enable_threads(unsigned shire_id);
    int main_internal(int argc, char * argv[]);

    static uint64_t get_emu_cycle()  { return emu_cycle; }
    static RVTimer& get_pu_rvtimer() { return pu_rvtimer; }

    static void activate_thread(int thread_id) { active_threads[thread_id] = true; }
    static void deactivate_thread(int thread_id) { active_threads[thread_id] = false; }
    static bool thread_is_active(int thread_id) { return active_threads[thread_id]; }

    static int running_threads_count() { return running_threads.size(); }

    static bool get_coherency_check() { return coherency_check; }
    static mem_directory& get_mem_directory() { return mem_dir; }
    static bool get_scp_check() { return scp_check; }
    static scp_directory& get_scp_directory() { return scp_dir; }

    static api_communicate &get_api_communicate() { return *api_listener; }

    static bool init_api_listener(const char *communication_path, bemu::MainMemory* memory);

protected:

    // Returns whether a container contains an element
    template<class _container, class _Ty>
    static inline bool contains(_container _C, const _Ty& _Val) {
        return std::find(_C.begin(), _C.end(), _Val) != _C.end();
    }

    // Returns whether a thread is running (not sleeping/waiting)
    static bool thread_is_running(int thread_id) { return contains(running_threads, thread_id); }

    // Checks if a sleeping thread (FCC) has to wake up when receiving an interrupt
    static void raise_interrupt_wakeup_check(unsigned thread_id, uint64_t mask, const char *str);

private:

#ifdef SYSEMU_DEBUG
    struct pc_breakpoint_t {
        uint64_t pc;
        int      thread; // -1 == all threads
    };

    static std::list<pc_breakpoint_t> pc_breakpoints;
    static int                        debug_steps;

    static bool pc_breakpoints_exists(uint64_t pc, int thread);
    static bool pc_breakpoints_add(uint64_t pc, int thread);
    static void pc_breakpoints_dump(int thread);
    static void pc_breakpoints_clear_for_thread(int thread);
    static void pc_breakpoints_clear(void);

    static void memdump(uint64_t addr, uint64_t size);

    static bool process_dbg_cmd(std::string cmd);
    static bool get_pc_break(uint64_t &pc, int &thread);
    static void debug_init(void);
    static void debug_check(void);
#endif

    static uint64_t        emu_cycle;
    static std::list<int>  running_threads; // List of running threads
    static std::list<int>  wfi_wait_threads; // List of threads waiting in a WFI
    static std::list<int>  fcc_wait_threads[EMU_NUM_FCC_COUNTERS_PER_THREAD]; // List of threads waiting for an FCC
    static std::list<int>  port_wait_threads; // List of threads waiting for a port write
    static std::bitset<EMU_NUM_THREADS> active_threads; // List of threads being simulated
    static uint16_t        pending_fcc[EMU_NUM_THREADS][EMU_NUM_FCC_COUNTERS_PER_THREAD]; // Pending FastCreditCounter list
    static uint64_t        current_pc[EMU_NUM_THREADS]; // PC for each thread
    static int             global_log_min;
    static RVTimer         pu_rvtimer;
    static uint64_t        minions_en;
    static uint64_t        shires_en;
    static bool            coherency_check;
    static mem_directory   mem_dir;
    static bool            scp_check;
    static scp_directory   scp_dir;

    static std::unique_ptr<api_communicate> api_listener;
    static sys_emu_cmd_options cmd_options;
};


#endif// _SYS_EMU_H_
