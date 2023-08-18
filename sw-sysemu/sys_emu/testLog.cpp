/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>

#include "testLog.h"
#include "sys_emu.h"

#if __has_include("filesystem")
#include <filesystem>
#elif __has_include("experimental/filesystem")
#include <experimental/filesystem>
namespace std {
namespace filesystem = std::experimental::filesystem;
}
#endif
namespace fs = std::filesystem;

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

void testLog::endm() {
  if (!msgStarted_)
    *outputStream_ << "endm without msg start (string=" << os_.str() << ")" << std::endl;
  else if (msgInLogLevel_)
    *outputStream_ << "" << os_.str() << std::endl;
  os_.str("");
  os_.clear();
  if (fatal_) {
    if (device_ && device_->get_api_communicate()) {
      device_->get_api_communicate()->notify_fatal_error(os_.str());
    } else {
      endSim();
    }
  } else if (errors_ >= maxErrors_) {
    if (!simEnded()) {
      *outputStream_ << "Stopping simulation because max number of errors reached (" << maxErrors_ << ")" << std::endl;
      if (device_ && device_->get_api_communicate()) {
        device_->get_api_communicate()->notify_fatal_error(
          "Stopping simulation because max number of errors reached (" + std::to_string(maxErrors_) + ")");
      } else {
        endSim();
      }
    }
  }
  msgInLogLevel_ = true;
  msgStarted_ = false;
}

void testLog::dumpTraceBufferIfFatal(const bemu::Agent& agent) {

  /* high level application provides the cookie file to get device traces on FATAL errors.*/
  static const std::string sysemuTraceDumpCookiePath =
    std::filesystem::path(std::filesystem::temp_directory_path().string() + "/" + "sysemuTraceDumpCookie." +
                          std::to_string(getuid()) + "." + std::to_string(getpid()) + ".bin");


  if (!fatal_ or !std::filesystem::exists(sysemuTraceDumpCookiePath)) {
    return;
  }

  uint32_t numDev = 0;
  std::vector<uint64_t> traceAddress;
  std::vector<size_t> bufsize;
    
  auto traceAddrPtrInfo = std::ifstream(sysemuTraceDumpCookiePath, std::ios::binary | std::ios::in);
    
  if (!traceAddrPtrInfo.good()) {
    std::cout << "WARNING, Could not open the " << sysemuTraceDumpCookiePath << "file." << std::endl;
    return;
  }
    
  traceAddrPtrInfo.read((char *)&numDev, sizeof(uint32_t));

  for (int i = 0; i < numDev; i++) {
    uint64_t addr;
    size_t buffersize;

    traceAddrPtrInfo.read((char *)&addr, sizeof(uint64_t));
    traceAddress.emplace_back(addr);
    traceAddrPtrInfo.read((char *)&buffersize, sizeof(size_t));
    bufsize.emplace_back(buffersize);
  }

  for (int i=0; i< traceAddress.size(); i++) {
    std::vector<std::byte> dstbuf(bufsize.at(i));
    agent.chip->memory.read(agent, traceAddress.at(i), dstbuf.size(), dstbuf.data());
    
    auto traceInfoStream = std::ofstream("traceKernels_OnFatal_dev_"+ std::to_string(i) + ".bin", std::ios::binary | std::ios::out);  
    traceInfoStream.write((char*)dstbuf.data(), dstbuf.size());
    traceInfoStream.flush();
    traceInfoStream.close();
  }
  
}
