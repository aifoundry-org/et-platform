/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#ifndef _FLB_CHECKER_H_
#define _FLB_CHECKER_H_

// Local
#include "agent.h"
#include "emu_defines.h"

// STD
#include <stdint.h>
#include <vector>

class flb_checker : public bemu::Agent
{
  struct flb_info_t
  {
      std::vector<uint32_t> alive_threads; // List of alive threads
      uint32_t              limit;         // Limit for the fLB entry
  };

  private:
    flb_info_t shire_flb_info[EMU_NUM_SHIRES][32];

  public:
    flb_checker(bemu::System* chip=nullptr) : bemu::Agent(chip) {}

    flb_checker(const flb_checker&) = default;
    flb_checker& operator=(const flb_checker&) = default;
    flb_checker(flb_checker&&) = default;
    flb_checker& operator=(flb_checker&&) = default;

    std::string name() const { return "FLB-Checker"; }

    // Accessors
    void access(uint32_t oldval, uint32_t limit, uint32_t flb, uint32_t thread);

    // Logging variables
    uint32_t log_shire = 64; // None by default

};

#endif

