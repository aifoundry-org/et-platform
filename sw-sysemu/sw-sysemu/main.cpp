/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <memory>

#include "sys_emu.h"

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
    api_comm = std::unique_ptr<api_communicate>(nullptr);

    if (!status) {
        std::ostringstream stream;
        sys_emu::get_command_line_help(stream);
        std::cout << stream.str();
        return EXIT_FAILURE;
    }

    auto emu = std::make_unique<sys_emu>(cmd_options, api_comm.get());
    return emu.get()->main_internal();
}
