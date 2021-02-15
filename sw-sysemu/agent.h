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


// forward declaration
class System;


//
// A system agent abstract class
//
struct Agent {
    virtual std::string name() const = 0;

    System* chip = nullptr;
};


//
// An external agent
//
struct Noagent final: public Agent {
    Noagent(System* system) { this->chip = system; }

    std::string name() const override { return std::string(""); }
};


} // namespace bemu

#endif // BEMU_AGENT_H
