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
/*! \file device_trace_access_primitives.H
    \brief A C header file that has generic implemenation of Trace memory
           access primitives. All memory operations are normal operations
           without any special access primitive.
*/
/***********************************************************************/

#ifndef TRACE_SP_PRIMITIVES_H
#define TRACE_SP_PRIMITIVES_H

/* TODO: Provide timer API for SP to get timestamp. */
#define ET_TRACE_GET_TIMESTAMP(ret_val)      (ret_val+=1);

#endif
