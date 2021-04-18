/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#pragma once
#include <g3log/g3log.hpp>
#include <g3log/logworker.hpp>
#include <g3log/loglevels.hpp>
#include "DefaultSinks.h"
#include <stdlib.h>

namespace logging {
const LEVELS 
    VLOG_HIGH{g3::kDebugValue-100, {"VERBOSE_HIGH"}},
    VLOG_MID{g3::kDebugValue-99, {"VERBOSE_MID"}},
    VLOG_LOW{g3::kDebugValue-98, {"VERBOSE_LOW"}};
class Logger {
public:

  explicit Logger() {
    using namespace g3;
    logWorker_ = LogWorker::createLogWorker();
  }

  void init() {
    initializeLogging(logWorker_.get());    
  }
protected:
  std::unique_ptr<g3::LogWorker> logWorker_;
};

class LoggerDefault : public Logger {
public:
  explicit LoggerDefault(bool initialize = true) {
    logWorker_->addSink(std::make_unique<ColoredOutput>(),
                                      &ColoredOutput::ReceiveLogMessage);


    auto etVlog = getenv("ET_VLOG");
    bool activate_vlogh = false;
    bool activate_vlogm = false;
    bool activate_vlogl = false;
    
    if (etVlog != nullptr) {
      auto verboseLevel = std::stoi(etVlog);
      activate_vlogh = verboseLevel >= 2;
      activate_vlogm = verboseLevel >= 1;
      activate_vlogl = verboseLevel >= 0;
    }

    g3::only_change_at_initialization::addLogLevel(VLOG_HIGH, activate_vlogh);
    g3::only_change_at_initialization::addLogLevel(VLOG_MID, activate_vlogm);
    g3::only_change_at_initialization::addLogLevel(VLOG_LOW, activate_vlogl);
#ifdef NDEBUG
    g3::log_levels::disable(DEBUG);
#endif
    if (initialize) {
      init();
    }
  }
};
}