/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef _API_COMMUNICATE_
#define _API_COMMUNICATE_

#include <cstdint>
#include <string>

// Forward declaration
namespace bemu {
class System;
}

// Class that receives commands from the runtime API and forwards it to SoC
class api_communicate {
public:
    virtual ~api_communicate() = default;
    virtual void set_system(bemu::System*) = 0;
    virtual void process(void) = 0;
    virtual bool raise_host_interrupt(uint32_t bitmap) = 0;
    virtual bool host_memory_read(uint64_t host_addr, uint64_t size, void *data) = 0;
    virtual bool host_memory_write(uint64_t host_addr, uint64_t size, const void *data) = 0;
    virtual void notify_iatu_ctrl_2_reg_write(int pcie_id, uint32_t iatu, uint32_t value) = 0;
    virtual void notify_fatal_error(const std::string& = "") = 0;
};

#endif // _API_COMMUNICATE_
