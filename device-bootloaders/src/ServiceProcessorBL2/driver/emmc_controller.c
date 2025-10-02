/*-------------------------------------------------------------------------
* Copyright (C) 2023, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/
/************************************************************************/
/*! \file emmc_controller.c
    \brief eMMC controller implementation
*/
/***********************************************************************/

#include <string.h>
#include "hwinc/hal_device.h"
#include "etsoc/isa/io.h"
#include "hwinc/sp_cru_reset.h"

#include "bl2_emmc_controller_impl.h"

#include "delays.h"

#include "bl2_sp_pll.h"
#include "log.h"

#include "bl_error_code.h"

static const EMMC_Command_Direction commands_directions[] = {
    [EMMC_JEDEC_CMD8] = EMMC_CMD_READ,   [EMMC_JEDEC_CMD14] = EMMC_CMD_READ,
    [EMMC_JEDEC_CMD17] = EMMC_CMD_READ,  [EMMC_JEDEC_CMD18] = EMMC_CMD_READ,
    [EMMC_JEDEC_CMD21] = EMMC_CMD_READ,  [EMMC_JEDEC_CMD19] = EMMC_CMD_WRITE,
    [EMMC_JEDEC_CMD23] = EMMC_CMD_WRITE, [EMMC_JEDEC_CMD24] = EMMC_CMD_WRITE,
    [EMMC_JEDEC_CMD25] = EMMC_CMD_WRITE, [EMMC_JEDEC_CMD26] = EMMC_CMD_WRITE,
    [EMMC_JEDEC_CMD27] = EMMC_CMD_WRITE
};

static const uint8_t EMMC_JEDEC_HS200_TUNING_PATTERN[128] = {
    0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc, 0xcc,
    0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff, 0xff, 0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee, 0xff,
    0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd, 0xdd, 0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff, 0xbb,
    0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff, 0xff, 0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee, 0xff,
    0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc,
    0xcc, 0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff, 0xff, 0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee,
    0xff, 0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd, 0xdd, 0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff,
    0xbb, 0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff, 0xff, 0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee,
};

static uint16_t gs_emmc_divider = 0;
/* 0x0 -0x7F range */
static uint8_t gs_emmc_sdclkdl_dc = 0xF;

/* We declare this buffer to be 32-bits, as EMMC Read/Write register is 32-bit
 * wide */
/* So we should generate a 4-byte access */
/* This avoids unecessary pointer arithmetics in data transfer functions */
static uint32_t gs_emmc_ext_csd_register[EMMC_SIZE_BYTES_EXT_CSD_CARD_REGISTER / sizeof(uint32_t)];

static ET_EMMC_DEV_t gs_emmc_dev;

inline int __attribute__((always_inline))
wait_for_command_complete(volatile uint16_t *int_status, uint16_t mask, uint16_t cmd)
{
    uint32_t retries = 5000000;
    while (!(*int_status & mask))
    {
        retries--;
        if (retries == 0)
        {
            Log_Write(LOG_LEVEL_ERROR, "ERROR: Waiting for command complete - Timeout, CMD%d\r\n",
                      cmd);
            return ERROR_EMMC_WAIT_FOR_CMD_COMPLETE_TIMEOUT;
        }
    }

    *int_status = mask;

    return SUCCESS;
}

inline int __attribute__((always_inline))
wait_for_buffer_ready(volatile uint16_t *int_status, uint16_t mask, const char *errorMsg,
                      uint16_t cmd)
{
    uint16_t retries = 50000;
    while (!(*int_status & mask))
    {
        retries--;
        if (retries == 0)
        {
            Log_Write(LOG_LEVEL_ERROR, "ERROR: %s - Timeout, CMD%d\r\n", errorMsg, cmd);
            return ERROR_EMMC_WAIT_READ_BUFFER_READY_TIMEOUT;
        }
        usdelay(100);
    }
    *int_status = mask;

    return SUCCESS;
}

inline int __attribute__((always_inline))
read_through_buffer(uint32_t *const data_buffer, uint16_t block_size, uint32_t block_count,
                    const ET_EMMC_DEV_t *dev, uint16_t cmd)
{
    uint32_t processed_blocks = 0;
    uint32_t data_buff_offset = 0;
    uint32_t const emmc_data_buffer_size_bytes = sizeof(dev->regs->crypto.BUF_DATA_R);

    do
    {
        if (wait_for_buffer_ready(&dev->regs->crypto.NORMAL_INT_STAT_R,
                                  NORMAL_INT_STAT_R__BUF_RD_READY__MASK,
                                  "Waiting for buffer read ready", cmd) != 0)
        {
            return ERROR_EMMC_WAIT_READ_BUFFER_READY_TIMEOUT;
        }

        dev->regs->crypto.NORMAL_INT_STAT_R = NORMAL_INT_STAT_R__BUF_RD_READY__MASK;

        // read block data
        data_buff_offset = processed_blocks * block_size / emmc_data_buffer_size_bytes;

        for (uint16_t i = 0; i < (block_size / emmc_data_buffer_size_bytes); i++)
        {
            data_buffer[i + data_buff_offset] = dev->regs->crypto.BUF_DATA_R;
        }
        processed_blocks++;
    } while (processed_blocks < block_count);

    return SUCCESS;
}

inline int __attribute__((always_inline))
write_through_buffer(uint32_t *const data_buffer, uint16_t block_size, uint32_t block_count,
                     const ET_EMMC_DEV_t *dev, uint16_t cmd)
{
    uint32_t processed_blocks = 0;
    uint32_t data_buff_offset = 0;
    uint32_t const emmc_data_buffer_size_bytes = sizeof(dev->regs->crypto.BUF_DATA_R);

    do
    {
        if (wait_for_buffer_ready(&dev->regs->crypto.NORMAL_INT_STAT_R,
                                  NORMAL_INT_STAT_R__BUF_WR_READY__MASK,
                                  "Waiting for buffer write ready", cmd) != 0)
        {
            return ERROR_EMMC_WAIT_WRITE_BUFFER_READY_TIMEOUT;
        }

        dev->regs->crypto.NORMAL_INT_STAT_R = NORMAL_INT_STAT_R__BUF_WR_READY__MASK;

        // write block data
        data_buff_offset = processed_blocks * block_size / emmc_data_buffer_size_bytes;

        for (uint16_t i = 0; i < (block_size / emmc_data_buffer_size_bytes); i++)
        {
            dev->regs->crypto.BUF_DATA_R = data_buffer[i + data_buff_offset];
        }
        processed_blocks++;
    } while (processed_blocks < block_count);
    return SUCCESS;
}

static int cmd_no_dt(const ET_EMMC_DEV_t *dev, uint16_t cmd, uint32_t arg, uint8_t resp_type)
{
    uint32_t retries;
    uint16_t timeout_ms;
    uint32_t timeout_10us_times;
    uint8_t cmd_type = NORMAL_CMD;
    uint8_t idx_chk = 0;
    uint8_t crc_chk = 0;

    retries = 10000;

    timeout_ms = 2000;
    while (dev->regs->crypto.PSTATE_REG & PSTATE_REG__DAT_LINE_ACTIVE__MASK)
    {
        timeout_ms--;
        if (timeout_ms == 0)
        {
            Log_Write(LOG_LEVEL_ERROR, "ERROR: Data line in use - time out, CMD%d\r\n", cmd);
            return ERROR_EMMC_GENERAL;
        }
        msdelay(1);
    }

    while (dev->regs->crypto.PSTATE_REG & PSTATE_REG__CMD_INHIBIT__MASK)
    {
        retries--;
        if (retries == 0)
        {
            Log_Write(LOG_LEVEL_ERROR, "ERROR: CMD line used - time out, CMD%d\r\n", cmd);
            return ERROR_EMMC_CMD_LINE_IN_USE_TIMEOUT;
        }
    }

    // !!! Not required at the moment, only for slower modes
    // Check if host driver issues SD command with/withouh using DAT lines
    // including busy singal
    //  If use DAT line
    // Check if it is abort command
    //  if yes check dev->regs->crypto.PSTATE_REG &
    // PSTATE_REG__CMD_INHIBIT_DAT__MASK
    // if no continue

    // Set ARGUMENT_R to generate command
    dev->regs->crypto.ARGUMENT_R = arg;

    // for commands CMD12/CMD52 or CMD0/CMD52 set CMD_TYPE field to 0x3
    if (cmd == EMMC_JEDEC_CMD0)
    {
        cmd_type = ABORT_CMD;
    }
    // Enable command complete status and transfer complete status
    dev->regs->crypto.NORMAL_INT_STAT_EN_R |= (NORMAL_INT_STAT_EN_R__CMD_COMPLETE_STAT_EN__MASK |
                                               NORMAL_INT_STAT_EN_R__XFER_COMPLETE_STAT_EN__MASK);

    // Set CMD_R - write to this register starts command
    dev->regs->crypto.CMD_R = (uint16_t)(
        cmd << CMD_R__CMD_INDEX__SHIFT | resp_type << CMD_R__RESP_TYPE_SELECT__SHIFT |
        cmd_type << CMD_R__CMD_TYPE__SHIFT | idx_chk << CMD_R__CMD_IDX_CHK_ENABLE__SHIFT |
        crc_chk << CMD_R__CMD_CRC_CHK_ENABLE__SHIFT);

    if ((dev->regs->crypto.XFER_MODE_R & XFER_MODE_R__RESP_INT_DISABLE__MASK) == 0)
    {
        timeout_10us_times = 10000; // 100ms in total
        while (!(dev->regs->crypto.NORMAL_INT_STAT_R & NORMAL_INT_STAT_R__CMD_COMPLETE__MASK))
        {
            timeout_10us_times--;
            if (cmd == EMMC_JEDEC_CMD21)
                break;
            if (timeout_10us_times == 0)
            {
                Log_Write(LOG_LEVEL_ERROR,
                          "ERROR: Waiting for command complete - Timeout, CMD%d\r\n", cmd);
                return ERROR_EMMC_WAIT_FOR_CMD_COMPLETE_TIMEOUT;
            }
            usdelay(10);
        }

        // Clear CMD_COMPLETE status bit (W1C register)
        dev->regs->crypto.NORMAL_INT_STAT_R = NORMAL_INT_STAT_R__CMD_COMPLETE__MASK;

#ifdef EMMC_DEBUG_PRINTS
        // Get Response Data from Response registers to get necessary information
        // about issued command
        uint32_t resp[4] = { 0 };
        resp[0] = dev->regs->crypto.RESP01_R;
        Log_Write(LOG_LEVEL_CRITICAL, "\t\tRESP01 0x%X\r\n", resp[0]);
        if (resp_type == RESP2)
        {
            resp[1] = dev->regs->crypto.RESP23_R;
            Log_Write(LOG_LEVEL_CRITICAL, "\t\tRESP23 0x%X\r\n", resp[1]);
            resp[2] = dev->regs->crypto.RESP45_R;
            Log_Write(LOG_LEVEL_CRITICAL, "\t\tRESP45 0x%X\r\n", resp[2]);
            resp[3] = dev->regs->crypto.RESP67_R;
            Log_Write(LOG_LEVEL_CRITICAL, "\t\tRESP67 0x%X\r\n", resp[3]);
        }
#endif
    }

    // A part
    // Check if command uses Transfer Complete Interrupt
    if (((cmd == EMMC_JEDEC_CMD7) || (cmd == EMMC_JEDEC_CMD6)) &&
        (dev->regs->crypto.NORMAL_INT_STAT_EN_R &
         NORMAL_INT_STAT_EN_R__XFER_COMPLETE_STAT_EN__MASK))
    { // FIXME
        // investigate when
        // we need to wait
        // on transfer
        // complete
        // interrupt

        retries = 500000;
        while ((dev->regs->crypto.NORMAL_INT_STAT_R & NORMAL_INT_STAT_R__XFER_COMPLETE__MASK) == 0)
        {
            retries--;
            if (retries == 0)
            {
                Log_Write(LOG_LEVEL_ERROR,
                          "ERROR: Waiting for transfer complete - Timeout, CMD%d\r\n", cmd);
                return ERROR_EMMC_WAIT_FOR_CMD_COMPLETE_TIMEOUT;
            }
        }

        // clear treansfer complete status
        dev->regs->crypto.NORMAL_INT_STAT_R = NORMAL_INT_STAT_R__XFER_COMPLETE__MASK;
    }

    // !!! check errors in response status - resp_err_int? (Not required)
    // if ((resp_type != RESP3) && (resp[0] & 0xFDF9A080))
    //  return -1;
    // else
    return SUCCESS;
}

static int cmd_dt_no_dma(const ET_EMMC_DEV_t *dev, uint16_t cmd, uint32_t arg, uint16_t block_size,
                         uint32_t block_count, uint32_t *data_buff, EMMC_MODE_t mode)
{
    uint8_t direction = (uint8_t)(commands_directions[cmd]);
    uint8_t idx_chk = 0;
    uint8_t crc_chk = 0;
    uint8_t err_chk = 0;
    uint8_t resp_int = 0;

    uint8_t multi_block = block_count > 1;

    /* if the command is not tuning, and mode is not HS400 (it has fixed block
   * size)*/

    if (EMMC_IS_BLOCK_SIZE_VARIABLE(cmd, mode))
    {
        cmd_no_dt(dev, EMMC_JEDEC_CMD16, block_size, RESP1);
    }

    if ((cmd != EMMC_JEDEC_CMD21) && ((cmd == EMMC_JEDEC_CMD24) || (cmd == EMMC_JEDEC_CMD17)))
    {
        cmd_no_dt(dev, EMMC_JEDEC_CMD23, block_count, RESP1);
    }

    switch (direction)
    {
        case EMMC_READ:
            if (cmd == EMMC_JEDEC_CMD21)
            {
                idx_chk = 0x1;
                crc_chk = 0x1;
                err_chk = 0x0;
            }
            break;

        case EMMC_WRITE:
            break;

        default:
            Log_Write(LOG_LEVEL_ERROR, "CMD%d is not DATA_XFER NO_DMA or not supported yet\r\n",
                      cmd);
            return ERROR_EMMC_CMD_NOT_SUPPORTED;
    }

    if (block_size && block_size <= 0x800)
    {
        dev->regs->crypto.BLOCKSIZE_R =
            (uint16_t)((dev->regs->crypto.BLOCKSIZE_R & ~BLOCKSIZE_R__XFER_BLOCK_SIZE__MASK) |
                       (block_size << BLOCKSIZE_R__XFER_BLOCK_SIZE__SHIFT));
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "ERROR: Unexpected block size: 0x%X\r\n", block_size);
        return ERROR_EMMC_UNEXPECTED_BLOCK_SIZE;
    }

    // This seems to be needed for when host ver4 = 1 in host_ctrl2_r
    dev->regs->crypto.SDMASA_R = block_count;

    dev->regs->crypto.BLOCKCOUNT_R = 0x0;

    dev->regs->crypto.ARGUMENT_R = arg;

    // Enable command complete status and transfer complete status
    dev->regs->crypto.NORMAL_INT_STAT_EN_R |= (NORMAL_INT_STAT_EN_R__CMD_COMPLETE_STAT_EN__MASK |
                                               NORMAL_INT_STAT_EN_R__XFER_COMPLETE_STAT_EN__MASK |
                                               NORMAL_INT_STAT_EN_R__BUF_WR_READY_STAT_EN__MASK |
                                               NORMAL_INT_STAT_EN_R__BUF_RD_READY_STAT_EN__MASK);

    if (err_chk)
        resp_int = 0x1;
    dev->regs->crypto.XFER_MODE_R =
        (uint16_t)((resp_int << XFER_MODE_R__RESP_INT_DISABLE__SHIFT) |
                   (err_chk << XFER_MODE_R__RESP_ERR_CHK_ENABLE__SHIFT) |
                   (0x0 << XFER_MODE_R__RESP_TYPE__SHIFT) |
                   (multi_block << XFER_MODE_R__MULTI_BLK_SEL__SHIFT) |
                   (direction << XFER_MODE_R__DATA_XFER_DIR__SHIFT) |
                   (multi_block << XFER_MODE_R__AUTO_CMD_ENABLE__SHIFT) | // enable cmd23 for
                                                                          // multi block
                                                                          // transfer
                   (multi_block << XFER_MODE_R__BLOCK_COUNT_ENABLE__SHIFT) |
                   (0x0 << XFER_MODE_R__DMA_ENABLE__SHIFT));

    // Set CMD_R - write to this register starts command
    dev->regs->crypto.CMD_R = (uint16_t)(
        cmd << CMD_R__CMD_INDEX__SHIFT | RESP1 << CMD_R__RESP_TYPE_SELECT__SHIFT |
        NORMAL_CMD << CMD_R__CMD_TYPE__SHIFT | idx_chk << CMD_R__CMD_IDX_CHK_ENABLE__SHIFT |
        crc_chk << CMD_R__CMD_CRC_CHK_ENABLE__SHIFT | CMD_R__DATA_PRESENT_SEL__MASK);

    if (cmd == EMMC_JEDEC_CMD21)
    {
        return SUCCESS;
    }

    if (SUCCESS != wait_for_command_complete(&dev->regs->crypto.NORMAL_INT_STAT_R,
                                             NORMAL_INT_STAT_R__CMD_COMPLETE__MASK, cmd))
    {
        return ERROR_EMMC_GENERAL;
    }

    // Clear CMD_COMPLETE status bit (W1C register)
    dev->regs->crypto.NORMAL_INT_STAT_R = NORMAL_INT_STAT_R__CMD_COMPLETE__MASK;

#ifdef EMMC_DEBUG_PRINTS
    // Get Response Data from Response registers to get necessary information
    // about issued command
    Log_Write(LOG_LEVEL_CRITICAL, "\t\tRESP01 0x%X\r\n", dev->regs->crypto.RESP01_R);
#endif

    switch (direction)
    {
        case EMMC_READ:

            read_through_buffer(data_buff, block_size, block_count, dev, cmd);
            break;

        case EMMC_WRITE:

            write_through_buffer(data_buff, block_size, block_count, dev, cmd);
            break;

        default:
            Log_Write(LOG_LEVEL_ERROR, "Unknown transfer direction\r\n");
            return ERROR_EMMC_GENERAL;
    }

    return SUCCESS;
} // cmd_dt_no_dma()

static int check_internal_clk(const ET_EMMC_DEV_t *dev)
{
    uint32_t timeout_ms_times;

    // Check CLK_CTRL_R.INTERNAL_CLK_STABLE, 150ms according to datasheet
    timeout_ms_times = 150;
    while ((dev->regs->crypto.CLK_CTRL_R & CLK_CTRL_R__INTERNAL_CLK_STABLE__MASK) == 0x0)
    {
        timeout_ms_times--;
        if (timeout_ms_times == 0)
        {
            Log_Write(LOG_LEVEL_ERROR, "ERROR: 150ms Timeout - CLK is not stable\r\n");
            return ERROR_EMMC_CLOCK_IS_NOT_STABLE_AFTER_150MS;
        }
        usdelay(1000);
    }

    return SUCCESS;
}
static int emmc_ext_csd_register_populate(const ET_EMMC_DEV_t *dev, EMMC_MODE_t mode)
{
    return cmd_dt_no_dma(dev, EMMC_JEDEC_CMD8, EMMC_STUFF_BITS_32,
                         EMMC_SIZE_BYTES_EXT_CSD_CARD_REGISTER, 1, gs_emmc_ext_csd_register, mode);
}

static uint8_t emmc_ext_csd_register_get_byte(uint16_t byte_offset)
{
    const uint8_t *ext_csd_ptr = (uint8_t *)gs_emmc_ext_csd_register;
    return ext_csd_ptr[byte_offset];
}

static int emmc_ext_csd_register_set_byte(const ET_EMMC_DEV_t *dev, uint8_t byte_offset,
                                          uint8_t byte_value)
{
    uint8_t *ext_csd_ptr = (uint8_t *)gs_emmc_ext_csd_register;

    int status = cmd_no_dt(dev, EMMC_JEDEC_CMD6,
                           (uint32_t)((EMMC_JEDEC_CMD6_ARG_ACCESS_SET_BITS << 24) |
                                      (byte_offset << 16) | (byte_value << 8)),
                           RESP1b);
    if (status != SUCCESS)
    {
        Log_Write(LOG_LEVEL_ERROR, "ERROR: EXT_CSD setting byte %d failed\r\n", byte_offset);
        return status;
    }

    ext_csd_ptr[byte_offset] = byte_value;
    return SUCCESS;
}

static int emmc_device_reset(const ET_EMMC_DEV_t *dev)
{
    // toggle emmc device reset pin
    dev->regs->crypto_vendor1.EMMC_CTRL_R &= (uint16_t)(~EMMC_CTRL_R__EMMC_RST_N__MASK);
    usdelay(500); // 500uS delay for toggling the reset
    dev->regs->crypto_vendor1.EMMC_CTRL_R |= EMMC_CTRL_R__EMMC_RST_N__MASK;

    return SUCCESS;
}

static int reset_host_controller(ET_EMMC_DEV_t *dev)
{
    // This reset affects the entire Host Controller except for the card detection
    // circuit
    dev->regs->crypto.SW_RST_R = SW_RST_R__SW_RST_ALL__MASK;

    return SUCCESS;
}

static void print_error_report(const ET_EMMC_DEV_t *dev)
{
    Log_Write(LOG_LEVEL_CRITICAL, "NORMAL_INT_STAT_EN_R = 0x%x\r\n",
              dev->regs->crypto.NORMAL_INT_STAT_EN_R);
    Log_Write(LOG_LEVEL_CRITICAL, "NORMAL_INT_STAT_R    = 0x%x\r\n",
              dev->regs->crypto.NORMAL_INT_STAT_R);
    Log_Write(LOG_LEVEL_CRITICAL, "ERROR_INT_STAT_EN_R  = 0x%x\r\n",
              dev->regs->crypto.ERROR_INT_STAT_EN_R);
    Log_Write(LOG_LEVEL_CRITICAL, "ERROR_INT_STAT_R     = 0x%x\r\n",
              dev->regs->crypto.ERROR_INT_STAT_R);
    Log_Write(LOG_LEVEL_CRITICAL, "AUTO_CMD_STAT_R      = 0x%x\r\n",
              dev->regs->crypto.AUTO_CMD_STAT_R);
    Log_Write(LOG_LEVEL_CRITICAL, "ADMA_ERR_STAT_R      = 0x%x\r\n",
              dev->regs->crypto.ADMA_ERR_STAT_R);
}

static void print_id_and_capacity(const ET_EMMC_DEV_t *dev)
{
    uint8_t manufacturer_id;

    Log_Write(LOG_LEVEL_DEBUG, "Read device's CID\r\n");

    cmd_no_dt(dev, EMMC_JEDEC_CMD2, EMMC_STUFF_BITS_32, RESP2);

    manufacturer_id = EMMC_GET_MANUFACTURER_ID(dev->regs->crypto.RESP67_R);

    Log_Write(LOG_LEVEL_CRITICAL, "EMMC:[txt] Manufacturer: ");

    switch (manufacturer_id)
    {
        case EMMC_MANUFACTURER_ID_SAMSUNG:
            Log_Write(LOG_LEVEL_CRITICAL, "SAMSUNG\r\n");
            break;

        case EMMC_MANUFACTURER_ID_SWISSBIT:
            Log_Write(LOG_LEVEL_CRITICAL, "SWISSBIT\r\n");
            break;

        case EMMC_MANUFACTURER_ID_KINGSTON:
            Log_Write(LOG_LEVEL_CRITICAL, "KINGSTON\r\n");
            break;

        case EMMC_MANUFACTURER_ID_ISSI:
            Log_Write(LOG_LEVEL_CRITICAL, "ISSI\r\n");
            break;

        case EMMC_MANUFACTURER_ID_TOSHIBA:
            Log_Write(LOG_LEVEL_CRITICAL, "TOSHIBA\r\n");
            break;

        default:
            Log_Write(LOG_LEVEL_CRITICAL, "unknown to firmware: 0x%x\r\n", manufacturer_id);
            break;
    }

    uint64_t sector_count = 0;

    sector_count |=
        (uint32_t)(emmc_ext_csd_register_get_byte(EMMC_EXT_CSD_BYTE_OFFSET_SEC_COUNT_BYTE_3) << 24);
    sector_count |=
        (uint32_t)(emmc_ext_csd_register_get_byte(EMMC_EXT_CSD_BYTE_OFFSET_SEC_COUNT_BYTE_2) << 16);
    sector_count |=
        (uint32_t)(emmc_ext_csd_register_get_byte(EMMC_EXT_CSD_BYTE_OFFSET_SEC_COUNT_BYTE_1) << 8);
    sector_count |=
        (uint32_t)(emmc_ext_csd_register_get_byte(EMMC_EXT_CSD_BYTE_OFFSET_SEC_COUNT_BYTE_0));

    uint64_t capacity_bytes = sector_count * 512;
    uint8_t capacity_gb = (uint8_t)(capacity_bytes / 1000000000);
    uint8_t capacity_gb_frac = (uint8_t)((capacity_bytes % 1000000000) / 10000000);

    Log_Write(LOG_LEVEL_CRITICAL, "EMMC:[txt] Capacity: %u.%u GB\r\n", capacity_gb,
              capacity_gb_frac);
}

static int emmc_phy_config_sequence(const ET_EMMC_DEV_t *dev, EMMC_MODE_t mode)
{
    uint64_t regVal;
    uint32_t timeout_ms_times;

    // Table 4-6 User Guide
    // TXSLEW = 0x2 - this is the reset value but still write just in case
    // weak pull up == 0x1, rst_sel = 0x1
    // set the pull up on the interface
    regVal = (0x2 << CMDPAD_CNFG__TXSLEW_CTRL_N__SHIFT) |
             (0x2 << CMDPAD_CNFG__TXSLEW_CTRL_P__SHIFT) | (0x1 << CMDPAD_CNFG__WEAKPULL_EN__SHIFT) |
             (0x1 << CMDPAD_CNFG__RXSEL__SHIFT);
    dev->regs->crypto_phy.CMDPAD_CNFG = (uint16_t)regVal;

    regVal = (0x2 << RSTNPAD_CNFG__TXSLEW_CTRL_N__SHIFT) |
             (0x2 << RSTNPAD_CNFG__TXSLEW_CTRL_P__SHIFT) |
             (0x1 << RSTNPAD_CNFG__WEAKPULL_EN__SHIFT) | (0x1 << RSTNPAD_CNFG__RXSEL__SHIFT);
    dev->regs->crypto_phy.RSTNPAD_CNFG = (uint16_t)regVal;

    regVal = (0x2 << DATPAD_CNFG__TXSLEW_CTRL_N__SHIFT) |
             (0x2 << DATPAD_CNFG__TXSLEW_CTRL_P__SHIFT) | (0x1 << DATPAD_CNFG__WEAKPULL_EN__SHIFT) |
             (0x1 << DATPAD_CNFG__RXSEL__SHIFT);
    dev->regs->crypto_phy.DATPAD_CNFG = (uint16_t)regVal;

    // weak pull up == 0x0, rx_sel = 0x0
    regVal = (0x2 << CLKPAD_CNFG__TXSLEW_CTRL_N__SHIFT) |
             (0x2 << CLKPAD_CNFG__TXSLEW_CTRL_P__SHIFT) | (0x0 << CLKPAD_CNFG__WEAKPULL_EN__SHIFT) |
             (0x0 << CLKPAD_CNFG__RXSEL__SHIFT);
    dev->regs->crypto_phy.CLKPAD_CNFG = (uint16_t)regVal;

    if (mode == EMMC_MODE_HS400)
    {
        // STB for HS400
        regVal |= (0x2 << STBPAD_CNFG__TXSLEW_CTRL_N__SHIFT) |
                  (0x2 << STBPAD_CNFG__TXSLEW_CTRL_P__SHIFT) |
                  (0x2 << STBPAD_CNFG__WEAKPULL_EN__SHIFT) | (0x1 << STBPAD_CNFG__RXSEL__SHIFT);
        dev->regs->crypto_phy.STBPAD_CNFG = (uint16_t)regVal;
    }
    // PAD_SN = 0x0 PAD_SP = 0x1 for 100 ohm
    // PAD_SN = 0x4 PAD_SP = 0x5 for 66 ohm
    // PAD_SN = 0x8 PAD_SP = 0x9 for 50 ohm *** default ***
    // PAD_SN = 0xC PAD_SP = 0xD for 40 ohm
    // PAD_SN = 0xE PAD_SP = 0xF for 33 ohm
    regVal = (dev->regs->crypto_phy.PHY_CNFG & (uint64_t)(~PHY_CNFG__PAD_SN__MASK)) |
             (0x8 << PHY_CNFG__PAD_SN__SHIFT);
    regVal = (regVal & (uint64_t)(~PHY_CNFG__PAD_SP__MASK)) | (0x9 << PHY_CNFG__PAD_SP__SHIFT);
    dev->regs->crypto_phy.PHY_CNFG = (uint32_t)regVal;

    // Delay line config
    dev->regs->crypto_phy.COMMDL_CNFG = 0;
    dev->regs->crypto_phy.SDCLKDL_CNFG = 0;
    dev->regs->crypto_phy.SMPLDL_CNFG = 8;
    dev->regs->crypto_phy.ATDL_CNFG = 8;
    dev->regs->crypto_phy.SDCLKDL_DC = gs_emmc_sdclkdl_dc;

    // Wait for the phy powergood
    timeout_ms_times = 50; //
    regVal = dev->regs->crypto_phy.PHY_CNFG & (uint64_t)(PHY_CNFG__PHY_PWRGOOD__MASK);
    while (regVal == EMMC_DEVICE_POWER_IS_BAD)
    {
        timeout_ms_times--;
        if (timeout_ms_times == 0)
            return ERROR_EMMC_POWER_IS_BAD;
        usdelay(1000);
        regVal = dev->regs->crypto_phy.PHY_CNFG & (uint64_t)(PHY_CNFG__PHY_PWRGOOD__MASK);
    }

    // De-asserting PHY reset
    dev->regs->crypto_phy.PHY_CNFG |= PHY_CNFG__PHY_RSTN__MASK;

    return SUCCESS;
}

inline void __attribute__((always_inline)) emmc_subsystem_resets_release(void)
{
    volatile Reset_Manager *const pReset_Manager = (Reset_Manager *)(R_SP_CRU_BASEADDR);

    pReset_Manager->rm_emmc = RESET_MANAGER_RM_EMMC_BCLK_RSTN_MODIFY(pReset_Manager->rm_emmc, 1);
    pReset_Manager->rm_emmc = RESET_MANAGER_RM_EMMC_ACLK_RSTN_MODIFY(pReset_Manager->rm_emmc, 1);
    pReset_Manager->rm_emmc = RESET_MANAGER_RM_EMMC_TCLK_RSTN_MODIFY(pReset_Manager->rm_emmc, 1);
    pReset_Manager->rm_emmc = RESET_MANAGER_RM_EMMC_CCLK_RSTN_MODIFY(pReset_Manager->rm_emmc, 1);
}

static int emmc_host_ctrl_clock_setup_sequence(const ET_EMMC_DEV_t *dev)
{
    uint64_t regVal;
    uint64_t host_ctrl2_preset_value;

    // Get the preset value from host_ctrl2
    host_ctrl2_preset_value = dev->regs->crypto.HOST_CTRL2_R;
    host_ctrl2_preset_value &= HOST_CTRL2_R__PRESET_VAL_ENABLE__MASK;

    // Start
    // Calculate a divisor for SD clock frequency:
    // ? If CAPABILITIES2_R.CLK_MUL is non-zero, use programmable clock mode
    regVal = dev->regs->crypto.CAPABILITIES2_R & CAPABILITIES2_R__CLK_MUL__MASK;
    if (regVal)
    {
        // Not supported in our host controller - we can remove this check
        Log_Write(LOG_LEVEL_DEBUG, "Use programmable clock mode\r\n");
        // if preset is 1, value is automatically set
        if (host_ctrl2_preset_value == 0)
        {
            // Set CLK_CTRL_R.FREQ_SEL and
            // CLK_CTRL_R.CLK_GEN_SELECT as per results in (1)
            regVal = (dev->regs->crypto.CLK_CTRL_R & (uint64_t)(~CLK_CTRL_R__FREQ_SEL__MASK)) |
                     (0xff << CLK_CTRL_R__FREQ_SEL__SHIFT) | CLK_CTRL_R__CLK_GEN_SELECT__MASK;
            dev->regs->crypto.CLK_CTRL_R = (uint16_t)regVal;
        }
    }
    // or
    // ? If CAPABILITIES_1_R.BASE_CLK_FREQ is 0, use a different method
    else
    {
        Log_Write(LOG_LEVEL_DEBUG, "Use divided clock mode\r\n");
        regVal = dev->regs->crypto.CAPABILITIES1_R & CAPABILITIES1_R__BASE_CLK_FREQ__MASK;
        if (regVal)
        {
            // if preset is 1, value is automatically set
            if (host_ctrl2_preset_value == 0)
            {
                // Set CLK_CTRL_R.FREQ_SEL and
                // CLK_CTRL_R.CLK_GEN_SELECT as per results in (1)
                // Base clock frequency is 200Mhz (for PLL1=2GHz)
                // To get a 400Khz clock, set the FREQ_SEL to 0xff == 255
                // 200MHz / (2 x 255) = ~400KHz
                Log_Write(LOG_LEVEL_DEBUG, "Set dividers for desired frequency\r\n");
                regVal =
                    (dev->regs->crypto.CLK_CTRL_R &
                     (uint64_t)(~(CLK_CTRL_R__UPPER_FREQ_SEL__MASK | CLK_CTRL_R__FREQ_SEL__MASK |
                                  CLK_CTRL_R__CLK_GEN_SELECT__MASK))) |
                    (0xff << CLK_CTRL_R__FREQ_SEL__SHIFT);
                dev->regs->crypto.CLK_CTRL_R = (uint16_t)regVal;
            }
            else
            {
                Log_Write(LOG_LEVEL_DEBUG, "PRESET is 1, value is automatically set\r\n");
            }
        }
        else
        {
            Log_Write(LOG_LEVEL_ERROR, "BASE_CLK_FREQ is 0, neeed to use a different method\r\n");
            return ERROR_EMMC_GENERAL;
        }
    }

    Log_Write(LOG_LEVEL_DEBUG, "Enable internal EMMC CLK\r\n");
    // Set CLK_CTRL_R.INTERNAL_CLK_EN
    regVal = dev->regs->crypto.CLK_CTRL_R | CLK_CTRL_R__INTERNAL_CLK_EN__MASK;
    dev->regs->crypto.CLK_CTRL_R = (uint16_t)regVal;

    if (0 != check_internal_clk(dev))
        return ERROR_EMMC_GENERAL;

    Log_Write(LOG_LEVEL_DEBUG, "Enable internal EMMC PLL\r\n");
    // Set CLK_CTRL_R.PLL_ENABLE
    regVal = dev->regs->crypto.CLK_CTRL_R | CLK_CTRL_R__PLL_ENABLE__MASK;
    dev->regs->crypto.CLK_CTRL_R = (uint16_t)regVal;

    if (0 != check_internal_clk(dev))
        return ERROR_EMMC_GENERAL;

    return 0;
} // emmc_host_ctrl_clock_setup_sequence()

static int emmc_host_ctrl_setup_sequence(const ET_EMMC_DEV_t *dev)
{
    uint64_t regVal;
    /*
   eMMC Interface
   Start
   Set Common Parameters for all Versions:
   ? PWR_CTRL_R.SD_BUS_VOL_VDD1
   Set the eMMC bus voltage select to 1.8v
  */
    regVal = (dev->regs->crypto.PWR_CTRL_R & (uint64_t)(~PWR_CTRL_R__SD_BUS_VOL_VDD1__MASK)) |
             (0x5 << PWR_CTRL_R__SD_BUS_VOL_VDD1__SHIFT);
    dev->regs->crypto.PWR_CTRL_R = (uint8_t)regVal;

    // ? TOUT_CTRL_R.TOUT_CNT
    // Set the eMMC data timeout counter value to the max
    regVal = (dev->regs->crypto.TOUT_CTRL_R & (uint64_t)(~TOUT_CTRL_R__TOUT_CNT__MASK)) |
             (0xe << TOUT_CTRL_R__TOUT_CNT__SHIFT);
    dev->regs->crypto.TOUT_CTRL_R = (uint8_t)regVal;

    // ? HOST_CTRL2_R.UHS2_IF_ENABLE=0
    regVal = dev->regs->crypto.HOST_CTRL2_R & (uint64_t)(~HOST_CTRL2_R__UHS2_IF_ENABLE__MASK);
    dev->regs->crypto.HOST_CTRL2_R = (uint16_t)regVal;

    // ? EMMC_CTRL_R.CARD_IS_EMMC=1
    regVal = dev->regs->crypto_vendor1.EMMC_CTRL_R | (uint64_t)(EMMC_CTRL_R__CARD_IS_EMMC__MASK);
    dev->regs->crypto_vendor1.EMMC_CTRL_R = (uint16_t)regVal;

    // Set CLK_CTRL_R
    // [See Host Controller Clock Setup Sequence]
    if (emmc_host_ctrl_clock_setup_sequence(dev) != SUCCESS)
    {
        Log_Write(LOG_LEVEL_ERROR, " ERROR: Host controler Clock sequence failed\r\n");
        return ERROR_EMMC_GENERAL;
    }

    // Set Version 4 Parameters:
    // ? HOST_CTRL2_R.HOST_VER4_ENABLE=1
    regVal = dev->regs->crypto.HOST_CTRL2_R | (uint64_t)(HOST_CTRL2_R__HOST_VER4_ENABLE__MASK);
    dev->regs->crypto.HOST_CTRL2_R = (uint16_t)regVal;

    // ? HOST_CTRL2_R.ADDRESSING=1 (If CAPABILITIES_1_R.SYS_ADDR_64_V4=1)
    if (dev->regs->crypto.CAPABILITIES1_R & CAPABILITIES1_R__SYS_ADDR_64_V4__MASK)
    {
        regVal = dev->regs->crypto.HOST_CTRL2_R | (uint64_t)(HOST_CTRL2_R__ADDRESSING__MASK);
        dev->regs->crypto.HOST_CTRL2_R = (uint16_t)regVal;
    }
    // set auto cmd23 enable
    dev->regs->crypto.HOST_CTRL2_R |= HOST_CTRL2_R__CMD23_ENABLE__MASK;

    /* Release eMMC resets */
    emmc_subsystem_resets_release();

    // disable burst - AXI slave doesn't support burst
    // see RTLMIN-6395
    dev->regs->crypto_vendor1.MBIU_CTRL_R =
        EMMC_CRYPTO_VENDOR1_MBIU_CTRL_R_BURST_INCR16_EN_BURST_INCR16_EN_FALSE |
        EMMC_CRYPTO_VENDOR1_MBIU_CTRL_R_BURST_INCR8_EN_BURST_INCR8_EN_FALSE |
        EMMC_CRYPTO_VENDOR1_MBIU_CTRL_R_BURST_INCR4_EN_BURST_INCR4_EN_FALSE |
        EMMC_CRYPTO_VENDOR1_MBIU_CTRL_R_UNDEFL_INCR_EN_UNDEFL_INCR_EN_FALSE;

    /*
    enable errors
    0x13FF
  */
    dev->regs->crypto.ERROR_INT_STAT_EN_R = ERROR_INT_STAT_EN_R__CMD_TOUT_ERR_STAT_EN__MASK |
                                            ERROR_INT_STAT_EN_R__CMD_CRC_ERR_STAT_EN__MASK |
                                            ERROR_INT_STAT_EN_R__CMD_END_BIT_ERR_STAT_EN__MASK |
                                            ERROR_INT_STAT_EN_R__CMD_IDX_ERR_STAT_EN__MASK |
                                            ERROR_INT_STAT_EN_R__DATA_TOUT_ERR_STAT_EN__MASK |
                                            ERROR_INT_STAT_EN_R__DATA_CRC_ERR_STAT_EN__MASK |
                                            ERROR_INT_STAT_EN_R__DATA_END_BIT_ERR_STAT_EN__MASK |
                                            ERROR_INT_STAT_EN_R__CUR_LMT_ERR_STAT_EN__MASK |
                                            ERROR_INT_STAT_EN_R__AUTO_CMD_ERR_STAT_EN__MASK |
                                            ERROR_INT_STAT_EN_R__ADMA_ERR_STAT_EN__MASK |
                                            ERROR_INT_STAT_EN_R__BOOT_ACK_ERR_STAT_EN__MASK;

    // 1.8V signaling
    dev->regs->crypto.HOST_CTRL2_R |= HOST_CTRL2_R__SIGNALING_EN__MASK;

    return SUCCESS;
} // emmc_host_ctrl_setup_sequence()

static int emmc_card_clock_supply(const ET_EMMC_DEV_t *dev)
{
    uint64_t regVal;

    // ? CLK_CTRL_R.SD_CLK_EN=1
    regVal = dev->regs->crypto.CLK_CTRL_R | (uint64_t)(CLK_CTRL_R__SD_CLK_EN__MASK);
    dev->regs->crypto.CLK_CTRL_R = (uint16_t)regVal;

    return SUCCESS;
}

static int emmc_card_init_sequence(const ET_EMMC_DEV_t *dev, bool verbose)
{
    uint64_t regVal;
    // Send EMMC_JEDEC_CMD1 with address mode required by host
    Log_Write(LOG_LEVEL_DEBUG, "Request OCR - Issue CMD1 for the first time\r\n");
    cmd_no_dt(dev, EMMC_JEDEC_CMD1, EMMC_CAPACITY_GRE_2G_READY_STATE, RESP3);

    // Check for OCR bit busy
    Log_Write(LOG_LEVEL_DEBUG, "Waiting OCR to be ready\r\n");
    uint32_t retries = 100000;
    regVal = dev->regs->crypto.RESP01_R;
    if (0 == regVal)
    {
        if (verbose)
        {
            Log_Write(LOG_LEVEL_ERROR, "ERROR: Device has not finished the power up routine\r\n");
        }

        return ERROR_EMMC_GENERAL;
    }
    while ((regVal & 0x80000000) == 0)
    {
        retries--;
        if (retries == 0)
        {
            Log_Write(LOG_LEVEL_ERROR, "ERROR: Timeout, OCR is not ready.\r\n");
            return ERROR_EMMC_GENERAL;
        }
        // Send EMMC_JEDEC_CMD1 with address mode required by host
        if (regVal)
        {
            Log_Write(LOG_LEVEL_DEBUG, "Repeat CMD1\r\n");
            cmd_no_dt(dev, EMMC_JEDEC_CMD1, EMMC_CAPACITY_GRE_2G_READY_STATE, RESP3);
        }
        regVal = dev->regs->crypto.RESP01_R;
    }
    Log_Write(LOG_LEVEL_DEBUG, "OCR has been read\r\n");

    if ((regVal == EMMC_CAPACITY_LEQ_2G_READY_STATE) | (regVal == EMMC_CAPACITY_GRE_2G_READY_STATE))
    {
        Log_Write(LOG_LEVEL_DEBUG, "Device is compliant, OCR=0x%lX\r\n", regVal);
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "ERROR: Device is NOT compliant, OCR=0x%lX\r\n", regVal);
        // Power down the BUS
        dev->regs->crypto.PWR_CTRL_R &= (uint8_t)(~PWR_CTRL_R__SD_BUS_PWR_VDD1__MASK);
        return ERROR_EMMC_GENERAL;
    }

    // Is support for low voltage power up available?
    // if yes add code for it, if not just continue

    // Obtain device's CID - CMD2
    Log_Write(LOG_LEVEL_DEBUG, "Obtain device's CID\r\n");
    cmd_no_dt(dev, EMMC_JEDEC_CMD2, EMMC_STUFF_BITS_32,
              RESP2); // argument - stuff bits

    uint32_t manufacturer_id = dev->regs->crypto.RESP67_R;

    Log_Write(LOG_LEVEL_DEBUG, "ManufacturerID=0x%x\r\n",
              EMMC_GET_MANUFACTURER_ID(manufacturer_id));

    // Set relative address - CMD3
    Log_Write(LOG_LEVEL_DEBUG, "Set relative address RCA=0x%X\r\n", EMMC_RCA);
    cmd_no_dt(dev, EMMC_JEDEC_CMD3, EMMC_RCA_31_16,
              RESP1); // 31-16 RCA; 15-0 sutff bits

    // CMD7 to set the device in transfer state
    cmd_no_dt(dev, EMMC_JEDEC_CMD7, EMMC_RCA_31_16, RESP1b); // select card

    return SUCCESS;
} // emmc_card_init_sequence()

static int emmc_card_setup_sequence(const ET_EMMC_DEV_t *dev, bool verbose)
{
    uint64_t regVal;
    uint32_t timeout_us;

    // ? HOST_CTRL2_R.UHS2_IF_ENABLE=0
    regVal = dev->regs->crypto.HOST_CTRL2_R & (uint64_t)(~HOST_CTRL2_R__UHS2_IF_ENABLE__MASK);
    dev->regs->crypto.HOST_CTRL2_R = (uint16_t)regVal;

    // ? PWR_CTRL_R.SD_BUS_PWR_VDD1
    regVal = (dev->regs->crypto.PWR_CTRL_R | (uint64_t)PWR_CTRL_R__SD_BUS_PWR_VDD1__MASK);
    dev->regs->crypto.PWR_CTRL_R = (uint8_t)regVal;

    // ? HOST_CTRL2_R.UHS_MODE_SEL=0
    regVal = (dev->regs->crypto.HOST_CTRL2_R & (uint64_t)(~HOST_CTRL2_R__UHS_MODE_SEL__MASK)) |
             (0x0 << HOST_CTRL2_R__UHS_MODE_SEL__SHIFT);
    dev->regs->crypto.HOST_CTRL2_R = (uint16_t)regVal;

    // Enable card clock
    emmc_card_clock_supply(dev);

    // Wait for voltage ramp up time and provide at least 74 SD clocks (200mhz)
    // before issuing command
    // 200MHz (   5ns) clk, 74x   5ns=370ns
    // 200KHz (5000ns) clk, 74x5000ns=370000ns
    // 400KHz (2500ns) clk, 74x2500ns=185000ns
    timeout_us = 400; // setup for 200KHz
    Log_Write(LOG_LEVEL_DEBUG, "Waiting for voltage ramp up time\r\n");
    while (timeout_us--)
        usdelay(10);

    // Card Initalization and Identification sequence
    if (SUCCESS != emmc_card_init_sequence(dev, verbose))
    {
        if (verbose)
        {
            Log_Write(LOG_LEVEL_ERROR, "ERROR: Card init failed\r\n");
        }

        return ERROR_EMMC_GENERAL;
    }

    // set 26-bit data length mode
    dev->regs->crypto.HOST_CTRL2_R |= HOST_CTRL2_R__ADMA2_LEN_MODE__MASK;

    return SUCCESS;
} // emmc_card_setup_sequence()

int Emmc_Probe(EMMC_MODE_t mode, bool verbose)
{
    gs_emmc_dev.regs = (Emmc *)R_PU_EMMC_CFG_BASEADDR;

    Log_Write(LOG_LEVEL_DEBUG, "Reset entire host controller\r\n");
    if (reset_host_controller(&gs_emmc_dev) != SUCCESS)
    {
        if (verbose)
        {
            Log_Write(LOG_LEVEL_ERROR, "ERROR: Host controller reset failed\r\n");
        }
        return ERROR_EMMC_HOST_CONTROLLER_RESET_FAILED;
    }

    Log_Write(LOG_LEVEL_DEBUG, "PHY config started\r\n");
    if (emmc_phy_config_sequence(&gs_emmc_dev, mode) != SUCCESS)
    {
        if (verbose)
        {
            Log_Write(LOG_LEVEL_ERROR, "ERROR: PHY config sequence failed\r\n");
        }
        return ERROR_EMMC_PHY_CONFIG_SEQUENCE_FAILED;
    }

    Log_Write(LOG_LEVEL_DEBUG, "Host controller sequence started\r\n");
    if (emmc_host_ctrl_setup_sequence(&gs_emmc_dev) != SUCCESS)
    {
        if (verbose)
        {
            Log_Write(LOG_LEVEL_ERROR, "ERROR: Host controler sequence failed\r\n");
        }
        return ERROR_EMMC_HOST_CONTROLLER_SEQUENCE_FAILED;
    }

    /*
    This depends on EXT_CSD[RST_N_FUNCTION] but we can't read it until we setup
    card for the first time
  */
    Log_Write(LOG_LEVEL_DEBUG, "Reset eMMC device\r\n");
    if (emmc_device_reset(&gs_emmc_dev) != SUCCESS)
    {
        if (verbose)
        {
            Log_Write(LOG_LEVEL_ERROR, "ERROR: eMMC device reset failed\r\n");
        }
        return ERROR_EMMC_DEVICE_RESET_FAILED;
    }

    Log_Write(LOG_LEVEL_DEBUG, "Starting Card Setup\r\n");
    if (emmc_card_setup_sequence(&gs_emmc_dev, verbose) != SUCCESS)
    {
        if (verbose)
        {
            Log_Write(LOG_LEVEL_ERROR, "ERROR: Card setup sequence failed\r\n");
        }
        return ERROR_EMMC_CARD_SETUP_SEQUENCE_FAILED;
    }

    Log_Write(LOG_LEVEL_DEBUG, "Getting EXT_CSD content\r\n");
    if (emmc_ext_csd_register_populate(&gs_emmc_dev, mode) != SUCCESS)
    {
        if (verbose)
        {
            Log_Write(LOG_LEVEL_ERROR, "ERROR: Getting EXT_CSD failed\r\n");
        }

        return ERROR_EMMC_EXT_CSD_READ_ERROR;
    }

    return SUCCESS;
}

static int emmc_set_high_speed_timing(const ET_EMMC_DEV_t *dev)
{
    // EMMC_JEDEC_CMD6 set to High speed timing - jedec 7.4.65
    return cmd_no_dt(dev, EMMC_JEDEC_CMD6,
                     (EMMC_JEDEC_CMD6_ARG_ACCESS_WRITE_BYTE << 24 |
                      EMMC_EXT_CSD_BYTE_OFFSET_HS_TIMING << 16 | EMMC_HS_TIMING_HIGH_SPEED << 8),
                     RESP1b);
}

static int emmc_set_width_and_strobe(const ET_EMMC_DEV_t *dev)
{
    // EMMC_JEDEC_CMD6 set width, ddr strobe - jedec 7.4.65
    // 183[7] = 1'b1 (enhanced strobe) , 183[3:0] = 0x6 (DDR) - write 8'b1000_0110
    return cmd_no_dt(dev, EMMC_JEDEC_CMD6,
                     (EMMC_JEDEC_CMD6_ARG_ACCESS_WRITE_BYTE << 24 |
                      EMMC_EXT_CSD_BYTE_OFFSET_BUS_WIDTH << 16 |
                      (EMMC_BUS_WIDTH_8_BIT_DDR | EMMC_BUS_WIDTH_STROBE_ENABLE) << 8),
                     RESP1b);
}

static int host_speed_mode_sequence(const ET_EMMC_DEV_t *dev, uint8_t enh_strobe,
                                    EMMC_MODE_t uhs_mode, uint8_t bus_width)
{
    uint64_t regVal;

    // following from "Figure 4-41 Programming Sequence to Switch to Various Speed
    // Modes in an eMMC Device" user guide
    // clear the enhance strobe mode in the controller
    regVal = dev->regs->crypto_vendor1.EMMC_CTRL_R &
             (uint64_t)(~EMMC_CTRL_R__ENH_STROBE_ENABLE__MASK);
    dev->regs->crypto_vendor1.EMMC_CTRL_R = (uint16_t)regVal;
    // set the controller regs for corresponding speed UHS_MODE
    regVal = (dev->regs->crypto.HOST_CTRL2_R & (uint64_t)(~HOST_CTRL2_R__UHS_MODE_SEL__MASK)) |
             (uint64_t)(uhs_mode << HOST_CTRL2_R__UHS_MODE_SEL__SHIFT);
    dev->regs->crypto.HOST_CTRL2_R = (uint16_t)regVal;

    regVal = dev->regs->crypto.HOST_CTRL1_R | HOST_CTRL1_R__HIGH_SPEED_EN__MASK;
    dev->regs->crypto.HOST_CTRL1_R = (uint8_t)regVal;

    switch (bus_width)
    {
        case 8:
            regVal = dev->regs->crypto.HOST_CTRL1_R | HOST_CTRL1_R__EXT_DAT_XFER__MASK;
            dev->regs->crypto.HOST_CTRL1_R = (uint8_t)regVal;
            break;

        case 4:
            regVal = dev->regs->crypto.HOST_CTRL1_R | HOST_CTRL1_R__DAT_XFER_WIDTH__MASK;
            dev->regs->crypto.HOST_CTRL1_R = (uint8_t)regVal;
            break;

        default:
            break;
    }

    // set the enhance strobe mode in the controller
    if (enh_strobe)
    {
        regVal = dev->regs->crypto_vendor1.EMMC_CTRL_R | EMMC_CTRL_R__ENH_STROBE_ENABLE__MASK;
        dev->regs->crypto_vendor1.EMMC_CTRL_R = (uint16_t)regVal;
    }
    return SUCCESS;
} // host_speed_mode_sequence

static int emmc_card_clock_stop(const ET_EMMC_DEV_t *dev)
{
    uint64_t regVal;

    // ? CLK_CTRL_R.SD_CLK_EN=0
    regVal = dev->regs->crypto.CLK_CTRL_R & (uint64_t)(~CLK_CTRL_R__SD_CLK_EN__MASK);
    dev->regs->crypto.CLK_CTRL_R = (uint16_t)regVal;

    return SUCCESS;
}

static int emmc_clockspeedup_sequence(const ET_EMMC_DEV_t *dev, uint16_t freq_sel)
{
    uint64_t regVal;

    // Execute frequency change sequence - figure 4-6 user guide
    emmc_card_clock_stop(dev);
    // Set CLK_CTRL_R.PLL_ENABLE=0
    regVal = dev->regs->crypto.CLK_CTRL_R & (uint64_t)(~CLK_CTRL_R__PLL_ENABLE__MASK);
    dev->regs->crypto.CLK_CTRL_R = (uint16_t)regVal;

    regVal = dev->regs->crypto.CLK_CTRL_R &
             (uint64_t)(~(CLK_CTRL_R__UPPER_FREQ_SEL__MASK | CLK_CTRL_R__FREQ_SEL__MASK));
    regVal |= (uint64_t)(
        ((freq_sel << CLK_CTRL_R__FREQ_SEL__SHIFT) & CLK_CTRL_R__FREQ_SEL__MASK) |
        ((freq_sel >> CLK_CTRL_R__FREQ_SEL__WIDTH) << CLK_CTRL_R__UPPER_FREQ_SEL__SHIFT));
    dev->regs->crypto.CLK_CTRL_R = (uint16_t)regVal;
    // Set CLK_CTRL_R.PLL_ENABLE=1
    regVal = dev->regs->crypto.CLK_CTRL_R | CLK_CTRL_R__PLL_ENABLE__MASK;
    dev->regs->crypto.CLK_CTRL_R = (uint16_t)regVal;

    if (SUCCESS != check_internal_clk(dev))
        return ERROR_EMMC_GENERAL;

    emmc_card_clock_supply(dev);

    return SUCCESS;
} // emmc_clockspeedup_sequence

static int emmc_hs400_phydll_config_sequence(const ET_EMMC_DEV_t *dev)
{
    uint32_t timeout_ms;
    uint8_t regVal;

    // Stop SD/eMMC Device clock Program: CLK_CTRL_R.SD_CLK_EN = 0
    emmc_card_clock_stop(dev);

    // Configure DLL Settings registers
    // DLL_CNFG1 =0x20
    // DLL_CNFG2=<UserSpecified>
    // DLLDL_CNFG=0x60
    // DLL_OFFST=<optional>
    // DLLLBT_CNFG=<UserSpecified>
    dev->regs->crypto_phy.DLL_CNFG1 = 0x20;
    // Set jump_step (DLL_CNFG2) to 10 when not sure of usage (PHY databook, Table
    // 9-3)
    dev->regs->crypto_phy.DLL_CNFG2 =
        0xA; // FIXME Real silicon may need to set some value 0x0 - 0x3f
    dev->regs->crypto_phy.DLLDL_CNFG = 0x60;
    dev->regs->crypto_phy.DLL_OFFST =
        0xe; // FIXME Real silicon may need to set some value 0x0 - 0xe
    dev->regs->crypto_phy.DLLLBT_CNFG =
        0xffff; // FIXME Real silicon may need to set some value 0x0 - 0xffff

    // Restart card clock CLK_CTRL_R.SD_CLK_EN = 1
    emmc_card_clock_supply(dev);

    // Enable DLL DLL_CTRL = 0x1
    dev->regs->crypto_phy.DLL_CTRL |= DLL_CTRL__DLL_EN__MASK;

    // Poll DLL_STATUS register. Wait for DLL Lock DLL_STATUS.LOCK_STS==1
    timeout_ms = 150;
    regVal = dev->regs->crypto_phy.DLL_STATUS;
    while (!(regVal & DLL_STATUS__LOCK_STS__MASK))
    {
        timeout_ms--;
        if (timeout_ms == 0)
        {
            Log_Write(LOG_LEVEL_ERROR, "ERROR: 150ms Timeout - DLL is not locked\r\n");
            return ERROR_EMMC_GENERAL;
        }
        usdelay(1000);
        regVal = dev->regs->crypto_phy.DLL_STATUS;
    }
    // DLL_STATUS.ERR_STS==1 ?
    // IF LOCK_STS =1 and ERR_STS = 1 then DLL is locked to default and
    // has errors. Transactions at this phase can fail
    if (regVal & DLL_STATUS__ERROR_STS__MASK)
    {
        Log_Write(LOG_LEVEL_ERROR, "ERROR: PHY DLL has error, DLL_STATUS = 0x%x\r\n", regVal);
        return ERROR_EMMC_GENERAL;
    }

    return SUCCESS;

} // emmc_hs400_phydll_config_sequence()

static int emmc_mode_select_sequence(const ET_EMMC_DEV_t *dev, EMMC_MODE_t mode)
{
    uint32_t hs_timing;
    uint8_t data_strobe_en = 0;

    int status;

    switch (mode)
    {
        case EMMC_MODE_LEGACY:
            Log_Write(LOG_LEVEL_CRITICAL, "EMMC:[txt] Legacy mode selected\r\n");
            hs_timing = EMMC_HS_TIMING_BACKWARDS_COMPATIBILITY;
            if (gs_emmc_divider < EMMC_200_25_MHZ_DIV)
                gs_emmc_divider = EMMC_200_25_MHZ_DIV;
            break;
        case EMMC_MODE_HSSDR:
        case EMMC_MODE_HSDDR:
            Log_Write(LOG_LEVEL_CRITICAL, "EMMC:[txt] High Speed mode selected\r\n");
            hs_timing = EMMC_HS_TIMING_HIGH_SPEED;
            if (gs_emmc_divider < EMMC_200_50_MHZ_DIV)
                gs_emmc_divider = EMMC_200_50_MHZ_DIV;
            break;
        case EMMC_MODE_HS200:
            Log_Write(LOG_LEVEL_CRITICAL, "EMMC:[txt] HS200 mode selected\r\n");
            hs_timing = EMMC_HS_TIMING_HS200;
            break;
        case EMMC_MODE_HS400:
            Log_Write(LOG_LEVEL_CRITICAL, "EMMC:[txt] HS400 mode selected\r\n");
            hs_timing = EMMC_HS_TIMING_HS400;
            break;
        default:
            hs_timing = EMMC_HS_TIMING_BACKWARDS_COMPATIBILITY;
    }

    if (mode == EMMC_MODE_HS400)
    {
        status = emmc_set_high_speed_timing(dev);
        if (status != SUCCESS)
        {
            Log_Write(LOG_LEVEL_ERROR, "Error setting high-speed timing\r\n");
            return status;
        }

        // set the SDCLK to High speed <52Mhz
        status = emmc_clockspeedup_sequence(dev, EMMC_200_50_MHZ_DIV);
        if (status != SUCCESS)
        {
            Log_Write(LOG_LEVEL_ERROR, "Error setting clockspeedup sequence\r\n");
            return status;
        }

        status = emmc_set_width_and_strobe(dev);
        if (status != SUCCESS)
        {
            Log_Write(LOG_LEVEL_ERROR, "Error seting width and strobe\r\n");
            return status;
        }

        data_strobe_en = 1;
    }
    else
    {
        // EMMC_JEDEC_CMD6 set to 8b mode - jedec 7.4.67
        cmd_no_dt(dev, EMMC_JEDEC_CMD6,
                  (EMMC_JEDEC_CMD6_ARG_ACCESS_WRITE_BYTE << 24 |
                   EMMC_EXT_CSD_BYTE_OFFSET_BUS_WIDTH << 16 | EMMC_BUS_WIDTH_8_BIT_SDR << 8),
                  RESP1b);
    }

    // EMMC_JEDEC_CMD6 set hs_timing - jedec 7.4.65
    status = cmd_no_dt(dev, EMMC_JEDEC_CMD6,
                       (EMMC_JEDEC_CMD6_ARG_ACCESS_WRITE_BYTE << 24 |
                        EMMC_EXT_CSD_BYTE_OFFSET_HS_TIMING << 16 | hs_timing << 8),
                       RESP1b);
    if (status != SUCCESS)
    {
        Log_Write(LOG_LEVEL_ERROR, "Error setting hs_timing\r\n");
        return status;
    }

    // Set the frequency
    status = emmc_clockspeedup_sequence(dev, gs_emmc_divider);
    if (status != SUCCESS)
    {
        Log_Write(LOG_LEVEL_ERROR, "Error setting clock frequency\r\n");
        return status;
    }
    // set data strobe, speed mode and bus width

    status = host_speed_mode_sequence(dev, data_strobe_en, mode, 0x8);
    if (status != SUCCESS)
    {
        Log_Write(LOG_LEVEL_ERROR, "Error host_speed_mode_sequence\r\n");
        return status;
    }

    if (mode == EMMC_MODE_HS400)
    {
        // Figure 4-61 DLL Configuration Flow Sequence
        status = emmc_hs400_phydll_config_sequence(dev);
        if (status != SUCCESS)
        {
            Log_Write(LOG_LEVEL_ERROR, "Error phydll_config_sequence()\r\n");
            return status;
        }
    }

    return SUCCESS;
}

static int emmc_HS200_tuning_sequence(const ET_EMMC_DEV_t *dev)
{
    uint64_t regVal;
    uint32_t data_buff[32];

    memcpy(data_buff, EMMC_JEDEC_HS200_TUNING_PATTERN, 128);

    // reset tuning
    regVal = dev->regs->crypto.HOST_CTRL2_R & (uint64_t)(~HOST_CTRL2_R__SAMPLE_CLK_SEL__MASK);
    dev->regs->crypto.HOST_CTRL2_R = (uint16_t)regVal;

    // Refer to Figure 4-10 Tuning Sequence in user guide
    // Set HOST_CTRL2_R.EXEC_TUNING=1
    regVal = dev->regs->crypto.HOST_CTRL2_R | HOST_CTRL2_R__EXEC_TUNING__MASK;
    dev->regs->crypto.HOST_CTRL2_R = (uint16_t)regVal;

    // enable BUF_RD_READY INT
    dev->regs->crypto.NORMAL_INT_STAT_EN_R |= (NORMAL_INT_STAT_EN_R__BUF_RD_READY_STAT_EN__MASK);

    Log_Write(LOG_LEVEL_DEBUG, "HS200 tuning - it may take a long...\r\n");

    do
    {
        // Issue tuning command (EMMC_JEDEC_CMD21)
        cmd_dt_no_dma(dev, EMMC_JEDEC_CMD21, EMMC_STUFF_BITS_32, 128, 1, data_buff,
                      EMMC_MODE_HS200);

        // Check if NORMAL_INT_STAT_R.BUF_RD_READY = 1?
        uint32_t timeout = 10000;
        while (!(dev->regs->crypto.NORMAL_INT_STAT_R & NORMAL_INT_STAT_R__BUF_RD_READY__MASK))
        {
            timeout--;
            if (timeout == 0)
            {
                Log_Write(LOG_LEVEL_ERROR, "ERROR: Waiting for BUF_RD_READY int - Timeout\r\n");
                return -1;
            }
            usdelay(10);
        }

        // Clear BUF_RD_READY status bit
        dev->regs->crypto.NORMAL_INT_STAT_R = NORMAL_INT_STAT_R__BUF_RD_READY__MASK;

        // Check if HOST_CTRL2_R.EXEC_TUNING = 1 ? If so, execute the sequence again
    } while (dev->regs->crypto.HOST_CTRL2_R & HOST_CTRL2_R__EXEC_TUNING__MASK);

    // Check if HOST_CTRL2_R.SAMPLE_CLK_SEL=1?
    if (!(dev->regs->crypto.HOST_CTRL2_R & HOST_CTRL2_R__SAMPLE_CLK_SEL__MASK))
    {
        Log_Write(LOG_LEVEL_ERROR, "ERROR: Tuning failed\r\n");
        print_error_report(dev);
        return ERROR_EMMC_GENERAL;
    }

    Log_Write(LOG_LEVEL_DEBUG, "AT_STAT_R: 0x%x\r\n", dev->regs->crypto_vendor1.AT_STAT_R);
    Log_Write(LOG_LEVEL_DEBUG, "HS200 tuning done\r\n");
    return SUCCESS;
} // emmc_HS200_tuning_sequence()

static int emmc_diag_iomode(EMMC_MODE_t mode)
{
    int status = Emmc_Iomode_Blk_Rd(0x100, (uint32_t *)0x8000000000, 512, 1, mode);
    return status;
}

static int emmc_diag_dma(void)
{
    int status = Emmc_Adma2_Rd(0x100, (uint32_t *)0x8100000000, 512, 1);
    return status;
}

int Emmc_Setup(EMMC_MODE_t mode)
{
    uint8_t rst_func = emmc_ext_csd_register_get_byte(EMMC_EXT_CSD_BYTE_OFFSET_RST_N_FUNCTION);

    Log_Write(LOG_LEVEL_DEBUG, "EXT_CSD[RST_N_FUNCTION] = 0x%x\r\n", rst_func);
    if (0 == rst_func)
    {
        Log_Write(LOG_LEVEL_DEBUG, "Enabling RST_N_FUNCTION\r\n");
        /*
       [31:26] Set to 0
       [25:24] Access
       [23:16] Index
       [15:8] Value
       [7:3] Set to 0
       [2:0] Cmd Set
    */
        if (SUCCESS != emmc_ext_csd_register_set_byte(&gs_emmc_dev,
                                                      EMMC_EXT_CSD_BYTE_OFFSET_RST_N_FUNCTION,
                                                      EMMC_EXT_CSD_RST_N_FUNCTION_ENABLE))
        {
            Log_Write(LOG_LEVEL_ERROR, "RST_N_FUNCTION set failed\r\n");
            return ERROR_EMMC_GENERAL;
        }
    }

    if (emmc_mode_select_sequence(&gs_emmc_dev, mode) != SUCCESS)
    {
        Log_Write(LOG_LEVEL_ERROR, "ERROR: emmc_mode_select_sequence() failed\r\n");
        return ERROR_EMMC_MODE_SELECT_SEQUENCE_FAILED;
    }
    Log_Write(LOG_LEVEL_DEBUG, "Mode setup done, divider is %d\r\n", gs_emmc_divider);

    if (mode == EMMC_MODE_HS200 && SUCCESS != emmc_HS200_tuning_sequence(&gs_emmc_dev))
    {
        Log_Write(LOG_LEVEL_ERROR, "ERROR: HS200 tuning sequence failed\r\n");
        return ERROR_EMMC_HS200_TUNING_SEQUENCE_FAILED;
    }

    /* EMMC controller is driven by PLL1 clock divided by 10*/

    uint32_t pll1_clock_frequency_mhz;
    uint32_t emmc_clock_frequency_mhz;

    get_pll_frequency(PLL_ID_SP_PLL_1, &pll1_clock_frequency_mhz);
    emmc_clock_frequency_mhz = pll1_clock_frequency_mhz / 10;

    if (gs_emmc_divider > 0)
    {
        uint32_t freq_final = emmc_clock_frequency_mhz / gs_emmc_divider;

        if (freq_final > 0)
        {
            Log_Write(LOG_LEVEL_CRITICAL, "EMMC:[txt] EMMC main clock is %d MHz\r\n", freq_final);
        }
        else
        {
            Log_Write(LOG_LEVEL_CRITICAL, "EMMC:[txt] EMMC main clock is %d KHz\r\n",
                      emmc_clock_frequency_mhz * 1000 / gs_emmc_divider);
        }
    }

    else
    {
        Log_Write(LOG_LEVEL_CRITICAL, "EMMC:[txt] EMMC main clock is %d MHz\r\n",
                  emmc_clock_frequency_mhz);
    }

    print_id_and_capacity(&gs_emmc_dev);

    if (SUCCESS != emmc_diag_iomode(mode))
    {
        Log_Write(LOG_LEVEL_ERROR, "EMMC:[txt] eMMC IOMODE diag failed\r\n");
        return ERROR_EMMC_DIAG_IOMODE_FAILED;
    }

    if (SUCCESS != emmc_diag_dma())
    {
        Log_Write(LOG_LEVEL_ERROR, "EMMC:[txt] eMMC DMA diag failed\r\n");
        return ERROR_EMMC_DIAG_DMA_FAILED;
    }

    return SUCCESS;
}

static int cmd_dt_adma2(const ET_EMMC_DEV_t *dev, uint16_t cmd, uint32_t arg, uint16_t dma_type,
                        uint64_t desc_addr, uint16_t block_size, uint32_t block_count)
{
    uint32_t timeout;
    uint8_t direction = (uint8_t)(commands_directions[cmd]);
    uint8_t idx_chk = 0;
    uint8_t crc_chk = 0;
    uint8_t err_chk = 0;
    uint8_t resp_int = 0;

    uint8_t multi_block = block_count > 1;

    // disable cmd23 enable
    dev->regs->crypto.HOST_CTRL2_R &= (uint16_t)(~HOST_CTRL2_R__CMD23_ENABLE__MASK);

    // dma type setup
    dma_type = ADMA2_SELECT;
    dev->regs->crypto.HOST_CTRL1_R =
        (uint8_t)((dev->regs->crypto.HOST_CTRL1_R & (~HOST_CTRL1_R__DMA_SEL__MASK)) |
                  (dma_type << HOST_CTRL1_R__DMA_SEL__SHIFT));

    // set descriptor address
    dev->regs->crypto.ADMA_SA_LOW_R = (uint32_t)(desc_addr & 0x0FFFFFFFF);
    dev->regs->crypto.ADMA_SA_HIGH_R = (uint32_t)((desc_addr >> 32) & 0x0FFFFFFFF);

    dev->regs->crypto.BLOCKSIZE_R =
        (uint16_t)((dev->regs->crypto.BLOCKSIZE_R & ~BLOCKSIZE_R__XFER_BLOCK_SIZE__MASK) |
                   (block_size << BLOCKSIZE_R__XFER_BLOCK_SIZE__SHIFT));

    dev->regs->crypto.BLOCKCOUNT_R = 0x0;
    dev->regs->crypto.SDMASA_R = block_count;

    dev->regs->crypto.ARGUMENT_R = arg;

    if ((EMMC_READ != direction) && (EMMC_WRITE != direction))
    {
        Log_Write(LOG_LEVEL_ERROR, "CMD%d is not DATA_XFER ADMA or not supported yet\r\n", cmd);
        return ERROR_EMMC_CMD_NOT_ADMA_TRANSFER;
    }

    // Enable command complete status and transfer complete status
    dev->regs->crypto.NORMAL_INT_STAT_EN_R |= (NORMAL_INT_STAT_EN_R__CMD_COMPLETE_STAT_EN__MASK |
                                               NORMAL_INT_STAT_EN_R__XFER_COMPLETE_STAT_EN__MASK |
                                               NORMAL_INT_STAT_EN_R__BUF_WR_READY_STAT_EN__MASK |
                                               NORMAL_INT_STAT_EN_R__BUF_RD_READY_STAT_EN__MASK);

    if (err_chk)
        resp_int = 0x1;
    dev->regs->crypto.XFER_MODE_R =
        (uint16_t)((resp_int << XFER_MODE_R__RESP_INT_DISABLE__SHIFT) |
                   (err_chk << XFER_MODE_R__RESP_ERR_CHK_ENABLE__SHIFT) |
                   (0x0 << XFER_MODE_R__RESP_TYPE__SHIFT) |
                   (multi_block << XFER_MODE_R__MULTI_BLK_SEL__SHIFT) |
                   (direction << XFER_MODE_R__DATA_XFER_DIR__SHIFT) |
                   (0x1 << XFER_MODE_R__AUTO_CMD_ENABLE__SHIFT) | // enable cmd12
                   (multi_block << XFER_MODE_R__BLOCK_COUNT_ENABLE__SHIFT) |
                   (0x1 << XFER_MODE_R__DMA_ENABLE__SHIFT));

    // Set CMD_R - write to this register starts command
    dev->regs->crypto.CMD_R = (uint16_t)(
        cmd << CMD_R__CMD_INDEX__SHIFT | RESP1 << CMD_R__RESP_TYPE_SELECT__SHIFT |
        NORMAL_CMD << CMD_R__CMD_TYPE__SHIFT | idx_chk << CMD_R__CMD_IDX_CHK_ENABLE__SHIFT |
        crc_chk << CMD_R__CMD_CRC_CHK_ENABLE__SHIFT | CMD_R__DATA_PRESENT_SEL__MASK);

    if ((dev->regs->crypto.XFER_MODE_R & XFER_MODE_R__RESP_INT_DISABLE__MASK) == 0)
    {
        // Enable command complete status and transfer complete status
        dev->regs->crypto.NORMAL_INT_STAT_EN_R |=
            (NORMAL_INT_STAT_EN_R__CMD_COMPLETE_STAT_EN__MASK |
             NORMAL_INT_STAT_EN_R__XFER_COMPLETE_STAT_EN__MASK);
        timeout = 10000;
        while (!(dev->regs->crypto.NORMAL_INT_STAT_R & NORMAL_INT_STAT_R__CMD_COMPLETE__MASK))
        {
            timeout--;
            if (timeout == 0)
            {
                Log_Write(LOG_LEVEL_ERROR,
                          "ERROR: Waiting for command complete - Timeout, CMD%d\r\n", cmd);
                return ERROR_EMMC_WAIT_FOR_CMD_COMPLETE_TIMEOUT;
            }
        }

        // Clear CMD_COMPLETE status bit (W1C register)
        dev->regs->crypto.NORMAL_INT_STAT_R = NORMAL_INT_STAT_R__CMD_COMPLETE__MASK;

#ifdef EMMC_DEBUG_PRINTS
        // Get Response Data from Response registers to get necessary information
        // about issued command
        Log_Write(LOG_LEVEL_CRITICAL, "\t\tRESP01 0x%X\r\n", dev->regs->crypto.RESP01_R);
#endif
    }

    timeout = 100000; // x 0.1ms
    while (--timeout)
    {
        if (dev->regs->crypto.ERROR_INT_STAT_R & ERROR_INT_STAT_R__ADMA_ERR__MASK)
        {
            // clear adma err interrupt
            dev->regs->crypto.ERROR_INT_STAT_R = ERROR_INT_STAT_R__ADMA_ERR__MASK;
            // ADMA_ERR_STAT_R__ADMA_ERR_STATES__MASK   0x00000003
            //  0 - stop dma
            //  1 - fetch descriptor
            //  2 - unused
            //  3 - transfer data
            // ADMA_ERR_STAT_R__ADMA_LEN_ERR__MASK      0x00000004
            Log_Write(LOG_LEVEL_ERROR, "ADMA error 0x%X occured - aborting transaction\r\n",
                      dev->regs->crypto.ADMA_ERR_STAT_R);
            return ERROR_EMMC_ADMA_TRANSFER_ERROR;
        }

        if (dev->regs->crypto.NORMAL_INT_STAT_R & NORMAL_INT_STAT_R__XFER_COMPLETE__MASK)
        {
            // clear treansfer complete status
            dev->regs->crypto.NORMAL_INT_STAT_R = NORMAL_INT_STAT_R__XFER_COMPLETE__MASK;
            break;
        }

        usdelay(100);
    } // while
    if (timeout == 0)
    {
        Log_Write(LOG_LEVEL_ERROR, "ERROR: DMA transfer timeout\r\n");
        print_error_report(dev);
        return ERROR_EMMC_ADMA_TRANSFER_TIMEOUT;
    }

    return SUCCESS;
}

int Emmc_Iomode_Blk_Wr(uint32_t addr, uint32_t *data_buff, uint16_t block_size,
                       uint32_t block_count, EMMC_MODE_t mode)
{
    uint16_t command;
    if (block_count == 1)
        command = EMMC_JEDEC_CMD24;
    else
        command = EMMC_JEDEC_CMD25;

    return cmd_dt_no_dma(&gs_emmc_dev, command, addr, block_size, block_count, data_buff, mode);
}

int Emmc_Iomode_Blk_Rd(uint32_t addr, uint32_t *data_buff, uint16_t block_size,
                       uint32_t block_count, EMMC_MODE_t mode)
{
    uint16_t command;
    if (block_count == 1)
        command = EMMC_JEDEC_CMD17;
    else
        command = EMMC_JEDEC_CMD18;

    return cmd_dt_no_dma(&gs_emmc_dev, command, addr, block_size, block_count, data_buff, mode);
}

int Emmc_Adma2_Wr(uint32_t addr, uint32_t *data_buff, uint16_t block_size, uint32_t block_count)
{
    int status;
    desc_line_t *desc_table;

    desc_table = (desc_line_t *)R_PU_SRAM_LO_BASEADDR;                // We must use PU_SRAM here
                                                                      // because EMMC can't
                                                                      // access SP_SRAM
    desc_table->len_10b = ((block_size * block_count) >> 16) & 0x3FF; // upper 10
    desc_table->len_16b = (block_size * block_count) & 0x0FFFF;
    desc_table->address = (long)data_buff;
    desc_table->act = ADMA2_ATTR_TRAN;
    desc_table->interr = 1;
    desc_table->end = 1;
    desc_table->valid = 1;

    status = cmd_dt_adma2(&gs_emmc_dev, EMMC_JEDEC_CMD25, addr, ADMA2_SELECT, (uint64_t)desc_table,
                          block_size, block_count);

    return status;
}

int Emmc_Adma2_Rd(uint32_t addr, uint32_t *data_buff, uint16_t block_size, uint32_t block_count)
{
    int status;
    desc_line_t *desc_table;

    desc_table = (desc_line_t *)R_PU_SRAM_LO_BASEADDR; // We must use PU_SRAM here
                                                       // because EMMC can't
                                                       // access SP_SRAM

    desc_table->len_10b = ((block_size * block_count) >> 16) & 0x3FF; // upper 10
    desc_table->len_16b = (block_size * block_count) & 0x0FFFF;
    desc_table->address = (long)data_buff;
    desc_table->act = ADMA2_ATTR_TRAN;
    desc_table->interr = 1;
    desc_table->end = 1;
    desc_table->valid = 1;

    status = cmd_dt_adma2(&gs_emmc_dev, EMMC_JEDEC_CMD18, addr, ADMA2_SELECT, (uint64_t)desc_table,
                          block_size, block_count);

    return status;
}

union AlignedBuffer
{
    uint8_t bytes[EMMC_BLOCK_SIZE];
    uint64_t dummy; // Ensure proper alignment for 64-bit data
};

int Emmc_read_to_buffer(uint8_t *buffer, size_t size, uint64_t sector)
{
    union AlignedBuffer loc_dataBuff;

    // Multi-block transfers
    if (size > EMMC_BLOCK_SIZE)
    {
        uint32_t num_blocks = (uint32_t)(size / EMMC_BLOCK_SIZE);
        size_t remaining_size = size % EMMC_BLOCK_SIZE;

        // Iterate over each block
        for (uint32_t block_idx = 0; block_idx < num_blocks; block_idx++)
        {
            if (0 == Emmc_Iomode_Blk_Rd((uint32_t)(sector + block_idx),
                                        (uint32_t *)(void *)loc_dataBuff.bytes, EMMC_BLOCK_SIZE, 1,
                                        EMMC_MODE_HS200))
            {
                // Copy the data from loc_dataBuff.bytes to the target buffer
                memcpy(buffer + block_idx * EMMC_BLOCK_SIZE, loc_dataBuff.bytes, EMMC_BLOCK_SIZE);
            }
            else
            {
                return -1;
            }
        }

        // Handle remaining bytes
        if (remaining_size > 0)
        {
            if (0 == Emmc_Iomode_Blk_Rd((uint32_t)(sector + num_blocks),
                                        (uint32_t *)(void *)loc_dataBuff.bytes, EMMC_BLOCK_SIZE, 1,
                                        EMMC_MODE_HS200))
            {
                // Copy the data from loc_dataBuff.bytes to the target buffer
                memcpy(buffer + num_blocks * EMMC_BLOCK_SIZE, loc_dataBuff.bytes, remaining_size);
            }
            else
            {
                return -1;
            }
        }
    }
    else
    {
        // Single, or less, block transfer
        if (0 == Emmc_Iomode_Blk_Rd((uint32_t)sector, (uint32_t *)(void *)loc_dataBuff.bytes,
                                    EMMC_BLOCK_SIZE, 1, EMMC_MODE_HS200))
        {
            // Copy the data from loc_dataBuff.bytes to the target buffer
            memcpy(buffer, loc_dataBuff.bytes, size);
        }
        else
        {
            return -1;
        }
    }

    return SUCCESS;
}
