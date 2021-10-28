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
