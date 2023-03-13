//******************************************************************************
// Copyright (C) 2020 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef MDIAPITEST_H
#define MDIAPITEST_H

#define COMPUTE_KERNEL_DEVICE_ADDRESS 0x8005802000
#define MM_FW_MASTER_SDATA_BASE 0x8001000000
#define MDI_TEST_DEFAULT_SHIRE_ID 0
#define MDI_TEST_DEFAULT_THREAD_MASK 0x1
#define MDI_TEST_DEFAULT_HARTID 0
#define MDI_TEST_GPR_WRITE_TEST_DATA 0xa5a5a5a512345678
#define MDI_TEST_WRITE_MEM_TEST_DATA 0x12345678
#define MDI_TEST_CSR_PC_REG 0x20
#define MDI_TEST_CSR_WRITE_PC_TEST_ADDRESS 0x0000008005802040
#define MDI_TEST_BP_ADDRESS_OFFSET 0x20

#endif // MDIAPITEST_H