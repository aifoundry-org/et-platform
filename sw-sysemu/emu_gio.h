/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef _EMU_GIO_H
#define _EMU_GIO_H

#include <cinttypes>
#include <bitset>

#include "emu_defines.h"
#include "testLog.h"


#define LOG_HART_IF(severity, agent, cond, format, ...) do { \
    if (LOG_##severity >= bemu::log.getLogLevel() && bemu::log_thread[bemu::hart_index(agent)] && (cond)) { \
        bemu::lprintf(LOG_##severity, "[%s] " format, (agent).name().c_str(), __VA_ARGS__); \
    } \
} while (0)


#define LOG_HART(severity, agent, format, ...) do { \
    if (LOG_##severity >= bemu::log.getLogLevel() && bemu::log_thread[bemu::hart_index(agent)]) { \
        bemu::lprintf(LOG_##severity, "[%s] " format, (agent).name().c_str(), __VA_ARGS__); \
    } \
} while (0)


#define LOG_AGENT(severity, agent, format, ...) do { \
    if (LOG_##severity >= bemu::log.getLogLevel()) { \
        bemu::lprintf(LOG_##severity, "[%s] " format, (agent).name().c_str(), __VA_ARGS__); \
    } \
} while (0)


#define LOG_NOTHREAD(severity, format, ...) do { \
    if (LOG_##severity >= bemu::log.getLogLevel()) { \
        bemu::lprintf(LOG_##severity, format, __VA_ARGS__); \
    } \
} while (0)


namespace bemu {


extern std::bitset<EMU_NUM_THREADS> log_thread;
extern testLog                      log;

void lprintf(logLevel level, const char* fmt, ...) __attribute__ ((format (printf, 2, 3)));

void log_set_threads(const std::bitset<EMU_NUM_THREADS> &threads);


} // namespace bemu

#endif // _EMU_GIO_H
