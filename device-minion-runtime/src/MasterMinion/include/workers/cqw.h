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
*       Header/Interface description for Completion Queue Worker
*       components public interface
*
***********************************************************************/
#ifndef CQW_DEFS_H
#define CQW_DEFS_H

#include "common_defs.h"

#define     CQW_BASE_HART_ID     2059
#define     CQW_NUM              1
#define     CQW_MAX_HART_ID      \
                CQW_BASE_HART_ID + CQW_NUM

/*! \fn void CQW_Launch(void)
    \brief Launch the Completion Queue Worker
    \param [in] HART ID on which the Completion Queue Worker should be 
           launched
*/
void CQW_Launch(uint32_t hart_id);

#endif /* CQW_DEFS_H */