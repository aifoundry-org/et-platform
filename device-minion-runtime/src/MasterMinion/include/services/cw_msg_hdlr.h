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
*       Header/Interface to access CW cmessage handler
*
***********************************************************************/
#ifndef CW_MSG_HDLR_H
#define CW_MSG_HDLR_H

#include <common_defs.h>

/*! \fn SP_Message_Handler(void* message_buffer, uint16_t message_size)
    \brief Interface to handle completion worker messages.
    \param [in] message_buffer: pointer to message buffer
    \param [in] message_size: message size
    \param [out] int8_t: message handling return status
*/
int8_t CW_Message_Handler(void* message_buffer, uint16_t message_size);

#endif /* CW_MSG_HDLR_H */