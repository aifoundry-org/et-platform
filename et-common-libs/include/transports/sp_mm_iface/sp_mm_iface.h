/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
/*! \file sp_mm_iface.h
    \brief Provides the Service Processor (SP) to Master Minion (MM)
    interfaces.
*/
/***********************************************************************/
#ifndef __SP_MM_IFACE_H__
#define __SP_MM_IFACE_H__

#include "etsoc/common/common_defs.h"
#include "transports/sp_mm_iface/sp_mm_comms_spec.h"
#include "transports/vq/vq.h"

/*! \def SP_MM_IFACE_INTERRUPT_SP
    \brief Macro used to direct interrupt to SP
*/
#define SP_MM_IFACE_INTERRUPT_SP   -1

/*! \def SP_MM_IFACE_ERROR_INVALID_TARGET
    \brief SP MM Iface - Invalid target error
*/
#define SP_MM_IFACE_ERROR_INVALID_TARGET   -1

/*! \def SP_MM_IFACE_ERROR_VQ_BAD_HEAD
    \brief SP MM Iface - Bad head pointer error
*/
#define SP_MM_IFACE_ERROR_VQ_BAD_HEAD      -2

/*! \def SP_MM_IFACE_ERROR_VQ_BAD_TAIL
    \brief SP MM Iface - Bad tail pointer error
*/
#define SP_MM_IFACE_ERROR_VQ_BAD_TAIL      -3

/*! \def SP_MM_IFACE_INIT_MSG_HDR
    \brief Helper to initialize msg header
*/
#define SP_MM_IFACE_INIT_MSG_HDR(hdr, id, size, hart_id)      \
    ((struct dev_cmd_hdr_t *)hdr)->msg_id = id;               \
    ((struct dev_cmd_hdr_t *)hdr)->msg_size = size;           \
    ((struct dev_cmd_hdr_t *)hdr)->issuing_hart_id = hart_id;

/* Supported SP to MM interface targets */
enum sp_mm_target_t {
    SP_SQ=0, /* For MM to submit commands to SP, Push from MM */
    SP_CQ, /* For SP to submit responses to MM, Push from SP */
    MM_SQ, /* For SP to submit commands to MM, Push from SP */
    MM_CQ /* For MM to submit responses to SP, Push from MM */
};

/*! \def SP_MM_VERIFY_VQ_TAIL
    \brief Enables VQ tail pointer verification
*/
//#define SP_MM_VERIFY_VQ_TAIL

/*! \fn int8_t SP_MM_Iface_Init(void)
    \brief SP to MM interface initialization
    \return Status indicating success or negative error
*/
int8_t SP_MM_Iface_Init(void);

/*! \fn int8_t SP_MM_Iface_Push(uint8_t target, const void* p_buff, uint32_t size)
    \brief Push command to specified SP to MM interface target
    \param target target SP to MM interface
    \param p_cmd reference to command to push
    \param cmd_size size of command to push
    \return Status indicating success or negative error
*/
int8_t SP_MM_Iface_Push(uint8_t target, const void* p_buff, uint32_t size);

/*! \fn int32_t SP_MM_Iface_Pop(uint8_t target, void* rx_buff)
    \brief Pop command from specified SP to MM interface target
    \param target target SP to MM interface
    \param rx_buff Buffer to receive popped command or response message
    \return Status indicating success or negative error
*/
int32_t SP_MM_Iface_Pop(uint8_t target, void* rx_buff);

/*! \fn bool SP_MM_Iface_Data_Available(uint8_t target)
    \brief Check if data is available on the specified SP to MM interface target
    \param target target SP to MM interface
    \return Boolean indicating status
*/
bool SP_MM_Iface_Data_Available(uint8_t target);

/*! \fn int8_t SP_MM_Iface_Verify_Tail(uint8_t target)
    \brief Check if the tail of a VQ matches the shared and local copy
    \param target target SP to MM interface
    \return success or error code
*/
int8_t SP_MM_Iface_Verify_Tail(uint8_t target);


/*! \fn vq_cb_t* SP_MM_Iface_Get_VQ_Base_Addr(uint8_t target)
    \brief Provides pointer to virtual queue control block
    \param target target SP to MM interface
    \return Pointer to VQ or NULL
*/
vq_cb_t*  SP_MM_Iface_Get_VQ_Base_Addr(uint8_t target);

#endif /* __SP_MM_IFACE_H__ */