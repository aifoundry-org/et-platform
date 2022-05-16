/*-------------------------------------------------------------------------
* Copyright (C) 2021, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/
#pragma once

#include <memory>
namespace logging {

class Logger;

class Instance {
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