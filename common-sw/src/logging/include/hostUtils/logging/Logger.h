/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#pragma once
#include <hostUtils/logging/LoggingExport.h>

#include "DefaultSinks.h"

#include <g3log/g3log.hpp>
#include <g3log/logworker.hpp>
#include <g3log/loglevels.hpp>
#include <cstdlib>

namespace logging {

const LEVELS
    VLOG_HIGH{g3::kDebugValue-100, {"VERBOSE_HIGH"}},
    VLOG_MID{g3::kDebugValue-99, {"VERBOSE_MID"}},
    VLOG_LOW{g3::kDebugValue-98, {"VERBOSE_LOW"}};

class LOGGING_API Logger {
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

class LOGGING_API LoggerDefault : public Logger {
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

} // end namespace logging