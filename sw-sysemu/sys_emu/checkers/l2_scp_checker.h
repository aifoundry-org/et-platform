/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef _L2_SCP_CHECKER_H_
#define _L2_SCP_CHECKER_H_

// Global
#include <list>

// Local
#include "emu_defines.h"
#include "cache.h"
#include "agent.h"

#define L2_SCP_ENTRIES 65536

// Info of an L2 Scp fill for a minion
struct l2_scp_fill_info_t
{
    uint32_t line;
    uint32_t id;
};

// L2 Scp status
enum l2_scp_status {
   L2Scp_Fill,
   L2Scp_Valid
};

struct shire_scp_info_t
{
    l2_scp_status                 l2_scp_line_status[L2_SCP_ENTRIES];  // Status of all the L2 scp entries
    uint64_t                      l2_scp_line_addr[L2_SCP_ENTRIES];    // Address that was prefetched to each L2 scp entries
    std::list<l2_scp_fill_info_t> l2_scp_fills[EMU_MINIONS_PER_SHIRE]; // For each minion in the shire list of outstanding l2 scp fills
};

class l2_scp_checker : public bemu::Agent
{
  public:
    // Creator
    l2_scp_checker(bemu::System* chip = nullptr);

    // Copy/Move
    l2_scp_checker(const l2_scp_checker&) = default;
    l2_scp_checker& operator=(const l2_scp_checker&) = default;
    l2_scp_checker(l2_scp_checker&&) = default;
    l2_scp_checker& operator=(l2_scp_checker&&) = default;

    std::string name() const { return "L2-SCP-Checker"; }

    // Accessors
    void l2_scp_fill(uint32_t thread, uint32_t idx, uint32_t id, uint64_t src_addr);
    void l2_scp_wait(uint32_t thread, uint32_t id);
    void l2_scp_read(uint32_t thread, uint64_t addr);

    // Logging variables
    uint32_t log_shire  = 64;              // None by default
    uint32_t log_line   = 1 * 1024 * 1024; // None by default
    uint32_t log_minion = 2048;            // None by default

  private:
    shire_scp_info_t  shire_scp_info[EMU_NUM_SHIRES];
};

#endif

