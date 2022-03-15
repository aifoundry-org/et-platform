/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include "sys_emu.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fcntl.h>
#include <iostream>
#include <list>
#include <locale>
#include <sys/stat.h>
#include <sys/types.h>
#include <tuple>
#include <unistd.h>

#include "api_communicate.h"
#include "checkers/l2_scp_checker.h"
#include "devices/rvtimer.h"
#include "emu_gio.h"
#include "esrs.h"
#include "gdbstub.h"
#include "insn.h"
#include "log.h"
#include "memory/dump_data.h"
#include "memory/main_memory.h"
#include "mmu.h"
#include "processor.h"
#include "profiling.h"
#include "sys_emu.h"
#ifdef HAVE_BACKTRACE
#include "crash_handler.h"
#endif


static void
halt_all_threads(bemu::System& chip)
{
    // FIXME: How do we halt waiting harts?
    for (auto& hart : chip.active) {
       hart.enter_debug_mode(bemu::Debug_entry::Cause::haltreq);
    }
    for (auto& hart : chip.awaking) {
       hart.enter_debug_mode(bemu::Debug_entry::Cause::haltreq);
    }
}


void
sys_emu::raise_timer_interrupt(uint64_t shire_mask)
{
    for (int shire = 0; shire < EMU_NUM_SHIRES; ++shire) {
        if ((shire_mask >> shire) & 1) {
            chip.raise_machine_timer_interrupt(shire);
        }
    }
}

void
sys_emu::clear_timer_interrupt(uint64_t shire_mask)
{
    for (int shire = 0; shire < EMU_NUM_SHIRES; ++shire) {
        if ((shire_mask >> shire) & 1) {
            chip.clear_machine_timer_interrupt(shire);
        }
    }
}

void
sys_emu::raise_software_interrupt(unsigned shire, uint64_t thread_mask)
{
    chip.raise_machine_software_interrupt(shire, thread_mask);
}

void
sys_emu::clear_software_interrupt(unsigned shire, uint64_t thread_mask)
{
    chip.clear_machine_software_interrupt(shire, thread_mask);
}

void
sys_emu::raise_external_interrupt(unsigned shire)
{
    chip.raise_machine_external_interrupt(shire);
}

void
sys_emu::clear_external_interrupt(unsigned shire)
{
    chip.clear_machine_external_interrupt(shire);
}

void
sys_emu::raise_external_supervisor_interrupt(unsigned shire)
{
    chip.raise_supervisor_external_interrupt(shire);
}

void
sys_emu::clear_external_supervisor_interrupt(unsigned shire)
{
    chip.clear_supervisor_external_interrupt(shire);
}

void
sys_emu::evl_dv_handle_irq_inj(bool raise, uint64_t subopcode, uint64_t shire_mask)
{
    switch (subopcode) {
    case ET_DIAG_IRQ_INJ_MEI:
        for (unsigned shire = 0; shire < EMU_NUM_SHIRES; ++shire) {
            if ((shire_mask >> shire) & 1) {
                if (raise) {
                    raise_external_interrupt(shire);
                } else {
                    clear_external_interrupt(shire);
                }
            }
        }
        break;
    case ET_DIAG_IRQ_INJ_SEI:
        for (unsigned shire = 0; shire < EMU_NUM_SHIRES; ++shire) {
            if ((shire_mask >> shire) & 1) {
                if (raise) {
                    raise_external_supervisor_interrupt(shire);
                } else {
                    clear_external_supervisor_interrupt(shire);
                }
            }
        }
        break;
    case ET_DIAG_IRQ_INJ_TI:
        if (raise) {
            raise_timer_interrupt(shire_mask);
        } else {
            clear_timer_interrupt(shire_mask);
        }
        break;
    }
}

void
sys_emu::breakpoint_insert(uint64_t addr)
{
    LOG_AGENT(DEBUG, agent, "Inserting breakpoint at address 0x%" PRIx64 "", addr);
    breakpoints.insert(addr);
}

void
sys_emu::breakpoint_remove(uint64_t addr)
{
    LOG_AGENT(DEBUG, agent, "Removing breakpoint at address 0x%" PRIx64 "", addr);
    breakpoints.erase(addr);
}

bool
sys_emu::breakpoint_exists(uint64_t addr)
{
    return contains(breakpoints, addr);
}


void
sys_emu::disconnect_gdbstub()
{
    breakpoints.clear();
    single_step.reset();
    for (auto& hart : chip.cpu) {
        if (hart.is_halted()) {
            hart.start_running();
        }
    }
}


sys_emu::sys_emu(const sys_emu_cmd_options &cmd_options, api_communicate *api_comm)
{
    this->cmd_options = cmd_options;
    this->api_listener = api_comm;

    chip.set_emu(this);

    // Setup logging
    chip.log.setDevice(this);
    chip.log_trigger_insn = cmd_options.log_trigger_insn;
    chip.log_trigger_hart = cmd_options.log_trigger_hart;
    chip.log_trigger_start = cmd_options.log_trigger_start;
    chip.log_trigger_stop = cmd_options.log_trigger_stop;
    chip.log_trigger_count = 0;
    chip.log_dynamic = (chip.log_trigger_insn != 0) && (chip.log_trigger_stop > chip.log_trigger_start) && (chip.log_trigger_hart < EMU_NUM_THREADS);
    chip.log.setLogLevel(cmd_options.log_en && (!chip.log_dynamic || (chip.log_trigger_start == 0)) ? LOG_DEBUG : LOG_INFO);
    chip.log_thread = cmd_options.log_thread;

    if (!cmd_options.log_path.empty()) {
        log_file.open(cmd_options.log_path);
        if (!log_file.is_open()) {
            LOG_AGENT(FTL, agent, "Unable to open log file: %s", cmd_options.log_path.c_str());
        }
        chip.log.setOutputStream(&log_file);
    }

    // Reset the SoC
    emu_cycle = 0;
    mem_check = cmd_options.mem_check;
    mem_checker_ = mem_checker{&chip};
    mem_checker_.log_addr = cmd_options.mem_checker_log_addr;
    mem_checker_.log_minion = cmd_options.mem_checker_log_minion;
    l1_scp_check = cmd_options.l1_scp_check;
    l1_scp_checker_ = l1_scp_checker{&chip};
    l1_scp_checker_.log_minion = cmd_options.l1_scp_checker_log_minion;
    l2_scp_check = cmd_options.l2_scp_check;
    new (&l2_scp_checker_) l2_scp_checker{&chip};
    l2_scp_checker_.log_shire = cmd_options.l2_scp_checker_log_shire;
    l2_scp_checker_.log_line = cmd_options.l2_scp_checker_log_line;
    l2_scp_checker_.log_minion = cmd_options.l2_scp_checker_log_minion;
    flb_check = cmd_options.flb_check;
    flb_checker_ = flb_checker{&chip};
    flb_checker_.log_shire = cmd_options.flb_checker_log_shire;
    tstore_check = cmd_options.tstore_check;
    tstore_checker_ = tstore_checker{&chip};
    tstore_checker_.log_addr = cmd_options.tstore_checker_log_addr;
    tstore_checker_.log_thread = cmd_options.tstore_checker_log_thread;
    breakpoints.clear();
    single_step.reset();

    if (cmd_options.elf_files.empty() && cmd_options.file_load_files.empty() &&
        cmd_options.mem_desc_file.empty() && cmd_options.api_comm_path.empty()) {
        LOG_AGENT(FTL, agent, "%s", "Need an ELF file, a file load, a mem_desc file or runtime API!");
    }

    // Init emu
    chip.init(bemu::System::Stepping::A0);
    memcpy(&chip.memory_reset_value, &cmd_options.mem_reset, MEM_RESET_PATTERN_SIZE);

    // Parses the ELF files and memory description
    for (const auto &elf: cmd_options.elf_files) {
        LOG_AGENT(INFO, agent, "Loading ELF: \"%s\"", elf.c_str());
        try {
            chip.load_elf(elf.c_str());
        }
        catch (...) {
            LOG_AGENT(FTL, agent, "Error loading ELF \"%s\"", elf.c_str());
        }
    }
    if (!cmd_options.mem_desc_file.empty()) {
        parse_mem_file(cmd_options.mem_desc_file.c_str());
    }

    // Load files
    for (const auto &info: cmd_options.file_load_files) {
        LOG_AGENT(INFO, agent, "Loading file @ 0x%" PRIx64 ": \"%s\"", info.addr, info.file.c_str());
        try {
            chip.load_raw(info.file.c_str(), info.addr);
        }
        catch (...) {
            LOG_AGENT(FTL, agent, "Error loading file \"%s\"", info.file.c_str());
        }
    }

    // Perform 32 bit writes
    for (const auto &info: cmd_options.mem_write32s) {
        LOG_AGENT(INFO, agent, "Writing 32-bit value 0x%" PRIx32 " to 0x%" PRIx64, info.value, info.addr);
        chip.memory.write(agent, info.addr, sizeof(info.value),
                          reinterpret_cast<bemu::MainMemory::const_pointer>(&info.value));
    }

    // Setup PU UART0 RX stream
    if (!cmd_options.pu_uart0_rx_file.empty()) {
        int fd = open(cmd_options.pu_uart0_rx_file.c_str(), O_RDONLY, 0666);
        if (fd < 0) {
            LOG_AGENT(FTL, agent, "Error opening \"%s\"", cmd_options.pu_uart0_rx_file.c_str());
        }
        chip.pu_uart0_set_rx_fd(fd);
    } else {
        chip.pu_uart0_set_rx_fd(STDIN_FILENO);
    }

    // Setup PU UART1 RX stream
    if (!cmd_options.pu_uart1_rx_file.empty()) {
        int fd = open(cmd_options.pu_uart1_rx_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            LOG_AGENT(FTL, agent, "Error opening \"%s\"", cmd_options.pu_uart1_rx_file.c_str());
        }
        chip.pu_uart1_set_rx_fd(fd);
    } else {
        chip.pu_uart1_set_rx_fd(STDIN_FILENO);
    }

    // Setup SPIO UART0 RX stream
    if (!cmd_options.spio_uart0_rx_file.empty()) {
        int fd = open(cmd_options.spio_uart0_rx_file.c_str(), O_RDONLY | O_NONBLOCK | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            LOG_AGENT(FTL, agent, "Error opening \"%s\"", cmd_options.spio_uart0_rx_file.c_str());
        }
        chip.spio_uart0_set_rx_fd(fd);
    } else {
        chip.spio_uart0_set_rx_fd(STDIN_FILENO);
    }

    // Setup SPIO UART1 RX stream
    if (!cmd_options.spio_uart1_rx_file.empty()) {
        int fd = open(cmd_options.spio_uart1_rx_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            LOG_AGENT(FTL, agent, "Error opening \"%s\"", cmd_options.spio_uart1_rx_file.c_str());
        }
        chip.spio_uart1_set_rx_fd(fd);
    } else {
        chip.spio_uart1_set_rx_fd(STDIN_FILENO);
    }

    // Setup PU UART0 TX stream
    if (!cmd_options.pu_uart0_tx_file.empty()) {
        int fd = open(cmd_options.pu_uart0_tx_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            LOG_AGENT(FTL, agent, "Error creating \"%s\"", cmd_options.pu_uart0_tx_file.c_str());
        }
        chip.pu_uart0_set_tx_fd(fd);
    } else {
        chip.pu_uart0_set_tx_fd(STDOUT_FILENO);
    }

    // Setup PU UART1 TX stream
    if (!cmd_options.pu_uart1_tx_file.empty()) {
        int fd = open(cmd_options.pu_uart1_tx_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            LOG_AGENT(FTL, agent, "Error creating \"%s\"", cmd_options.pu_uart1_tx_file.c_str());
        }
        chip.pu_uart1_set_tx_fd(fd);
    } else {
        chip.pu_uart1_set_tx_fd(STDOUT_FILENO);
    }

    // Setup SPIO UART0 TX stream
    if (!cmd_options.spio_uart0_tx_file.empty()) {
        int fd = open(cmd_options.spio_uart0_tx_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            LOG_AGENT(FTL, agent, "Error creating \"%s\"", cmd_options.spio_uart0_tx_file.c_str());
        }
        chip.spio_uart0_set_tx_fd(fd);
    } else {
        chip.spio_uart0_set_tx_fd(STDOUT_FILENO);
    }

    // Setup SPIO UART1 TX stream
    if (!cmd_options.spio_uart1_tx_file.empty()) {
        int fd = open(cmd_options.spio_uart1_tx_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            LOG_AGENT(FTL, agent, "Error creating \"%s\"", cmd_options.spio_uart1_tx_file.c_str());
        }
        chip.spio_uart1_set_tx_fd(fd);
    } else {
        chip.spio_uart1_set_tx_fd(STDOUT_FILENO);
    }

    // Initialize Simulator API
    if (api_listener) {
        api_listener->set_system(&chip);
    }

    // Reset the cold-reset part of the system
    for (unsigned shire = 0; shire < EMU_NUM_SHIRES; ++shire) {
        chip.cold_reset(shire);
    }
    chip.cold_reset_mindm();
    chip.cold_reset_spdm();
    chip.cold_reset_memshire();

    // Configure the simulation parameters
    for (unsigned shire = 0; shire < EMU_NUM_MINION_SHIRES; ++shire) {
        if (((cmd_options.shires_en >> shire) & 1) == 0) {
            chip.config_simulated_harts(shire, 0, false, false);
            continue;
        }
        for (unsigned n = 0; n < EMU_NEIGH_PER_SHIRE; ++n) {
            unsigned neigh = n + shire * EMU_NEIGH_PER_SHIRE;
            chip.config_reset_pc(neigh, cmd_options.reset_pc);
        }
        chip.config_simulated_harts(shire, cmd_options.minions_en,
                                    cmd_options.second_thread,
                                    !cmd_options.mins_dis);
    }
    if (((cmd_options.shires_en >> EMU_IO_SHIRE_SP) & 1) == 0)  {
        chip.config_simulated_harts(EMU_IO_SHIRE_SP, 0, false, false);
    } else {
        chip.config_reset_pc(EMU_IO_SHIRE_SP_NEIGH, cmd_options.sp_reset_pc);
        chip.config_simulated_harts(EMU_IO_SHIRE_SP, cmd_options.minions_en,
                                    false, !cmd_options.sp_dis);
    }

    // Reset the warm-reset part of the system
    for (unsigned shire = 0; shire < EMU_NUM_SHIRES; ++shire) {
        chip.begin_warm_reset(shire);
        chip.end_warm_reset(shire);
    }

    // Initialize xregs passed to command line
    for (auto &info: cmd_options.set_xreg) {
        chip.cpu[info.thread].xregs[info.xreg] = info.value;
    }
}


////////////////////////////////////////////////////////////////////////////////
// Main function implementation
////////////////////////////////////////////////////////////////////////////////

int sys_emu::main_internal() {
#ifdef HAVE_BACKTRACE
    Crash_handler __crash_handler;
#endif

#ifdef SYSEMU_PROFILING
    profiling_init(this);
#endif

    int rv = EXIT_SUCCESS;

    bool gdb_enabled = cmd_options.gdb && (cmd_options.gdb_at_pc == ~0ull);
    if (cmd_options.gdb) {
        gdbstub_init(this, &chip);
    }

    LOG_AGENT(INFO, agent, "%s", "Starting emulation");

    double total_time = 0.0;
    const auto start_time = std::chrono::high_resolution_clock::now();

    // While there are active threads or the network emulator is still not done
    while (!chip.get_emu_done()
           && (emu_cycle < cmd_options.max_cycles)
           && (chip.has_active_harts()
               || (chip.has_sleeping_harts()
                   && ((api_listener != nullptr)
                       || chip.pu_rvtimer_is_active()
                       || chip.spio_rvtimer_is_active()))))
    {
        if (gdb_enabled) {
            switch (gdbstub_get_status()) {
            case GDBSTUB_STATUS_WAITING_CLIENT:
                gdbstub_accept_client();
                halt_all_threads(chip);
                // If we connect the debugger cycle counting goes out the
                // door; avoid finishing the simulation too early
                cmd_options.max_cycles = ~0ull;
                break;
            case GDBSTUB_STATUS_RUNNING:
                /* Non-blocking, consumes all the pending GDB commands */
                gdbstub_io();
                break;
            default:
                break;
            }
        }

        // Dynamic logging
        if (chip.log_dynamic) {
            auto& hart = chip.cpu[chip.log_trigger_hart];
            if ((hart.state == bemu::Hart::State::active) && (hart.inst.bits == chip.log_trigger_insn)) {
                chip.log_trigger_count++;
                if (chip.log_trigger_count == chip.log_trigger_start) {
                    chip.log.setLogLevel(LOG_DEBUG);
                }
                if (chip.log_trigger_count == chip.log_trigger_stop) {
                    chip.log.setLogLevel(LOG_INFO);
                }
            }
        }

        // Runtime API: Process new commands
        if (api_listener) {
            api_listener->process();
        }

        // Update peripherals/devices
        chip.tick_peripherals(emu_cycle);

        chip.active.splice(chip.active.cend(), chip.awaking);

        auto current_hart = chip.active.begin();
        while (current_hart != chip.active.end()) {
            auto hart = current_hart++;
            auto thread_id = hart_index(*hart);

            // This should happen even if the hart is sleeping or blocked
            hart->async_execute();

            if (cmd_options.gdb && !gdb_enabled && (hart->pc == cmd_options.gdb_at_pc)) {
                // Break and connect the debugger in the next iteration!
                gdb_enabled = true;
                break;
            }

            // Fetch and interrupts are blocked because another hart of this core is in exclusive mode
            if (hart->is_blocked()) {
                continue;
            }

            // If the hart is halted either do nothing or fetch and execute from the program buffer
            if (hart->is_halted()) {
                if (!hart->in_progbuf()) {
                    continue;
                }
                using Progbuf = bemu::Hart::Progbuf;
                try {
                    hart->fetch_progbuf();
                    hart->execute();
                    hart->advance_progbuf();
                }
                catch (const bemu::Trap& t) {
                    LOG_AGENT(WARN, *hart, "Program buffer trapped: %s", t.what());
                    hart->exit_progbuf(Progbuf::exception);
                }
                catch (const bemu::instruction_restart) {
                    LOG_AGENT(DEBUG, *hart, "%s", "Instruction killed and will be restarted");
                }
                catch (const bemu::memory_error& e) {
                    LOG_AGENT(WARN, *hart, "Program buffer bus error: 0x%" PRIx64, e.addr);
                    hart->exit_progbuf(Progbuf::exception);
                }
                catch (const std::exception& e) {
                    LOG_AGENT(FTL, *hart, "%s", e.what());
                }
                continue;
            }

            try {
                hart->check_pending_interrupts();
                if (!hart->is_waiting()) {
                    // Gets instruction and sets state
                    hart->fetch();

                    // Check for breakpoints
                    if ((gdbstub_get_status() == GDBSTUB_STATUS_RUNNING) && breakpoint_exists(hart->pc)) {
                        LOG_AGENT(DEBUG, *hart, "Hit breakpoint at address 0x%" PRIx64, hart->pc);
                        gdbstub_signal_break(thread_id);
                        halt_all_threads(chip);
                        continue;
                    }

                    // Dumping when M0:T0 reaches a PC
                    auto range = cmd_options.dump_at_pc.equal_range(thread_get_pc(0));
                    for (auto it = range.first; it != range.second; ++it) {
                        bemu::dump_data(chip.memory, agent,
                                        it->second.file.c_str(), it->second.addr, it->second.size);
                    }

                    // Logging
                    if (thread_get_pc(0) == cmd_options.log_at_pc) {
                        get_logger().setLogLevel(LOG_DEBUG);
                    } else if (thread_get_pc(0) == cmd_options.stop_log_at_pc) {
                        get_logger().setLogLevel(LOG_INFO);
                    }

                    // Executes the instruction
                    hart->execute();
                    hart->notify_pmu_minion_event(PMU_MINION_EVENT_RETIRED_INST0 + (thread_id & 1));
                    hart->advance_pc();
                }
            }
            catch (const bemu::Debug_entry& e) {
                hart->enter_debug_mode(e.cause);
            }
            catch (const bemu::Trap& t) {
                uint64_t old_pc = hart->pc;
                hart->take_trap(t);
                hart->advance_pc();
                if (hart->pc == old_pc) {
                    LOG_AGENT(FTL, *hart, "Trapping to the same address that "
                              "caused a trap (0x%" PRIx64 "). Avoiding "
                              "infinite trap recursion.", hart->pc);
                }
            }
            catch (const bemu::instruction_restart) {
                LOG_AGENT(DEBUG, *hart, "%s", "Instruction killed and will be restarted");
            }
            catch (const bemu::memory_error& e) {
                hart->advance_pc();
                hart->raise_interrupt(BUS_ERROR_INTERRUPT, e.addr);
            }
            catch (const std::exception& e) {
                LOG_AGENT(FTL, *hart, "%s", e.what());
            }

            // Check for single-step mode
            if ((gdbstub_get_status() == GDBSTUB_STATUS_RUNNING) && single_step[thread_id]) {
                LOG_AGENT(DEBUG, *hart, "%s", "Single-step done");
                gdbstub_signal_break(thread_id);
                single_step[thread_id] = false;
                hart->enter_debug_mode(bemu::Debug_entry::Cause::haltreq);
                continue;
            }
        }

        ++emu_cycle;
    }

    const auto elapsed = std::chrono::high_resolution_clock::now() - start_time;
    total_time +=
        std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();

    LOG_AGENT(INFO, agent, "Emulation performance: %lf cycles/sec (%"
              PRIu64 " cycles / %lf sec)",
              1e3 * double(emu_cycle) / total_time,
              emu_cycle,
              total_time * 1e-9);

    // Awaking harts are active harts at this point...
    chip.active.splice(chip.active.cend(), chip.awaking);

    // Convert active/sleeping lists to running/waiting lists. Also, in some
    // cases, a hart will issue a 'wfi' to "go to sleep" instead of signaling
    // an end-of-execution to the simulator. So here we consider all harts
    // that are only waiting for an interrupt to be nonexistent.
    if (chip.has_sleeping_harts()) {
        auto current_hart = chip.sleeping.begin();
        while (current_hart != chip.sleeping.end()) {
            auto hart = current_hart++;
            if (hart->waits == bemu::Hart::Waiting::interrupt) {
                hart->become_nonexistent();
            } else if (!hart->is_waiting()) {
                chip.active.push_back(*hart);
            }
        }
    }

    if ((emu_cycle == cmd_options.max_cycles) && chip.has_active_harts()) {
        LOG_AGENT(INFO, agent, "%s", "Running harts:");
        for (const auto& hart : chip.active) {
            LOG_AGENT(INFO, agent, "\tThread %u, PC: 0x%" PRIx64,
                      hart.mhartid, hart.pc);
        }
    }

    if (!chip.get_emu_done() && chip.has_sleeping_harts()) {
        LOG_AGENT(INFO, agent, "%s", "Waiting harts:");
        for (const auto& hart : chip.sleeping) {
            char waitreasons[1024];
            int pos = 0;
            waitreasons[0] = '\0';
            if (hart.is_waiting(bemu::Hart::Waiting::tload_0)) {
                pos += snprintf(&waitreasons[pos], 1023-pos, " tload_0");
            }
            if (hart.is_waiting(bemu::Hart::Waiting::tload_1)) {
                pos += snprintf(&waitreasons[pos], 1023-pos, " tload_1");
            }
            if (hart.is_waiting(bemu::Hart::Waiting::tload_L2_0)) {
                pos += snprintf(&waitreasons[pos], 1023-pos, " tload_L2_0");
            }
            if (hart.is_waiting(bemu::Hart::Waiting::tload_L2_1)) {
                pos += snprintf(&waitreasons[pos], 1023-pos, " tload_L2_1");
            }
            if (hart.is_waiting(bemu::Hart::Waiting::prefetch_0)) {
                pos += snprintf(&waitreasons[pos], 1023-pos, " prefetch_0");
            }
            if (hart.is_waiting(bemu::Hart::Waiting::prefetch_1)) {
                pos += snprintf(&waitreasons[pos], 1023-pos, " prefetch_1");
            }
            if (hart.is_waiting(bemu::Hart::Waiting::cacheop)) {
                pos += snprintf(&waitreasons[pos], 1023-pos, " cacheop");
            }
            if (hart.is_waiting(bemu::Hart::Waiting::tfma)) {
                pos += snprintf(&waitreasons[pos], 1023-pos, " tfma");
            }
            if (hart.is_waiting(bemu::Hart::Waiting::tstore)) {
                pos += snprintf(&waitreasons[pos], 1023-pos, " tstore");
            }
            if (hart.is_waiting(bemu::Hart::Waiting::reduce)) {
                pos += snprintf(&waitreasons[pos], 1023-pos, " reduce");
            }
            if (hart.is_waiting(bemu::Hart::Waiting::tquant)) {
                pos += snprintf(&waitreasons[pos], 1023-pos, " tquant");
            }
            if (hart.is_waiting(bemu::Hart::Waiting::interrupt)) {
                pos += snprintf(&waitreasons[pos], 1023-pos, " interrupt");
            }
            if (hart.is_waiting(bemu::Hart::Waiting::message)) {
                pos += snprintf(&waitreasons[pos], 1023-pos, " messsage");
            }
            if (hart.is_waiting(bemu::Hart::Waiting::credit0)) {
                pos += snprintf(&waitreasons[pos], 1023-pos, " fcc0");
            }
            if (hart.is_waiting(bemu::Hart::Waiting::credit1)) {
                pos += snprintf(&waitreasons[pos], 1023-pos, " fcc1");
            }
            if (hart.is_waiting(bemu::Hart::Waiting::tload_tenb)) {
                pos += snprintf(&waitreasons[pos], 1023-pos, " tload_tenb");
            }
            waitreasons[pos] = waitreasons[1023] = '\0';
            LOG_AGENT(INFO, agent, "\tThread H%u, PC: 0x%" PRIx64 " waits for%s",
                      hart.mhartid, hart.pc, pos ? waitreasons : " nothing");
        }
    }

    if (emu_cycle == cmd_options.max_cycles) {
        LOG_AGENT(ERR, agent, "Error, max cycles reached (%" SCNd64 ")",
                  cmd_options.max_cycles);
        rv = EXIT_FAILURE;
    } else if (!chip.get_emu_done() && chip.has_sleeping_harts()) {
        LOG_AGENT(ERR, agent, "%s", "Error, sleeping harts found");
        rv = EXIT_FAILURE;
    } else if (chip.get_emu_fail()) {
        LOG_AGENT(ERR, agent, "%s", "Error, test failure");
        rv = EXIT_FAILURE;
    }

    // Checks that the tensor store are drained
    if(tstore_check)
        tstore_checker_.is_empty();

    LOG_AGENT(INFO, agent, "%s", "Finishing emulation");

    if (cmd_options.gdb)
        gdbstub_fini();

    // Dumping
    for (const auto& dump: cmd_options.dump_at_end) {
        bemu::dump_data(chip.memory, agent,
                        dump.file.c_str(), dump.addr, dump.size);
    }

    if (!cmd_options.dump_mem.empty()) {
        bemu::dump_data(chip.memory, agent,
                        cmd_options.dump_mem.c_str(), chip.memory.first(),
                        (chip.memory.last() - chip.memory.first()) + 1);
    }
    if (!cmd_options.pu_uart0_rx_file.empty()) {
        close(chip.pu_uart0_get_rx_fd());
    }
    if (!cmd_options.pu_uart1_rx_file.empty()) {
        close(chip.pu_uart1_get_rx_fd());
    }
    if (!cmd_options.spio_uart0_rx_file.empty()) {
        close(chip.spio_uart0_get_rx_fd());
    }
    if (!cmd_options.spio_uart1_rx_file.empty()) {
        close(chip.spio_uart1_get_rx_fd());
    }
    if (!cmd_options.pu_uart0_tx_file.empty()) {
        close(chip.pu_uart0_get_tx_fd());
    }
    if (!cmd_options.pu_uart1_tx_file.empty()) {
        close(chip.pu_uart1_get_tx_fd());
    }
    if (!cmd_options.spio_uart0_tx_file.empty()) {
        close(chip.spio_uart0_get_tx_fd());
    }
    if (!cmd_options.spio_uart1_tx_file.empty()) {
        close(chip.spio_uart1_get_tx_fd());
    }
#ifdef SYSEMU_PROFILING
    if (!cmd_options.dump_prof_file.empty()) {
        profiling_flush();
        profiling_dump(cmd_options.dump_prof_file.c_str());
    }
    profiling_fini();
#endif

    return rv;
}
