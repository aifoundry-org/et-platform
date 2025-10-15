/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#include "sys_emu.h"

#include <cstdlib>
#include <iostream>
#include <memory>

////////////////////////////////////////////////////////////////////////////////
// Main function
// It is kept separate on purpose so that we can produce a libsysemu.a library
// that can be included in other simulator implementations and instantiations
////////////////////////////////////////////////////////////////////////////////

int main(int argc, char * argv[])
{
    auto result = sys_emu::parse_command_line_arguments(argc, argv);
    bool status = std::get<0>(result);
    sys_emu_cmd_options cmd_options = std::get<1>(result);

    if (!status) {
        sys_emu::get_command_line_help(std::cout);
        return EXIT_FAILURE;
    }

    std::unique_ptr<sys_emu> emu(new sys_emu(cmd_options));
    return emu.get()->main_internal();
}
