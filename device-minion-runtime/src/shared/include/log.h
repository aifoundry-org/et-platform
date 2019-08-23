#ifndef LOG_H
#define LOG_H

#include "log_levels.h"

#include <stdbool.h>
#include <stdint.h>

void log_set_level(log_level_t level);
int64_t log_write(log_level_t level, const char* const fmt, ...);

#endif
