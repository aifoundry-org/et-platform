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
/*! \file sp_iface.h
    \brief A C header that defines the Service Processor public
    interfaces.
*/
/***********************************************************************/
#ifndef SP_IFACE_DEFS_H
#define SP_IFACE_DEFS_H

/* mm_rt_svcs */
#include <etsoc/common/common_defs.h>
#include <transports/sp_mm_iface/sp_mm_iface.h> /* header from shared/helper lib */
#include <transports/sp_mm_iface/sp_mm_comms_spec.h>

/*! \def TIMEOUT_SP_IFACE_RESPONSE(x)
    \brief Timeout value (per 100ms) for SP response wait
*/
#define TIMEOUT_SP_IFACE_RESPONSE(x) (x * 2U)

/*! \def SP_IFACE_MM_HEARTBEAT_INTERVAL(x)
    \brief Periodic Interval value after which a heartbeat is sent to SP
*/
#define SP_IFACE_MM_HEARTBEAT_INTERVAL(x) (x * 1U)

/*! \fn int32_t SP_Iface_Init(void)
    \brief Initialize Mm interface to Service Processor (SP)
    \return Status indicating success or negative error
*/
int32_t SP_Iface_Init(void);

/*! \fn int32_t SP_Iface_Push_Cmd_To_MM2SP_SQ(void* p_cmd, uint32_t cmd_size)
    \brief Push command to Master Minion (MM) to Service Processor (SP)
    Submission Queue(SQ)
    \param p_cmd Pointer to command buffer
    \param cmd_size Size of command
    \return Status indicating success or negative error
*/
#define SP_Iface_Push_Cmd_To_MM2SP_SQ(p_cmd, cmd_size) SP_MM_Iface_Push(SP_SQ, p_cmd, cmd_size)

/*! \fn int32_t SP_Iface_Pop_Cmd_From_MM2SP_CQ(void* rx_buff)
    \brief Pop response from to Master Minion (MM) to Service Processor (SP)
    Completion Queue(CQ)
    \param rx_buff Buffer to receive response popped
    \return Number of bytes popped from Queue
*/
#define SP_Iface_Pop_Rsp_From_MM2SP_CQ(rx_buff) SP_MM_Iface_Pop(SP_CQ, rx_buff)

/*! \fn int32_t SP_Iface_Push_Rsp_To_SP2MM_CQ(void* p_cmd, uint32_t cmd_size)
    \brief Push command to Service Processor (SP) to Master Minion (MM)
    Completion Queue(CQ)
    \param p_cmd Pointer to command buffer
    \param cmd_size Size of command
    \return Number of bytes pushed into Queue
*/
#define SP_Iface_Push_Rsp_To_SP2MM_CQ(p_cmd, cmd_size) SP_MM_Iface_Push(MM_CQ, p_cmd, cmd_size)

/*! \fn int32_t SP_Iface_Pop_Cmd_From_SP2MM_SQ(void* rx_buff)
    \brief Pop response from to Service Processor (SP) to Master Minion (MM)
    Submission Queue(SQ)
    \param rx_buff Buffer to receive response popped
    \return Number of bytes popped from Queue
*/
#define SP_Iface_Pop_Cmd_From_SP2MM_SQ(rx_buff) SP_MM_Iface_Pop(MM_SQ, rx_buff)

/*! \fn int32_t SP_Iface_Processing(vq_cb_t* vq_cached , vq_cb_t *vq_shared,
    uint32_t vq_used_space, void *const shared_mem_ptr)
    \brief Prefetches the data from a virtual queue.
    \param vq_cached Pointer to cached virtual queue control block.
    \param vq_shared Pointer to shared virtual queue control block.
    \param shared_mem_ptr Pointer to the VQ shared memory buffer used as
    circular buffer
    \param vq_used_space Number of bytes used in VQ
    \return Status indicating success or negative error
*/
int32_t SP_Iface_Processing(
    vq_cb_t *vq_cached, vq_cb_t *vq_shared, void *shared_mem_ptr, uint64_t vq_used_space);

/*! \fn int32_t SP_Iface_Get_Shire_Mask(uint64_t *shire_mask)
    \brief A blocking API to obtain active compute shires mask from service processor
    \param shire_mask Pointer to shire mask variable, as return arg.
    \return Status indicating success or negative error
*/
int32_t SP_Iface_Get_Shire_Mask_And_Strap(uint64_t *shire_mask, uint8_t *lvdpl_strap);

/*! \fn int32_t SP_Iface_Reset_Minion(uint64_t shire_mask)
    \brief A blocking API to reset the Minions.
    \param shire_mask Mask of the shires to reset.
    \return Status indicating success or negative error
*/
int32_t SP_Iface_Reset_Minion(uint64_t shire_mask);

/*! \fn int32_t SP_Iface_Get_Boot_Freq(uint32_t *boot_freq)
    \brief A blocking API to obtain compute minin boot frequency from service processor
    \param boot_freq Pointer to boot frequency variable, as return arg.
    \return Status indicating success or negative error
*/
int32_t SP_Iface_Get_Boot_Freq(uint32_t *boot_freq);

/*! \fn int32_t SP_Iface_Get_Fw_Version(enum mm2sp_fw_type_e fw_type, uint16_t *major, uint16_t *minor,
    uint16_t *patch)
    \brief A blocking API to obtain FW version from service processor
    \param fw_type Type of the FW
    \param major Pointer to variable where major version is stored
    \param minor Pointer to variable where minor version is stored
    \param revision Pointer to variable where revision version is stored
    \return Status indicating success or negative error
*/
int32_t SP_Iface_Get_Fw_Version(
    mm2sp_fw_type_e fw_type, uint8_t *major, uint8_t *minor, uint8_t *revision);

/*! \fn int32_t SP_Iface_Get_DDR_Memory_Info(uint8_t *mem_size)
    \brief A blocking call to obtain DDR memory size from SP
    \param mem_size    DDR memory size
*/
int32_t SP_Iface_Get_DDR_Memory_Info(uint64_t *mem_size);

/*! \fn int32_t SP_Iface_Report_Error(mm2sp_error_type_e error_type, int16_t error_code)
    \brief A non-blocking API to report Master Minion error codes to Service Processor
    \param error_type Error type
    \param error_code Error Code
    \return Status indicating success or negative error
*/
int32_t SP_Iface_Report_Error(mm2sp_error_type_e error_type, int16_t error_code);

/*! \fn int32_t SP_Iface_Setup_MM_HeartBeat(void)
    \brief This function initializes the MM->SP heartbeat with a periodic timer.
    \return Status indicating success or negative error
*/
int32_t SP_Iface_Setup_MM_HeartBeat(void);

#endif
