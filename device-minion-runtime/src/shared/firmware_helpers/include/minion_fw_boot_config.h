#ifndef MINION_FW_BOOT_CONFIG_H
#define MINION_FW_BOOT_CONFIG_H

#include <stdint.h>
#include "layout.h"

typedef struct {
    uint64_t minion_shires;
} __attribute__((packed)) minion_fw_boot_config_t;

#ifndef __ASSEMBLER__
static_assert(sizeof(minion_fw_boot_config_t) <= FW_MINION_FW_BOOT_CONFIG_SIZE,
              "Minion FW boot config struct is larger than permitted!");
#endif

#endif
