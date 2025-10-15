/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/
#pragma once
#include <hostUtils/logging/LoggingExport.h>

#include <memory>

namespace logging {

class Logger;

class LOGGING_API Instance {
public:
  /// Public logging levels
  enum class LEVEL { FATAL = 0, WARNING, INFO, DEBUG };

  Instance();
  ~Instance();

  /// enables/disables logging level 'lvl'
  void setEnable(LEVEL lvl, bool enable);

private:
  std::unique_ptr<Logger> logger_; 
};

} // end namespace logging