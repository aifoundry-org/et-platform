/***********************************************************************
*
* Copyright (C) 2021 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/

// exporting tables from noc_reconfig_memshire.c

#include "slam_engine.h"

// from noc_reconfig_memshire.c
extern CSR_SLAM_TABLE reconfig_noc_4_west_memshires;
extern CSR_SLAM_TABLE reconfig_noc_2_west_memshires;
extern CSR_SLAM_TABLE reconfig_noc_1_west_memshires;
extern CSR_SLAM_TABLE reconfig_noc_4_east_memshires;
extern CSR_SLAM_TABLE reconfig_noc_2_east_memshires;
extern CSR_SLAM_TABLE reconfig_noc_1_east_memshires;

// from noc_reconfig_minshire.c
extern CSR_SLAM_TABLE reconfig_noc_16_minshires;
extern CSR_SLAM_TABLE reconfig_noc_8_minshires;
extern CSR_SLAM_TABLE reconfig_noc_4_minshires;
extern CSR_SLAM_TABLE reconfig_noc_2_minshires;
extern CSR_SLAM_TABLE reconfig_noc_1_minshires;

// from noc_reconfig_pshire.c
extern CSR_SLAM_TABLE reconfig_noc_pshire_low_os;
