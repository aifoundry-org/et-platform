/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
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
