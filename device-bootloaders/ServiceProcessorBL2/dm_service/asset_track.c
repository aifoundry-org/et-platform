#include "dm.h"
#include "dm_service.h"
#include "sp_host_iface.h"
#include "bl2_asset_trk.h"

static uint32_t PCIE_SPEED[5] = { PCIE_GEN_1, PCIE_GEN_2, PCIE_GEN_3, PCIE_GEN_4, PCIE_GEN_5 };

static int64_t dm_svc_asset_getmanufacturername(char *mfgName)
{
    // TODO : Retrieve this parameter from BL2 partition header SW-4327
    strcpy(mfgName, "Esperant");
    return 0;
}

static int64_t dm_svc_asset_getpartnumber(char *partNumber)
{
    // TODO : Retrieve this parameter from BL2 partition header SW-4327
    strcpy(partNumber, "ETPART01");
    return 0;
}

static int64_t dm_svc_asset_getserialnumber(char *serNumber)
{
    // TODO : Retrieve this parameter from BL2 partition header SW-4327
    strcpy(serNumber, "ETSERNO1");
    return 0;
}

static int64_t dm_svc_asset_getchiprevision(char *chipRev)
{
    uint64_t chipRevision;
    printf("Silicon Revision Info...\n");
    OTP_SILICON_REVISION_t silicon_revision;
    if (0 != sp_otp_get_silicon_revision(&silicon_revision)) {
        printf("sp_otp_get_silicon_revision() failed!\n");
        return -1;
    }

    chipRevision =
        (uint32_t)((silicon_revision.B.si_major_rev << 4) | silicon_revision.B.si_minor_rev);

    sprintf(chipRev, "%ld", chipRevision);

    return 0;
}

static int64_t dm_svc_asset_getPCIEspeed(char *pcieSpeed)
{
    uint32_t pcieGen, tmp;
    uint64_t pcieSpeedtmp;
    tmp = ioread32(PCIE_CUST_SS +
                   DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_LINK_DBG_2_ADDRESS);

    // Get the PCIE Gen
    pcieGen = DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_LINK_DBG_2_RATE_GET(tmp);

    // Get the speed
    pcieSpeedtmp = PCIE_SPEED[pcieGen - 1];

    sprintf(pcieSpeed, "%ld", pcieSpeedtmp);

    return 0;
}

static int64_t dm_svc_asset_getmodulerev(char *moduleRevision)
{
    // TODO : Retrieve this parameter from BL2 partition header SW-4327
    uint64_t revision = 1;
    sprintf(moduleRevision, "%ld", revision);
    return 0;
}

static int64_t dm_svc_asset_getformfactor(char *formFactor)
{
    // TODO : Retrieve this parameter from BL2 partition header SW-4327
    strcpy(formFactor, "Dual_M2");
    return 0;
}

static int64_t dm_svc_asset_getmemorydetails(char *memVendor, char *memPart)
{
    //TODO: Retrieve this parameter from SP BL2 DDR discovery SW-4354
    strcpy(memVendor, "Micron");
    strcpy(memPart, "ETDDR4");
    return 0;
}

static int64_t dm_svc_asset_getmemorysize(char *memSize)
{
    //TODO: Retrieve this parameter from SP BL2 DDR discovery SW-4354
    sprintf(memSize, "%ld", (uint64_t)16);
    return 0;
}

static int64_t dm_svc_asset_getmemorytype(char *memType)
{
    //TODO: Retrieve this parameter from SP BL2 DDR discovery SW-4354
    strcpy(memType, "ETLPDDR4");
    return 0;
}

#ifndef IMPLEMENTATION_BYPASS 
static int64_t asset_tracking_send_response(mbox_e mbox, uint32_t cmd_id, uint64_t req_start_time,
                                            char asset_info[])
{
    int64_t ret = 0;
    uint64_t rsp_complete_time;
    printf("asset_tracking_send_response -start\n");
    rsp_complete_time = timer_get_ticks_count();
    memset(&dm_cmd_rsp, 0, sizeof(struct dm_control_block));
    dm_cmd_rsp.cmd_id = cmd_id;
    dm_cmd_rsp.dev_latency = (rsp_complete_time - req_start_time);
    strncpy(dm_cmd_rsp.cmd_payload, asset_info, 8);
    printf("cmd_id: %d  dev_latency:%ld  rsp_payload: %s \r\n", dm_cmd_rsp.cmd_id,
           dm_cmd_rsp.dev_latency, dm_cmd_rsp.cmd_payload);
    ret = MBOX_send(mbox, &dm_cmd_rsp, sizeof(struct dm_control_block));
    printf("asset_tracking_send_response -end\n");
    return ret;
}

void asset_tracking_process_request(mbox_e mbox, uint32_t cmd_id)
{
    int64_t ret = 0;
    char req_asset_info[8] = { 0 }, mem_part[8] = { 0 };
    uint64_t req_start_time;

    req_start_time = timer_get_ticks_count();

    switch (cmd_id) {
    case DM_CMD_GET_MODULE_MANUFACTURE_NAME: {
        ret = dm_svc_asset_getmanufacturername(req_asset_info);
    } break;

    case DM_CMD_GET_MODULE_PART_NUMBER: {
        ret = dm_svc_asset_getpartnumber(req_asset_info);
    } break;

    case DM_CMD_GET_MODULE_SERIAL_NUMBER: {
        ret = dm_svc_asset_getserialnumber(req_asset_info);
    } break;

    case DM_CMD_GET_ASIC_CHIP_REVISION: {
        ret = dm_svc_asset_getchiprevision(req_asset_info);
    } break;

    case DM_CMD_GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED: {
        ret = dm_svc_asset_getPCIEspeed(req_asset_info);
    } break;

    case DM_CMD_GET_MODULE_REVISION: {
        ret = dm_svc_asset_getmodulerev(req_asset_info);
    } break;

    case DM_CMD_GET_MODULE_FORM_FACTOR: {
        ret = dm_svc_asset_getformfactor(req_asset_info);
    } break;

    case DM_CMD_GET_MODULE_MEMORY_VENDOR_PART_NUMBER: {
        ret = dm_svc_asset_getmemorydetails(req_asset_info, mem_part);
    } break;

    case DM_CMD_GET_MODULE_MEMORY_SIZE_MB: {
        ret = dm_svc_asset_getmemorysize(req_asset_info);
    } break;

    case DM_CMD_GET_MODULE_MEMORY_TYPE: {
        ret = dm_svc_asset_getmemorytype(req_asset_info);
    } break;
    }

    if (!ret) {
        printf("cmd_id : %d   response: %s\n", cmd_id, req_asset_info);
        ret = asset_tracking_send_response(mbox, cmd_id, req_start_time, req_asset_info);
    } else {
        printf("cmd_id : %d   error %ld\r\n", cmd_id, ret);
    }

    if (ret != 0) {
        printf("MBOX Send error %ld\r\n", ret);
    }
}

#else
static void asset_tracking_send_response(tag_id_t tag_id, uint64_t req_start_time, char asset_info[])
{
    struct device_mgmt_asset_tracking_rsp_t dm_rsp;

    strncpy(dm_rsp.asset_info.asset, asset_info, 8);

    FILL_RSP_HEADER(dm_rsp, tag_id,
                    DM_CMD_GET_MM_THREADS_STATE,
                    timer_get_ticks_count() - req_start_time,
                    DM_STATUS_SUCCESS);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_asset_tracking_rsp_t))) {
        printf("asset_tracking_send_response: Cqueue push error !\n");
    }
}

void asset_tracking_process_request(tag_id_t tag_id, msg_id_t msg_id)
{
    int64_t ret = 0;
    char req_asset_info[8] = { 0 }, mem_part[8] = { 0 };
    uint64_t req_start_time;

    req_start_time = timer_get_ticks_count();

    switch (msg_id) {
    case DM_CMD_GET_MODULE_MANUFACTURE_NAME: {
        ret = dm_svc_asset_getmanufacturername(req_asset_info);
    } break;

    case DM_CMD_GET_MODULE_PART_NUMBER: {
        ret = dm_svc_asset_getpartnumber(req_asset_info);
    } break;

    case DM_CMD_GET_MODULE_SERIAL_NUMBER: {
        ret = dm_svc_asset_getserialnumber(req_asset_info);
    } break;

    case DM_CMD_GET_ASIC_CHIP_REVISION: {
        ret = dm_svc_asset_getchiprevision(req_asset_info);
    } break;

    case DM_CMD_GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED: {
        ret = dm_svc_asset_getPCIEspeed(req_asset_info);
    } break;

    case DM_CMD_GET_MODULE_REVISION: {
        ret = dm_svc_asset_getmodulerev(req_asset_info);
    } break;

    case DM_CMD_GET_MODULE_FORM_FACTOR: {
        ret = dm_svc_asset_getformfactor(req_asset_info);
    } break;

    case DM_CMD_GET_MODULE_MEMORY_VENDOR_PART_NUMBER: {
        ret = dm_svc_asset_getmemorydetails(req_asset_info, mem_part);
    } break;

    case DM_CMD_GET_MODULE_MEMORY_SIZE_MB: {
        ret = dm_svc_asset_getmemorysize(req_asset_info);
    } break;

    case DM_CMD_GET_MODULE_MEMORY_TYPE: {
        ret = dm_svc_asset_getmemorytype(req_asset_info);
    } break;
    }

    if (!ret) {
        printf("cmd_id : %d   response: %s\n", msg_id, req_asset_info);
        asset_tracking_send_response(tag_id, req_start_time, req_asset_info);
    } else {
        printf("cmd_id : %d   error %ld\r\n", msg_id, ret);
    }
}

#endif
