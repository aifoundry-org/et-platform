/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#pragma once
#include <hostUtils/logging/LoggingExport.h>

#include <g3log/g3log.hpp>
#include <g3log/logworker.hpp>
#include <g3log/loglevels.hpp>
#include <iostream>

namespace logging {

struct LOGGING_API ColoredOutput {

// Linux xterm color
// http://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
  enum FG_Color {YELLOW = 33, RED = 31, GREEN=32, WHITE = 97, NOCOLOR};

  FG_Color GetColor(const LEVELS level) const {
     if (level.value == WARNING.value) { return YELLOW; }
     if (level.value <= DEBUG.value) { return GREEN; }
     if (g3::internal::wasFatal(level)) { return RED; }

     return NOCOLOR;
  }

  void ReceiveLogMessage(g3::LogMessageMover logEntry) {
     auto level = logEntry.get()._level;
     auto color = GetColor(level);
     std::string msg = logEntry.get().toString();

     std::stringstream ss;
     if (color == NOCOLOR) {
       ss << msg;
     } else {
       ss << "\033[" << color << "m" << msg << "\033[m";
     }
     std::cerr << ss.str() << std::flush;
  }
};

}
