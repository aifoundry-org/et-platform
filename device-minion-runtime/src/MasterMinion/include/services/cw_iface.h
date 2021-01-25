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
/*! \file cw_iface.h
    \brief A C header that defines the Compute Worker Interface 
    component's public interfaces
*/
/***********************************************************************/
#ifndef CW_IFACE_DEFS_H
#define CW_IFACE_DEFS_H

#include "common_defs.h"
#include "cm_to_mm_iface.h"

/*TODO: This mapping can go away once cleanup is odne */
#define CW_To_MM_Iface_Unicast_Receive(cb_idx, message) CM_To_MM_Iface_Unicast_Receive(cb_idx, const message)


int8_t CW_Iface_Processing(cm_iface_message_t *msg);

/* TODO: Move mm_to_cm_iface.h contents here */

#endif /* CW_IFACE_DEFS_H */