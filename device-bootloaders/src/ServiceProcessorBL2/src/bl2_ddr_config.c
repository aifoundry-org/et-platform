#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "etsoc_hal/rm_esr.h"
#include "etsoc_hal/cm_esr.h"
#include "etsoc_hal/pshire_esr.h"
#include "hal_device.h"

#include "bl2_ddr_config.h"

#include "ddr_config.h"

int ddr_config(void) {
    // configure the dram controllers and train the memory

    for (uint32_t memshire_id = 0; memshire_id < 8; memshire_id++)
        ddr_init(memshire_id);

    return 0;
}
