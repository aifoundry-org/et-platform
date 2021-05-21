/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include "testLog.h"

#include "sys_emu.h"

// static members of testLog
logLevel testLog::globalLogLevel_ = LOG_DEBUG;
logLevel testLog::defaultLogLevel_ = LOG_INFO;
bool testLog::logLevelsSet_ = false;
unsigned testLog::errors_ = 0;

// set maxErrors_ to 1 because sys_emu does not use testMain/testBase
unsigned testLog::maxErrors_ = 1u;

// nearly empty implementation of testLog functions, that only make sense in RTL simulations

void endSimAt(uint32_t extraTime __attribute__((unused)))
{
    exit(1);
}

void endSim()
{
    exit(1);
}

bool simEnded()
{
    return false;
}

void testLog::setLogLevels()
{
    logLevelsSet_ = true;
}

uint64_t testLog::simTime()
{
    return device_ ? device_->get_emu_cycle() : 0;
}

std::string testLog::simTimeStr()
{
    return std::to_string(testLog::simTime());
}
