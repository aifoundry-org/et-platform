#include "testLog.h"

// static members of testLog
logLevel testLog::globalLogLevel_ = LOG_DEBUG;
logLevel testLog::defaultLogLevel_ = LOG_INFO;
bool testLog::logLevelsSet_ = false;
unsigned testLog::errors_ = 0;

// set maxErrors_ to 1 because sys_emu does not use testMain/testBase
unsigned testLog::maxErrors_ = 1u;

// nearly empty implementation of testLog functions, that only make sense in RTL simulations

void endSimAt(uint32_t extraTime)
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

uint64_t testLog::simTime()
{
    extern uint64_t emu_cycle;
    return emu_cycle;
}

void testLog::setLogLevels()
{
    logLevelsSet_ = true;
}

std::string testLog::simTimeStr()
{
    return std::to_string(testLog::simTime());
}
