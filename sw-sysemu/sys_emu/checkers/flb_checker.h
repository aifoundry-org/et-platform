/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef _FLB_CHECKER_H_
#define _FLB_CHECKER_H_

#include <stdint.h>

#include "agent.h"

class flb_checker : public bemu::Agent
{
  public:
    flb_checker(bemu::System* chip=nullptr) : bemu::Agent(chip) {}

    flb_checker(const flb_checker&) = default;
    flb_checker& operator=(const flb_checker&) = default;
    flb_checker(flb_checker&&) = default;
    flb_checker& operator=(flb_checker&&) = default;

    std::string name() const { return "FLB-Checker"; }

    // Accessors
    void access(uint32_t oldval, uint32_t limit, uint32_t flb, uint32_t thread);

  private:
};

#endif

