/***********************************************************************
*
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/

/***********************************************************************/
/*! \file dmaw.h
    \brief A C header that defines the DMA Worker's public interfaces.
*/
/***********************************************************************/
#ifndef DMAW_DEFS_H
#define DMAW_DEFS_H

#include "common_defs.h"
#include "sync.h"
#include "vq.h"

/*! \def DMAW_MAX_HART_ID
    \brief A macro that provides the maximum HART ID the DMAW is configued
    to execute on.
*/
#define  DMAW_MAX_HART_ID      DMAW_BASE_HART_ID + DMAW_NUM

/*! \fn void DMAW_Init(void)
    \brief Initialize DMA Worker
    \return none
*/
void DMAW_Init(void);

/*! \fn void DMAW_Launch(void)
    \brief Launch the DMA Worker
    \param hart_id HART ID on which the DMA Worker should be launched
    \return none
*/
void DMAW_Launch(uint32_t hart_id);

#endif /* DMAW_DEFS_H */