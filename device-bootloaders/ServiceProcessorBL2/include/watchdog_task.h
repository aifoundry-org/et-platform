#ifndef __WDOG_TASK_H__
#define __WDOG_TASK_H__

#include "bl2_watchdog.h"

#define WDOG_TASK_PRIORITY         1
#define WDOG_TASK_STACK_SIZE       512
#define WDOG_DEFAULT_TIMEOUT_MSEC  10000

int32_t init_watchdog_service(void);

#endif