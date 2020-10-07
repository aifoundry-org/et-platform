#ifndef ASSET_TRACKING_SERVICE_H
#define ASSET_TRACKING_SERVICE_H

#include <stdint.h>
#include "mailbox.h"

// PCIE gen bit rates(GT/s) definition
#define PCIE_GEN_1   2
#define PCIE_GEN_2   5
#define PCIE_GEN_3   8
#define PCIE_GEN_4   16
#define PCIE_GEN_5   32

// Function prototypes 
void assetTrackingProcessRequest(mbox_e mbox, uint32_t cmd_id);

#endif
