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
#include "etsoc/drivers/pcie/pcie_int.h"
#include "config/mgmt_build_config.h"

/*! \def SP_MM_CQ_MAX_ELEMENT_SIZE
    \brief Macro for specifying the maximum element size of SP-Host SQ.
*/
#define SP_HOST_SQ_MAX_ELEMENT_SIZE 512U

/**
 * @brief Enum of supported virtual queue types
 */
typedef enum
{
    SQ,
    CQ
} vq_type_t;

/*! \fn int8_t SP_Host_Iface_Init(void)
    \brief Initialize the SP to Host Interface
    \param none
    \return status
*/
int8_t SP_Host_Iface_Init(void);

/*! \fn int8_t SP_Host_Iface_SQ_Init(void)
    \brief Initialize the SP to Host Interface SQs
    \param none
    \return Status indicating success or negative error
*/
int8_t SP_Host_Iface_SQ_Init(void);

/*! \fn int8_t SP_Host_Iface_CQ_Init(void)
    \brief Initialize the SP to Host Interface CQs
    \param none
    \return Status indicating success or negative error
*/
int8_t SP_Host_Iface_CQ_Init(void);

/*! \fn uint16_t SP_Host_Iface_SQ_Pop_Cmd(void *rx_buff)
    \brief Pop command from SP to Host SQ interface
    \param [in] rx_buff Pointer to command rx buffer
    \returns Command size or zero for error
*/
uint32_t SP_Host_Iface_SQ_Pop_Cmd(void *rx_buff);

/*! \fn uint32_t SP_Host_Iface_CQ_Push_Cmd(void *p_cmd, uint32_t cmd_size)
    \brief Push command to SP to Host interface CQ interface
    \param [in] p_cmd: Pointer to command rx buffer
    \param [in] cmd_size: Command size or zero for error
    \returns Status indicating success or negative error
*/
int8_t SP_Host_Iface_CQ_Push_Cmd(void *p_cmd, uint32_t cmd_size);

/*! \fn vq_cb_t* SP_Host_Iface_Get_VQ_Base_Addr(uint8_t vq_type)
    \brief Obtain pointer to virtual queue associated with the queue type
    \param vq_type Virtual Queue Type
    \return vq_cb_t* Pointer to the virtual queue control block
*/
vq_cb_t *SP_Host_Iface_Get_VQ_Base_Addr(uint8_t vq_type);

/******************************/
/* Special Optimized routines */
/******************************/

/*! \fn void SP_Host_Iface_Optimized_SQ_Update_Tail(vq_cb_t *sq_shared, vq_cb_t *sq_cached)
    \brief Update the value of SQ tail
    from cached VQ CB to shared VQ CB
    \param sq_shared Pointer to shared VQ CB
    \param sq_cached Pointer to cached VQ CB
*/
static inline void SP_Host_Iface_Optimized_SQ_Update_Tail(vq_cb_t *sq_shared, vq_cb_t *sq_cached)
{
    /* Update tail value in VQ memory */
    VQ_Set_Tail_Offset(sq_shared, VQ_Get_Tail_Offset(sq_cached));

    /* Notify host */
    pcie_interrupt_host(SP_SQ_NOTIFY_VECTOR);
}

#endif /* SP_HOST_IFACE_DEFS_H */
