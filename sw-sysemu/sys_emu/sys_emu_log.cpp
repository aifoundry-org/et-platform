#include "testLog.h"

// static members of testLog
logLevel testLog::globalLogLevel_ = LOG_DEBUG;
logLevel testLog::defaultLogLevel_ = LOG_INFO;
bool testLog::logLevelsSet_ = false;
unsigned testLog::errors_ = 0;
unsigned testLog::maxErrors_ = (unsigned) -1;

// nearly empty implementation of testLog functions, that only make sense in RTL simulations

void endSimAt(uint32_t extraTime) {  exit(1); }
void endSim() { exit(1); }
bool simEnded(){  return false; }
uint64_t testLog::simTime() { return 0; }

void testLog::setLogLevels() {
  logLevelsSet_ = true;
}

std::string testLog::simTimeStr(){
  return std::string ("");
}
