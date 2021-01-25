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
/*! \file cw.h
    \brief A C header that defines the CW Worker's public interfaces.
*/
/***********************************************************************/
#ifndef CW_DEFS_H
#define CW_DEFS_H

typedef enum {
    CW_SHIRE_STATE_UNKNOWN = 0,
    CW_SHIRE_STATE_READY,
    CW_SHIRE_STATE_RUNNING,
    CW_SHIRE_STATE_ERROR,
    CW_SHIRE_STATE_COMPLETE
} cw_shire_state_t;

int8_t CW_Init(void);
int8_t CW_Update_Shire_State(uint64_t shire, cw_shire_state_t shire_state);

#endif /* CW_DEFS_H */
