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
    uint64_t emu_cycle() const noexcept;

    explicit Agent(System* chip=nullptr) : chip(chip) {}
    virtual ~Agent() = default;

    System* chip;
};


//
// An external agent
//
struct Noagent final: public Agent {
    Noagent(System* chip, std::string name = "") : Agent(chip), m_name(std::move(name)) {}
    std::string name() const override { return m_name; }
private:
    std::string m_name;
};


} // namespace bemu

#endif // BEMU_AGENT_H
