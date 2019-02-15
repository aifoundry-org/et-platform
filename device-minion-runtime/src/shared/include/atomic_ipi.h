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

#ifndef _ATOMIC_IPI_H_
#define _ATOMIC_IPI_H_

// Atomic and IPI defines
#define ATOMIC_REGION 0x013FF40100ULL
#define IPI_THREAD0   0x013FF400C0ULL
#define IPI_THREAD1   0x013FF400D0ULL
#define IPI_NET       0x0108000800ULL
#define FCC0_MASTER   0x01083400C0ULL
#define FCC1_MASTER   0x01083400C8ULL

#endif

