/***********************************************************************
*
* Copyright (C) 2022 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/***********************************************************************/
/*! \file statw.h
    \brief A C header that defines the device statistics sampler
    worker's interface.
*/
/***********************************************************************/

#ifndef _STATW_
#define _STATW_

/*! \fn void STATW_Launch(uint32_t sqw_idx)
    \brief Initialize Device Stat Workers, used by dispatcher
    \param hart_id HART ID on which the Stat Worker should be launched
    \return none
*/
void STATW_Launch(uint32_t hart_id);

/*! \def STATW_SAMPLING_INTERVAL
    \brief Device statistics sampling interval
*/
#define STATW_SAMPLING_INTERVAL 500

#endif