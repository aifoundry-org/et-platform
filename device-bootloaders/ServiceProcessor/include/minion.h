#include <stdint.h>
#include "minion_esr_defines.h"

// TDB - SW - JIRA -2275 - Need to enable all Minions Shires (Show move this logic to Minion Firmware since it supports broadcast)
#define MAX_NUM_COMPUTE_SHIRE 4
#define MASTER_SHIRE_ID	32

int enable_master_shire(void);
int enable_compute_shire(void);
