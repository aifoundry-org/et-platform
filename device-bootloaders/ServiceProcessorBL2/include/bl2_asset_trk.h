#ifndef ASSET_TRACKING_SERVICE_H
#define ASSET_TRACKING_SERVICE_H


#include "dm.h"
#include "mailbox.h"
#include "esr_defines.h"
#include "sp_otp.h"
#include "io.h"
#include "pcie_device.h"

// PCIE gen bit rates(GT/s) definition
#define PCIE_GEN_1   2
#define PCIE_GEN_2   5
#define PCIE_GEN_3   8
#define PCIE_GEN_4   16
#define PCIE_GEN_5   32

// Function prototypes 
void asset_tracking_process_request(mbox_e mbox, uint32_t cmd_id);

#endif
