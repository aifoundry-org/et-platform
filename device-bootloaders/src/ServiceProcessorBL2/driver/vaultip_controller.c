/*-------------------------------------------------------------------------
* Copyright (C) 2018, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
************************************************************************/
/*! \file vaultip_controller.c
    \brief A C module that implements vaultip controller's sub system.

    Public interfaces:
        timer_init
        timer_get_ticks_count
*/
/***********************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "io.h"
#include "serial.h"
#include "crc32.h"
#include "etsoc_hal/inc/vaultip_hw.h"
#include "etsoc_hal/inc/vaultip_sw.h"
#include "etsoc_hal/inc/vaultip_sw_asset.h"
#include "vaultip_static_assets.h"
#include "system_registers.h"
#include "bl2_flash_fs.h"
#include "cache_flush_ops.h"
#include "bl2_reset.h"
#include "sp_otp.h"
/*#include "bl2_pll.h" */
#include "bl2_timer.h"
#include "bl2_vaultip_controller.h"
#include "etsoc_hal/inc/rm_esr.h"
#include "etsoc_hal/inc/spio_misc_esr.h"
#include "etsoc_hal/inc/hal_device.h"
#include "bl2_main.h"
#pragma GCC diagnostic ignored "-Wswitch-enum"

/* #define VERBOSE_FW_LOAD */
/* #define TRACK_ASSETS_COUNT */

/*! \def DEFAULT_FIRMWARE_CHECK_START_TIMEOUT
    \brief timeout used by the FW CHECK START
*/
#define DEFAULT_FIRMWARE_CHECK_START_TIMEOUT 0x10000 /* 64K loop iterations */

/*! \def DEFAULT_FIRMWARE_ACCEPTED_TIMEOUT
    \brief timeout used by the FW ACCEPTED
*/
#define DEFAULT_FIRMWARE_ACCEPTED_TIMEOUT 50000 /* 50 ms */

/*! \def DEFAULT_READ_TOKEN_TIMEOUT_1
    \brief timeout used by the SYSTEM_INFO command
*/
#define DEFAULT_READ_TOKEN_TIMEOUT_1 150000 /* 150 ms */

/*! \def DEFAULT_READ_TOKEN_TIMEOUT_2
    \brief timeout used by the FIPS-SELF-TEST command
*/
#define DEFAULT_READ_TOKEN_TIMEOUT_2 200000 /* 200 ms */

/*! \def DEFAULT_READ_TOKEN_TIMEOUT_3
    \brief timeout used by the regular commands
*/
#define DEFAULT_READ_TOKEN_TIMEOUT_3 10000 /* 10 ms */

/*! \def DEFAULT_READ_TOKEN_TIMEOUT_4
    \brief timeout used by the signature verify commands
*/
#define DEFAULT_READ_TOKEN_TIMEOUT_4 20000 /* 20 ms */

/*! \def DEFAULT_READ_TOKEN_TIMEOUT_5
    \brief timeout used by the hash, decrypt and MAC commands
*/
#define DEFAULT_READ_TOKEN_TIMEOUT_5 50000 /* 50ms */

/*! \def DEFAULT_READ_TOKEN_TIMEOUT_HUK
    \brief timeout used to read token
*/
#define DEFAULT_READ_TOKEN_TIMEOUT_HUK       1000000 /* 1000ms */

/*! \def DIVIDER_100
    \brief timeout used for token on OTP write
*/
#define DEFAULT_READ_TOKEN_TIMEOUT_OTP_WRITE 1000000 /* 1000ms */

/*! \def DEFAULT_READ_TOKEN_TIMEOUT_MC_INC
    \brief timeout for token while reading from OTP
*/
#define DEFAULT_READ_TOKEN_TIMEOUT_MC_INC    40000 /* 40ms */

#define PTR232LO(x) ((uint32_t)(((size_t)x) & 0xFFFFFFFFu))
#define PTR232HI(x) ((uint32_t)((((size_t)x) >> 32u) & 0xFFFFFFFFu))

static uint16_t gs_next_token_id = 0x3000;

static VAULTIP_INPUT_TOKEN_t gs_input_token;
static VAULTIP_OUTPUT_TOKEN_t gs_output_token;

static const uint32_t gs_firmware_output_token_timeout_1 =
    DEFAULT_READ_TOKEN_TIMEOUT_1; /* used by GetSystemInformation */
static const uint32_t gs_firmware_output_token_timeout_2 =
    DEFAULT_READ_TOKEN_TIMEOUT_2; /* used by FIPS-SELF-TEST */
static const uint32_t gs_firmware_output_token_timeout_3 =
    DEFAULT_READ_TOKEN_TIMEOUT_3; /* used by most other commands */
static const uint32_t gs_firmware_output_token_timeout_4 =
    DEFAULT_READ_TOKEN_TIMEOUT_4; /* used by signature verify */
static const uint32_t gs_firmware_output_token_timeout_5 =
    DEFAULT_READ_TOKEN_TIMEOUT_5; /* used by hash, decrypt, MAC */

#ifdef TRACK_ASSETS_COUNT
static uint32_t gs_asset_count = 0;
#endif

void memory_copy_8x32(volatile uint32_t *dst, const volatile uint32_t *src,
                      const volatile uint32_t *src_end);

static inline void *volatile_cast(volatile void *vp)
{
    union
    {
        volatile void *vp;
        void *p;
    } u;

    u.vp = vp;
    return u.p;
}

static inline void *const_cast(const void *vp)
{
    union
    {
        const void *vp;
        void *p;
    } u;

    u.vp = vp;
    return u.p;
}

static uint32_t gs_vault_dma_reloc_read = 0x0;
static uint32_t gs_vault_dma_reloc_write = 0x0;

inline static void set_vault_dma_reloc_read(uint32_t reloc)
{
    if (reloc != gs_vault_dma_reloc_read)
    {
        iowrite32(R_SP_MISC_BASEADDR + SPIO_MISC_ESR_VAULT_DMA_R_RELOC_ADDRESS,
                  SPIO_MISC_ESR_VAULT_DMA_R_RELOC_RD_CHANNEL_ADDR_SET(reloc & 0xFFu));
        gs_vault_dma_reloc_read = reloc;
        Log_Write(LOG_LEVEL_INFO, "*** gs_vault_dma_reloc_read = 0x%x ***\n", gs_vault_dma_reloc_read);
    }
}

inline static void set_vault_dma_reloc_write(uint32_t reloc)
{
    if (reloc != gs_vault_dma_reloc_write)
    {
        iowrite32(R_SP_MISC_BASEADDR + SPIO_MISC_ESR_VAULT_DMA_WR_RELOC_ADDRESS,
                  SPIO_MISC_ESR_VAULT_DMA_WR_RELOC_WT_CHANNEL_ADDR_SET(reloc & 0xFFu));
        gs_vault_dma_reloc_write = reloc;
        Log_Write(LOG_LEVEL_INFO, "*** gs_vault_dma_reloc_write = 0x%x ***\n", gs_vault_dma_reloc_write);
    }
}

static uint16_t get_next_token_id(void)
{
    return gs_next_token_id++;
}

static int vaultip_send_input_token(const VAULTIP_INPUT_TOKEN_t *pinput_token)
{
    volatile VAULTIP_HW_REGS_t *vaultip_regs = (VAULTIP_HW_REGS_t *)R_SP_VAULT_BASEADDR;
    MODULE_STATUS_t module_status;
    MAILBOX_STAT_t mailbox_stat;

    if (NULL == pinput_token)
    {
        return -1;
    }

    module_status.R = vaultip_regs->MODULE_STATUS.R;
    if (0 != module_status.B.FatalError || 0 == module_status.B.fw_image_checks_done ||
        0 == module_status.B.fw_image_accepted)
    {
        return -1;
    }

    mailbox_stat.R = vaultip_regs->MAILBOX_STAT.R;
    if (0 != mailbox_stat.B.mbx1_linked || 0 == mailbox_stat.B.mbx1_available)
    {
        return -1;
    }

    /* link mailbox */

    vaultip_regs->MAILBOX_CTRL.R = (MAILBOX_CTRL_t){ .B.mbx1_link = 1 }.R;
    mailbox_stat.R = vaultip_regs->MAILBOX_STAT.R;
    if (0 == mailbox_stat.B.mbx1_linked)
    {
        MESSAGE_ERROR("MAILBOX_STAT mbx1_linked is still 0!\n");
        return -1;
    }
    if (0 != mailbox_stat.B.mbx1_in_full)
    {
        MESSAGE_ERROR("MAILBOX_STAT mbx1_in_full is not 0!\n");
        return -1;
    }

    /* write the input token */

    memory_copy_8x32(vaultip_regs->MAILBOX1, &pinput_token->dw[0], &pinput_token->dw[64]);

    /* report that the input token was written */

    vaultip_regs->MAILBOX_CTRL.R = (MAILBOX_CTRL_t){ .B.mbx1_in_full = 1 }.R;
    mailbox_stat.R = vaultip_regs->MAILBOX_STAT.R;
    if (0 == mailbox_stat.B.mbx1_in_full)
    {
        MESSAGE_ERROR("MAILBOX_STAT mbx1_in_full is not 1!\n");
        return -1;
    }
    /*
    MESSAGE_INFO_DEBUG("Wrote token:\n");
    for (n = 0; n < 64; n+=8) {
        MESSAGE_INFO_DEBUG("  %08x %08x %08x %08x %08x %08x %08x %08x\n",
               pinput_token->dw[n+0], pinput_token->dw[n+1], pinput_token->dw[n+2], pinput_token->dw[n+3],
               pinput_token->dw[n+4], pinput_token->dw[n+5], pinput_token->dw[n+6], pinput_token->dw[n+7]);
    }

     if (!suppress_token_send_diagnostics) {
         MESSAGE_INFO_DEBUG("SI[0] = 0x%08x\n", pinput_token->dw_00);
     }*/

    return 0;
}

static int vaultip_read_output_token(VAULTIP_OUTPUT_TOKEN_t *poutput_token, uint32_t timeout)
{
    volatile VAULTIP_HW_REGS_t *vaultip_regs = (VAULTIP_HW_REGS_t *)R_SP_VAULT_BASEADDR;
    MODULE_STATUS_t module_status;
    MAILBOX_STAT_t mailbox_stat;
    uint64_t time_end;

    if (NULL == poutput_token)
    {
        return -1;
    }

    module_status.R = vaultip_regs->MODULE_STATUS.R;
    if (0 != module_status.B.FatalError || 0 == module_status.B.fw_image_checks_done ||
        0 == module_status.B.fw_image_accepted)
    {
        MESSAGE_ERROR("read_output_token: Error 1: MODULE_STATS=%08x\n", module_status.R);
        return -1;
    }

    mailbox_stat.R = vaultip_regs->MAILBOX_STAT.R;
    if (0 == mailbox_stat.B.mbx1_linked || 0 != mailbox_stat.B.mbx1_available)
    {
        MESSAGE_ERROR("read_output_token: Error 2: mailbox_stat=%08x\n", mailbox_stat.R);
        return -1;
    }

    /* wait for the output token to become available */

    time_end = timer_get_ticks_count() + (uint64_t)timeout;
    mailbox_stat.R = vaultip_regs->MAILBOX_STAT.R;
    while (0 == mailbox_stat.B.mbx1_out_full)
    {
        module_status.R = vaultip_regs->MODULE_STATUS.R;
        if (0 != module_status.B.FatalError)
        {
            MESSAGE_ERROR("read_output_token: Fatal error!\n");
            MESSAGE_ERROR("MAILBOX_STAT = 0x%08x\n", mailbox_stat.R);
            MESSAGE_ERROR("MODULE_STATUS = 0x%08x\n", module_status.R);
            return -1;
        }
        mailbox_stat.R = vaultip_regs->MAILBOX_STAT.R;
        if (timer_get_ticks_count() > time_end)
        {
            MESSAGE_ERROR("read_output_token: Timeout waiting for output token!\n");

            MESSAGE_ERROR("MAILBOX_STAT = 0x%08x\n", mailbox_stat.R);
            module_status.R = vaultip_regs->MODULE_STATUS.R;
            MESSAGE_ERROR("MODULE_STATUS = 0x%08x\n", module_status.R);
            return -1;
        }
    }

    /* read the output token */
    memory_copy_8x32(poutput_token->dw, &vaultip_regs->MAILBOX1[0], &vaultip_regs->MAILBOX1[64]);

    /* report that the output token was read*/
    vaultip_regs->MAILBOX_CTRL.R = (MAILBOX_CTRL_t){ .B.mbx1_out_empty = 1 }.R;
    mailbox_stat.R = vaultip_regs->MAILBOX_STAT.R;
    if (0 != mailbox_stat.B.mbx1_out_full)
    {
        MESSAGE_ERROR("MAILBOX_STAT mbx1_out_full is 1!\n");
        return -1;
    }

    /* unlink mailbox */

    vaultip_regs->MAILBOX_CTRL.R = (MAILBOX_CTRL_t){ .B.mbx1_unlink = 1 }.R;
    mailbox_stat.R = vaultip_regs->MAILBOX_STAT.R;
    if (1 == mailbox_stat.B.mbx1_linked)
    {
        MESSAGE_ERROR("MAILBOX_STAT mbx1_linked is still 1!\n");
        return -1;
    }

    /* if (!suppress_token_read_diagnostics) {
         MESSAGE_INFO_DEBUG("SO[0] = 0x%08x\n", poutput_token->dw_00);
     } */

    return 0;
}

static void print_failed_input_token_info(const uint32_t *input_token_words, uint32_t words_count)
{
    uint32_t n;

    (void)input_token_words;

    MESSAGE_INFO_DEBUG("Input token:\n");
    for (n = 0; n < words_count; n++)
    {
        MESSAGE_INFO_DEBUG("  si[%u] = %08x", n, input_token_words[n]);
        if (3 == (n % 4))
        {
            MESSAGE_INFO_DEBUG("\n");
        }
    }
    if (0 != (n % 4))
    {
        MESSAGE_INFO_DEBUG("\n");
    }
}

bool is_vaultip_disabled(void)
{
    uint32_t rm_status2;
    static bool initialized = false;
    static bool vaultip_disabled = false;

    if (!initialized) {
        if (0 != sp_otp_get_vaultip_chicken_bit(&vaultip_disabled)) {
            vaultip_disabled = false;
        }
        rm_status2 = ioread32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_STATUS2_ADDRESS);
        if (0 != RESET_MANAGER_RM_STATUS2_A0_UNLOCK_GET(rm_status2) &&
            0 != RESET_MANAGER_RM_STATUS2_SKIP_VAULT_GET(rm_status2)) {
            vaultip_disabled = true;
        }
    }

    return vaultip_disabled;
}

/* VAULTIP_TOKEN_SYSTEM_SUBCODE_SELF_TEST */
int vaultip_self_test(void)
{
    /* uint64_t time_start, time_end, time_delta; */
    volatile VAULTIP_HW_REGS_t *vaultip_regs = (VAULTIP_HW_REGS_t *)R_SP_VAULT_BASEADDR;

    (void)vaultip_regs;

    memset(&gs_input_token, 0, sizeof(gs_input_token));
    memset(&gs_output_token, 0, sizeof(gs_output_token));

    gs_input_token.dw_00.TokenID = get_next_token_id();
    gs_input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_SYSTEM;
    gs_input_token.dw_00.SubCode = VAULTIP_TOKEN_SYSTEM_SUBCODE_SELF_TEST;

    /*time_start = timer_get_ticks_count(); */

    if (0 != vaultip_send_input_token(&gs_input_token))
    {
        MESSAGE_ERROR("send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&gs_output_token, gs_firmware_output_token_timeout_2))
    {
        MESSAGE_ERROR("read_output_token() failed!\n");
        return -1;
    }

    /* time_end = timer_get_ticks_count(); */

    MESSAGE_INFO_DEBUG("MODULE_STATUS: %08x\n", vaultip_regs->MODULE_STATUS.R);
    if (1 == vaultip_regs->MODULE_STATUS.B.FIPS_mode &&
        0 == vaultip_regs->MODULE_STATUS.B.Non_FIPS_mode)
    {
        MESSAGE_INFO("FIPS ON\n");
    }

    /* time_delta = time_end - time_start;
      MESSAGE_INFO_DEBUG("SelfTest time: %lu\n", time_delta); */

    if (0 == gs_output_token.dw_00.Error)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int vaultip_get_system_information(uint32_t identity,
                                   VAULTIP_OUTPUT_TOKEN_SYSTEM_INFO_t *system_info)
{
    /* uint64_t time_start, time_end, time_delta; */
    volatile VAULTIP_HW_REGS_t *vaultip_regs = (VAULTIP_HW_REGS_t *)R_SP_VAULT_BASEADDR;

    (void)vaultip_regs;

    memset(&gs_input_token, 0, sizeof(gs_input_token));
    memset(&gs_output_token, 0, sizeof(gs_output_token));

    gs_input_token.dw_00.TokenID = get_next_token_id();
    gs_input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_SYSTEM;
    gs_input_token.dw_00.SubCode = VAULTIP_TOKEN_SYSTEM_SUBCODE_SYSTEM_INFORMATION;

    gs_input_token.dw_01.Identity = identity;

    /* time_start = timer_get_ticks_count(); */

    if (0 != vaultip_send_input_token(&gs_input_token))
    {
        MESSAGE_ERROR("send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&gs_output_token, gs_firmware_output_token_timeout_1))
    {
        MESSAGE_ERROR("read_output_token() failed!\n");
        return -1;
    }

    /* time_end = timer_get_ticks_count(); */

    MESSAGE_INFO_DEBUG("MODULE_STATUS: %08x\n", vaultip_regs->MODULE_STATUS.R);

    /* time_delta = time_end - time_start;
       MESSAGE_INFO_DEBUG("GetSystemInfo time: %lu\n", time_delta); */

    if (0 == gs_output_token.dw_00.Error)
    {
        *system_info = gs_output_token.system_info;
        return 0;
    }
    else
    {
        MESSAGE_ERROR_DEBUG("vaultip_get_system_information: gs_output_token = 0x%x\n",
                            gs_output_token.dw[0]);
        print_failed_input_token_info(gs_input_token.dw, 2);
        return -1;
    }
}

int vaultip_public_data_read(uint32_t identity, uint32_t asset_id, uint8_t *data_buffer,
                             uint32_t data_buffer_size, uint32_t *data_size)
{
    if (NULL == data_buffer || 0 == data_buffer_size || NULL == data_size)
    {
        MESSAGE_ERROR("Invalid arguments!\n");
        return -1;
    }
    memset(&gs_input_token, 0, sizeof(gs_input_token));
    memset(&gs_output_token, 0, sizeof(gs_output_token));

    gs_input_token.dw_01.Identity = identity;

    gs_input_token.dw_00.TokenID = get_next_token_id();
    gs_input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_ASSET_MANAGEMENT;
    gs_input_token.dw_00.SubCode = VAULTIP_TOKEN_ASSET_MANAGEMENT_SUBCODE_PUBLIC_DATA_READ;

    set_vault_dma_reloc_read(PTR232HI(data_buffer));
    set_vault_dma_reloc_write(PTR232HI(data_buffer));

    gs_input_token.public_data_read.dw_02.AS_ID = asset_id;
    gs_input_token.public_data_read.dw_03.OutputDataLength = data_buffer_size & 0x3FF;
    gs_input_token.public_data_read.dw_04.OutputDataAddress_31_00 = (uint32_t)(size_t)data_buffer;
    gs_input_token.public_data_read.dw_05.OutputDataAddress_63_32 = 0;

    l1_data_cache_flush_region(data_buffer, data_buffer_size);

    if (0 != vaultip_send_input_token(&gs_input_token))
    {
        MESSAGE_ERROR("send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&gs_output_token, gs_firmware_output_token_timeout_3))
    {
        MESSAGE_ERROR("read_output_token() failed!\n");
        return -1;
    }

    if (0 == gs_output_token.dw_00.Error)
    {
        if (NULL != data_size)
        {
            *data_size = gs_output_token.public_data_read.dw_01.DataLength;
        }
        return 0;
    }
    else
    {
        return -1;
    }
}

int vaultip_monotonic_counter_read(uint32_t identity, uint32_t asset_id, uint8_t *counter_buffer,
                                   uint32_t counter_buffer_size, uint32_t *data_size)
{
    if (NULL == counter_buffer || 0 == counter_buffer_size || NULL == data_size)
    {
        MESSAGE_ERROR("Invalid arguments!\n");
        return -1;
    }
    memset(&gs_input_token, 0, sizeof(gs_input_token));
    memset(&gs_output_token, 0, sizeof(gs_output_token));

    gs_input_token.dw_01.Identity = identity;

    gs_input_token.dw_00.TokenID = get_next_token_id();
    gs_input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_ASSET_MANAGEMENT;
    gs_input_token.dw_00.SubCode = VAULTIP_TOKEN_ASSET_MANAGEMENT_SUBCODE_MONOTONIC_COUNTER_READ;

    set_vault_dma_reloc_read(PTR232HI(counter_buffer));
    set_vault_dma_reloc_write(PTR232HI(counter_buffer));

    gs_input_token.monotonic_counter_read.dw_02.AS_ID = asset_id;
    gs_input_token.monotonic_counter_read.dw_03.OutputDataLength = counter_buffer_size & 0x3FF;
    gs_input_token.monotonic_counter_read.dw_04.OutputDataAddress_31_00 =
        (uint32_t)(size_t)counter_buffer;
    gs_input_token.monotonic_counter_read.dw_05.OutputDataAddress_63_32 = 0;

    l1_data_cache_flush_region(counter_buffer, counter_buffer_size);

    if (0 != vaultip_send_input_token(&gs_input_token))
    {
        MESSAGE_ERROR("send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&gs_output_token, gs_firmware_output_token_timeout_3))
    {
        MESSAGE_ERROR("read_output_token() failed!\n");
        return -1;
    }

    if (0 == gs_output_token.dw_00.Error)
    {
        *data_size = gs_output_token.monotonic_counter_read.dw_01.DataLength;
        return 0;
    }
    else
    {
        /* MESSAGE_ERROR_DEBUG("monotonic_counter_read: gs_output_token = 0x%x\n", gs_output_token.dw[0]);
           print_failed_input_token_info(gs_input_token.dw, 6); */
        return -1;
    }
}

int vaultip_monotonic_counter_increment(uint32_t identity, uint32_t asset_id)
{
    memset(&gs_input_token, 0, sizeof(gs_input_token));
    memset(&gs_output_token, 0, sizeof(gs_output_token));

    gs_input_token.dw_01.Identity = identity;

    gs_input_token.dw_00.TokenID = get_next_token_id();
    gs_input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_ASSET_MANAGEMENT;
    gs_input_token.dw_00.SubCode =
        VAULTIP_TOKEN_ASSET_MANAGEMENT_SUBCODE_MONOTONIC_COUNTER_INCREMENT;

    gs_input_token.monotonic_counter_increment.dw_02.AS_ID = asset_id;

    if (0 != vaultip_send_input_token(&gs_input_token))
    {
        MESSAGE_ERROR("send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&gs_output_token, DEFAULT_READ_TOKEN_TIMEOUT_MC_INC))
    {
        MESSAGE_ERROR("read_output_token() failed!\n");
        return -1;
    }

    if (0 == gs_output_token.dw_00.Error)
    {
        return 0;
    }
    else
    {
        MESSAGE_ERROR_DEBUG("monotonic_counter_increment: gs_output_token = 0x%x\n",
                            gs_output_token.dw[0]);
        print_failed_input_token_info(gs_input_token.dw, 3);
        return -1;
    }
}

int vaultip_otp_data_write(uint32_t identity, uint32_t asset_number, uint32_t policy_number,
                           bool CRC, const void *input_data, size_t input_data_length,
                           const void *associated_data, size_t associated_data_length)
{
    if (associated_data_length > 232)
    {
        return -1;
    }

    memset(&gs_input_token, 0, sizeof(gs_input_token));
    memset(&gs_output_token, 0, sizeof(gs_output_token));

    gs_input_token.dw_01.Identity = identity;

    gs_input_token.dw_00.TokenID = get_next_token_id();
    gs_input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_ASSET_MANAGEMENT;
    gs_input_token.dw_00.SubCode = VAULTIP_TOKEN_ASSET_MANAGEMENT_SUBCODE_OTP_DATA_WRITE;

    gs_input_token.otp_data_write.dw_02.AssetNumber = asset_number & 0x1Fu;
    gs_input_token.otp_data_write.dw_02.PolicyNumber = policy_number & 0x1Fu;
    gs_input_token.otp_data_write.dw_02.CRC = CRC ? 1 : 0;

    gs_input_token.otp_data_write.dw_03.InputDataLength = input_data_length & 0x3FFu;
    gs_input_token.otp_data_write.dw_03.AssociatedDataLength = associated_data_length & 0xFFu;

    set_vault_dma_reloc_read(PTR232HI(input_data));
    set_vault_dma_reloc_write(PTR232HI(input_data));

    gs_input_token.otp_data_write.dw_04.InputDataAddress_31_00 = PTR232LO(input_data);
    gs_input_token.otp_data_write.dw_05.InputDataAddress_63_32 = PTR232HI(input_data);

    memcpy(gs_input_token.otp_data_write.dw_63_06, associated_data, associated_data_length & 0xFFu);

    l1_data_cache_flush_region(input_data, input_data_length);

    if (0 != vaultip_send_input_token(&gs_input_token))
    {
        MESSAGE_ERROR("send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&gs_output_token, DEFAULT_READ_TOKEN_TIMEOUT_OTP_WRITE))
    {
        MESSAGE_ERROR("read_output_token() failed!\n");
        return -1;
    }

    if (0 == gs_output_token.dw_00.Error)
    {
        return 0;
    }
    else
    {
        MESSAGE_ERROR_DEBUG("otp_data_write: gs_output_token = 0x%x\n", gs_output_token.dw[0]);
        print_failed_input_token_info(gs_input_token.dw, 64);
        return -1;
    }
}

int vaultip_register_read(uint32_t identity, bool incremental_read, uint32_t number,
                          const uint32_t *address, uint32_t *result)
{
    uint32_t n;

    if (number > VAULTIP_INPUT_TOKEN_REGISTER_READ_MAXIMUM_NUMBER)
    {
        MESSAGE_ERROR("Invalid register number!\n");
        return -1;
    }
    memset(&gs_input_token, 0, sizeof(gs_input_token));
    memset(&gs_output_token, 0, sizeof(gs_output_token));

    gs_input_token.dw_01.Identity = identity;

    gs_input_token.dw_00.TokenID = get_next_token_id();
    gs_input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_SERVICE;
    gs_input_token.dw_00.SubCode = VAULTIP_TOKEN_SERVICE_SUBCODE_REGISTER_READ;

    gs_input_token.register_read.dw_02.Number = number & 0x3F;
    if (incremental_read)
    {
        gs_input_token.register_read.dw_02.Mode = 1;
        gs_input_token.register_read.dw_03_63[0].Address = address[0] & 0xFFFFu;
    }
    else
    {
        gs_input_token.register_read.dw_02.Mode = 0;
        for (n = 0; n < number; n++)
        {
            gs_input_token.register_read.dw_03_63[n].Address = address[n] & 0xFFFFu;
        }
    }

    if (0 != vaultip_send_input_token(&gs_input_token))
    {
        MESSAGE_ERROR("send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&gs_output_token, gs_firmware_output_token_timeout_3))
    {
        MESSAGE_ERROR("read_output_token() failed!\n");
        return -1;
    }

    if (0 == gs_output_token.dw_00.Error)
    {
        for (n = 0; n < number; n++)
        {
            result[n] = gs_output_token.register_read.dw_01_61[n].ReadData;
        }
        return 0;
    }
    else
    {
        MESSAGE_ERROR_DEBUG("register_read: gs_output_token = 0x%x\n", gs_output_token.dw[0]);
        print_failed_input_token_info(gs_input_token.dw, 4);
        return -1;
    }
}

int vaultip_register_write(uint32_t identity, bool incremental_write, uint32_t number,
                           const uint32_t *mask, const uint32_t *address, const uint32_t *value)
{
    uint32_t n;

    if (number > VAULTIP_INPUT_TOKEN_REGISTER_READ_MAXIMUM_NUMBER)
    {
        MESSAGE_ERROR("Invalid register number!\n");
        return -1;
    }
    memset(&gs_input_token, 0, sizeof(gs_input_token));
    memset(&gs_output_token, 0, sizeof(gs_output_token));

    gs_input_token.dw_01.Identity = identity;

    gs_input_token.dw_00.TokenID = get_next_token_id();
    gs_input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_SERVICE;
    gs_input_token.dw_00.SubCode = VAULTIP_TOKEN_SERVICE_SUBCODE_REGISTER_WRITE;

    gs_input_token.register_write.dw_02.Number = number & 0x3F;
    if (incremental_write)
    {
        if (number > VAULTIP_INPUT_TOKEN_REGISTER_WRITE_INCREMENTAL_MAXIMUM_NUMBER)
        {
            return -1;
        }
        gs_input_token.register_write.dw_02.Mode = 1;
        gs_input_token.register_write.incremental.Mask = mask[0];
        gs_input_token.register_write.incremental.Address = address[0] & 0xFFFFu;
        for (n = 0; n < number; n++)
        {
            gs_input_token.register_write.incremental.WriteData[n] = value[n];
        }
    }
    else
    {
        if (number > VAULTIP_INPUT_TOKEN_REGISTER_WRITE_NON_INCREMENTAL_MAXIMUM_NUMBER)
        {
            return -1;
        }
        gs_input_token.register_write.dw_02.Mode = 0;
        for (n = 0; n < number; n++)
        {
            gs_input_token.register_write.non_incremental.entries[n].Mask = mask[0];
            gs_input_token.register_write.non_incremental.entries[n].Address = address[0] & 0xFFFFu;
            gs_input_token.register_write.non_incremental.entries[n].Address = address[n] & 0xFFFFu;
        }
    }

    if (0 != vaultip_send_input_token(&gs_input_token))
    {
        MESSAGE_ERROR("send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&gs_output_token, gs_firmware_output_token_timeout_3))
    {
        MESSAGE_ERROR("read_output_token() failed!\n");
        return -1;
    }

    if (0 == gs_output_token.dw_00.Error)
    {
        return 0;
    }
    else
    {
        MESSAGE_ERROR_DEBUG("register_write: gs_output_token = 0x%x\n", gs_output_token.dw[0]);
        print_failed_input_token_info(gs_input_token.dw, 4);
        return -1;
    }
}

int vaultip_trng_configuration(uint32_t identity)
{
    memset(&gs_input_token, 0, sizeof(gs_input_token));
    memset(&gs_output_token, 0, sizeof(gs_output_token));

    gs_input_token.dw_00.TokenID = get_next_token_id();
    gs_input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_TRNG;
    gs_input_token.dw_00.SubCode = VAULTIP_TOKEN_TRNG_SUBCODE_CONFIGURE_NRBG;

    gs_input_token.dw_01.Identity = identity;

    gs_input_token.trng_configuration.dw_02.LST = 1;
    gs_input_token.trng_configuration.dw_02.RRD = 1;
    gs_input_token.trng_configuration.dw_02.AutoSeed = 32;
    gs_input_token.trng_configuration.dw_02.FroBlockKey = 0;
    gs_input_token.trng_configuration.dw_03.NoiseBlocks = 8;
    gs_input_token.trng_configuration.dw_03.Scale = 0;
    gs_input_token.trng_configuration.dw_03.SampleDiv = 0;
    gs_input_token.trng_configuration.dw_03.SampleCycles = 8;
    gs_input_token.trng_configuration.dw_03.Scale = 0;

    /* suppress_token_read_diagnostics = true; */
    print_failed_input_token_info(gs_input_token.dw, 4);

    if (0 != vaultip_send_input_token(&gs_input_token))
    {
        MESSAGE_ERROR("send_input_token() failed!\n");
        return -1;
    }

    /* suppress_token_read_diagnostics = false; */

    if (0 != vaultip_read_output_token(&gs_output_token, gs_firmware_output_token_timeout_3))
    {
        MESSAGE_ERROR("read_output_token() failed!\n");
        return -1;
    }

    if (0 == gs_output_token.dw_00.Error)
    {
        return 0;
    }
    else
    {
        MESSAGE_ERROR_DEBUG("trng_configuration: gs_output_token = 0x%x\n", gs_output_token.dw[0]);
        print_failed_input_token_info(gs_input_token.dw, 4);
        return -1;
    }
}

int vaultip_trng_get_random_number(void *dst, uint16_t size, bool raw)
{
    memset(&gs_input_token, 0, sizeof(gs_input_token));
    memset(&gs_output_token, 0, sizeof(gs_output_token));

    gs_input_token.dw_00.TokenID = get_next_token_id();
    gs_input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_TRNG;
    gs_input_token.dw_00.SubCode = VAULTIP_TOKEN_TRNG_SUBCODE_GET_RANDOM_NUMBER;

    set_vault_dma_reloc_read(PTR232HI(dst));
    set_vault_dma_reloc_write(PTR232HI(dst));

    gs_input_token.trn_get_random_number.dw_02.Size = size;
    gs_input_token.trn_get_random_number.dw_02.RawKey = raw ? VAULTIP_TRNG_RAW :
                                                              VAULTIP_TRNG_NORMAL;
    gs_input_token.trn_get_random_number.dw_03.OutputDataAddress_31_00 = PTR232LO(dst);
    gs_input_token.trn_get_random_number.dw_04.OutputDataAddress_63_32 = PTR232HI(dst);

    l1_data_cache_flush_region(dst, size);

    /* suppress_token_read_diagnostics = true; */

    if (0 != vaultip_send_input_token(&gs_input_token))
    {
        MESSAGE_ERROR("send_input_token() failed!\n");
        return -1;
    }

    /* suppress_token_read_diagnostics = false; */

    if (0 != vaultip_read_output_token(&gs_output_token, gs_firmware_output_token_timeout_3))
    {
        MESSAGE_ERROR("read_output_token() failed!\n");
        return -1;
    }

    if (0 == gs_output_token.dw_00.Error)
    {
        return 0;
    }
    else
    {
        MESSAGE_ERROR_DEBUG("trng_get_random_number: gs_output_token = 0x%x\n",
                            gs_output_token.dw[0]);
        print_failed_input_token_info(gs_input_token.dw, 5);
        return -1;
    }
}

int vaultip_provision_huk(uint32_t coid)
{
    memset(&gs_input_token, 0, sizeof(gs_input_token));
    memset(&gs_output_token, 0, sizeof(gs_output_token));

    gs_input_token.dw_00.TokenID = get_next_token_id();
    gs_input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_ASSET_MANAGEMENT;
    gs_input_token.dw_00.SubCode = VAULTIP_TOKEN_ASSET_MANAGEMENT_SUBCODE_PROVISION_RANDOM_HUK;

    gs_input_token.dw_01.Identity = coid;

    gs_input_token.provision_huk.dw_02.AssetNumber = VAULTIP_STATIC_ASSET_HUK;
    gs_input_token.provision_huk.dw_02.AutoSeed = 0;
    gs_input_token.provision_huk.dw_02.Size_128bit = 0;
    gs_input_token.provision_huk.dw_02.Size_256bit = 1;
    gs_input_token.provision_huk.dw_02.CRC = 1;
    gs_input_token.provision_huk.dw_02.KeyBlob = 0;

    gs_input_token.provision_huk.dw_03.NoiseBlocks = 8;
    gs_input_token.provision_huk.dw_03.SampleDiv = 0;
    gs_input_token.provision_huk.dw_03.SampleCycles = 16;

    gs_input_token.provision_huk.dw_04.AssociatedDataLength = 0;
    gs_input_token.provision_huk.dw_04.OutputDataLength = 0;

    if (0 != vaultip_send_input_token(&gs_input_token))
    {
        MESSAGE_ERROR("send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&gs_output_token, DEFAULT_READ_TOKEN_TIMEOUT_HUK))
    {
        MESSAGE_ERROR("read_output_token() failed!\n");
        return -1;
    }

    if (0 == gs_output_token.dw_00.Error)
    {
        return 0;
    }
    else
    {
        MESSAGE_ERROR_DEBUG("provision_huk: gs_output_token = 0x%x\n", gs_output_token.dw[0]);
        print_failed_input_token_info(gs_input_token.dw, 5);
        return -1;
    }
}

int vaultip_reset(uint32_t identity)
{
    memset(&gs_input_token, 0, sizeof(gs_input_token));
    memset(&gs_output_token, 0, sizeof(gs_output_token));

    gs_input_token.dw_00.TokenID = get_next_token_id();
    gs_input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_SYSTEM;
    gs_input_token.dw_00.SubCode = VAULTIP_TOKEN_SYSTEM_SUBCODE_RESET;

    gs_input_token.dw_01.Identity = identity;

    if (0 != vaultip_send_input_token(&gs_input_token))
    {
        MESSAGE_ERROR("send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&gs_output_token, gs_firmware_output_token_timeout_3))
    {
        MESSAGE_ERROR("read_output_token() failed!\n");
        return -1;
    }

    if (0 == gs_output_token.dw_00.Error)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int vaultip_hash(uint32_t identity, HASH_ALG_t hash_alg, const void *msg, size_t msg_size,
                 uint8_t *hash)
{
    void *temp_msg;
    uint32_t hash_size;
    uint32_t hash_type;

    switch (hash_alg)
    {
        case HASH_ALG_SHA2_256:
            hash_type = VAULTIP_HASH_ALGORITHM_SHA_256;
            hash_size = 256 / 8;
            break;
        case HASH_ALG_SHA2_384:
            hash_type = VAULTIP_HASH_ALGORITHM_SHA_384;
            hash_size = 384 / 8;
            break;
        case HASH_ALG_SHA2_512:
            hash_type = VAULTIP_HASH_ALGORITHM_SHA_512;
            hash_size = 512 / 8;
            break;
        default:
            MESSAGE_ERROR("hash: invalid hash_alg!\n");
            return -1;
    }

    memset(&gs_input_token, 0, sizeof(gs_input_token));
    memset(&gs_output_token, 0, sizeof(gs_output_token));

    set_vault_dma_reloc_read(PTR232HI(msg));
    set_vault_dma_reloc_write(PTR232HI(msg));

    gs_input_token.dw_00.TokenID = get_next_token_id();
    gs_input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_HASH;
    gs_input_token.dw_01.Identity = identity;
    gs_input_token.hash.dw_02.DataLength = (uint32_t)msg_size;
    temp_msg = const_cast(msg);
    gs_input_token.hash.dw_03.InputDataAddress_31_00 = PTR232LO(temp_msg);
    gs_input_token.hash.dw_04.InputDataAddress_63_32 = PTR232HI(temp_msg);
    gs_input_token.hash.dw_05.InputDataLength = msg_size & 0x1FFFFFu;
    gs_input_token.hash.dw_06.Algorithm = hash_type & 0xFu;
    gs_input_token.hash.dw_06.Mode = VAULTIP_HASH_MODE_INITIAL_FINAL;
    gs_input_token.hash.dw_24.TotalMessageLength_31_00 = (uint32_t)msg_size;
    gs_input_token.hash.dw_25.TotalMessageLength_60_32 = 0;

    l1_data_cache_flush_region(msg, msg_size);

    if (0 != vaultip_send_input_token(&gs_input_token))
    {
        MESSAGE_ERROR("hash: send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&gs_output_token, gs_firmware_output_token_timeout_5))
    {
        MESSAGE_ERROR("hash: read_output_token() failed!\n");
        return -1;
    }

    if (0 == gs_output_token.dw_00.Error)
    {
        memcpy(hash, &gs_output_token.hash.dw_02_17, hash_size);
        return 0;
    }
    else
    {
        MESSAGE_ERROR_DEBUG("hash: gs_output_token = 0x%x\n", gs_output_token.dw[0]);
        print_failed_input_token_info(gs_input_token.dw, 26);
        return -1;
    }
}

int vaultip_hash_update(uint32_t identity, HASH_ALG_t hash_alg, uint32_t digest_asset_id,
                        const void *msg, size_t msg_size, bool init)
{
    uint32_t hash_type;

    switch (hash_alg)
    {
        case HASH_ALG_SHA2_256:
            hash_type = VAULTIP_HASH_ALGORITHM_SHA_256;
            break;
        case HASH_ALG_SHA2_384:
            hash_type = VAULTIP_HASH_ALGORITHM_SHA_384;
            break;
        case HASH_ALG_SHA2_512:
            hash_type = VAULTIP_HASH_ALGORITHM_SHA_512;
            break;
        default:
            MESSAGE_ERROR("hash_update: invalid hash_alg!\n");
            return -1;
    }

    memset(&gs_input_token, 0, sizeof(gs_input_token));
    memset(&gs_output_token, 0, sizeof(gs_output_token));

    set_vault_dma_reloc_read(PTR232HI(msg));
    set_vault_dma_reloc_write(PTR232HI(msg));

    gs_input_token.dw_00.TokenID = get_next_token_id();
    gs_input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_HASH;
    gs_input_token.dw_01.Identity = identity;
    gs_input_token.hash.dw_02.DataLength = (uint32_t)msg_size;
    gs_input_token.hash.dw_03.InputDataAddress_31_00 = PTR232LO(msg);
    gs_input_token.hash.dw_04.InputDataAddress_63_32 = PTR232HI(msg);
    gs_input_token.hash.dw_05.InputDataLength = msg_size & 0x1FFFFFu;
    gs_input_token.hash.dw_06.Algorithm = hash_type & 0xFu;
    gs_input_token.hash.dw_06.Mode = init ? VAULTIP_HASH_MODE_INITIAL_NOT_FINAL :
                                            VAULTIP_HASH_MODE_CONTINUED_NOT_FINAL;
    gs_input_token.hash.dw_07.Digest_AS_ID = digest_asset_id;
    gs_input_token.hash.dw_24.TotalMessageLength_31_00 = 0;
    gs_input_token.hash.dw_25.TotalMessageLength_60_32 = 0;

    l1_data_cache_flush_region(msg, msg_size);

    if (0 != vaultip_send_input_token(&gs_input_token))
    {
        MESSAGE_ERROR("hash_update: send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&gs_output_token, gs_firmware_output_token_timeout_5))
    {
        MESSAGE_ERROR("hash_update: read_output_token() failed!\n");
        return -1;
    }

    if (0 == gs_output_token.dw_00.Error)
    {
        return 0;
    }
    else
    {
        MESSAGE_ERROR_DEBUG("hash_update: gs_output_token = 0x%x\n", gs_output_token.dw[0]);
        print_failed_input_token_info(gs_input_token.dw, 26);
        return -1;
    }
}

int vaultip_hash_final(uint32_t identity, HASH_ALG_t hash_alg, uint32_t digest_asset_id,
                       const void *msg, size_t msg_size, bool init, size_t total_msg_length,
                       uint8_t *hash)
{
    uint32_t hash_size;
    uint32_t hash_type;

    switch (hash_alg)
    {
        case HASH_ALG_SHA2_256:
            hash_type = VAULTIP_HASH_ALGORITHM_SHA_256;
            hash_size = 256 / 8;
            break;
        case HASH_ALG_SHA2_384:
            hash_type = VAULTIP_HASH_ALGORITHM_SHA_384;
            hash_size = 384 / 8;
            break;
        case HASH_ALG_SHA2_512:
            hash_type = VAULTIP_HASH_ALGORITHM_SHA_512;
            hash_size = 512 / 8;
            break;
        default:
            MESSAGE_ERROR("hash_final: invalid hash_alg!\n");
            return -1;
    }

    memset(&gs_input_token, 0, sizeof(gs_input_token));
    memset(&gs_output_token, 0, sizeof(gs_output_token));

    set_vault_dma_reloc_read(PTR232HI(msg));
    set_vault_dma_reloc_write(PTR232HI(msg));

    gs_input_token.dw_00.TokenID = get_next_token_id();
    gs_input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_HASH;
    gs_input_token.dw_01.Identity = identity;
    gs_input_token.hash.dw_02.DataLength = (uint32_t)msg_size;
    gs_input_token.hash.dw_03.InputDataAddress_31_00 = PTR232LO(msg);
    gs_input_token.hash.dw_04.InputDataAddress_63_32 = PTR232HI(msg);
    gs_input_token.hash.dw_05.InputDataLength = msg_size & 0x1FFFFFu;
    gs_input_token.hash.dw_06.Algorithm = hash_type & 0xFu;
    gs_input_token.hash.dw_06.Mode = init ? VAULTIP_HASH_MODE_INITIAL_FINAL :
                                            VAULTIP_HASH_MODE_CONTINUED_FINAL;
    gs_input_token.hash.dw_07.Digest_AS_ID = digest_asset_id;
    gs_input_token.hash.dw_24.TotalMessageLength_31_00 = (uint32_t)total_msg_length;
    gs_input_token.hash.dw_25.TotalMessageLength_60_32 = (total_msg_length >> 32u) & 0x1FFFFFFFu;

    l1_data_cache_flush_region(msg, msg_size);

    if (0 != vaultip_send_input_token(&gs_input_token))
    {
        MESSAGE_ERROR("hash_final: send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&gs_output_token, gs_firmware_output_token_timeout_5))
    {
        MESSAGE_ERROR("hash_final: read_output_token() failed!\n");
        return -1;
    }

    if (0 == gs_output_token.dw_00.Error)
    {
        memcpy(hash, gs_output_token.hash.dw_02_17, hash_size);
        return 0;
    }
    else
    {
        MESSAGE_ERROR_DEBUG("hash_final: gs_output_token = 0x%x\n", gs_output_token.dw[0]);
        print_failed_input_token_info(gs_input_token.dw, 26);
        return -1;
    }
}

int vaultip_mac_generate(uint32_t identity, ESPERANTO_MAC_TYPE_t mac_alg, uint32_t key_asset_id,
                         const void *msg, size_t msg_size, uint8_t *mac)
{
    uint32_t mac_type;
    uint32_t mac_size;

    switch (mac_alg)
    {
        case ESPERANTO_MAC_TYPE_AES_CMAC:
            mac_type = VAULTIP_MAC_ALGORITHM_AES_CMAC;
            mac_size = 128 / 8;
            break;
        case ESPERANTO_MAC_TYPE_HMAC_SHA2_256:
            mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_256;
            mac_size = 256 / 8;
            break;
        case ESPERANTO_MAC_TYPE_HMAC_SHA2_384:
            mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_384;
            mac_size = 384 / 8;
            break;
        case ESPERANTO_MAC_TYPE_HMAC_SHA2_512:
            mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_512;
            mac_size = 512 / 8;
            break;
        default:
            MESSAGE_ERROR("mac_generate: invalid mac_alg!\n");
            return -1;
    }

    memset(&gs_input_token, 0, sizeof(gs_input_token));
    memset(&gs_output_token, 0, sizeof(gs_output_token));

    set_vault_dma_reloc_read(PTR232HI(msg));
    set_vault_dma_reloc_write(PTR232HI(msg));

    gs_input_token.dw_00.TokenID = get_next_token_id();
    gs_input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_MAC;
    gs_input_token.dw_01.Identity = identity;
    gs_input_token.mac.dw_02.DataLength = (uint32_t)msg_size;
    gs_input_token.mac.dw_03.InputDataAddress_31_00 = PTR232LO(msg);
    gs_input_token.mac.dw_04.InputDataAddress_63_32 = PTR232HI(msg);
    gs_input_token.mac.dw_05.InputDataLength = msg_size & 0x1FFFFFu;
    gs_input_token.mac.dw_06.Algorithm = mac_type & 0xFu;
    gs_input_token.mac.dw_06.Mode = VAULTIP_MAC_MODE_INITIAL_FINAL;
    gs_input_token.mac.dw_06.AS_LoadKey = 1u;
    gs_input_token.mac.dw_06.AS_LoadMAC = 0u;
    gs_input_token.mac.dw_06.KeyLength = 0u;

    gs_input_token.mac.dw_07.MAC_AS_ID = 0;
    gs_input_token.dw[28] = key_asset_id;

    gs_input_token.mac.dw_24.TotalMessageLength_31_00 = msg_size & 0xFFFFFFFF;
    gs_input_token.mac.dw_25.TotalMessageLength_60_32 = 0;

    l1_data_cache_flush_region(msg, msg_size);

    if (0 != vaultip_send_input_token(&gs_input_token))
    {
        MESSAGE_ERROR("mac_generate: send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&gs_output_token, gs_firmware_output_token_timeout_5))
    {
        MESSAGE_ERROR("mac_generate: read_output_token() failed!\n");
        return -1;
    }

    if (0 == gs_output_token.dw_00.Error)
    {
        memcpy(mac, gs_output_token.mac.dw_02_17, mac_size);
        return 0;
    }
    else
    {
        MESSAGE_ERROR_DEBUG("mac_generate: gs_output_token = 0x%x\n", gs_output_token.dw[0]);
        print_failed_input_token_info(gs_input_token.dw, 60);
        return -1;
    }
}

int vaultip_mac_verify(uint32_t identity, ESPERANTO_MAC_TYPE_t mac_alg, uint32_t key_asset_id,
                       const void *msg, size_t msg_size, const uint8_t *mac)
{
    uint32_t mac_type;
    uint32_t mac_size;

    switch (mac_alg)
    {
        case ESPERANTO_MAC_TYPE_AES_CMAC:
            mac_type = VAULTIP_MAC_ALGORITHM_AES_CMAC;
            mac_size = 128 / 8;
            break;
        case ESPERANTO_MAC_TYPE_HMAC_SHA2_256:
            mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_256;
            mac_size = 256 / 8;
            break;
        case ESPERANTO_MAC_TYPE_HMAC_SHA2_384:
            mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_384;
            mac_size = 384 / 8;
            break;
        case ESPERANTO_MAC_TYPE_HMAC_SHA2_512:
            mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_512;
            mac_size = 512 / 8;
            break;
        default:
            MESSAGE_ERROR("mac_verify: invalid mac_alg!\n");
            return -1;
    }

    memset(&gs_input_token, 0, sizeof(gs_input_token));
    memset(&gs_output_token, 0, sizeof(gs_output_token));

    set_vault_dma_reloc_read(PTR232HI(msg));
    set_vault_dma_reloc_write(PTR232HI(msg));

    gs_input_token.dw_00.TokenID = get_next_token_id();
    gs_input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_MAC;
    gs_input_token.dw_01.Identity = identity;
    gs_input_token.mac.dw_02.DataLength = (uint32_t)msg_size;
    gs_input_token.mac.dw_03.InputDataAddress_31_00 = PTR232LO(msg);
    gs_input_token.mac.dw_04.InputDataAddress_63_32 = PTR232HI(msg);
    gs_input_token.mac.dw_05.InputDataLength = msg_size & 0x1FFFFFu;
    gs_input_token.mac.dw_06.Algorithm = mac_type & 0xFu;
    gs_input_token.mac.dw_06.Mode = VAULTIP_MAC_MODE_INITIAL_FINAL;
    gs_input_token.mac.dw_06.AS_LoadKey = 1u;
    gs_input_token.mac.dw_06.AS_LoadMAC = 1u;
    gs_input_token.mac.dw_06.KeyLength = 0u;

    gs_input_token.mac.dw_07.MAC_AS_ID = 0;
    memcpy(gs_input_token.mac.dw_23_08, mac, mac_size);

    gs_input_token.mac.dw_24.TotalMessageLength_31_00 = msg_size & 0xFFFFFFFF;
    gs_input_token.mac.dw_25.TotalMessageLength_60_32 = 0;

    gs_input_token.dw[28] = key_asset_id;

    l1_data_cache_flush_region(msg, msg_size);

    if (0 != vaultip_send_input_token(&gs_input_token))
    {
        MESSAGE_ERROR("mac_verify: send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&gs_output_token, gs_firmware_output_token_timeout_5))
    {
        MESSAGE_ERROR("mac_verify:read_output_token() failed!\n");
        return -1;
    }

    if (0 == gs_output_token.dw_00.Error)
    {
        return 0;
    }
    else
    {
        MESSAGE_ERROR_DEBUG("mac_verify: gs_output_token = 0x%x\n", gs_output_token.dw[0]);
        print_failed_input_token_info(gs_input_token.dw, 60);
        return -1;
    }
}

int vaultip_mac_update(uint32_t identity, ESPERANTO_MAC_TYPE_t mac_alg, uint32_t mac_asset_id,
                       uint32_t key_asset_id, const void *msg, size_t msg_size, bool init)
{
    uint32_t mac_type;

    switch (mac_alg)
    {
        case ESPERANTO_MAC_TYPE_AES_CMAC:
            mac_type = VAULTIP_MAC_ALGORITHM_AES_CMAC;
            break;
        case ESPERANTO_MAC_TYPE_HMAC_SHA2_256:
            mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_256;
            break;
        case ESPERANTO_MAC_TYPE_HMAC_SHA2_384:
            mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_384;
            break;
        case ESPERANTO_MAC_TYPE_HMAC_SHA2_512:
            mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_512;
            break;
        default:
            MESSAGE_ERROR("mac_update: invalid mac_alg!\n");
            return -1;
    }

    memset(&gs_input_token, 0, sizeof(gs_input_token));
    memset(&gs_output_token, 0, sizeof(gs_output_token));

    set_vault_dma_reloc_read(PTR232HI(msg));
    set_vault_dma_reloc_write(PTR232HI(msg));

    gs_input_token.dw_00.TokenID = get_next_token_id();
    gs_input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_MAC;
    gs_input_token.dw_01.Identity = identity;
    gs_input_token.mac.dw_02.DataLength = (uint32_t)msg_size;
    gs_input_token.mac.dw_03.InputDataAddress_31_00 = PTR232LO(msg);
    gs_input_token.mac.dw_04.InputDataAddress_63_32 = PTR232HI(msg);
    gs_input_token.mac.dw_05.InputDataLength = msg_size & 0x1FFFFFu;
    gs_input_token.mac.dw_06.Algorithm = mac_type & 0xFu;
    gs_input_token.mac.dw_06.Mode = init ? VAULTIP_MAC_MODE_INITIAL_FINAL :
                                           VAULTIP_MAC_MODE_CONTINUED_FINAL;
    gs_input_token.mac.dw_06.AS_LoadKey = 1u;
    gs_input_token.mac.dw_06.AS_LoadMAC = 0u;
    gs_input_token.mac.dw_06.KeyLength = 0u;

    gs_input_token.mac.dw_07.MAC_AS_ID = mac_asset_id;
    gs_input_token.dw[28] = key_asset_id;

    l1_data_cache_flush_region(msg, msg_size);

    if (0 != vaultip_send_input_token(&gs_input_token))
    {
        MESSAGE_ERROR("mac_update: send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&gs_output_token, gs_firmware_output_token_timeout_5))
    {
        MESSAGE_ERROR("mac_update: read_output_token() failed!\n");
        return -1;
    }

    if (0 == gs_output_token.dw_00.Error)
    {
        return 0;
    }
    else
    {
        MESSAGE_ERROR_DEBUG("mac_update: gs_output_token = 0x%x\n", gs_output_token.dw[0]);
        print_failed_input_token_info(gs_input_token.dw, 60);
        return -1;
    }
}

int vaultip_mac_final_generate(uint32_t identity, ESPERANTO_MAC_TYPE_t mac_alg,
                               uint32_t mac_asset_id, uint32_t key_asset_id, const void *msg,
                               size_t msg_size, size_t total_msg_size, bool init, uint8_t *mac)
{
    uint32_t mac_type;
    uint32_t mac_size;

    switch (mac_alg)
    {
        case ESPERANTO_MAC_TYPE_AES_CMAC:
            mac_type = VAULTIP_MAC_ALGORITHM_AES_CMAC;
            mac_size = 128 / 8;
            break;
        case ESPERANTO_MAC_TYPE_HMAC_SHA2_256:
            mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_256;
            mac_size = 256 / 8;
            break;
        case ESPERANTO_MAC_TYPE_HMAC_SHA2_384:
            mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_384;
            mac_size = 384 / 8;
            break;
        case ESPERANTO_MAC_TYPE_HMAC_SHA2_512:
            mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_512;
            mac_size = 512 / 8;
            break;
        default:
            MESSAGE_ERROR("mac_final_generate: invalid mac_alg!\n");
            return -1;
    }

    memset(&gs_input_token, 0, sizeof(gs_input_token));
    memset(&gs_output_token, 0, sizeof(gs_output_token));

    set_vault_dma_reloc_read(PTR232HI(msg));
    set_vault_dma_reloc_write(PTR232HI(msg));

    gs_input_token.dw_00.TokenID = get_next_token_id();
    gs_input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_MAC;
    gs_input_token.dw_01.Identity = identity;
    gs_input_token.mac.dw_02.DataLength = (uint32_t)msg_size;
    gs_input_token.mac.dw_03.InputDataAddress_31_00 = PTR232LO(msg);
    gs_input_token.mac.dw_04.InputDataAddress_63_32 = PTR232HI(msg);
    gs_input_token.mac.dw_05.InputDataLength = msg_size & 0x1FFFFFu;
    gs_input_token.mac.dw_06.Algorithm = mac_type & 0xFu;
    gs_input_token.mac.dw_06.Mode = init ? VAULTIP_MAC_MODE_INITIAL_FINAL :
                                           VAULTIP_MAC_MODE_CONTINUED_FINAL;
    gs_input_token.mac.dw_06.AS_LoadKey = 1u;
    gs_input_token.mac.dw_06.AS_LoadMAC = 0u;
    gs_input_token.mac.dw_06.KeyLength = 0u;

    gs_input_token.mac.dw_07.MAC_AS_ID = mac_asset_id;

    gs_input_token.mac.dw_24.TotalMessageLength_31_00 = total_msg_size & 0xFFFFFFFF;
    gs_input_token.mac.dw_25.TotalMessageLength_60_32 = 0;

    gs_input_token.dw[28] = key_asset_id;

    l1_data_cache_flush_region(msg, msg_size);

    if (0 != vaultip_send_input_token(&gs_input_token))
    {
        MESSAGE_ERROR("mac_final_generate: send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&gs_output_token, gs_firmware_output_token_timeout_5))
    {
        MESSAGE_ERROR("mac_final_generate: read_output_token() failed!\n");
        return -1;
    }

    if (0 == gs_output_token.dw_00.Error)
    {
        memcpy(mac, gs_output_token.mac.dw_02_17, mac_size);
        return 0;
    }
    else
    {
        MESSAGE_ERROR_DEBUG("mac_final_generate: gs_output_token = 0x%x\n", gs_output_token.dw[0]);
        print_failed_input_token_info(gs_input_token.dw, 60);
        return -1;
    }
}

int vaultip_mac_final_verify(uint32_t identity, ESPERANTO_MAC_TYPE_t mac_alg, uint32_t mac_asset_id,
                             uint32_t key_asset_id, const void *msg, size_t msg_size,
                             size_t total_msg_size, bool init, const uint8_t *mac)
{
    uint32_t mac_type;
    uint32_t mac_size;

    switch (mac_alg)
    {
        case ESPERANTO_MAC_TYPE_AES_CMAC:
            mac_type = VAULTIP_MAC_ALGORITHM_AES_CMAC;
            mac_size = 128 / 8;
            break;
        case ESPERANTO_MAC_TYPE_HMAC_SHA2_256:
            mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_256;
            mac_size = 256 / 8;
            break;
        case ESPERANTO_MAC_TYPE_HMAC_SHA2_384:
            mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_384;
            mac_size = 384 / 8;
            break;
        case ESPERANTO_MAC_TYPE_HMAC_SHA2_512:
            mac_type = VAULTIP_MAC_ALGORITHM_HMAC_SHA_512;
            mac_size = 512 / 8;
            break;
        default:
            MESSAGE_ERROR("mac_final_verify: invalid mac_alg!\n");
            return -1;
    }

    memset(&gs_input_token, 0, sizeof(gs_input_token));
    memset(&gs_output_token, 0, sizeof(gs_output_token));

    set_vault_dma_reloc_read(PTR232HI(msg));
    set_vault_dma_reloc_write(PTR232HI(msg));

    gs_input_token.dw_00.TokenID = get_next_token_id();
    gs_input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_MAC;
    gs_input_token.dw_01.Identity = identity;
    gs_input_token.mac.dw_02.DataLength = (uint32_t)msg_size;
    gs_input_token.mac.dw_03.InputDataAddress_31_00 = PTR232LO(msg);
    gs_input_token.mac.dw_04.InputDataAddress_63_32 = PTR232HI(msg);
    gs_input_token.mac.dw_05.InputDataLength = msg_size & 0x1FFFFFu;
    gs_input_token.mac.dw_06.Algorithm = mac_type & 0xFu;
    gs_input_token.mac.dw_06.Mode = init ? VAULTIP_MAC_MODE_INITIAL_FINAL :
                                           VAULTIP_MAC_MODE_CONTINUED_FINAL;
    gs_input_token.mac.dw_06.AS_LoadKey = 1u;
    gs_input_token.mac.dw_06.AS_LoadMAC = 1u;
    gs_input_token.mac.dw_06.KeyLength = 0u;

    gs_input_token.mac.dw_07.MAC_AS_ID = mac_asset_id;
    memcpy(gs_input_token.mac.dw_23_08, mac, mac_size);

    gs_input_token.mac.dw_24.TotalMessageLength_31_00 = total_msg_size & 0xFFFFFFFF;
    gs_input_token.mac.dw_25.TotalMessageLength_60_32 = 0;

    gs_input_token.dw[28] = key_asset_id;

    l1_data_cache_flush_region(msg, msg_size);

    if (0 != vaultip_send_input_token(&gs_input_token))
    {
        MESSAGE_ERROR("mac_final_verify: send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&gs_output_token, gs_firmware_output_token_timeout_5))
    {
        MESSAGE_ERROR("mac_final_verify: read_output_token() failed!\n");
        return -1;
    }

    if (0 == gs_output_token.dw_00.Error)
    {
        return 0;
    }
    else
    {
        MESSAGE_ERROR_DEBUG("mac_final_verify: gs_output_token = 0x%x\n", gs_output_token.dw[0]);
        print_failed_input_token_info(gs_input_token.dw, 60);
        return -1;
    }
}

static int vaultip_aes_cbc(uint32_t identity, uint32_t key_asset_id, uint8_t *IV, void *data,
                           size_t data_size, bool encrypt)
{
    memset(&gs_input_token, 0, sizeof(gs_input_token));
    memset(&gs_output_token, 0, sizeof(gs_output_token));

    set_vault_dma_reloc_read(PTR232HI(data));
    set_vault_dma_reloc_write(PTR232HI(data));

    gs_input_token.dw_00.TokenID = get_next_token_id();
    gs_input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_ENCRYPTION;
    gs_input_token.dw_01.Identity = identity;
    gs_input_token.encryption.dw_02.DataLength = (uint32_t)data_size;
    gs_input_token.encryption.dw_03.InputDataAddress_31_00 = PTR232LO(data);
    gs_input_token.encryption.dw_04.InputDataAddress_63_32 = PTR232HI(data);
    gs_input_token.encryption.dw_05.InputDataLength = data_size & 0x1FFFFFu;
    gs_input_token.encryption.dw_06.OutputDataAddress_31_00 = PTR232LO(data);
    gs_input_token.encryption.dw_07.OutputDataAddress_63_32 = PTR232HI(data);
    gs_input_token.encryption.dw_08.OutputDataLength = data_size & 0x1FFFFFu;
    /* gs_input_token.encryption.dw_09.AssociatedDataAddress_31_00 = 0; */
    /* gs_input_token.encryption.dw_10.AssociatedDataAddress_63_32 = 0; */
    gs_input_token.encryption.dw_11.Algorithm = VAULTIP_ENCRYPT_ALGORITHM_AES;
    gs_input_token.encryption.dw_11.Mode = VAULTIP_ENCRYPT_MODE_CBC;
    gs_input_token.encryption.dw_11.AS_LoadKey = 1;
    gs_input_token.encryption.dw_11.AS_LoadIV = 0;
    /* gs_input_token.encryption.dw_11.LoadParam = 0; */
    /* gs_input_token.encryption.dw_11.AS_SaveIV = 1; */
    /* gs_input_token.encryption.dw_11.GCM_Mode = 0; */
    gs_input_token.encryption.dw_11.Encrypt = encrypt ? 1 : 0;
    gs_input_token.encryption.dw_11.KeyLength = VAULTIP_ENCRYPT_KEY_LENGTH_256;
    /* gs_input_token.encryption.dw_11.NonceLength = 0; */
    /* gs_input_token.encryption.dw_11.TagLength_or_F8_SaltKeyLength = 0; */
    gs_input_token.encryption.dw_12.SaveIV_AS_ID = 0;
    memcpy(gs_input_token.encryption.dw_16_13, IV, 16);
    gs_input_token.encryption.dw_24_17[0].Key32 = key_asset_id;

    l1_data_cache_flush_region(data, data_size);

    if (0 != vaultip_send_input_token(&gs_input_token))
    {
        MESSAGE_ERROR("aes_cbc: send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&gs_output_token, gs_firmware_output_token_timeout_5))
    {
        MESSAGE_ERROR("aes_cbc: read_output_token() failed!\n");
        return -1;
    }

    if (0 == gs_output_token.dw_00.Error)
    {
        memcpy(IV, gs_output_token.encryption.dw_05_02, 16);
        return 0;
    }
    else
    {
        MESSAGE_ERROR_DEBUG("aes_cbc: gs_output_token = 0x%x\n", gs_output_token.dw[0]);
        print_failed_input_token_info(gs_input_token.dw, 41);
        return -1;
    }
}

int vaultip_aes_cbc_encrypt(uint32_t identity, uint32_t key_asset_id, uint8_t *IV, void *data,
                            size_t data_size)
{
    return vaultip_aes_cbc(identity, key_asset_id, IV, data, data_size, true);
}

int vaultip_aes_cbc_decrypt(uint32_t identity, uint32_t key_asset_id, uint8_t *IV, void *data,
                            size_t data_size)
{
    return vaultip_aes_cbc(identity, key_asset_id, IV, data, data_size, false);
}

int vaultip_asset_create(uint32_t identity, uint32_t policy_31_00, uint32_t policy_63_32,
                         VAULTIP_INPUT_TOKEN_ASSET_CREATE_WORD_4_t other_settings,
                         uint32_t lifetime, uint32_t *asset_id)
{
    memset(&gs_input_token, 0, sizeof(gs_input_token));
    memset(&gs_output_token, 0, sizeof(gs_output_token));

    gs_input_token.dw_00.TokenID = get_next_token_id();
    gs_input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_ASSET_MANAGEMENT;
    gs_input_token.dw_00.SubCode = VAULTIP_TOKEN_ASSET_MANAGEMENT_SUBCODE_ASSET_CREATE;
    gs_input_token.dw_01.Identity = identity;
    gs_input_token.asset_create.dw_02.Policy_31_00 = policy_31_00;
    gs_input_token.asset_create.dw_03.Policy_63_32 = policy_63_32;
    gs_input_token.asset_create.dw_04 = other_settings;

    gs_input_token.asset_create.dw_05.Lifetime = lifetime;

    /* MESSAGE_INFO_DEBUG("asset_create:
     policy=0x%08x_%08x, settings=0x%08x\n", policy_63_32, policy_31_00, other_settings);
     for (n = 0; n < 6; n++) {
         MESSAGE_INFO_DEBUG("asset_create[%u]=%08x\n", n, gs_input_token.dw[n]);
     } */

    if (0 != vaultip_send_input_token(&gs_input_token))
    {
        MESSAGE_ERROR("send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&gs_output_token, gs_firmware_output_token_timeout_3))
    {
        MESSAGE_ERROR("read_output_token() failed!\n");
        return -1;
    }

    if (0 == gs_output_token.dw_00.Error)
    {
        *asset_id = gs_output_token.asset_create.dw_01.AS_ID;
#ifdef TRACK_ASSETS_COUNT
        gs_asset_count++;
        MESSAGE_INFO_DEBUG("Created asset id 0x%08x (%u)\n",
                           gs_output_token.asset_create.dw_01.AS_ID, gs_asset_count);
#endif
        return 0;
    }
    else
    {
        MESSAGE_ERROR_DEBUG("asset_create: gs_output_token = 0x%x\n", gs_output_token.dw[0]);
        print_failed_input_token_info(gs_input_token.dw, 6);
        return -1;
    }
}

int vaultip_asset_load_plaintext(uint32_t identity, uint32_t asset_id, const void *data,
                                 uint32_t data_size)
{
    memset(&gs_input_token, 0, sizeof(gs_input_token));
    memset(&gs_output_token, 0, sizeof(gs_output_token));

    set_vault_dma_reloc_read(PTR232HI(data));
    set_vault_dma_reloc_write(PTR232HI(data));

    gs_input_token.dw_00.TokenID = get_next_token_id();
    gs_input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_ASSET_MANAGEMENT;
    gs_input_token.dw_00.SubCode = VAULTIP_TOKEN_ASSET_MANAGEMENT_SUBCODE_ASSET_LOAD;
    gs_input_token.dw_01.Identity = identity;
    gs_input_token.asset_load.dw_02.AS_ID = asset_id;
    gs_input_token.asset_load.dw_03.PlainText = 1;
    /* gs_input_token.asset_load.dw_03.RFC5869 = 1; // todo: verify if this is required */
    gs_input_token.asset_load.dw_03.InputDataLength = data_size & 0x3FFu;
    gs_input_token.asset_load.dw_04.InputDataAddress_31_00 = PTR232LO(data);
    gs_input_token.asset_load.dw_05.InputDataAddress_63_32 = PTR232HI(data);

    /* for (n = 0; n < 6; n++) {
         MESSAGE_INFO_DEBUG("asset_load[%u]=%08x\n", n, gs_input_token.dw[n]);
     }

        MESSAGE_INFO_DEBUG("asset_load_plaintext: assetid=0x%08x, length=0x%08x\n", asset_id, data_size);
    */
    l1_data_cache_flush_region(data, data_size);

    if (0 != vaultip_send_input_token(&gs_input_token))
    {
        MESSAGE_ERROR("send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&gs_output_token, gs_firmware_output_token_timeout_3))
    {
        MESSAGE_ERROR("read_output_token() failed!\n");
        return -1;
    }

    if (0 == gs_output_token.dw_00.Error)
    {
        return 0;
    }
    else
    {
        MESSAGE_ERROR("asset_load_plaintext: gs_output_token[0]=%08x\n", gs_output_token.dw[0]);
        return -1;
    }
}

int vaultip_asset_load_derive(uint32_t identity, uint32_t asset_id, uint32_t kdk_asset_id,
                              const uint8_t *key_expansion_IV, uint32_t key_expansion_IV_length,
                              const uint8_t *associated_data, uint32_t associated_data_size,
                              const uint8_t *salt, uint32_t salt_size)
{
    memset(&gs_input_token, 0, sizeof(gs_input_token));
    memset(&gs_output_token, 0, sizeof(gs_output_token));

    set_vault_dma_reloc_read(PTR232HI(salt));
    set_vault_dma_reloc_write(PTR232HI(key_expansion_IV));

    gs_input_token.dw_00.TokenID = get_next_token_id();
    gs_input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_ASSET_MANAGEMENT;
    gs_input_token.dw_00.SubCode = VAULTIP_TOKEN_ASSET_MANAGEMENT_SUBCODE_ASSET_LOAD;
    gs_input_token.dw_01.Identity = identity;
    gs_input_token.asset_load.dw_02.AS_ID = asset_id;
    gs_input_token.asset_load.dw_03.Derive = 1;
    gs_input_token.asset_load.dw_03.Counter = 1; /* todo: verify if this is required or desired */
    gs_input_token.asset_load.dw_03.RFC5869 = 1; /* todo: verify if this is required or desired */
    gs_input_token.asset_load.dw_03.AssociatedDataLength = associated_data_size & 0xFFu;
    gs_input_token.asset_load.dw_03.InputDataLength = salt_size & 0x3FFu;
    gs_input_token.asset_load.dw_04.InputDataAddress_31_00 = PTR232LO(salt);
    gs_input_token.asset_load.dw_05.InputDataAddress_63_32 = PTR232HI(salt);
    gs_input_token.asset_load.dw_06.OutputDataAddress_31_00 = PTR232LO(key_expansion_IV);
    gs_input_token.asset_load.dw_07.OutputDataAddress_63_32 = PTR232HI(key_expansion_IV);
    gs_input_token.asset_load.dw_08.OutputDataLength = key_expansion_IV_length & 0x7FF;
    gs_input_token.asset_load.dw_09.Key_AS_ID = kdk_asset_id;
    memcpy(gs_input_token.asset_load.dw_10_63[0].AssociatedData, associated_data,
           associated_data_size);

    l1_data_cache_flush_region(salt, salt_size);
    l1_data_cache_flush_region(key_expansion_IV, key_expansion_IV_length);

    if (0 != vaultip_send_input_token(&gs_input_token))
    {
        MESSAGE_ERROR("send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&gs_output_token, gs_firmware_output_token_timeout_3))
    {
        MESSAGE_ERROR("read_output_token() failed!\n");
        return -1;
    }

    if (0 == gs_output_token.dw_00.Error)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int vaultip_asset_delete(uint32_t identity, uint32_t asset_id)
{
    memset(&gs_input_token, 0, sizeof(gs_input_token));
    memset(&gs_output_token, 0, sizeof(gs_output_token));

    gs_input_token.dw_00.TokenID = get_next_token_id();
    gs_input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_ASSET_MANAGEMENT;
    gs_input_token.dw_00.SubCode = VAULTIP_TOKEN_ASSET_MANAGEMENT_SUBCODE_ASSET_DELETE;
    gs_input_token.dw_01.Identity = identity;
    gs_input_token.asset_delete.dw_02.AS_ID = asset_id;

    if (0 != vaultip_send_input_token(&gs_input_token))
    {
        MESSAGE_ERROR("send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&gs_output_token, gs_firmware_output_token_timeout_3))
    {
        MESSAGE_ERROR("read_output_token() failed!\n");
        return -1;
    }

    if (0 == gs_output_token.dw_00.Error)
    {
#ifdef TRACK_ASSETS_COUNT
        gs_asset_count--;
        MESSAGE_INFO_DEBUG("Deleted asset 0x%08x (%u)\n", asset_id, gs_asset_count);
#endif
        return 0;
    }
    else
    {
        return -1;
    }
}

int vaultip_static_asset_search(uint32_t identity, VAULTIP_STATIC_ASSET_ID_t asset_number,
                                uint32_t *asset_id, uint32_t *data_length)
{
    memset(&gs_input_token, 0, sizeof(gs_input_token));
    memset(&gs_output_token, 0, sizeof(gs_output_token));

    gs_input_token.dw_00.TokenID = get_next_token_id();
    gs_input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_ASSET_MANAGEMENT;
    gs_input_token.dw_00.SubCode = VAULTIP_TOKEN_ASSET_MANAGEMENT_SUBCODE_ASSET_SEARCH;
    gs_input_token.dw_01.Identity = identity;
    gs_input_token.static_asset_search.dw_04.AssetNumber = asset_number & 0x3Fu;

    if (0 != vaultip_send_input_token(&gs_input_token))
    {
        MESSAGE_ERROR("send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&gs_output_token, gs_firmware_output_token_timeout_3))
    {
        MESSAGE_ERROR("read_output_token() failed!\n");
        return -1;
    }

    if (0 == gs_output_token.dw_00.Error)
    {
        *asset_id = gs_output_token.static_asset_search.dw_01.AS_ID;
        *data_length = gs_output_token.static_asset_search.dw_02.DataLength;
        return 0;
    }
    else
    {
        return -1;
    }
}

int vaultip_public_key_ecdsa_verify(EC_KEY_CURVE_ID_t curve_id, uint32_t identity,
                                    uint32_t public_key_asset_id,
                                    uint32_t curve_parameters_asset_id,
                                    uint32_t temp_message_digest_asset_id, const void *message,
                                    uint32_t message_size, uint32_t hash_data_length,
                                    const void *sig_data_address, uint32_t sig_data_size)
{
    uint32_t modulus_size;
    uint32_t modulus_words;

    switch (curve_id)
    {
        case EC_KEY_CURVE_NIST_P256:
            modulus_size = 256;
            break;
        case EC_KEY_CURVE_NIST_P384:
            modulus_size = 384;
            break;
        case EC_KEY_CURVE_NIST_P521:
            modulus_size = 521;
            break;
        default:
            MESSAGE_ERROR("public_key_ec_verify: invalid curve_id!\n");
            return -1;
    }
    modulus_words = (modulus_size + 31) / 32;

    memset(&gs_input_token, 0, sizeof(gs_input_token));
    memset(&gs_output_token, 0, sizeof(gs_output_token));

    if (PTR232HI(message) != PTR232HI(sig_data_address))
    {
        Log_Write(LOG_LEVEL_ERROR,
            "vaultip_public_key_ecdsa_verify: vaultip limitation - the high bits of the message and signature address differ!\n");
        return -1;
    }

    set_vault_dma_reloc_read(PTR232HI(message));
    set_vault_dma_reloc_write(PTR232HI(message));

    gs_input_token.dw_00.TokenID = get_next_token_id();
    gs_input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_PUBLIC_KEY_OPERATION;
    gs_input_token.dw_00.SubCode = VAULTIP_TOKEN_PUBLIC_KEY_SUBCODE_USES_ASSETS;
    gs_input_token.dw_01.Identity = identity;
    gs_input_token.public_key.dw_02.Command = VAULTIP_PUBLIC_KEY_COMMAND_ECDSA_VERIFY;
    gs_input_token.public_key.dw_02.Mwords = (uint8_t)modulus_words;
    gs_input_token.public_key.dw_02.Nwords = (uint8_t)modulus_words;
    gs_input_token.public_key.dw_04.KeyAssetRef = public_key_asset_id;
    gs_input_token.public_key.dw_05.ParamAssetRef = curve_parameters_asset_id;
    gs_input_token.public_key.dw_06.IOAssetRef = temp_message_digest_asset_id;
    gs_input_token.public_key.dw_07.InputDataSize = message_size & 0xFFFu;
    gs_input_token.public_key.dw_07.OutputDataSize_or_SigDataSize = sig_data_size & 0xFFFu;
    gs_input_token.public_key.dw_08.InputDataAddress_31_00 = PTR232LO(message);
    gs_input_token.public_key.dw_09.InputDataAddress_63_32 = PTR232HI(message);
    gs_input_token.public_key.dw_10.SigDataAddress_31_00 = PTR232LO(sig_data_address);
    gs_input_token.public_key.dw_11.SigDataAddress_63_32 = PTR232HI(sig_data_address);
    gs_input_token.public_key.dw_12_63.HashDataLength = hash_data_length;

    l1_data_cache_flush_region(message, message_size);
    l1_data_cache_flush_region(sig_data_address, sig_data_size);

    if (0 != vaultip_send_input_token(&gs_input_token))
    {
        MESSAGE_ERROR("send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&gs_output_token, gs_firmware_output_token_timeout_4))
    {
        MESSAGE_ERROR("read_output_token() failed!\n");
        return -1;
    }

    if (0 == gs_output_token.dw_00.Error)
    {
        return 0;
    }
    else
    {
        MESSAGE_ERROR_DEBUG("public_key_ecdsa_verify: output token[0] = 0x%08x\n",
                            gs_output_token.dw[0]);
        return -1;
    }
}

int vaultip_public_key_rsa_pss_verify(uint32_t modulus_size, uint32_t identity,
                                      uint32_t public_key_asset_id,
                                      uint32_t temp_message_digest_asset_id, const void *message,
                                      uint32_t message_size, uint32_t hash_data_length,
                                      const void *sig_data_address, uint32_t sig_data_size,
                                      uint32_t salt_length)
{
    uint32_t modulus_words;

    switch (modulus_size)
    {
        case 4096:
        case 3072:
        case 2048:
            break;
        default:
            MESSAGE_ERROR("public_key_rsa_pss_verify: invalid modulus_size!\n");
            return -1;
    }
    modulus_words = (modulus_size + 31) / 32;

    memset(&gs_input_token, 0, sizeof(gs_input_token));
    memset(&gs_output_token, 0, sizeof(gs_output_token));

    if (PTR232HI(message) != PTR232HI(sig_data_address))
    {
        Log_Write(LOG_LEVEL_ERROR,
            "vaultip_public_key_rsa_pss_verify: vaultip limitation - \
             the high bits of the message and signature address differ!\n");
        return -1;
    }

    set_vault_dma_reloc_read(PTR232HI(message));
    set_vault_dma_reloc_write(PTR232HI(message));

    gs_input_token.dw_00.TokenID = get_next_token_id();
    gs_input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_PUBLIC_KEY_OPERATION;
    gs_input_token.dw_00.SubCode = VAULTIP_TOKEN_PUBLIC_KEY_SUBCODE_USES_ASSETS;
    gs_input_token.dw_01.Identity = identity;
    gs_input_token.public_key.dw_02.Command = VAULTIP_PUBLIC_KEY_COMMAND_RSA_PSS_VERIFY;
    gs_input_token.public_key.dw_02.Mwords = (uint8_t)modulus_words;
    gs_input_token.public_key.dw_02.Nwords = (uint8_t)modulus_words;
    gs_input_token.public_key.dw_03.OtherLen = salt_length & 0xFFu;
    gs_input_token.public_key.dw_04.KeyAssetRef = public_key_asset_id;
    gs_input_token.public_key.dw_06.IOAssetRef = temp_message_digest_asset_id;
    gs_input_token.public_key.dw_07.InputDataSize = message_size & 0xFFFu;
    gs_input_token.public_key.dw_07.OutputDataSize_or_SigDataSize = sig_data_size & 0xFFFu;
    gs_input_token.public_key.dw_08.InputDataAddress_31_00 = PTR232LO(message);
    gs_input_token.public_key.dw_09.InputDataAddress_63_32 = PTR232HI(message);
    gs_input_token.public_key.dw_10.SigDataAddress_31_00 = PTR232LO(sig_data_address);
    gs_input_token.public_key.dw_11.SigDataAddress_63_32 = PTR232HI(sig_data_address);
    gs_input_token.public_key.dw_12_63.HashDataLength = hash_data_length;

    l1_data_cache_flush_all();

    if (0 != vaultip_send_input_token(&gs_input_token))
    {
        MESSAGE_ERROR("send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&gs_output_token, gs_firmware_output_token_timeout_4))
    {
        MESSAGE_ERROR("read_output_token() failed!\n");
        return -1;
    }

    if (0 == gs_output_token.dw_00.Error)
    {
        return 0;
    }
    else
    {
        MESSAGE_ERROR_DEBUG("vaultip_public_key_rsa_pss_verify: output token[0] = 0x%08x\n",
                            gs_output_token.dw[0]);
        print_failed_input_token_info(gs_input_token.dw, 13);
        return -1;
    }
}

int vaultip_clock_switch(uint32_t identity, uint32_t token)
{
    memset(&gs_input_token, 0, sizeof(gs_input_token));
    memset(&gs_output_token, 0, sizeof(gs_output_token));

    gs_input_token.dw_00.TokenID = get_next_token_id();
    gs_input_token.dw_00.OpCode = VAULTIP_TOKEN_OPCODE_SERVICE;
    gs_input_token.dw_00.SubCode = VAULTIP_TOKEN_SERVICE_SUBCODE_CLOCK_SWITCH;
    gs_input_token.dw_01.Identity = identity;
    gs_input_token.dw[2] = token;

    if (0 != vaultip_send_input_token(&gs_input_token))
    {
        MESSAGE_ERROR("send_input_token() failed!\n");
        return -1;
    }

    if (0 != vaultip_read_output_token(&gs_output_token, gs_firmware_output_token_timeout_3))
    {
        MESSAGE_ERROR("read_output_token() failed!\n");
        return -1;
    }

    if (0 == gs_output_token.dw_00.Error)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}
