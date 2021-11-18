/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef _TSTORE_CHECKER_H_
#define _TSTORE_CHECKER_H_

// Local
#include "emu_defines.h"
#include "cache.h"
#include "agent.h"

// STD
#include <list>

class tstore_checker : public bemu::Agent
{
private:
    struct coop_tstore
    {
        uint64_t address; // Base Address
        uint64_t stride;  // Stride between lines
        uint32_t coop;    // Cooperative encoding: 0 -> none, 1 -> 2 minion, 3 -> 4 minion
        uint32_t lines;   // Number of lines in the store
        uint32_t cols;    // Number of bytes per line
    };

    std::list<coop_tstore> pending_list[EMU_NUM_THREADS];     

private:
    bool check_and_drain_head(uint32_t thread_id);

public:
    // Constructor
    tstore_checker(bemu::System* chip = nullptr);

    // Copy/Move
    tstore_checker(const tstore_checker&) = default;
    tstore_checker& operator=(const tstore_checker&) = default;
    tstore_checker(tstore_checker&&) = default;
    tstore_checker& operator=(tstore_checker&&) = default;

    std::string name() const { return "TStore-Checker"; }

    // Accessors
    void execute(uint32_t thread_id, uint64_t address, uint64_t stride, uint32_t coop, uint32_t lines, uint32_t cols);
    void check_and_drain(uint32_t thread_id);
    void is_empty();

    // Logging variables
    uint64_t log_addr = 1;
    uint32_t log_thread = 4096;

};

#endif

