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
/*! \file asset_track.c
    \brief A C module that implements the Asset Tracking service

    Public interfaces:
        asset_tracking_process_request
*/
/***********************************************************************/
#include "bl2_asset_trk.h"

static int32_t asset_svc_getmanufacturername(char *mfg_name)
{
    return get_manufacturer_name(mfg_name);
}

static int32_t asset_svc_getpartnumber(char *part_number)
{
    return get_part_number(part_number);
}

static int32_t
asset_svc_setpartnumber(const struct device_mgmt_set_module_part_number_cmd_t *dm_cmd)
{
    return set_part_number(dm_cmd->part_number);
}

static int32_t asset_svc_getserialnumber(char *ser_number)
{
    return get_serial_number(ser_number);
}

static int32_t asset_svc_getchiprevision(char *chip_rev)
{
    return get_chip_revision(chip_rev);
}

static int32_t asset_svc_getPCIEspeed(char *pcie_speed)
{
    return get_PCIE_speed(pcie_speed);
}

static int32_t asset_svc_getmodulerev(char *module_revision)
{
    return get_module_rev(module_revision);
}

static int32_t asset_svc_getformfactor(char *form_factor)
{
    return get_form_factor(form_factor);
}

static int32_t asset_svc_getmemoryvendorID(char *vendorID)
{
    return get_memory_vendor_ID(vendorID);
}

static int32_t asset_svc_getmemorysize(char *mem_size)
{
    return get_memory_size(mem_size);
}

static int32_t asset_svc_getmemorytype(char *mem_type)
{
    return get_memory_type(mem_type);
}

static int32_t asset_svc_getvminlut(char *vmin_lut)
{
    return get_vmin_lut(vmin_lut);
}

static int32_t asset_svc_setvminlut(const struct device_mgmt_set_vmin_lut_cmd_t *dm_cmd)
{
    return set_vmin_lut(dm_cmd->lut);
}

static void asset_tracking_send_response(tag_id_t tag_id, msg_id_t msg_id, uint64_t req_start_time,
                                         char asset_info[], int32_t status)
{
    struct device_mgmt_asset_tracking_rsp_t dm_rsp;

    Log_Write(LOG_LEVEL_DEBUG, "Response for msg_id = %u, tag_id = %u\n", msg_id, tag_id);

    memcpy(&dm_rsp.asset_info.asset, asset_info, sizeof(struct asset_info_t));

    FILL_RSP_HEADER(dm_rsp, tag_id, msg_id, timer_get_ticks_count() - req_start_time, status);

    if (0 !=
        SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_asset_tracking_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "asset_tracking_send_response: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       asset_tracking_process_request
*
*   DESCRIPTION
*
*       This function takes command ID as input from Host,
*       and accordingly calls the respective asset tracking
*       functions
*
*   INPUTS
*
*       msg_id      Unique enum representing specific command
*       buffer      Pointer to command buffer
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void asset_tracking_process_request(tag_id_t tag_id, msg_id_t msg_id, const void *buffer)
{
    int32_t ret = 0;
    char req_asset_info[sizeof(struct asset_info_t)] = { 0 };
    uint64_t req_start_time;

    Log_Write(LOG_LEVEL_INFO, "Asset tracking request id: %d\n", msg_id);

    req_start_time = timer_get_ticks_count();

    switch (msg_id)
    {
        case DM_CMD_GET_MODULE_MANUFACTURE_NAME:
            ret = asset_svc_getmanufacturername(req_asset_info);
            break;
        case DM_CMD_GET_MODULE_PART_NUMBER:
            ret = asset_svc_getpartnumber(req_asset_info);
            break;
        case DM_CMD_SET_MODULE_PART_NUMBER: {
            const struct device_mgmt_set_module_part_number_cmd_t *set_module_part_number_cmd =
                buffer;
            ret = asset_svc_setpartnumber(set_module_part_number_cmd);
            break;
        }
        case DM_CMD_GET_MODULE_SERIAL_NUMBER:
            ret = asset_svc_getserialnumber(req_asset_info);
            break;
        case DM_CMD_GET_ASIC_CHIP_REVISION:
            ret = asset_svc_getchiprevision(req_asset_info);
            break;
        case DM_CMD_GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED:
            ret = asset_svc_getPCIEspeed(req_asset_info);
            break;
        case DM_CMD_GET_MODULE_REVISION:
            ret = asset_svc_getmodulerev(req_asset_info);
            break;
        case DM_CMD_GET_MODULE_FORM_FACTOR:
            ret = asset_svc_getformfactor(req_asset_info);
            break;
        case DM_CMD_GET_MODULE_MEMORY_VENDOR_PART_NUMBER:
            ret = asset_svc_getmemoryvendorID(req_asset_info);
            break;
        case DM_CMD_GET_MODULE_MEMORY_SIZE_MB:
            ret = asset_svc_getmemorysize(req_asset_info);
            break;
        case DM_CMD_GET_MODULE_MEMORY_TYPE:
            ret = asset_svc_getmemorytype(req_asset_info);
            break;
        case DM_CMD_GET_VMIN_LUT:
            ret = asset_svc_getvminlut(req_asset_info);
            break;
        case DM_CMD_SET_VMIN_LUT: {
            const struct device_mgmt_set_vmin_lut_cmd_t *set_module_vmin_lut_cmd = buffer;
            ret = asset_svc_setvminlut(set_module_vmin_lut_cmd);
            break;
        }

        default:
            Log_Write(LOG_LEVEL_ERROR, "Unsupported Message ID: %d\n", msg_id);
    }

    asset_tracking_send_response(tag_id, msg_id, req_start_time, req_asset_info, ret);

    if (0 != ret)
    {
        Log_Write(LOG_LEVEL_ERROR, "Asset tracking svc error: %d\n", ret);
    }
}
