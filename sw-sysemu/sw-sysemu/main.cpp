/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <vector>

#include "sys_emu.h"
#include "sim_api_communicate.h"

static const char *sim_api_options_help =
"\n Simulator API options:\n\
     -sim_api                 Enable the use of the Simulator API to talk to the runtime\n\
     -api_comm <path>         Path to socket that feeds runtime API commands.\n\
";

int main(int argc, char *argv[])
{
    static const struct option sim_api_options[] = {
        {"sim_api",        no_argument,       0, 's'},
        {"api_comm",       required_argument, 0, 'p'},
        {"sim_api_async",  no_argument,       0, 'a'},
        {nullptr,          0,                 0,  0 }
    };

    int opt;
    bool use_sim_api = false;
    std::string socket_path;
    std::vector<char *> base_options = {argv[0]};

    opterr = 0; // don't error on non-recognized options

    while ((opt = getopt_long_only(argc, argv, "-", sim_api_options, nullptr)) != -1) {
        switch (opt) {
        case 'p':
            socket_path = std::string(optarg);
            break;
        case 's':
            use_sim_api = true;
            break;
        case 'a':
            LOG_NOTHREAD(WARN, "%s", "Ignoring deprecated option '-sim_api_async'");
            break;
        case '?': // non-recognized option
            base_options.push_back(argv[optind - 1]);
            break;
        case 1: // non-option argument
            base_options.push_back(argv[optind - 1]);
            break;
        }
    }

    optind = 1; // reset getopt index
    opterr = 1;

    auto result = sys_emu::parse_command_line_arguments(base_options.size(), base_options.data());
    bool status = std::get<0>(result);
    sys_emu_cmd_options cmd_options = std::get<1>(result);

    std::unique_ptr<api_communicate> api_comm;
    if (use_sim_api)
        api_comm = std::unique_ptr<api_communicate>(new sim_api_communicate(socket_path));
    else
        api_comm = std::unique_ptr<api_communicate>(nullptr);

    if (!status) {
        std::ostringstream stream;
        sys_emu::get_command_line_help(stream);
        std::cout << stream.str() << sim_api_options_help;
        return EXIT_FAILURE;
    }

    sys_emu emu;
    return emu.main_internal(cmd_options, std::move(api_comm));
}
