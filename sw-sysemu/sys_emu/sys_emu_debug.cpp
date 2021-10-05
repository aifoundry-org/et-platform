/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include <cinttypes>
#include <iostream>
#include "mmu.h"
#include "sys_emu.h"

bool sys_emu::pc_breakpoints_exists(uint64_t pc, int thread)
{
    return pc_breakpoints.end() !=
        std::find_if(pc_breakpoints.begin(), pc_breakpoints.end(),
            [&](const pc_breakpoint_t &b) {
                return (b.pc == pc) && ((b.thread == -1) || b.thread == thread);
            }
        );
}

bool sys_emu::pc_breakpoints_add(uint64_t pc, int thread)
{
    if (pc_breakpoints_exists(pc, thread))
        return false;

    // If the breakpoint we are adding is global, remove all the local
    // breakpoints with the same pc
    if (thread == -1) {
        pc_breakpoints.remove_if(
            [&](const pc_breakpoint_t &b) {
                return b.pc == pc;
            }
        );
    }

    pc_breakpoints.push_back({pc, thread});
    return true;
}

void sys_emu::pc_breakpoints_dump(int thread)
{
    for (auto &it: pc_breakpoints) {
        if (it.thread == -1) // Global breakpoint
            printf("Breakpoint set for all threads at PC 0x%" PRIx64 "\n", it.pc);
        else if ((thread == -1) || (thread == it.thread))
            printf("Breakpoint set for thread %d at PC 0x%" PRIx64 "\n", it.thread, it.pc);
    }
}

void sys_emu::pc_breakpoints_clear_for_thread(int thread)
{
    pc_breakpoints.remove_if(
        [&](const pc_breakpoint_t &b) {
            return b.thread == thread;
        }
    );
}

void sys_emu::pc_breakpoints_clear(void)
{
    pc_breakpoints.clear();
}

static const char * help_dbg =
"\
help|h:                Print this message\n\
run|r:                 Execute until the end or a breakpoint is reached\n\
step|s [n]:            Execute n cycles (or 1 if not specified)\n\
pc [N]:                Dump PC of thread N (0 <= N < 2048)\n\
xdump|x [N]:           Dump GPRs of thread N (0 <= N < 2048)\n\
fdump|f [N]:           Dump FPRs of thread N (0 <= N < 2048)\n\
csr [N] <off>:         Dump the CSR at offset \"off\" of thread N (0 <= N < 2048)\n\
mdump|m <addr> <size>: Dump size bytes of memory at addr\n\
break|b [N] <PC>:      Set a breakpoint for the provided PC and thread N\n\
list_breaks [N]:       List the currently active breakpoints for a given thread N, or all if N == 0.\n\
clear_breaks [N]:      Clear all the breakpoints previously set if no N, or for thread N\n\
quit|q:                Terminate the program\n\
";

static size_t split(const std::string &txt, std::vector<std::string> &strs, char ch = ' ')
{
   size_t pos = txt.find( ch );
   size_t initialPos = 0;
   strs.clear();

   // Decompose statement
   while( pos != std::string::npos ) {
      strs.push_back( txt.substr( initialPos, pos - initialPos ) );
      initialPos = pos + 1;

      pos = txt.find( ch, initialPos );
   }

   // Add the last one
   strs.push_back( txt.substr( initialPos, std::min( pos, txt.size() ) - initialPos + 1 ) );

   return strs.size();
}

std::string sys_emu::dump_xregs(unsigned thread_id)
{
    std::stringstream str;
    if (thread_id < EMU_NUM_THREADS) {
        for (size_t ii = 0; ii < bemu::NXREGS; ++ii) {
            str << "XREG[" << std::dec << ii << "] = 0x" << std::hex << thread_get_reg(thread_id, ii) << "\n";
        }
    }
    return str.str();
}

std::string sys_emu::dump_fregs(unsigned thread_id)
{
    std::stringstream str;
    if (thread_id < EMU_NUM_THREADS) {
        for (size_t ii = 0; ii < bemu::NFREGS; ++ii) {
            const auto value = thread_get_freg(thread_id, ii);
            for (size_t jj = 0; jj < bemu::VLEN/32; ++jj) {
                str << "FREG[" << std::dec << ii << "][" << jj <<  "] = 0x" << std::hex << value.u32[jj] << "\t";
            }
            str << "\n";
        }
    }
    return str.str();
}

void sys_emu::memdump(uint64_t addr, uint64_t size)
{
    char ascii[17] = {0};
    for (uint64_t i = 0; i < size; i++) {
        uint8_t data;
        chip.memory.read(agent, addr + i, 1, &data);
        printf("%02X ", data);
        ascii[i % 16] = std::isprint(data) ? data : '.';
        if ((i + 1) % 8 == 0 || (i + 1) == size) {
            printf(" ");
            if ((i + 1) % 16 == 0) {
                printf("|  %s \n", ascii);
            } else if (i+1 == size) {
                ascii[(i+1) % 16] = '\0';
                if ((i + 1) % 16 <= 8)
                    printf(" ");
                for (uint64_t j = (i+1) % 16; j < 16; j++)
                    printf("   ");
                printf("|  %s \n", ascii);
            }
        }
    }
}

bool sys_emu::process_dbg_cmd(std::string cmd) {
   bool prompt = true;
   std::vector<std::string> command;
   size_t num_args = split(cmd, command);
   debug_steps = -1;
   // Miscellaneous
   if ((cmd == "h") || (cmd == "help")) {
      printf("%s", help_dbg);
   } else if ((cmd == "q") || (cmd == "quit")) {
      exit(0);
   // Simulation control
   } else if ((command[0] == "r") || (command[0] == "run")) {
      prompt = false;
   } else if ((command[0] == "") || (command[0] == "s") || (command[0] == "step")) {
      debug_steps = (num_args > 1) ? std::stoi(command[1]) : 1;
      prompt = false;
   // Breakpoints
   } else if ((command[0] == "b") || (command[0] == "break")) {
      uint64_t pc_break = thread_get_pc(0);
      int thread = -1;
      if (num_args == 2) {
        pc_break = std::stoull(command[1], nullptr, 0);
      } else if (num_args > 2) {
        thread = std::stoi(command[1]);
        pc_break = std::stoull(command[2], nullptr, 0);
      }
      if (pc_breakpoints_add(pc_break, thread)) {
        if (thread == -1)
          printf("Set breakpoint for all threads at PC 0x%" PRIx64 "\n", pc_break);
        else
          printf("Set breakpoint for thread %d at PC 0x%" PRIx64 "\n", thread, pc_break);
     }
   } else if ((command[0] == "list_breaks")) {
      int thread = -1;
      if (num_args > 1)
        thread = std::stoi(command[1]);
      pc_breakpoints_dump(thread);
   } else if ((command[0] == "clear_breaks")) {
      if (num_args > 1)
        pc_breakpoints_clear_for_thread(std::stoi(command[1]));
      else
        pc_breakpoints_clear();
   // Architectural State Dumping
   } else if (command[0] == "pc") {
      uint32_t thid = (num_args > 1) ? std::stoi(command[1]) : 0;
      printf("PC[%d] = 0x%" PRIx64 "\n", thid, thread_get_pc(thid));
   } else if ((command[0] == "x") || (command[0] == "xdump")) {
      std::string str = dump_xregs((num_args > 1) ? std::stoi(command[1]) : 0);
      printf("%s\n", str.c_str());
   } else if ((command[0] == "f") || command[0] == "fdump") {
      std::string str = dump_fregs((num_args > 1) ? std::stoi(command[1]) : 0);
      printf("%s\n", str.c_str());
   } else if (command[0] == "csr") {
      uint32_t thid = 0;
      uint16_t offset = 0;
      if (num_args > 2) {
        thid = std::stoi(command[1]);
        offset = std::stoul(command[2], nullptr, 0);
      } else if (num_args > 1) {
        offset = std::stoul(command[1], nullptr, 0);
      }
      try {
        printf("CSR[%d][0x%x] = 0x%" PRIx64 "\n", thid, offset & 0xfff, thread_get_csr(thid, offset & 0xfff));
      }
      catch (const bemu::trap_t&) {
        printf("Unrecognized CSR register\n");
      }
   } else if ((command[0] == "m") || (command[0] == "mdump")) {
      if (num_args > 2) {
          uint64_t addr = std::stoull(command[1], nullptr, 0);
          uint64_t size = std::stoull(command[2], nullptr, 0);
          memdump(addr, size);
      }
   } else {
      printf("Unknown command\n\n");
      printf("%s", help_dbg);
   }
   return prompt;
}

bool sys_emu::get_pc_break(uint64_t &pc, int &thread) {
   for (int s = 0; s < EMU_NUM_SHIRES; s++)
   {
      if (((cmd_options.shires_en >> s) & 1) == 0) continue;

      unsigned shire_minion_count = (s == EMU_IO_SHIRE_SP ? 1 : EMU_MINIONS_PER_SHIRE);
      unsigned minion_thread_count = (s == EMU_IO_SHIRE_SP ? 1 : EMU_THREADS_PER_MINION);

      for (unsigned m = 0; m < shire_minion_count; m++)
      {
         if (((cmd_options.minions_en >> m) & 1) == 0) continue;
         for (unsigned ii = 0; ii < minion_thread_count; ii++) {
            unsigned thread_id = s * EMU_THREADS_PER_SHIRE + m * EMU_THREADS_PER_MINION + ii;
            uint64_t thread_pc = thread_get_pc(thread_id);
            if (pc_breakpoints_exists(thread_pc, thread_id)) {
               pc = thread_pc;
               thread = thread_id;
               return true;
            }
         }
      }
   }
   return false;
}

void sys_emu::debug_init(void) {
    debug_steps = 1;
}

void sys_emu::debug_check(void) {
    // Check if any thread has reached a breakpoint
    int break_thread;
    uint64_t break_pc;
    bool break_reached = get_pc_break(break_pc, break_thread);
    if (break_reached)
        printf("Thread %d reached breakpoint at PC 0x%" PRIx64 "\n", break_thread, break_pc);

    if ((break_reached == true) || (debug_steps == 0)) {
       bool retry = false;
       bool prompt = true;
       std::string line;
       do {
          printf("\n$ ");
          std::getline(std::cin, line);
          try {
             prompt = process_dbg_cmd(line);
             retry = false;
          }
          catch (const std::exception& e)
          {
             printf("\nError parsing command. Please retry\n\n");
             printf("%s", help_dbg);
             retry = true;
          }
       } while (prompt || retry);
    }
    if (debug_steps > 0) debug_steps--;
}
