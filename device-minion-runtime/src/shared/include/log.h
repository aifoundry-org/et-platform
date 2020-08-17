#ifndef LOG_H
#define LOG_H

#include "log_levels.h"

#include <stdint.h>

int64_t log_write(log_level_t level, const char *const fmt, ...);

#endif
