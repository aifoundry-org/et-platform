/* vim: set sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#include <cstring>
#include "emu_gio.h"
#include "sys_emu.h"

extern uint64_t md_log_addr;
extern uint32_t md_log_minion;
extern uint32_t sd_l1_log_minion;
extern uint32_t sd_l2_log_shire;
extern uint32_t sd_l2_log_line;
extern uint32_t sd_l2_log_minion;

static const char * help_msg =
"\n ET System Emulator\n\n\
     sys_emu [options]\n\n\
 Where options are one of:\n\
     -api_comm <path>         Path to socket that feeds runtime API commands.\n\
"
#ifdef SYSEMU_DEBUG
"    -d                       Start in interactive debug mode (must have been compiled with SYSEMU_DEBUG)\n"
#endif
"\
     -dump_addr <addr>        At the end of simulation, address where to start the dump. Only valid if -dump_file is used\n\
     -dump_size <size>        At the end of simulation, size of the dump. Only valid if -dump_file is used\n\
     -dump_file <path>        At the end of simulation, file in which to dump\n\
     -dump_mem <path>         At the end of simulation, file where to dump ALL the memory content\n\
"
#ifdef SYSEMU_PROFILING
"    -dump_prof <path>        Path to the file in which to dump the profiling content at the end of the simulation\n"
#endif
"\
     -elf <path>              Path to an ELF file to load.\n\
     -l                       Enable Logging\n\
     -lm <minion>             Log a given Minion ID only. (default: all)\n\
     -master_min              Enables master minion to send interrupts to compute minions.\n\
     -max_cycles <cycles>     Stops execution after provided number of cycles (default: 10M)\n\
     -mem_desc <path>         Path to a file describing the memory regions to create and what code to load there\n\
     -mem_reset <byte>        Reset value of main memory (default: 0)\n\
     -minions <mask>          A mask of Minions that should be enabled in each Shire (default: 1 Minion/Shire)\n\
     -mins_dis                Minions start disabled\n\
     -pu_uart_tx_file <path>  Path to the file in which to dump the contents of PU UART TX\n\
     -pu_uart1_tx_file <path> Path to the file in which to dump the contents of PU UART1 TX\n\
     -reset_pc <addr>         Sets boot program counter (default: 0x8000001000)\n\
     -shires <mask>           A mask of Shires that should be enabled. (default: 1 Shire)\n\
     -single_thread           Disable 2nd Minion thread\n\
     -sp_reset_pc <addr>      Sets Service Processor boot program counter (default: 0x40000000)\n\
     -coherency_check         Enables cache coherency checks\n\
     -coherency_check_minion  Enables cache coherency check prints for a specific minion (default: 2048 [2048 => no minion, -1 => all minions])\n\
     -coherency_check_addr    Enables cache coherency check prints for a specific address (default: 0x1 [none])\n\
     -scp_check               Enables SCP checks\n\
     -l1_scp_check_minion     Enables L1 SCP check prints for a specific minion (default: 2048 [2048 => no minion, -1 => all minions])\n\
     -l2_scp_check_shire      Enables L2 SCP check prints for a specific shire (default: 64 [64 => no shire, -1 => all shires])\n\
     -l2_scp_check_line       Enables L2 SCP check prints for a specific minion (default: 1048576 [1048576 => no L2 scp line, -1 => all L2 scp lines])\n\
     -l2_scp_check_minion     Enables L2 SCP check prints for a specific minion (default: 2048 [2048 => no minion, -1 => all minions])\n\
     -log_at_pc               Enables logging when minion reaches a given PC\n\
     -stop_log_at_pc          Disables logging when minion reaches a given PC\n\
     -dump_at_pc_pc           Dump when PC M0:T0 reaches this PC\n\
     -dump_at_pc_addr         Address where to start the dump\n\
     -dump_at_pc_size         Size of the dump\n\
     -dump_at_pc_file         File where to store the dump\n\
";

std::tuple<bool, struct sys_emu_cmd_options>
sys_emu::parse_command_line_arguments(int argc, char* argv[])
{
    sys_emu_cmd_options cmd_options;
    int dump_option = 0;
    uint64_t dump_at_end_addr = 0, dump_at_end_size = 0;
    uint64_t dump_at_pc_pc, dump_at_pc_addr, dump_at_pc_size;

    for(int i = 1; i < argc; i++)
    {
        if (cmd_options.max_cycle)
        {
            cmd_options.max_cycle = false;
            sscanf(argv[i], "%" SCNu64, &cmd_options.max_cycles);
        }
        else if (cmd_options.elf)
        {
            cmd_options.elf = false;
            cmd_options.elf_file = argv[i];
        }
        else if(cmd_options.mem_desc)
        {
            cmd_options.mem_desc = false;
            cmd_options.mem_desc_file = argv[i];
        }
        else if(cmd_options.api_comm)
        {
            cmd_options.api_comm = false;
            cmd_options.api_comm_path = argv[i];
        }
        else if(cmd_options.minions)
        {
            sscanf(argv[i], "%" PRIx64, &minions_en);
            cmd_options.minions = 0;
        }
        else if(cmd_options.shires)
        {
            sscanf(argv[i], "%" PRIx64, &shires_en);
            cmd_options.shires = 0;
        }
        else if(cmd_options.reset_pc_flag)
        {
          sscanf(argv[i], "%" PRIx64, &cmd_options.reset_pc);
          cmd_options.reset_pc_flag = false;
        }
        else if(cmd_options.sp_reset_pc_flag)
        {
          sscanf(argv[i], "%" PRIx64, &cmd_options.sp_reset_pc);
          cmd_options.sp_reset_pc_flag = false;
        }
        else if(dump_option == 1)
        {
            sscanf(argv[i], "%" PRIx64, &dump_at_end_addr);
            dump_option = 0;
        }
        else if(dump_option == 2)
        {
            dump_at_end_size = atoi(argv[i]);
            dump_option = 0;
        }
        else if(dump_option == 3)
        {
            sys_emu_cmd_options::dump_info dump = {
                dump_at_end_addr,
                dump_at_end_size,
                std::string(argv[i])
             };
            cmd_options.dump_at_end.push_back(dump);
            dump_option = 0;
        }
        else if(dump_option == 4)
        {
            cmd_options.log_min = atoi(argv[i]);
            dump_option = 0;
        }
        else if(dump_option == 5)
        {
            cmd_options.dump_mem = argv[i];
            dump_option = 0;
        }
        else if(dump_option == 6)
        {
            cmd_options.pu_uart_tx_file = argv[i];
            dump_option = 0;
        }
        else if(dump_option == 7)
        {
            cmd_options.pu_uart1_tx_file = argv[i];
            dump_option = 0;
        }
        else if(dump_option == 8)
        {
            md_log_minion = atoi(argv[i]);
            dump_option = 0;
        }
        else if(dump_option == 9)
        {
            sscanf(argv[i], "%" PRIx64, &md_log_addr);
            dump_option = 0;
        }
        else if(dump_option == 10)
        {
            sd_l1_log_minion = atoi(argv[i]);
            dump_option = 0;
        }
        else if(dump_option == 11)
        {
            sd_l2_log_shire = atoi(argv[i]);
            dump_option = 0;
        }
        else if(dump_option == 12)
        {
            sd_l2_log_line = atoi(argv[i]);
            dump_option = 0;
        }
        else if(dump_option == 13)
        {
            sd_l2_log_minion = atoi(argv[i]);
            dump_option = 0;
        }
        else if(dump_option == 14)
        {
            sscanf(argv[i], "%" PRIx64, &cmd_options.log_at_pc);
            dump_option = 0;
        }
        else if(dump_option == 15)
        {
            sscanf(argv[i], "%" PRIx64, &cmd_options.stop_log_at_pc);
            dump_option = 0;
        }
        else if(dump_option == 16)
        {
            sscanf(argv[i], "%" PRIx64, &dump_at_pc_pc);
            dump_option = 0;
        }
        else if(dump_option == 17)
        {
            sscanf(argv[i], "%" PRIx64, &dump_at_pc_addr);
            dump_option = 0;
        }
        else if(dump_option == 18)
        {
            sscanf(argv[i], "%" PRIx64, &dump_at_pc_size);
            dump_option = 0;
        }
        else if(dump_option == 19)
        {
            sys_emu_cmd_options::dump_info dump = {
                dump_at_pc_addr,
                dump_at_pc_size,
                std::string(argv[i])
            };
            cmd_options.dump_at_pc.emplace(dump_at_pc_pc, dump);
             dump_option = 0;
         }
        else if(cmd_options.mem_reset_flag)
        {
            cmd_options.mem_reset = atoi(argv[i]);
            cmd_options.mem_reset_flag = false;
        }
#ifdef SYSEMU_PROFILING
        else if(cmd_options.dump_prof == 1)
        {
            cmd_options.dump_prof_file = argv[i];
            cmd_options.dump_prof = 0;
        }
#endif
        else if(strcmp(argv[i], "-max_cycles") == 0)
        {
            cmd_options.max_cycle = true;
        }
        else if(strcmp(argv[i], "-elf") == 0)
        {
            cmd_options.elf = true;
        }
        else if(strcmp(argv[i], "-mem_desc") == 0)
        {
            cmd_options.mem_desc = true;
        }
        else if(strcmp(argv[i], "-api_comm") == 0)
        {
            cmd_options.api_comm = true;
        }
        else if(strcmp(argv[i], "-master_min") == 0)
        {
            cmd_options.master_min = true;
        }
        else if(strcmp(argv[i], "-minions") == 0)
        {
            cmd_options.minions = true;
        }
        else if(strcmp(argv[i], "-shires") == 0)
        {
            cmd_options.shires = true;
        }
        else if(strcmp(argv[i], "-reset_pc") == 0)
        {
            cmd_options.reset_pc_flag = true;
        }
        else if(strcmp(argv[i], "-sp_reset_pc") == 0)
        {
            cmd_options.sp_reset_pc_flag = true;
        }
        else if(strcmp(argv[i], "-coherency_check") == 0)
        {
            coherency_check = true;
        }
        else if(strcmp(argv[i], "-scp_check") == 0)
        {
            scp_check = true;
        }
        else if(strcmp(argv[i], "-mem_reset") == 0)
        {
            cmd_options.mem_reset_flag = true;
        }
        else if(strcmp(argv[i], "-dump_addr") == 0)
        {
            dump_option = 1;
        }
        else if(strcmp(argv[i], "-dump_size") == 0)
        {
            dump_option = 2;
        }
        else if(strcmp(argv[i], "-dump_file") == 0)
        {
            dump_option = 3;
        }
        else if(strcmp(argv[i], "-lm") == 0)
        {
            dump_option = 4;
        }
        else if(strcmp(argv[i], "-dump_mem") == 0)
        {
            dump_option = 5;
        }
        else if(strcmp(argv[i], "-pu_uart_tx_file") == 0)
        {
            dump_option = 6;
        }
        else if(strcmp(argv[i], "-pu_uart1_tx_file") == 0)
        {
            dump_option = 7;
        }
        else if(strcmp(argv[i], "-coherency_check_minion") == 0)
        {
            dump_option = 8;
        }
        else if(strcmp(argv[i], "-coherency_check_addr") == 0)
        {
            dump_option = 9;
        }
        else if(strcmp(argv[i], "-l1_scp_check_minion") == 0)
        {
            dump_option = 10;
        }
        else if(strcmp(argv[i], "-l2_scp_check_shire") == 0)
        {
            dump_option = 11;
        }
        else if(strcmp(argv[i], "-l2_scp_check_line") == 0)
        {
            dump_option = 12;
        }
        else if(strcmp(argv[i], "-l2_scp_check_minion") == 0)
        {
            dump_option = 13;
        }
        else if(strcmp(argv[i], "-log_at_pc") == 0)
        {
            dump_option = 14;
        }
        else if(strcmp(argv[i], "-stop_log_at_pc") == 0)
        {
            dump_option = 15;
        }
        else if(strcmp(argv[i], "-dump_at_pc_pc") == 0)
        {
            dump_option = 16;
        }
        else if(strcmp(argv[i], "-dump_at_pc_addr") == 0)
        {
            dump_option = 17;
        }
        else if(strcmp(argv[i], "-dump_at_pc_size") == 0)
        {
            dump_option = 18;
        }
        else if(strcmp(argv[i], "-dump_at_pc_file") == 0)
        {
            dump_option = 19;
        }
        else if(strcmp(argv[i], "-m") == 0)
        {
            cmd_options.create_mem_at_runtime = true;
            LOG_NOTHREAD(WARN, "%s", "Ignoring deprecated option '-m'");
        }
        else if(strcmp(argv[i], "-l") == 0)
        {
            cmd_options.log_en = true;
        }
        else if (  (strcmp(argv[i], "-h") == 0)
                 ||(strcmp(argv[i], "-help") == 0)
                 ||(strcmp(argv[i], "--help") == 0)) {
           printf("%s", help_msg);
           std::tuple<bool, sys_emu_cmd_options> ret_value(false, sys_emu_cmd_options());
           return ret_value;
        }
        else if (strcmp(argv[i], "-single_thread") == 0)
        {
            cmd_options.second_thread = false;
        }
#ifdef SYSEMU_DEBUG
        else if(strcmp(argv[i], "-d") == 0)
        {
            cmd_options.debug = true;
        }
#endif
#ifdef SYSEMU_PROFILING
        else if(strcmp(argv[i], "-dump_prof") == 0)
        {
            cmd_options.dump_prof = 1;
        }
#endif
        else if(strcmp(argv[i], "-mins_dis") == 0)
        {
            cmd_options.mins_dis = true;
        }
        else
        {
            LOG_NOTHREAD(FTL, "Unknown parameter %s", argv[i]);
        }
    }

    std::tuple<bool, sys_emu_cmd_options> ret_value(true, cmd_options);
    return ret_value;
}
