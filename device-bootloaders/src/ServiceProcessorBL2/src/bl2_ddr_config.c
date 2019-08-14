#include "bl2_ddr_config.h"

#include "ddr_config.h"

int ddr_config(void) {
    ddr_init(0);
    return 0;
}
