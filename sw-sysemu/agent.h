/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_AGENT_H
#define BEMU_AGENT_H

#include <string>

namespace bemu {


//
// A system agent abstract class
//
struct Agent {
    virtual long shireid() const = 0;
    virtual std::string name() const = 0;
};


//
// An agent that returns -1 for shireid()
//
struct Noshire_agent: public Agent {
    long shireid() const override { return -1L; }
    std::string name() const override { return std::string(""); }
};


} // namespace bemu

#endif // BEMU_AGENT_H
