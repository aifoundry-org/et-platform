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
#include "minion_debug.h"
#include "FreeRTOS.h"
#include "queue.h"

extern QueueHandle_t q_dm_mdi_bp_notify_handle;

static void send_status_response(tag_id_t tag_id, msg_id_t msg_id, uint64_t req_start_time,
                                 int32_t status)
{
    struct device_mgmt_default_rsp_t dm_rsp = { 0 };

    Log_Write(LOG_LEVEL_INFO, "MDI Response: %s\n", __func__);

    FILL_RSP_HEADER(dm_rsp, tag_id, msg_id, timer_get_ticks_count() - req_start_time, status)

    dm_rsp.payload = status;

    Log_Write(LOG_LEVEL_INFO, "Response for msg_id = %u, tag_id = %u, dm_rsp.payload:%u\n",
              dm_rsp.rsp_hdr.rsp_hdr.msg_id, dm_rsp.rsp_hdr.rsp_hdr.msg_id, dm_rsp.payload);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_default_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "send_status_response: Cqueue push error!\n");
    }
}

static void send_mdi_hart_selection_response(tag_id_t tag_id, msg_id_t msg_id,
                                             uint64_t req_start_time, int32_t status)
{
    struct mdi_hart_selection_rsp_t mdi_rsp = { 0 };

    Log_Write(LOG_LEVEL_INFO, "MDI Response: %s\n", __func__);

    FILL_RSP_HEADER(mdi_rsp, tag_id, msg_id, timer_get_ticks_count() - req_start_time, status)

    /* MDI lib function does not return status for Select/Unselect Hart operation. 
        Sending success status by default in response */
    mdi_rsp.status = (uint32_t)status;

    Log_Write(LOG_LEVEL_INFO, "Response for msg_id = %u, tag_id = %u, mdi_rsp.status:%u\n",
              mdi_rsp.rsp_hdr.rsp_hdr.msg_id, mdi_rsp.rsp_hdr.rsp_hdr.msg_id, mdi_rsp.status);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&mdi_rsp, sizeof(struct mdi_hart_selection_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "Cqueue push error!\n");
    }
}

static void send_mdi_hart_control_response(tag_id_t tag_id, msg_id_t msg_id,
                                           uint64_t req_start_time, int32_t status)
{
    struct mdi_hart_control_rsp_t mdi_rsp = { 0 };

    Log_Write(LOG_LEVEL_INFO, "MDI Response: %s\n", __func__);

    FILL_RSP_HEADER(mdi_rsp, tag_id, msg_id, timer_get_ticks_count() - req_start_time, SUCCESS)
    mdi_rsp.status = (uint32_t)status;

    Log_Write(LOG_LEVEL_INFO, "Response for msg_id = %u, tag_id = %u, mdi_rsp.status:%u\n",
              mdi_rsp.rsp_hdr.rsp_hdr.msg_id, mdi_rsp.rsp_hdr.rsp_hdr.msg_id, mdi_rsp.status);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&mdi_rsp, sizeof(struct mdi_hart_control_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "Cqueue push error!\n");
    }
}

static void send_mdi_bp_control_response(tag_id_t tag_id, msg_id_t msg_id, uint64_t req_start_time,
                                         int32_t status)
{
    struct mdi_bp_control_rsp_t mdi_rsp = { 0 };

    Log_Write(LOG_LEVEL_INFO, "MDI Response: %s\n", __func__);

    FILL_RSP_HEADER(mdi_rsp, tag_id, msg_id, timer_get_ticks_count() - req_start_time, status)

    /* MDI lib function does not return status currently for Set_PC_Breakpoint/Unset_PC_Breakpoint.
       Sending success status by default in response. */
    mdi_rsp.status = (uint32_t)status;

    Log_Write(LOG_LEVEL_INFO, "Response for msg_id = %u, tag_id = %u, mdi_rsp.status:%u\n",
              mdi_rsp.rsp_hdr.rsp_hdr.msg_id, mdi_rsp.rsp_hdr.rsp_hdr.msg_id, mdi_rsp.status);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&mdi_rsp, sizeof(struct mdi_bp_control_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "Cqueue push error!\n");
    }
}

static void send_mdi_ss_control_response(tag_id_t tag_id, msg_id_t msg_id, uint64_t req_start_time,
                                         int32_t status)
{
    struct mdi_ss_control_rsp_t mdi_rsp = { 0 };

    Log_Write(LOG_LEVEL_INFO, "MDI Response: %s\n", __func__);

    FILL_RSP_HEADER(mdi_rsp, tag_id, msg_id, timer_get_ticks_count() - req_start_time, status)
    mdi_rsp.status = (uint32_t)status;

    Log_Write(LOG_LEVEL_INFO, "Response for msg_id = %u, tag_id = %u, mdi_rsp.status:%u\n",
              mdi_rsp.rsp_hdr.rsp_hdr.msg_id, mdi_rsp.rsp_hdr.rsp_hdr.msg_id, mdi_rsp.status);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&mdi_rsp, sizeof(struct mdi_ss_control_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "Cqueue push error!\n");
    }
}

static void mdi_select_hart(tag_id_t tag_id, msg_id_t msg_id, uint64_t req_start_time, void *buffer)
{
    int32_t status = SUCCESS;

    Log_Write(LOG_LEVEL_INFO, "MDI Request: DM_CMD_MDI_SELECT_HART\n");

    const struct mdi_hart_selection_cmd_t *mdi_cmd_req = (struct mdi_hart_selection_cmd_t *)buffer;

    /* Select a Hart */
    for (uint8_t neigh_id = 0; neigh_id < NUM_NEIGH_PER_SHIRE; neigh_id++)
    {
        if (mdi_cmd_req->cmd_attr.thread_mask & (0xFFFFULL << (HARTS_PER_NEIGH * neigh_id)))
        {
            Select_Harts((uint8_t)mdi_cmd_req->cmd_attr.shire_id, neigh_id);
        }
    }

    /* MDI lib function does not return status for Select/Unselect Hart operation. 
        Sending success status by default in response */
    send_mdi_hart_selection_response(tag_id, msg_id, req_start_time, status);
}

static void mdi_unselect_hart(tag_id_t tag_id, msg_id_t msg_id, uint64_t req_start_time,
                              void *buffer)
{
    int32_t status = SUCCESS;

    Log_Write(LOG_LEVEL_INFO, "MDI Request: DM_CMD_MDI_UNSELECT_HART\n");

    const struct mdi_hart_selection_cmd_t *mdi_cmd_req = (struct mdi_hart_selection_cmd_t *)buffer;

    /* Unselect a Hart */
    for (uint8_t neigh_id = 0; neigh_id < NUM_NEIGH_PER_SHIRE; neigh_id++)
    {
        if (mdi_cmd_req->cmd_attr.thread_mask & (0xFFFFULL << (HARTS_PER_NEIGH * neigh_id)))
        {
            Unselect_Harts((uint8_t)mdi_cmd_req->cmd_attr.shire_id, neigh_id);
        }
    }

    /* MDI lib function does not return status for Select/Unselect Hart operation. 
        Sending success status by default in response */
    send_mdi_hart_selection_response(tag_id, msg_id, req_start_time, status);
}

static void mdi_halt_hart(tag_id_t tag_id, msg_id_t msg_id, uint64_t req_start_time)
{
    Log_Write(LOG_LEVEL_INFO, "MDI Request: DM_CMD_MDI_HALT_HART\n");
    int32_t status = Halt_Harts() ? SUCCESS : MDI_CORE_NOT_HALTED;
    send_mdi_hart_control_response(tag_id, msg_id, req_start_time, status);
}

static void mdi_resume_hart(tag_id_t tag_id, msg_id_t msg_id, uint64_t req_start_time)
{
    Log_Write(LOG_LEVEL_INFO, "MDI Request: DM_CMD_MDI_RESUME_HART\n");
    int32_t status = Resume_Harts() ? SUCCESS : MDI_CORE_NOT_RESUME;
    send_mdi_hart_control_response(tag_id, msg_id, req_start_time, status);
}

static void mdi_reset_hart(tag_id_t tag_id, msg_id_t msg_id, uint64_t req_start_time)
{
    bool ret = false;

    Log_Write(LOG_LEVEL_INFO, "MDI Request: DM_CMD_MDI_RESET_HART\n");

    /* MDI lib does not support reset hart functionality yet */

    send_mdi_hart_control_response(tag_id, msg_id, req_start_time, ret);
}

static int32_t get_hart_status(uint64_t hart_id)
{
    int32_t hart_status = -1;

    if (HART_HALT_STATUS(hart_id))
    {
        return MDI_HART_STATUS_HALTED;
    }

    if (HART_RUNNING_STATUS(hart_id))
    {
        return MDI_HART_STATUS_RUNNING;
    }

    if (HART_EXCEPTION_STATUS(hart_id))
    {
        return MDI_HART_STATUS_EXCEPTION;
    }

    if (HART_ERROR_STATUS(hart_id))
    {
        return MDI_HART_STATUS_ERROR;
    }

    return hart_status;
}

static void mdi_hart_status(tag_id_t tag_id, msg_id_t msg_id, uint64_t req_start_time, void *buffer)
{
    Log_Write(LOG_LEVEL_INFO, "MDI Request: DM_CMD_MDI_GET_HART_STATUS\n");
    const struct mdi_hart_control_cmd_t *mdi_cmd_req = (struct mdi_hart_control_cmd_t *)buffer;

    /* Get the status of the Hart  */
    int32_t status = get_hart_status(mdi_cmd_req->cmd_attr.hart_id);

    Log_Write(LOG_LEVEL_INFO, "Hart ID:%ld  Hart Status: %d\n", mdi_cmd_req->cmd_attr.hart_id,
              status);

    send_mdi_hart_control_response(tag_id, msg_id, req_start_time, status);
}

static void mdi_set_breakpoint(tag_id_t tag_id, msg_id_t msg_id, uint64_t req_start_time,
                               void *buffer)
{
    int32_t status = SUCCESS;
    struct mdi_bp_control_cmd_t *mdi_cmd_req = (struct mdi_bp_control_cmd_t *)buffer;

    Log_Write(LOG_LEVEL_INFO, "MDI Request: DM_CMD_MDI_SET_BREAKPOINT\n");

    Log_Write(LOG_LEVEL_INFO, "Hart ID: %ld, BP Address: %lx, Mode:%ld  Timeout: %ld\n",
              mdi_cmd_req->cmd_attr.hart_id, mdi_cmd_req->cmd_attr.bp_address,
              mdi_cmd_req->cmd_attr.mode, mdi_cmd_req->cmd_attr.bp_event_wait_timeout);

    Set_PC_Breakpoint(mdi_cmd_req->cmd_attr.hart_id, mdi_cmd_req->cmd_attr.bp_address,
                      mdi_cmd_req->cmd_attr.mode);
 
    /* Send BP Halt success/failure event to host */
    xQueueSend(q_dm_mdi_bp_notify_handle, (void*)mdi_cmd_req, portMAX_DELAY);

    /* Send MDI Set BP cmd response */
    send_mdi_bp_control_response(tag_id, msg_id, req_start_time, status);

}

static void mdi_unset_breakpoint(tag_id_t tag_id, msg_id_t msg_id, uint64_t req_start_time,
                                 void *buffer)
{
    int32_t status = SUCCESS;
    const struct mdi_bp_control_cmd_t *mdi_cmd_req = (struct mdi_bp_control_cmd_t *)buffer;

    Log_Write(LOG_LEVEL_INFO, "MDI Request: DM_CMD_MDI_UNSET_BREAKPOINT\n");

    Log_Write(LOG_LEVEL_INFO, "Hart ID: %ld\n", mdi_cmd_req->cmd_attr.hart_id);

    Unset_PC_Breakpoint(mdi_cmd_req->cmd_attr.hart_id);

    send_mdi_bp_control_response(tag_id, msg_id, req_start_time, status);
}

static void mdi_enable_single_step(tag_id_t tag_id, msg_id_t msg_id, uint64_t req_start_time)
{
    int32_t status = -1;

    Log_Write(LOG_LEVEL_INFO, "MDI Request: DM_CMD_MDI_ENABLE_SINGLE_STEP\n");

    /* Not supported in MDI lib */
    /* https://esperantotech.atlassian.net/browse/SW-11247 */

    send_mdi_ss_control_response(tag_id, msg_id, req_start_time, status);
}

static void mdi_disable_single_step(tag_id_t tag_id, msg_id_t msg_id, uint64_t req_start_time)
{
    int32_t status = -1;

    Log_Write(LOG_LEVEL_INFO, "MDI Request: DM_CMD_MDI_DISABLE_SINGLE_STEP\n");

    /* Not supported in MDI lib */
    /* https://esperantotech.atlassian.net/browse/SW-11247 */

    send_mdi_ss_control_response(tag_id, msg_id, req_start_time, status);
}

static void mdi_read_gpr(tag_id_t tag_id, msg_id_t msg_id, uint64_t req_start_time, void *buffer)
{
    int32_t status = SUCCESS;
    const struct mdi_read_gpr_cmd_t *mdi_cmd_req = (struct mdi_read_gpr_cmd_t *)buffer;
    struct mdi_read_gpr_rsp_t mdi_rsp;

    Log_Write(LOG_LEVEL_INFO, "MDI Request: DM_CMD_MDI_READ_GPR\n");

    if (mdi_cmd_req->cmd_attr.gpr_index < NO_OF_GPR_REGS)
    {
        mdi_rsp.data = Read_GPR(mdi_cmd_req->cmd_attr.hart_id, mdi_cmd_req->cmd_attr.gpr_index);
        Log_Write(LOG_LEVEL_INFO, "GPR Index:%x, GPR value:%lx\n", mdi_cmd_req->cmd_attr.gpr_index,
                  mdi_rsp.data);
    }
    else
    {
        Log_Write(LOG_LEVEL_WARNING, "Invalid GPR Read Request\r\n");
        status = MDI_INVALID_GPR_INDEX;
    }

    FILL_RSP_HEADER(mdi_rsp, tag_id, msg_id, timer_get_ticks_count() - req_start_time, status)

    Log_Write(LOG_LEVEL_INFO, "Response for msg_id = %u, tag_id = %u\n",
              mdi_rsp.rsp_hdr.rsp_hdr.msg_id, mdi_rsp.rsp_hdr.rsp_hdr.msg_id);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&mdi_rsp, sizeof(struct mdi_read_gpr_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "Cqueue push error!\n");
    }
}

static void mdi_dump_gpr(tag_id_t tag_id, msg_id_t msg_id, uint64_t req_start_time, void *buffer)
{
    int32_t status = SUCCESS;
    const struct mdi_dump_gpr_cmd_t *mdi_cmd_req = (struct mdi_dump_gpr_cmd_t *)buffer;
    struct mdi_dump_gpr_rsp_t mdi_rsp;

    Log_Write(LOG_LEVEL_INFO, "MDI Request: DM_CMD_MDI_DUMP_GPR\n");

    const uint64_t *gprs = Read_All_GPR(mdi_cmd_req->hart_id);

    FILL_RSP_HEADER(mdi_rsp, tag_id, msg_id, timer_get_ticks_count() - req_start_time, status)
    memcpy(&mdi_rsp.gprs, gprs, sizeof(uint64_t) * NO_OF_GPR_REGS);

    Log_Write(LOG_LEVEL_INFO, "Response for msg_id = %u, tag_id = %u\n",
              mdi_rsp.rsp_hdr.rsp_hdr.msg_id, mdi_rsp.rsp_hdr.rsp_hdr.msg_id);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&mdi_rsp, sizeof(struct mdi_read_gpr_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "Cqueue push error!\n");
    }
}

static void mdi_write_gpr(tag_id_t tag_id, msg_id_t msg_id, uint64_t req_start_time, void *buffer)
{
    int32_t status = SUCCESS;
    const struct mdi_write_gpr_cmd_t *mdi_cmd_req = (struct mdi_write_gpr_cmd_t *)buffer;

    Log_Write(LOG_LEVEL_INFO, "MDI Request: DM_CMD_MDI_WRITE_GPR\n");

    Log_Write(LOG_LEVEL_INFO, "Hart ID: %ld, GPR:%x, Data:%lx\n", mdi_cmd_req->cmd_attr.hart_id,
              mdi_cmd_req->cmd_attr.gpr_index, mdi_cmd_req->cmd_attr.data);

    Write_GPR(mdi_cmd_req->cmd_attr.hart_id, mdi_cmd_req->cmd_attr.gpr_index,
              mdi_cmd_req->cmd_attr.data);

    send_status_response(tag_id, msg_id, req_start_time, status);
}

static void mdi_write_csr(tag_id_t tag_id, msg_id_t msg_id, uint64_t req_start_time, void *buffer)
{
    int32_t status = SUCCESS;
    const struct mdi_write_csr_cmd_t *mdi_cmd_req = (struct mdi_write_csr_cmd_t *)buffer;

    Log_Write(LOG_LEVEL_INFO, "MDI Request: DM_CMD_MDI_WRITE_CSR\n");

    uint32_t csr_name = (mdi_cmd_req->cmd_attr.csr_name == GDB_RISCV_PC_INDEX) ?
                            MINION_CSR_DPC_OFFSET :
                            mdi_cmd_req->cmd_attr.csr_name;

    Log_Write(LOG_LEVEL_INFO, "Hart ID: %ld, CSR:%x, Data:%lx\n", mdi_cmd_req->cmd_attr.hart_id,
              csr_name, mdi_cmd_req->cmd_attr.data);

    Write_CSR(mdi_cmd_req->cmd_attr.hart_id, csr_name, mdi_cmd_req->cmd_attr.data);

    send_status_response(tag_id, msg_id, req_start_time, status);
}

static void mdi_read_csr(tag_id_t tag_id, msg_id_t msg_id, uint64_t req_start_time, void *buffer)
{
    int32_t status = SUCCESS;
    const struct mdi_read_csr_cmd_t *mdi_cmd_req = (struct mdi_read_csr_cmd_t *)buffer;
    struct mdi_read_csr_rsp_t mdi_rsp;

    Log_Write(LOG_LEVEL_INFO, "MDI Request: DM_CMD_MDI_READ_CSR\n");

    uint32_t csr_name = (mdi_cmd_req->cmd_attr.csr_name == GDB_RISCV_PC_INDEX) ?
                            MINION_CSR_DPC_OFFSET :
                            mdi_cmd_req->cmd_attr.csr_name;

    mdi_rsp.data = Read_CSR(mdi_cmd_req->cmd_attr.hart_id, csr_name);

    FILL_RSP_HEADER(mdi_rsp, tag_id, msg_id, timer_get_ticks_count() - req_start_time, status)

    Log_Write(LOG_LEVEL_INFO, "Response for msg_id = %u, tag_id = %u, CSR Value:%lx\n",
              mdi_rsp.rsp_hdr.rsp_hdr.msg_id, mdi_rsp.rsp_hdr.rsp_hdr.msg_id, mdi_rsp.data);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&mdi_rsp, sizeof(struct mdi_read_csr_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "Cqueue push error!\n");
    }
}

static void mdi_mem_read(tag_id_t tag_id, msg_id_t msg_id, uint64_t req_start_time, void *buffer)
{
    int32_t status = SUCCESS;
    const struct mdi_mem_read_cmd_t *mdi_cmd_req = (struct mdi_mem_read_cmd_t *)buffer;
    struct mdi_mem_read_rsp_t mdi_rsp = { 0 };

    Log_Write(LOG_LEVEL_INFO, "MDI Request: DM_CMD_MDI_READ_MEM\n");

    if (mdi_cmd_req->cmd_attr.address >= HOST_MANAGED_DRAM_START &&
        mdi_cmd_req->cmd_attr.address <= HOST_MANAGED_DRAM_END)
    {
        status = ETSOC_Memory_Read_Write_Cacheable((const void *)mdi_cmd_req->cmd_attr.address,
                                                   &mdi_rsp.data, mdi_cmd_req->cmd_attr.size);

        Log_Write(LOG_LEVEL_DEBUG, "Mem Read Status:%d  Read address: %lx, Value: %lx\r\n", status,
                  mdi_cmd_req->cmd_attr.address, mdi_rsp.data);
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "MDI Invalid Memory read request!\n");
        status = MDI_INVALID_MEMORY_READ_REQUEST;
    }

    FILL_RSP_HEADER(mdi_rsp, tag_id, msg_id, timer_get_ticks_count() - req_start_time, status)
    Log_Write(LOG_LEVEL_INFO, "Response for msg_id = %u, tag_id = %u\n",
              mdi_rsp.rsp_hdr.rsp_hdr.msg_id, mdi_rsp.rsp_hdr.rsp_hdr.msg_id);
    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&mdi_rsp, sizeof(struct mdi_mem_read_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "Cqueue push error!\n");
    }
}

static void mdi_mem_write(tag_id_t tag_id, msg_id_t msg_id, uint64_t req_start_time, void *buffer)
{
    int32_t status = SUCCESS;
    const struct mdi_mem_write_cmd_t *mdi_cmd_req = (struct mdi_mem_write_cmd_t *)buffer;
    struct mdi_mem_write_rsp_t mdi_rsp = { 0 };

    Log_Write(LOG_LEVEL_INFO, "MDI Request: DM_CMD_MDI_WRITE_MEM\n");

    status = ETSOC_Memory_Read_Write_Cacheable(&mdi_cmd_req->cmd_attr.data,
                                               (void *)mdi_cmd_req->cmd_attr.address,
                                               mdi_cmd_req->cmd_attr.size);

    //  SP to request CM to perform memory write using the program buffers interface.
    //  https://esperantotech.atlassian.net/browse/SW-11458

    Log_Write(LOG_LEVEL_DEBUG, "Write address: 0x%lx, Data: %lx\n, Size:%d  Status:%d\r\n",
              mdi_cmd_req->cmd_attr.address, mdi_cmd_req->cmd_attr.data, mdi_cmd_req->cmd_attr.size,
              status);

    FILL_RSP_HEADER(mdi_rsp, tag_id, msg_id, timer_get_ticks_count() - req_start_time, status)
    Log_Write(LOG_LEVEL_INFO, "Response for msg_id = %u, tag_id = %u, mdi_rsp.status:%u\n",
              mdi_rsp.rsp_hdr.rsp_hdr.msg_id, mdi_rsp.rsp_hdr.rsp_hdr.msg_id, status);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&mdi_rsp, sizeof(struct mdi_mem_write_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "Cqueue push error!\n");
    }
}

void minion_debug_request(tag_id_t tag_id, msg_id_t msg_id, void *buffer)
{
    uint64_t req_start_time;
    req_start_time = timer_get_ticks_count();

    switch (msg_id)
    {
        case DM_CMD_MDI_SELECT_HART:
            mdi_select_hart(tag_id, msg_id, req_start_time, buffer);
            break;

        case DM_CMD_MDI_UNSELECT_HART:
            mdi_unselect_hart(tag_id, msg_id, req_start_time, buffer);
            break;

        case DM_CMD_MDI_HALT_HART:
            mdi_halt_hart(tag_id, msg_id, req_start_time);
            break;

        case DM_CMD_MDI_RESUME_HART:
            mdi_resume_hart(tag_id, msg_id, req_start_time);
            break;

        case DM_CMD_MDI_RESET_HART:
            mdi_reset_hart(tag_id, msg_id, req_start_time);
            break;

        case DM_CMD_MDI_GET_HART_STATUS:
            mdi_hart_status(tag_id, msg_id, req_start_time, buffer);
            break;

        case DM_CMD_MDI_SET_BREAKPOINT:
            mdi_set_breakpoint(tag_id, msg_id, req_start_time, buffer);
            break;

        case DM_CMD_MDI_UNSET_BREAKPOINT:
            mdi_unset_breakpoint(tag_id, msg_id, req_start_time, buffer);
            break;

        case DM_CMD_MDI_ENABLE_SINGLE_STEP:
            mdi_enable_single_step(tag_id, msg_id, req_start_time);
            break;

        case DM_CMD_MDI_DISABLE_SINGLE_STEP:
            mdi_disable_single_step(tag_id, msg_id, req_start_time);
            break;

        case DM_CMD_MDI_READ_GPR:
            mdi_read_gpr(tag_id, msg_id, req_start_time, buffer);
            break;

        case DM_CMD_MDI_DUMP_GPR:
            mdi_dump_gpr(tag_id, msg_id, req_start_time, buffer);
            break;

        case DM_CMD_MDI_WRITE_GPR:
            mdi_write_gpr(tag_id, msg_id, req_start_time, buffer);
            break;

        case DM_CMD_MDI_READ_CSR:
            mdi_read_csr(tag_id, msg_id, req_start_time, buffer);
            break;

        case DM_CMD_MDI_WRITE_CSR:
            mdi_write_csr(tag_id, msg_id, req_start_time, buffer);
            break;

        case DM_CMD_MDI_READ_MEM:
            mdi_mem_read(tag_id, msg_id, req_start_time, buffer);
            break;

        case DM_CMD_MDI_WRITE_MEM:
            mdi_mem_write(tag_id, msg_id, req_start_time, buffer);
            break;

        default:
            Log_Write(LOG_LEVEL_ERROR, "[PC VQ] Invalid message id: %d\r\n", msg_id);
            break;
    }
}
