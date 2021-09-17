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
*       SP to Host communications
*
***********************************************************************/
#ifndef SP_HOST_IFACE_DEFS_H
#define SP_HOST_IFACE_DEFS_H

#include "etsoc/common/common_defs.h"
#include "transports/vq/vq.h"
#include "sp_vq_build_config.h"

/**
 * @brief Enum of supported virtual queue types
 */
typedef enum {
    SQ,
    CQ
} vq_type_t;

/*! \fn int8_t SP_Host_Iface_Init(void)
    \brief Initialize SP to Host Interface
    \param none
    \return status
*/
int8_t SP_Host_Iface_Init(void);

/*! \fn int8_t SP_Iface_SQs_Init(void)
    \brief Initialize SP Interface SQs
    \param none
    \return status
*/
int8_t SP_Host_Iface_SQ_Init(void);

/*! \fn int8_t SP_Iface_CQs_Init(void)
    \brief Initialize SP Interface CQs
    \param none
    \return status
*/
int8_t SP_Host_Iface_CQ_Init(void);

/*! \fn uint16_t SP_Iface_Peek_SQ_Cmd_Size(uint8_t sq_id)
    \brief SP interface to peek SQ command size
    \returns [out] command size
*/
uint32_t SP_Host_Iface_SQ_Pop_Cmd(void* rx_buff);

/*! \fn uint32_t SP_Iface_CQ_Push_Cmd(uint8_t cq_id, void* rx_buff)
    \brief SP interface CQ push API
    \param [in] p_cmd: Pointer to command rx buffer
    \param [in] cmd_size: Command size
    \returns [out] Status
*/
int8_t SP_Host_Iface_CQ_Push_Cmd(void* p_cmd, uint32_t cmd_size);

/*! \fn vq_cb_t* SP_Host_Iface_Get_VQ_Base_Addr(uint8_t vq_type)
    \brief Obtain pointer to virtual queue associated
    with the queue type
    \param vq_type Virtual Queue Type
    \return vq_cb_t* Pointer to the virtual queue control block
*/
vq_cb_t* SP_Host_Iface_Get_VQ_Base_Addr(uint8_t vq_type);

/******************************/
/* Special Optimized routines */
/******************************/

/*! \fn void SP_Host_Iface_Optimized_SQ_Update_Tail(vq_cb_t *sq_shared, vq_cb_t *sq_cached)
    \brief This function is used to update the value of tail
    from cached VQ CB to shared VQ CB
    \param sq_shared Pointer to shared VQ CB
    \param sq_cached Pointer to cached VQ CB
*/
static inline void SP_Host_Iface_Optimized_SQ_Update_Tail(vq_cb_t *sq_shared, vq_cb_t *sq_cached)
{
    /* Update tail value in VQ memory */
    VQ_Set_Tail_Offset(sq_shared, VQ_Get_Tail_Offset(sq_cached));
}

#endif /* SP_HOST_IFACE_DEFS_H */
