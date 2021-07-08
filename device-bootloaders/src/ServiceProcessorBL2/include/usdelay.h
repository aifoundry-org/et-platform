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

#ifndef __USDELAY_H__
#define __USDELAY_H__

#include <stdint.h>

/*! \fn int usdelay(uint32_t usec)
    \brief This function delay usec micro seconds
    \param None
    \return Status indicating success or negative error
*/
int usdelay(uint32_t usec);

#endif  //__USDELAY_H__
