/*-------------------------------------------------------------------------
* Copyright (C) 2018, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

#ifndef __BL2_FIRMWARE_UPDATE_H__
#define __BL2_FIRMWARE_UPDATE_H__

//TODO: Once SW-4849 is fixed, DM Firmware update service should retrieve this
//      address from Device Interface registers
#define DEVICE_FW_UPDATE_REGION_SIZE 0x400000
#define DEVICE_FW_UPDATE_REGION_BASE 0x8004000000ULL 

#endif
