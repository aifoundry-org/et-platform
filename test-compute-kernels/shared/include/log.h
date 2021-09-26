#ifndef LOG_H
#define LOG_H

#include "etsoc/common/log_common.h"

#include <stdint.h>
#include <stdio.h>

void log_set_level(log_level_t level);
log_level_t get_log_level(void);
int64_t log_write(log_level_t level, const char *const fmt, ...);
int64_t log_write_str(log_level_t level, const char *str, size_t length);

#endif
