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
************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       Header/Interface description for the Master component's public
*       interfaces
*
***********************************************************************/
#ifndef DISPATCHER_DEFS_H
#define DISPATCHER_DEFS_H

#include "common_defs.h"

#define     DISPATCHER_BASE_HART_ID     2048
#define     DISPATCHER_NUM              1
#define     DISPATCHER_MAX_HART_ID      \
                DISPATCHER_BASE_HART_ID + DISPATCHER_NUM

/*! \fn void Dispatcher_Launch(void)
    \brief Launch a dispatcher instance on HART ID requested
    \param [in] HART ID to launch the dispatcher
*/
void Dispatcher_Launch(uint32_t hart_id);

#endif /* DISPATCHER_DEFS_H */