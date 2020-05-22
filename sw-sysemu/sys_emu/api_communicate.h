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

// STD
#include <list>
#include <string>

// Forward declarations
namespace bemu {
struct MainMemory;
}

// Class that receives commands from the runtime API and forwards it to SoC
class api_communicate
{
    public:
        api_communicate();
        virtual ~api_communicate() { }

        virtual bool init() = 0;
        virtual bool is_enabled() = 0;
        virtual bool is_done() = 0;
        virtual void get_next_cmd(std::list<int> *enabled_threads) = 0;
        virtual void set_comm_path(const std::string &comm_path) = 0;
        virtual bool raise_host_interrupt() = 0;

        void set_memory(bemu::MainMemory* mem) {
            this->mem = mem;
        }

    protected:
        bemu::MainMemory* mem;  // Pointer to the memory
};

#endif // _API_COMMUNICATE_
