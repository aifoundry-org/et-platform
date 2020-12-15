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
*       Header/Interface description for lock services
*
***********************************************************************/
#ifndef LOCK_DEFS_H
#define LOCK_DEFS_H

typedef char lock_t;

/*! \fn void Lock_Acquire(lock_t lock)
    \brief Acquire Lock
    \param [in] lock to acquire
*/
void Lock_Acquire(lock_t lock);

/*! \fn log_level_t Log_Get_Level(void)
    \brief Get log level
    \param [out] Get the current global log level
*/
void Lock_Release(lock_t lock);

#endif /* LOCK_DEFS_H */