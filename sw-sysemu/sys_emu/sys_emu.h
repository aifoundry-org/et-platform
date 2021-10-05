/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef _SYS_EMU_H_
#define _SYS_EMU_H_

#include <algorithm>
#include <bitset>
#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <fstream>

#include "api_communicate.h"
#include "system.h"
#include "checkers/flb_checker.h"
#include "checkers/l1_scp_checker.h"
#include "checkers/l2_scp_checker.h"
#include "checkers/mem_checker.h"


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
    struct file_load_info {
        uint64_t addr;
        std::string file;
    };

    struct dump_info {
        uint64_t addr;
        uint64_t size;
        std::string file;
    };

    struct set_xreg_info {
        uint64_t thread;
        uint64_t xreg;
        uint64_t value;
    };

    struct mem_write32 {
        uint64_t addr;
        uint32_t value;
    };

    std::vector<std::string> elf_files;
    std::vector<file_load_info> file_load_files;
    std::vector<mem_write32> mem_write32s;
    std::string mem_desc_file;
    std::string api_comm_path;
    uint64_t    minions_en                   = 1;
    uint64_t    shires_en                    = 1;
    bool        master_min                   = false;
    bool        second_thread                = true;
    bool        log_en                       = false;
    std::string log_path;
    std::bitset<EMU_NUM_THREADS> log_thread;
    std::vector<dump_info> dump_at_end;
    std::unordered_multimap<uint64_t, dump_info> dump_at_pc;
    std::string dump_mem;
    uint64_t    reset_pc                     = RESET_PC;
    uint64_t    sp_reset_pc                  = SP_RESET_PC;
    std::vector<set_xreg_info> set_xreg;
    bool        coherency_check              = false;
    uint64_t    max_cycles                   = 10000000;
    bool        mins_dis                     = false;
    bool        sp_dis                       = false;
    uint32_t    mem_reset                    = 0;
    std::string pu_uart0_rx_file;
    std::string pu_uart1_rx_file;
    std::string spio_uart0_rx_file;
    std::string spio_uart1_rx_file;
    std::string pu_uart0_tx_file;
    std::string pu_uart1_tx_file;
    std::string spio_uart0_tx_file;
    std::string spio_uart1_tx_file;
    uint64_t    log_at_pc                    = ~0ull;
    uint64_t    stop_log_at_pc               = ~0ull;
    bool        display_trap_info            = false;
    bool        gdb                          = false;
    bool        mem_check                    = false;
    uint64_t    mem_checker_log_addr         = 1;
    uint32_t    mem_checker_log_minion       = 2048;
    bool        l1_scp_check                 = false;
    uint32_t    l1_scp_checker_log_minion    = 2048;
    bool        l2_scp_check                 = false;
    uint32_t    l2_scp_checker_log_shire     = 64;
    uint32_t    l2_scp_checker_log_line      = 1 * 1024 * 1024;
    uint32_t    l2_scp_checker_log_minion    = 2048;
    bool        flb_check                    = false;
    uint32_t    flb_checker_log_shire        = 64;
#ifdef SYSEMU_PROFILING
    std::string dump_prof_file;
#endif
#ifdef SYSEMU_DEBUG
    bool        debug                        = false;
#endif
};

struct sys_emu_coop_tload
{
    bool     tenb;
    uint32_t id;
    uint32_t coop_id;
    uint32_t min_mask;
    uint32_t neigh_mask;
};


class api_communicate;

// Driver for a bemu::System instance
class sys_emu
{
public:
    sys_emu(const sys_emu_cmd_options& cmd_options, api_communicate* api_comm = nullptr);
    virtual ~sys_emu() = default;

    /// Function used for parsing the command line arguments
    static std::tuple<bool, struct sys_emu_cmd_options>
    parse_command_line_arguments(int argc, char* argv[]);
    static void get_command_line_help(std::ostream& stream);

    uint64_t thread_get_pc(unsigned thread_id) { return chip.cpu[thread_id].pc; }
    void thread_set_pc(unsigned thread_id, uint64_t pc) { chip.cpu[thread_id].pc = pc; }
    uint64_t thread_get_reg(int thread_id, int reg) { return chip.cpu[thread_id].xregs[reg]; }
    void thread_set_reg(int thread_id, int reg, uint64_t data) { chip.cpu[thread_id].xregs[reg] = data; }
    bemu::freg_t thread_get_freg(int thread_id, int reg) { return chip.cpu[thread_id].fregs[reg]; }
    void thread_set_freg(int thread_id, int reg, bemu::freg_t data) { chip.cpu[thread_id].fregs[reg] = data; }
    uint64_t thread_get_csr(int thread_id, int csr) { return chip.get_csr(thread_id, csr); }
    void thread_set_csr(int thread_id, int csr, uint32_t data) { chip.set_csr(thread_id, csr, data); }

    void fcc_to_threads(unsigned shire_id, unsigned thread_dest,
                               uint64_t thread_mask, unsigned cnt_dest);
    void msg_to_thread(unsigned thread_id);
    void send_ipi_redirect_to_threads(unsigned shire_id, uint64_t thread_mask);
    void raise_timer_interrupt(uint64_t shire_mask);
    void clear_timer_interrupt(uint64_t shire_mask);
    void raise_software_interrupt(unsigned shire_id, uint64_t thread_mask);
    void clear_software_interrupt(unsigned shire_id, uint64_t thread_mask);
    void raise_external_interrupt(unsigned shire_id);
    void clear_external_interrupt(unsigned shire_id);
    void raise_external_supervisor_interrupt(unsigned shire_id);
    void clear_external_supervisor_interrupt(unsigned shire_id);
    void evl_dv_handle_irq_inj(bool raise, uint64_t subopcode, uint64_t shire_mask);
    void shire_enable_threads(unsigned shire_id, uint32_t thread0_disable, uint32_t thread1_disable);
    void recalculate_thread_disable(unsigned shire_id);
    int main_internal();

    uint64_t get_emu_cycle()  { return emu_cycle; }

    bool thread_is_disabled(unsigned thread) { return !chip.cpu[thread].enabled; }

    void activate_thread(int thread_id) { active_threads[thread_id] = true; }
    void deactivate_thread(int thread_id) { active_threads[thread_id] = false; }
    bool thread_is_active(int thread_id) { return active_threads[thread_id]; }

    // gdbstub needs these
    void thread_read_memory(int thread, uint64_t addr, uint64_t size, uint8_t* buffer) {
        chip.memory.read(chip.cpu[thread], addr, size, buffer);
    }
    void thread_write_memory(int thread, uint64_t addr, uint64_t size, const uint8_t* buffer) {
        chip.memory.write(chip.cpu[thread], addr, size, buffer);
    }

    // PCIe DMA needs this
    bemu::MainMemory& get_memory() { return chip.memory; }

    // Returns whether a thread is running (not sleeping/waiting)
    bool thread_is_running(int thread_id) { return contains(running_threads, thread_id); }
    void thread_set_running(int thread_id) {
        if (thread_is_active(thread_id) &&
            /* && !thread_is_disabled(thread_id) && */
            !contains(running_threads, thread_id)) {
            running_threads.push_back(thread_id);
        }
    }
    int running_threads_count() { return running_threads.size(); }

    void thread_set_single_step(int thread_id) { single_step[thread_id] = true; }

    bool get_mem_check() { return mem_check; }
    mem_checker& get_mem_checker() { return mem_checker_; }
    bool get_l1_scp_check() { return l1_scp_check; }
    l1_scp_checker& get_l1_scp_checker() { return l1_scp_checker_; }
    bool get_l2_scp_check() { return l2_scp_check; }
    l2_scp_checker& get_l2_scp_checker() { return l2_scp_checker_; }
    bool get_flb_check() { return flb_check; }
    flb_checker& get_flb_checker() { return flb_checker_; }
    bool get_display_trap_info() { return cmd_options.display_trap_info; }

    void coop_tload_add(uint32_t thread_id, bool tenb, uint32_t id, uint32_t coop_id, uint32_t min_mask, uint32_t neigh_mask);
    bool coop_tload_check(uint32_t thread_id, bool tenb, uint32_t id, uint32_t & requested_mask, uint32_t & present_mask);
    bool coop_tload_all_present(uint32_t thread_id, const sys_emu_coop_tload & coop_tload, uint32_t & requested_mask, uint32_t & present_mask);
    void coop_tload_mark_done(uint32_t thread_id, const sys_emu_coop_tload & coop_tload);
    uint32_t coop_tload_get_thread_id(uint32_t thread_id, uint32_t neigh, uint32_t min);

    void breakpoint_insert(uint64_t addr);
    void breakpoint_remove(uint64_t addr);
    bool breakpoint_exists(uint64_t addr);

    bool parse_mem_file(const char* filename);

    api_communicate* get_api_communicate() { return api_listener; }

    testLog& get_logger() { return chip.log; }

protected:

    // Returns whether a container contains an element
    template<class _container, class _Ty>
    static inline bool contains(_container _C, const _Ty& _Val) {
        return std::find(_C.begin(), _C.end(), _Val) != _C.end();
    }

    // Checks if a sleeping thread (FCC, WFI, stall) has to wake up when receiving an interrupt
    void raise_interrupt_wakeup_check(unsigned thread_id);

private:

#ifdef SYSEMU_DEBUG
    struct pc_breakpoint_t {
        uint64_t pc;
        int      thread; // -1 == all threads
    };

    std::list<pc_breakpoint_t> pc_breakpoints;
    int                        debug_steps;

    bool pc_breakpoints_exists(uint64_t pc, int thread);
    bool pc_breakpoints_add(uint64_t pc, int thread);
    void pc_breakpoints_dump(int thread);
    void pc_breakpoints_clear_for_thread(int thread);
    void pc_breakpoints_clear(void);

    void memdump(uint64_t addr, uint64_t size);

    std::string dump_xregs(unsigned thread_id);
    std::string dump_fregs(unsigned thread_id);
    bool process_dbg_cmd(std::string cmd);
    bool get_pc_break(uint64_t &pc, int &thread);
    void debug_init(void);
    void debug_check(void);
#endif

    bemu::System    chip;

    std::ofstream   log_file;
    uint64_t        emu_cycle = 0;
    std::list<int>  running_threads; // List of running threads
    std::list<int>  wfi_stall_wait_threads; // List of threads waiting in a WFI or stall
    std::list<int>  fcc_wait_threads[EMU_NUM_FCC_COUNTERS_PER_THREAD]; // List of threads waiting for an FCC
    std::list<int>  port_wait_threads; // List of threads waiting for a port write
    std::bitset<EMU_NUM_THREADS> active_threads; // List of threads being simulated
    uint16_t        pending_fcc[EMU_NUM_THREADS][EMU_NUM_FCC_COUNTERS_PER_THREAD]; // Pending FastCreditCounter list
    std::list<sys_emu_coop_tload> coop_tload_pending_list[EMU_NUM_THREADS];                      // List of pending cooperative tloads per thread
    bool            mem_check = false;
    mem_checker     mem_checker_{&chip};
    bool            l1_scp_check = false;
    l1_scp_checker  l1_scp_checker_{&chip};
    bool            l2_scp_check = false;
    l2_scp_checker  l2_scp_checker_{&chip};
    bool            flb_check = false;
    flb_checker     flb_checker_{&chip};
    std::unordered_set<uint64_t> breakpoints;
    std::bitset<EMU_NUM_THREADS> single_step;

    bemu::Noagent   agent{&chip, "SYS-EMU"};

    api_communicate* api_listener = nullptr;
    sys_emu_cmd_options cmd_options;
};


#endif// _SYS_EMU_H_
