/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef _L1_SCP_CHECKER_H_
#define _L1_SCP_CHECKER_H_

// Global
#include <list>

// Local
#include "emu_defines.h"
#include "cache.h"

// L1 Scp status
enum l1_scp_status {
   L1Scp_Invalid,
   L1Scp_Fill,
   L1Scp_Valid
};

struct minion_scp_info_t
{
    l1_scp_status l1_scp_line_status[L1_SCP_ENTRIES];
    uint32_t      l1_scp_line_id[L1_SCP_ENTRIES];
};

class l1_scp_checker
{
  public:
    // Creator
    l1_scp_checker();

    // Accessors
    void l1_scp_fill(uint32_t thread, uint32_t idx, uint32_t id);
    void l1_scp_wait(uint32_t thread, uint32_t id);
    void l1_scp_read(uint32_t thread, uint32_t idx);

  private:
    minion_scp_info_t minion_scp_info[EMU_NUM_MINIONS];
};

#endif

