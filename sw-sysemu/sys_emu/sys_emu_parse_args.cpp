/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include <cstring>
#include <getopt.h>
#include "emu_gio.h"
#include "sys_emu.h"

static const char * help_msg =
"\n ET System Emulator\n\n\
     sys_emu [options]\n\n\
 Where options are:\n\
     -elf_load <path>         Path to an ELF file to load. Can be used multiple times.\n\
     -file_load <addr>,<path> Address and path to the file to load. Can be used multiple times.\n\
     -mem_write32 <addr>,<value> Write the 32 bit value to address at init. Can be used multiple times.\n\
     -mem_desc <path>         Path to a file describing what ELFs and files to load\n\
     -l                       Set logging verbosity to DEBUG\n\
     -lt <thread>             Log a given Thread. Can be used multiple times. (default: all)\n\
     -lm <minion>             Log a given Minion. Can be used multiple times. (default: all)\n\
     -ls <shire>,<threads>    Log given Threads of a Shire. Can be used multiple times. (default: all)\n\
     -lp <path>               Redirect log output to path. (default: stdout)\n\
     -ltrigger_insn <instn>   Logging verbosity will be set to DEBUG after finding this instruction. (hex format, default: 0)\n\
     -ltrigger_hart <hart>    Logging verbosity will be set to DEBUG after this hart finds the trigger instruction. (default: 0)\n\
     -ltrigger_start <count>  Logging verbosity will be set to DEBUG after finding the trigger instruction this many times. (default: 0)\n\
     -ltrigger_stop <count>   Logging verbosity will be set back to INFO after finding the trigger instruction this many times. (default: 0)\n\
     -minions <mask>          A mask of Minions that should be enabled in each Shire (default: 1 Minion/Shire)\n\
     -shires <mask>           A mask of Shires that should be enabled. (default: 1 Shire)\n\
     -single_thread           Disable 2nd Minion thread\n\
     -mins_dis                Minions (not including SP) start disabled\n\
     -sp_dis                  SP starts disabled\n\
     -reset_pc <addr>         Sets boot program counter (default: 0x8000001000)\n\
     -sp_reset_pc <addr>      Sets Service Processor boot program counter (default: 0x40000000)\n\
     -set_xreg <t>,<r>,<val>  Sets the xregister (integer) <r> of thread <t> to value <val>. <t> can be 'sp' for the Service Processor\n\
     -max_cycles <cycles>     Stops execution after provided number of cycles (default: 10M)\n\
     -mem_reset <byte>        Reset value of main memory (default: 0)\n\
     -mem_reset32 <uint32>    Reset value of main memory (default: 0)\n\
     -pu_uart0_rx_file <path> Path to the file in which to read the contents of PU UART0 RX (default is stdin)\n\
     -pu_uart1_rx_file <path> Path to the file in which to read the contents of PU UART1 RX (default is stdin)\n\
     -spio_uart0_rx_file <path> Path to the file in which to read the contents of SPIO UART0 RX (default is stdin)\n\
     -spio_uart1_rx_file <path> Path to the file in which to read the contents of SPIO UART1 RX (default is stdin)\n\
     -pu_uart0_tx_file <path> Path to the file in which to dump the contents of PU UART0 TX (default is stdout)\n\
     -pu_uart1_tx_file <path> Path to the file in which to dump the contents of PU UART1 TX (default is stdout)\n\
     -spio_uart0_tx_file <path> Path to the file in which to dump the contents of SPIO UART0 TX (default is stdout)\n\
     -spio_uart1_tx_file <path> Path to the file in which to dump the contents of SPIO UART1 TX (default is stdout)\n\
     -log_at_pc <PC>          Enables logging when minion reaches a given PC\n\
     -stop_log_at_pc <PC>     Disables logging when minion reaches a given PC\n\
     -display_trap_info       Displays trap logging in the INFO channel instead of DEBUG\n\
     -dump_addr <addr>        At the end of simulation, address where to start the dump. Only valid if -dump_file is used\n\
     -dump_size <size>        At the end of simulation, size of the dump. Only valid if -dump_file is used\n\
     -dump_file <path>        At the end of simulation, file in which to dump\n\
     -dump_mem <path>         At the end of simulation, file where to dump ALL the memory content\n\
     -dump_at_pc_pc <PC>      Dump when PC M0:T0 reaches this PC\n\
     -dump_at_pc_addr <addr>  Address where to start the dump\n\
     -dump_at_pc_size <size>  Size of the dump\n\
     -dump_at_pc_file <file>  File where to store the dump\n\
     -mem_check               Enables memory coherency checks\n\
     -mem_check_minion        Enables memory coherency check prints for a specific minion (default: 2048 [2048 => no minion, -1 => all minions])\n\
     -mem_check_addr          Enables memory coherency check prints for a specific address (default: 0x1 [none])\n\
     -l1_scp_check            Enables L1 SCP checks\n\
     -l1_scp_check_minion     Enables L1 SCP check prints for a specific minion (default: 2048 [2048 => no minion, -1 => all minions])\n\
     -l2_scp_check            Enables L2 SCP checks\n\
     -l2_scp_check_shire      Enables L2 SCP check prints for a specific shire (default: 64 [64 => no shire, -1 => all shires])\n\
     -l2_scp_check_line       Enables L2 SCP check prints for a specific minion (default: 1048576 [1048576 => no L2 scp line, -1 => all L2 scp lines])\n\
     -l2_scp_check_minion     Enables L2 SCP check prints for a specific minion (default: 2048 [2048 => no minion, -1 => all minions])\n\
     -flb_check               Enables FLB checks\n\
     -flb_check_shire         Enables FLB check prints for a specific shire (default: 64 [64 => no shire, -1 => all shires])\n\
     -tstore_check            Enables TensorStore checks\n\
     -tstore_check_addr       Enables TensorStore check prints for a specific address (default: 0x1 [none])\n\
     -tstore_check_thread     Enables TensorStore check prints for a specific thread (default: 4096 [4096 => no thread, -1 => all threads])\n\
     -gdb                     Start the GDB stub for remote debugging at the start of simulation\n\
     -gdb_at_pc <PC>          Start the GDB stub for remote debugging at a given PC\n\
"
#ifdef SYSEMU_PROFILING
"    -dump_prof <path>        Path to the file in which to dump the profiling content at the end of the simulation\n"
#endif
"\
";

static int strsplit(char *str, const char *delimiters, char *tokens[], int max_tokens)
{
    int n = 0;
    char *tok = strtok(str, delimiters);

    while ((tok != NULL) && (n < max_tokens)) {
        tokens[n++] = tok;
        tok = strtok(NULL, delimiters);
    }

    return n;
}

// Util macros for the cmdline parser
#define SE_WARN(msg) std::cerr << "sys_emu: " << msg << std::endl;
#define SE_ERROR(msg) do { SE_WARN(msg); return std::make_pair(false, cmd_options); } while (0)

std::tuple<bool, struct sys_emu_cmd_options>
sys_emu::parse_command_line_arguments(int argc, char* argv[])
{
    sys_emu_cmd_options cmd_options;
    int opt;
    int index = 0;
    uint64_t dump_at_end_addr = 0, dump_at_end_size = 0;
    uint64_t dump_at_pc_pc, dump_at_pc_addr, dump_at_pc_size;

    static const struct option long_options[] = {
        {"elf",                    required_argument, nullptr, 0}, // same as '-elf', kept for backwards compatibility
        {"elf_load",               required_argument, nullptr, 0},
        {"file_load",              required_argument, nullptr, 0},
        {"mem_write32",            required_argument, nullptr, 0},
        {"mem_desc",               required_argument, nullptr, 0},
        {"l",                      no_argument,       nullptr, 0},
        {"lt",                     required_argument, nullptr, 0},
        {"lm",                     required_argument, nullptr, 0},
        {"ls",                     required_argument, nullptr, 0},
        {"lp",                     required_argument, nullptr, 0},
        {"ltrigger_insn",          required_argument, nullptr, 0},
        {"ltrigger_hart",          required_argument, nullptr, 0},
        {"ltrigger_start",         required_argument, nullptr, 0},
        {"ltrigger_stop",          required_argument, nullptr, 0},
        {"minions",                required_argument, nullptr, 0},
        {"shires",                 required_argument, nullptr, 0},
        {"master_min",             no_argument,       nullptr, 0}, // deprecated, use -shires <mask> to enable Master Shire and SP
        {"single_thread",          no_argument,       nullptr, 0},
        {"mins_dis",               no_argument,       nullptr, 0},
        {"sp_dis",                 no_argument,       nullptr, 0},
        {"reset_pc",               required_argument, nullptr, 0},
        {"sp_reset_pc",            required_argument, nullptr, 0},
        {"set_xreg",               required_argument, nullptr, 0},
        {"max_cycles",             required_argument, nullptr, 0},
        {"mem_reset",              required_argument, nullptr, 0},
        {"mem_reset32",            required_argument, nullptr, 0},
        {"pu_uart0_rx_file",       required_argument, nullptr, 0},
        {"pu_uart1_rx_file",       required_argument, nullptr, 0},
        {"spio_uart0_rx_file",     required_argument, nullptr, 0},
        {"spio_uart1_rx_file",     required_argument, nullptr, 0},
        {"pu_uart_tx_file",        required_argument, nullptr, 0}, // same as '-pu_uart0_tx_file', kept for backwards compatibility
        {"pu_uart0_tx_file",       required_argument, nullptr, 0},
        {"pu_uart1_tx_file",       required_argument, nullptr, 0},
        {"spio_uart0_tx_file",     required_argument, nullptr, 0},
        {"spio_uart1_tx_file",     required_argument, nullptr, 0},
        {"log_at_pc",              required_argument, nullptr, 0},
        {"stop_log_at_pc",         required_argument, nullptr, 0},
        {"display_trap_info",      no_argument,       nullptr, 0},
        {"dump_addr",              required_argument, nullptr, 0},
        {"dump_size",              required_argument, nullptr, 0},
        {"dump_file",              required_argument, nullptr, 0},
        {"dump_mem",               required_argument, nullptr, 0},
        {"dump_at_pc_pc",          required_argument, nullptr, 0},
        {"dump_at_pc_addr",        required_argument, nullptr, 0},
        {"dump_at_pc_size",        required_argument, nullptr, 0},
        {"dump_at_pc_file",        required_argument, nullptr, 0},
        {"mem_check",              no_argument,       nullptr, 0},
        {"mem_check_minion",       required_argument, nullptr, 0},
        {"mem_check_addr",         required_argument, nullptr, 0},
        {"l1_scp_check",           no_argument,       nullptr, 0},
        {"l1_scp_check_minion",    required_argument, nullptr, 0},
        {"l2_scp_check",           no_argument,       nullptr, 0},
        {"l2_scp_check_shire",     required_argument, nullptr, 0},
        {"l2_scp_check_line",      required_argument, nullptr, 0},
        {"l2_scp_check_minion",    required_argument, nullptr, 0},
        {"flb_check",              no_argument,       nullptr, 0},
        {"flb_check_shire",        required_argument, nullptr, 0},
        {"tstore_check",           no_argument,       nullptr, 0},
        {"tstore_check_addr",      required_argument, nullptr, 0},
        {"tstore_check_thread",    required_argument, nullptr, 0},
        {"gdb",                    no_argument,       nullptr, 0},
        {"gdb_at_pc",              required_argument, nullptr, 0},
        {"m",                      no_argument,       nullptr, 0},
#ifdef SYSEMU_PROFILING
        {"dump_prof",              required_argument, nullptr, 0},
#endif
        {"help",                   no_argument,       nullptr, 0},
        {nullptr,                  0,                 nullptr, 0}
    };

    while ((opt = getopt_long_only(argc, argv, "", long_options, &index)) != -1) {
        if (opt == '?') {
            SE_ERROR("Wrong arguments");
        }

        const char *const name = long_options[index].name;

        if (!strcmp(name, "elf_load") || !strcmp(name, "elf"))
        {
            cmd_options.elf_files.push_back({std::string(optarg)});
        }
        else if (!strcmp(name, "file_load"))
        {
            char *tokens[2];
            int ntokens = strsplit(optarg, ",", tokens, 2);
            if (ntokens == 2) {
                uint64_t addr = strtoull(tokens[0], nullptr, 0);
                const char *path = tokens[1];
                cmd_options.file_load_files.push_back({addr, path});
            }
        }
        else if (!strcmp(name, "mem_write32"))
        {
            char *tokens[2];
            int ntokens = strsplit(optarg, ",", tokens, 2);
            if (ntokens == 2) {
                uint64_t addr = strtoull(tokens[0], nullptr, 0);
                uint32_t value = strtoul(tokens[1], nullptr, 0);
                cmd_options.mem_write32s.push_back({addr, value});
            }
        }
        else if (!strcmp(name, "mem_desc"))
        {
            cmd_options.mem_desc_file = optarg;
        }
        else if (!strcmp(name, "l"))
        {
            cmd_options.log_en = true;
        }
        else if (!strcmp(name, "lt"))
        {
            cmd_options.log_thread[atoi(optarg)] = true;
        }
        else if (!strcmp(name, "lm"))
        {
            unsigned minion = 2 * atoi(optarg);
            cmd_options.log_thread[minion] = true;
            cmd_options.log_thread[minion + 1] = true;
        }
        else if (!strcmp(name, "ls"))
        {
            char *tokens[2];
            int ntokens = strsplit(optarg, ",", tokens, 2);
            if (ntokens == 2) {
                uint64_t threads;
                unsigned shire = atoi(tokens[0]);
                sscanf(tokens[1], "%" PRIx64, &threads);

                if (shire == IO_SHIRE_ID)
                    shire = EMU_IO_SHIRE_SP;

                unsigned thread0 = EMU_THREADS_PER_SHIRE * shire;
                unsigned shire_thread_count = (shire == EMU_IO_SHIRE_SP ? 1 : EMU_THREADS_PER_SHIRE);

                for (unsigned t = 0; t < shire_thread_count; ++t) {
                    if (threads & (1ULL << t))
                        cmd_options.log_thread[thread0 + t] = true;
                }
            }
        }
        else if (!strcmp(name, "lp"))
        {
            cmd_options.log_path = optarg;
        }
        else if (!strcmp(name, "ltrigger_insn"))
        {
            uint64_t instruction;
            sscanf(optarg, "%" PRIx64, &instruction);
            cmd_options.log_trigger_insn = instruction;
        }
        else if (!strcmp(name, "ltrigger_hart"))
        {
            cmd_options.log_trigger_hart = atoi(optarg);
        }
        else if (!strcmp(name, "ltrigger_start"))
        {
            cmd_options.log_trigger_start = atoi(optarg);
        }
        else if (!strcmp(name, "ltrigger_stop"))
        {
            cmd_options.log_trigger_stop = atoi(optarg);
        }
        else if (!strcmp(name, "minions"))
        {
            sscanf(optarg, "%" PRIx64, &cmd_options.minions_en);
        }
        else if (!strcmp(name, "shires"))
        {
            sscanf(optarg, "%" PRIx64, &cmd_options.shires_en);
        }
        else if (!strcmp(name, "master_min"))
        {
            SE_WARN("Ignoring deprecated option '-master_min'");
        }
        else if (!strcmp(name, "single_thread"))
        {
            cmd_options.second_thread = false;
        }
        else if (!strcmp(name, "mins_dis"))
        {
            cmd_options.mins_dis = true;
        }
        else if (!strcmp(name, "sp_dis"))
        {
            cmd_options.sp_dis = true;
        }
        else if (!strcmp(name, "reset_pc"))
        {
            sscanf(optarg, "%" PRIx64, &cmd_options.reset_pc);
        }
        else if (!strcmp(name, "sp_reset_pc"))
        {
            sscanf(optarg, "%" PRIx64, &cmd_options.sp_reset_pc);
        }
        else if (!strcmp(name, "set_xreg"))
        {
            uint64_t thread, xreg, value;
            char *tokens[3];

            int ntokens = strsplit(optarg, ",", tokens, 3);
            if (ntokens != 3) {
                SE_ERROR("Command line option '-set_xreg': Wrong number of arguments");
            }

            if (!strcmp(tokens[0], "sp")) {
                thread = EMU_IO_SHIRE_SP_THREAD;
            } else {
                thread = atoi(tokens[0]);
                if (thread == IO_SHIRE_SP_HARTID) {
                    thread = EMU_IO_SHIRE_SP_THREAD;
                } else if (thread >= EMU_NUM_THREADS) {
                    SE_ERROR("Command line option '-set_xreg': Invalid thread");
                }
            }

            xreg = atoi(tokens[1]);
            if (xreg == 0 || xreg >= bemu::NXREGS) {
                SE_ERROR("Command line option '-set_xreg': Invalid xreg");
            }

            value = strtoull(tokens[2], nullptr, 0);
            cmd_options.set_xreg.push_back({thread, xreg, value});
        }
        else if (!strcmp(name, "max_cycles"))
        {
            sscanf(optarg, "%" SCNu64, &cmd_options.max_cycles);
        }
        else if (!strcmp(name, "mem_reset"))
        {
          cmd_options.mem_reset = strtol(optarg, NULL, 0) & 0xFF;
          cmd_options.mem_reset |= cmd_options.mem_reset << 8;
          cmd_options.mem_reset |= cmd_options.mem_reset << 16;
        }
        else if (!strcmp(name, "mem_reset32"))
        {
          cmd_options.mem_reset = strtol(optarg, NULL, 0);
        }
        else if (!strcmp(name, "pu_uart0_rx_file"))
        {
            cmd_options.pu_uart0_rx_file = optarg;
        }
        else if (!strcmp(name, "pu_uart1_rx_file"))
        {
            cmd_options.pu_uart1_rx_file = optarg;
        }
        else if (!strcmp(name, "spio_uart0_rx_file"))
        {
            cmd_options.spio_uart0_rx_file = optarg;
        }
        else if (!strcmp(name, "spio_uart1_rx_file"))
        {
            cmd_options.spio_uart1_rx_file = optarg;
        }
        else if (!strcmp(name, "pu_uart0_tx_file") || !strcmp(name, "pu_uart_tx_file"))
        {
            cmd_options.pu_uart0_tx_file = optarg;
        }
        else if (!strcmp(name, "pu_uart1_tx_file"))
        {
            cmd_options.pu_uart1_tx_file = optarg;
        }
        else if (!strcmp(name, "spio_uart0_tx_file"))
        {
            cmd_options.spio_uart0_tx_file = optarg;
        }
        else if (!strcmp(name, "spio_uart1_tx_file"))
        {
            cmd_options.spio_uart1_tx_file = optarg;
        }
        else if (!strcmp(name, "log_at_pc"))
        {
            sscanf(optarg, "%" PRIx64, &cmd_options.log_at_pc);
        }
        else if (!strcmp(name, "stop_log_at_pc"))
        {
            sscanf(optarg, "%" PRIx64, &cmd_options.stop_log_at_pc);
        }
        else if (!strcmp(name, "display_trap_info"))
        {
            cmd_options.display_trap_info = true;
        }
        else if (!strcmp(name, "dump_addr"))
        {
            sscanf(optarg, "%" PRIx64, &dump_at_end_addr);
        }
        else if (!strcmp(name, "dump_size"))
        {
            dump_at_end_size = atoi(optarg);
        }
        else if (!strcmp(name, "dump_file"))
        {
            cmd_options.dump_at_end.push_back({dump_at_end_addr, dump_at_end_size,
                std::string(optarg)});
        }
        else if (!strcmp(name, "dump_mem"))
        {
            cmd_options.dump_mem = optarg;
        }
        else if (!strcmp(name, "dump_at_pc_pc"))
        {
            sscanf(optarg, "%" PRIx64, &dump_at_pc_pc);
        }
        else if (!strcmp(name, "dump_at_pc_addr"))
        {
            sscanf(optarg, "%" PRIx64, &dump_at_pc_addr);
        }
        else if (!strcmp(name, "dump_at_pc_size"))
        {
            sscanf(optarg, "%" PRIx64, &dump_at_pc_size);
        }
        else if (!strcmp(name, "dump_at_pc_file"))
        {
            sys_emu_cmd_options::dump_info dump = {
                dump_at_pc_addr,
                dump_at_pc_size,
                std::string(optarg)
            };
            cmd_options.dump_at_pc.emplace(dump_at_pc_pc, dump);
        }
        else if (!strcmp(name, "mem_check"))
        {
            cmd_options.mem_check = true;
        }
        else if (!strcmp(name, "mem_check_minion"))
        {
            cmd_options.mem_checker_log_minion = atoi(optarg);
        }
        else if (!strcmp(name, "mem_check_addr"))
        {
            sscanf(optarg, "%" PRIx64, &cmd_options.mem_checker_log_addr);
        }
        else if (!strcmp(name, "l1_scp_check"))
        {
            cmd_options.l1_scp_check = true;
        }
        else if (!strcmp(name, "l1_scp_check_minion"))
        {
            cmd_options.l1_scp_checker_log_minion = atoi(optarg);
        }
        else if (!strcmp(name, "l2_scp_check"))
        {
            cmd_options.l2_scp_check = true;
        }
        else if (!strcmp(name, "l2_scp_check_shire"))
        {
            cmd_options.l2_scp_checker_log_shire = atoi(optarg);
        }
        else if (!strcmp(name, "l2_scp_check_line"))
        {
            cmd_options.l2_scp_checker_log_line = atoi(optarg);
        }
        else if (!strcmp(name, "l2_scp_check_minion"))
        {
            cmd_options.l2_scp_checker_log_minion = atoi(optarg);
        }
        else if (!strcmp(name, "flb_check"))
        {
            cmd_options.flb_check = true;
        }
        else if (!strcmp(name, "flb_check_shire"))
        {
            cmd_options.flb_checker_log_shire = atoi(optarg);
        }
        else if (!strcmp(name, "tstore_check"))
        {
            cmd_options.tstore_check = true;
        }
        else if (!strcmp(name, "tstore_check_addr"))
        {
            sscanf(optarg, "%" PRIx64, &cmd_options.tstore_checker_log_addr);
        }
        else if (!strcmp(name, "tstore_check_thread"))
        {
            cmd_options.tstore_checker_log_thread = atoi(optarg);
        }
        else if (!strcmp(name, "gdb"))
        {
            cmd_options.gdb = true;
        }
        else if (!strcmp(name, "gdb_at_pc"))
        {
            cmd_options.gdb = true;
            sscanf(optarg, "%" PRIx64, &cmd_options.gdb_at_pc);
        }
        else if (!strcmp(name, "m"))
        {
            SE_WARN("Ignoring deprecated option '-m'");
        }
#ifdef SYSEMU_PROFILING
        else if (!strcmp(name, "dump_prof"))
        {
            cmd_options.dump_prof_file = optarg;
        }
#endif
        else if (!strcmp(name, "help")) {
            return std::make_pair(false, cmd_options);
        }
    }

    // Enable logging for all threads if no filter has been specified
    if (cmd_options.log_thread.none()) cmd_options.log_thread.set();

    return std::make_pair(true, cmd_options);
}

void sys_emu::get_command_line_help(std::ostream& stream)
{
    stream << help_msg;
}
