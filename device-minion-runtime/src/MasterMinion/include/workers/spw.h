/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
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

/* mm_rt_svcs*/
#include <etsoc/common/common_defs.h>

/* mm specific headers */
#include "config/mm_config.h"

/*! \fn void SPW_Launch(uint32_t hart_id)
    \brief Launch the Service Processor Queue Worker
    \param hart_id HART ID on which the Service Processor Queue Worker should be launched
    \return none
*/
void SPW_Launch(uint32_t hart_id);

#endif /* SPW_DEFS_H */
