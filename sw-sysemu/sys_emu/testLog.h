/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef _TESTLOG_H_
#define _TESTLOG_H_

class sys_emu;

#include <cstdint>
#include <iostream>
#include <sstream>

enum logLevel {LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERR, LOG_FTL, NR_LOG_LEVELS};

void endSimAt(uint32_t extraTime=0); // time in time units (ps)
void set_stop_time(uint64_t t);
void endSim();
bool simEnded();


#define DEFAULT_CLOCK_PERIOD 1000
#define time2Cycles(x) (x/DEFAULT_CLOCK_PERIOD)

class testLog
{
public:

 testLog(const std::string &name = "", logLevel logLvl = LOG_INFO):
  name_(name), outputStream_(&std::cout)
  {
    msgStarted_ = false;
    msgInLogLevel_ = false;
    fatal_ = false;
    if (!logLevelsSet_) setLogLevels();
    setLogLevel(logLvl);
  }
  void setOutputStream(std::ostream* output)
  {
    outputStream_ = output;
  }

  testLog(const testLog&) = delete;
  testLog& operator=(const testLog&) = delete;

  // Note: The cosim testLog does not have a device
  void setDevice(sys_emu* device) { device_ =  device; }
  void setLogLevel(logLevel level) {
    logLevel_ = level;
    // cannot mask errors or fatal
    if (logLevel_ > LOG_ERR)  logLevel_ = LOG_ERR;
  }
  enum logLevel getLogLevel() {  return logLevel_; }

  void setName (const std::string name) { name_ = name; }

  template <class T>
  testLog &operator<<(T x) {
    if (msgInLogLevel_)
      os_<<x;
    return *this;
  }
  testLog &operator<<(logLevel l) {
    if(msgStarted_) *outputStream_<<"previous msg did not finish => ["<<name_.c_str()<<"]"<<os_.str()<<std::endl;

    msgStarted_=true;

    if ( l >= globalLogLevel_  && l >= logLevel_ ) {
      os_<< simTimeStr() <<": ";
      switch(l) {
      case LOG_DEBUG: os_<<"DEBUG "; break;
      case LOG_INFO: os_<<"INFO "; break;
      case LOG_WARN:os_<<"WARN "; break;
      case LOG_ERR:os_<<"ERROR "; break;
      default: os_<<"FATAL "; fatal_ = true;
      }
      os_<<name_.c_str()<<": ";
      msgInLogLevel_ = true;
    }
    else msgInLogLevel_ = false;
    if (l >= LOG_ERR) errors_++;
    return *this;
  }
  testLog & operator<<(testLog& m(testLog&) ) {
    return m(*this);
  }
  void endl () {
    if (msgStarted_) os_ << std::endl;
  }
  void endm();

private:
  std::string name_;
  std::ostringstream os_;
  std::ostream* outputStream_;
  bool msgStarted_;
  bool msgInLogLevel_;
  bool fatal_;
  static unsigned errors_;
  static logLevel globalLogLevel_;
  static logLevel defaultLogLevel_;
  static bool logLevelsSet_;
 public:
  static logLevel getGlobalLogLevel() {
    if (!logLevelsSet_) setLogLevels();
    return globalLogLevel_;
  }
  static logLevel getDefaultLogLevel() {
    if (!logLevelsSet_) setLogLevels();
    return defaultLogLevel_;
  }
  static unsigned maxErrors_;
  static unsigned getErrors() {return errors_;}

  uint64_t simTime();
  unsigned simCycle() { return time2Cycles(simTime()); }
  std::string simTimeStr();
  static void setLogLevels();

 private:
  logLevel logLevel_;
  sys_emu* device_ = nullptr;
};


static inline testLog& endm( testLog& log) {
  log.endm();
  return log;
}

static inline testLog& endl( testLog& log) {
  log.endl();
  return log;
}

#endif
