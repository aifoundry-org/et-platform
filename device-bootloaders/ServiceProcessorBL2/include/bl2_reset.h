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

#ifndef __BL2_RESET_H__
#define __BL2_RESET_H__

int release_memshire_from_reset(void);
int release_minions_from_cold_reset(void);
int release_minions_from_warm_reset(void);
void release_etsoc_reset(void);
void pcie_reset_flr(void);
void pcie_reset_warm(void);

#endif
