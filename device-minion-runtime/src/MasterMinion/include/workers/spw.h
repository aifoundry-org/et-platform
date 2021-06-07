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
/***********************************************************************/
/*! \file spw.h
    \brief A C header that defines the Service Processor Worker's
    public interfaces.
*/
/***********************************************************************/
#ifndef SPW_DEFS_H
#define SPW_DEFS_H

#include "config/mm_config.h"
#include "common_defs.h"

/*! \fn void SPW_Launch(uint32_t hart_id)
    \brief Launch the Service Processor Queue Worker
    \param hart_id HART ID on which the Service Processor Queue Worker should be launched
    \return none
*/
void SPW_Launch(uint32_t hart_id);

#endif /* SPW_DEFS_H */
