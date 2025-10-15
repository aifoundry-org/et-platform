/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
/*! \file dispatcher.h
    \brief A C header that defines the Dispatcher component's public
    interfaces.
*/
/***********************************************************************/
#ifndef DISPATCHER_DEFS_H
#define DISPATCHER_DEFS_H

/* mm_rt_svcs */
#include <etsoc/common/common_defs.h>

/* mm specific headers */
#include "config/mm_config.h"

/*! \fn void Dispatcher_Launch(void)
    \brief Launch a dispatcher instance on HART ID requested
    \param hart_id HART ID to launch the dispatcher
    \return none
*/
void Dispatcher_Launch(uint32_t hart_id);

#endif /* DISPATCHER_DEFS_H */
