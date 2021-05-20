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
*       Header/Interface description for public interfaces that provide
*       SP to Minion communications
*
***********************************************************************/
#ifndef MM_IFACE_H
#define MM_IFACE_H

#include "common_defs.h"
#include "sp_mm_iface.h" /* header from shared/helper lib */

int8_t MM_Iface_Init(void);

#define MM_Iface_Push_Cmd_To_SP2MM_SQ(p_cmd, cmd_size)   \
        SP_MM_Iface_Push(SP_SQ, p_cmd, cmd_size)
#define MM_Iface_Pop_Cmd_From_SP2MM_CQ(rx_buff)   \
        SP_MM_Iface_Pop(SP_CQ, rx_buff)

#define MM_Iface_Pop_Cmd_From_MM2SP_SQ(rx_buff)   \
        SP_MM_Iface_Pop(MM_SQ, rx_buff)
#define MM_Iface_Push_Cmd_To_MM2SP_CQ(p_cmd, cmd_size)   \
        SP_MM_Iface_Push(MM_CQ, p_cmd, cmd_size)

//int8_t MM_Iface_Process_MM2SP_Cmds(void* cmd);

#endif