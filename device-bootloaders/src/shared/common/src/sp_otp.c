/*-------------------------------------------------------------------------
* Copyright (C) 2019,2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

//#define PRINT_OTP_STATUS

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bl_error_code.h"
#include "etsoc/isa/io.h"
#include "sp_otp.h"

#include "hwinc/sp_cru_reset.h"
#include "hwinc/sp_cru.h"
#include "hwinc/hal_device.h"

#define OTP_ENTRY_SIZE_BYTES 4u
#define OTP_BANK_SIZE_BYTES  16u

#define OTP_BANK_SIZE_ENTRIES (OTP_BANK_SIZE_BYTES / OTP_ENTRY_SIZE_BYTES)

#define OTP_CALC_START_BANK_INDEX(entry_index) ((entry_index) / OTP_BANK_SIZE_ENTRIES)

#define OTP_CALC_END_BANK_INDEX(entry_index, entry_count, entry_size) \
    ((((entry_index)*OTP_ENTRY_SIZE_BYTES) + (entry_count) * (entry_size)-1) / OTP_BANK_SIZE_BYTES)

#define WRCK_TIMEOUT 1000

static uint32_t gs_sp_otp_lock_bits[2];
static bool gs_is_otp_available;
static OTP_CHICKEN_BITS_t gs_chicken_bits;
static MISC_CONFIGURATION_BITS_t gs_misc_configuration;

int sp_otp_init(void)
{
    uint32_t rm_status2;
    const volatile uint32_t *sp_otp_data = (uint32_t *)R_SP_EFUSE_BASEADDR;
    uint32_t main_wrck;
    uint32_t vault_wrck;
    uint32_t timeout;

    /* Use the fast clock (100 MHz) to drive the SP WRCK */

    main_wrck = ioread32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_CLK_MAIN_WRCK_ADDRESS);
    main_wrck = CLOCK_MANAGER_CM_CLK_MAIN_WRCK_SEL_MODIFY(main_wrck, 1);
    iowrite32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_CLK_MAIN_WRCK_ADDRESS, main_wrck);
    timeout = WRCK_TIMEOUT;
    do
    {
        timeout--;
        if (0 == timeout)
        {
            main_wrck = CLOCK_MANAGER_CM_CLK_MAIN_WRCK_SEL_MODIFY(main_wrck, 0);
            iowrite32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_CLK_MAIN_WRCK_ADDRESS, main_wrck);
            break;
        }
        main_wrck = ioread32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_CLK_MAIN_WRCK_ADDRESS);
    } while (0 == CLOCK_MANAGER_CM_CLK_MAIN_WRCK_STABLE_GET(main_wrck));

    /* Use the fast clock (100 MHz) to drive the VaultIP WRCK */
    vault_wrck = ioread32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_CLK_VAULT_WRCK_ADDRESS);
    vault_wrck = CLOCK_MANAGER_CM_CLK_VAULT_WRCK_SEL_MODIFY(vault_wrck, 1);
    iowrite32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_CLK_VAULT_WRCK_ADDRESS, vault_wrck);
    timeout = WRCK_TIMEOUT;
    do
    {
        timeout--;
        if (0 == timeout)
        {
            vault_wrck = CLOCK_MANAGER_CM_CLK_VAULT_WRCK_SEL_MODIFY(vault_wrck, 0);
            iowrite32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_CLK_VAULT_WRCK_ADDRESS, vault_wrck);
            break;
        }
        vault_wrck = ioread32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_CLK_VAULT_WRCK_ADDRESS);
    } while (0 == CLOCK_MANAGER_CM_CLK_VAULT_WRCK_STABLE_GET(vault_wrck));

    // check the bootstrap pins to test if the OTP is available
    rm_status2 = ioread32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_STATUS2_ADDRESS);
    if (RESET_MANAGER_RM_STATUS2_ERROR_SMS_UDR_GET(rm_status2))
    {
        gs_sp_otp_lock_bits[0] = 0xFFFFFFFF;
        gs_sp_otp_lock_bits[1] = 0xFFFFFFFF;
        gs_chicken_bits.R = 0xFFFFFFFF;
        gs_misc_configuration.R = 0xFFFFFFFF;
        gs_is_otp_available = false;
    }
    else
    {
        gs_sp_otp_lock_bits[0] = sp_otp_data[SP_OTP_INDEX_LOCK_REG_BITS_31_00_OFFSET];
        gs_sp_otp_lock_bits[1] = sp_otp_data[SP_OTP_INDEX_LOCK_REG_BITS_63_32_OFFSET];
        gs_chicken_bits.R = sp_otp_data[SP_OTP_INDEX_CHICKEN_BITS];
        gs_misc_configuration.R = sp_otp_data[SP_OTP_INDEX_MISC_CONFIGURATION];
        gs_is_otp_available = true;
    }

    return 0;
}

static inline bool otp_is_bank_locked(uint32_t bank_index)
{
    uint32_t reg_index = bank_index / 32;
    uint32_t bit_index = bank_index & 0x1Fu;
    uint32_t mask = 1u << bit_index;

    if (gs_sp_otp_lock_bits[reg_index] & mask)
    {
        return false;
    }
    else
    {
        return true;
    }
}

static inline bool otp_is_bank_range_locked(uint32_t start_idx, uint32_t end_idx)
{
    for (uint32_t idx = start_idx; idx <= end_idx; idx++)
    {
        if (!otp_is_bank_locked(idx))
        {
            return false;
        }
    }
    return true;
}

int sp_otp_read(uint32_t index, uint32_t *result)
{
    const volatile uint32_t *sp_otp_data = (uint32_t *)R_SP_EFUSE_BASEADDR;

    if (NULL == result)
    {
        return ERROR_INVALID_ARGUMENT;
    }
    if (index >= 256)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!gs_is_otp_available)
    {
        *result = 0xFFFFFFFF;
    }
    else
    {
        *result = sp_otp_data[index];
    }

    return 0;
}

static bool sp_wrck_ensure_100Mhz(void)
{
    uint32_t timeout;
    uint32_t wrck = ioread32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_CLK_MAIN_WRCK_ADDRESS);
    uint32_t off = CLOCK_MANAGER_CM_CLK_MAIN_WRCK_OFF_GET(wrck);
    uint32_t sel = CLOCK_MANAGER_CM_CLK_MAIN_WRCK_SEL_GET(wrck);
    uint32_t stable = CLOCK_MANAGER_CM_CLK_MAIN_WRCK_STABLE_GET(wrck);

    // If it's already at 100MHz and stable, we are done
    if ((off == 0) && (sel == 1) && (stable == 1))
    {
        return true;
    }

    // If the WRCK clock is off for low power, turn it on
    if (off)
    {
        wrck = CLOCK_MANAGER_CM_CLK_MAIN_WRCK_OFF_MODIFY(wrck, 0);
    }

    // Use the fast clock (100 MHz) to drive the SP WRCK
    if (sel == 0)
    {
        wrck = CLOCK_MANAGER_CM_CLK_MAIN_WRCK_SEL_MODIFY(wrck, 1);
    }

    // Write new configuration
    iowrite32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_CLK_MAIN_WRCK_ADDRESS, wrck);

    timeout = WRCK_TIMEOUT;
    do
    {
        timeout--;
        if (0 == timeout)
        {
            // WRCK failed to lock at 100MHz, switch back to 10MHz and return failure
            wrck = CLOCK_MANAGER_CM_CLK_MAIN_WRCK_SEL_MODIFY(wrck, 0);
            iowrite32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_CLK_MAIN_WRCK_ADDRESS, wrck);
            return false;
        }
        wrck = ioread32(R_SP_CRU_BASEADDR + CLOCK_MANAGER_CM_CLK_MAIN_WRCK_ADDRESS);
    } while (0 == CLOCK_MANAGER_CM_CLK_MAIN_WRCK_STABLE_GET(wrck));

    return true;
}

int sp_otp_write(uint32_t offset, uint32_t value)
{
    volatile uint32_t *sp_otp_data = (uint32_t *)R_SP_EFUSE_BASEADDR;
    uint32_t bank_index = OTP_CALC_START_BANK_INDEX(offset);
    uint32_t old_value;
    uint32_t new_value;

    if (!gs_is_otp_available)
    {
        return ERROR_SP_OTP_OTP_NOT_AVAILABLE;
    }

    if (otp_is_bank_locked(bank_index))
    {
#ifdef PRINT_OTP_STATUS
        MESSAGE_ERROR("OTP register %02x is locked!\n", offset);
#endif
        return 0;
    }

    // SMS server (synopsys) is built with 100Mhz as target clock. Make sure SP WRCK is at 100Mhz.
    if (!sp_wrck_ensure_100Mhz())
    {
        return ERROR_SP_OTP_SP_WRCK_NOT_100MHZ;
    }

    old_value = sp_otp_data[offset];
    value = value & old_value;
    sp_otp_data[offset] = value;
    new_value = sp_otp_data[offset];
#ifdef PRINT_OTP_STATUS
    MESSAGE_INFO_DEBUG("Set OTP[%02x] to 0x%08x, result: 0x%08x\n", offset, value, new_value);
#endif
    if (SP_OTP_INDEX_LOCK_REG_BITS_31_00_OFFSET == offset)
    {
        gs_sp_otp_lock_bits[0] = new_value;
    }
    else if (SP_OTP_INDEX_LOCK_REG_BITS_63_32_OFFSET == offset)
    {
        gs_sp_otp_lock_bits[1] = new_value;
    }

    return 0;
}

int sp_otp_get_neighborhood_status_mask(uint32_t index, uint32_t *value)
{
    if (!gs_is_otp_available)
    {
        return ERROR_SP_OTP_OTP_NOT_AVAILABLE;
    }

    if ((index > 3) || (value == NULL))
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if (0 != sp_otp_read(SP_OTP_INDEX_NEIGHBORHOOD_STATUS_NH0_NH31 + index, value))
    {
        *value = 0;
        return ERROR_SP_OTP_OTP_READ;
    }

    return 0;
}

int sp_otp_get_neighborhood_status_nh128_nh135_other(
    OTP_NEIGHBORHOOD_STATUS_NH128_NH135_OTHER_t *status)
{
    if (!gs_is_otp_available)
    {
        return ERROR_SP_OTP_OTP_NOT_AVAILABLE;
    }

    if (status == NULL)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if (0 != sp_otp_read(SP_OTP_INDEX_NEIGHBORHOOD_STATUS_NH128_NH135_OTHER, &(status->R)))
    {
        status->R = 0;
        return ERROR_SP_OTP_OTP_READ;
    }

    return 0;
}

int sp_otp_get_shire_speed(uint8_t shire_num, uint8_t *speed)
{
    uint32_t otp_index;
    uint32_t word;

    if (!gs_is_otp_available)
    {
        return ERROR_SP_OTP_OTP_NOT_AVAILABLE;
    }

    if (speed == NULL)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    otp_index = (uint8_t)(SP_OTP_INDEX_SHIRE_SPEED + (shire_num / 8));

    if (0 != sp_otp_read(otp_index, &word))
    {
        return ERROR_SP_OTP_OTP_READ;
    }

    *speed = (word >> ((shire_num % 8) * 4)) & 0xF;

    return 0;
}

int sp_otp_get_pll_configuration_data(OTP_PLL_CONFIGURATION_OVERRIDE_t *table, uint32_t table_size,
                                      uint32_t *count)
{
    uint32_t index;
    uint32_t wr_index;
    uint32_t valid_count = 0;
    OTP_PLL_CONFIGURATION_OVERRIDE_t otp_cfg_override;
    const uint32_t otp_pll_bank_start_index =
        OTP_CALC_START_BANK_INDEX(SP_OTP_INDEX_PLL_CFG_OVERRIDE);
    const uint32_t otp_pll_bank_end_index = OTP_CALC_END_BANK_INDEX(
        SP_OTP_INDEX_PLL_CFG_OVERRIDE, SP_OTP_MAX_PLL_CONFIG_ENTRIES_COUNT, sizeof(table[0]));

    if (!gs_is_otp_available)
    {
        return ERROR_SP_OTP_OTP_NOT_AVAILABLE;
    }

    if (!otp_is_bank_range_locked(otp_pll_bank_start_index, otp_pll_bank_end_index))
    {
        // Not all OTP PLL banks are locked
        *count = 0;
        return 0;
    }

    for (index = 0; index < SP_OTP_MAX_PLL_CONFIG_ENTRIES_COUNT; index++)
    {
        if (0 != sp_otp_read(SP_OTP_INDEX_PLL_CFG_OVERRIDE + index, &(otp_cfg_override.R)))
        {
            return ERROR_SP_OTP_OTP_READ;
        }
        if (0 == otp_cfg_override.B.IGN)
        {
            valid_count++;
        }
    }

    if (NULL != count)
    {
        *count = valid_count;
    }

    if (NULL == table || table_size < valid_count)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    wr_index = 0;
    for (index = 0; index < SP_OTP_MAX_PLL_CONFIG_ENTRIES_COUNT; index++)
    {
        if (0 != sp_otp_read(SP_OTP_INDEX_PLL_CFG_OVERRIDE + index, &(otp_cfg_override.R)))
        {
            for (uint32_t temp_index = 0; temp_index < wr_index; temp_index++)
            {
                table[temp_index].R = 0;
            }
            return ERROR_SP_OTP_OTP_READ;
        }
        if (0 == otp_cfg_override.B.IGN)
        {
            table[wr_index].R = otp_cfg_override.R;
            wr_index++;
            if (wr_index == valid_count)
            {
                break;
            }
        }
    }

    return 0;
}

int sp_otp_get_pll_configuration_delay(OTP_PLL_CONFIGURATION_DELAY_t *config_delay)
{
    if (!gs_is_otp_available)
    {
        return ERROR_SP_OTP_OTP_NOT_AVAILABLE;
    }

    if (NULL == config_delay)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!otp_is_bank_locked(OTP_CALC_START_BANK_INDEX(SP_OTP_INDEX_PLL_CONFIG_DELAY)))
    {
        config_delay->R = 0xFFFFFFFF;
        return 0;
    }

    if (0 != sp_otp_read(SP_OTP_INDEX_PLL_CONFIG_DELAY, &(config_delay->R)))
    {
        config_delay->R = 0;
        return ERROR_SP_OTP_OTP_READ;
    }

    return 0;
}

int sp_otp_get_pll_lock_timeout(OTP_PLL_LOCK_TIMEOUT_t *lock_timeout)
{
    if (!gs_is_otp_available)
    {
        return ERROR_SP_OTP_OTP_NOT_AVAILABLE;
    }

    if (NULL == lock_timeout)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!otp_is_bank_locked(OTP_CALC_START_BANK_INDEX(SP_OTP_INDEX_PLL_LOCK_TIMEOUT)))
    {
        lock_timeout->R = 0xFFFFFFFF;
        return 0;
    }

    if (0 != sp_otp_read(SP_OTP_INDEX_PLL_LOCK_TIMEOUT, &(lock_timeout->R)))
    {
        lock_timeout->R = 0;
        return ERROR_SP_OTP_OTP_READ;
    }

    return 0;
}

int sp_otp_get_uart_configuration_data(OTP_UART_CONFIGURATION_OVERRIDE_t *configuration)
{
    if (!gs_is_otp_available)
    {
        return ERROR_SP_OTP_OTP_NOT_AVAILABLE;
    }

    if (NULL == configuration)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!otp_is_bank_locked(OTP_CALC_START_BANK_INDEX(SP_OTP_INDEX_UART_CFG_OVERRIDE)))
    {
        configuration->R = 0xFFFFFFFF;
        return 0;
    }

    if (0 != sp_otp_read(SP_OTP_INDEX_UART_CFG_OVERRIDE, &(configuration->R)))
    {
        configuration->R = 0;
        return ERROR_SP_OTP_OTP_READ;
    }

    return 0;
}

int sp_otp_get_spi_configuration_data(OTP_SPI_CONFIGURATION_OVERRIDE_t *pll_100,
                                      OTP_SPI_CONFIGURATION_OVERRIDE_t *pll_75,
                                      OTP_SPI_CONFIGURATION_OVERRIDE_t *pll_50,
                                      OTP_SPI_CONFIGURATION_OVERRIDE_t *pll_off)
{
    if (!gs_is_otp_available)
    {
        return ERROR_SP_OTP_OTP_NOT_AVAILABLE;
    }

    if (!otp_is_bank_locked(OTP_CALC_START_BANK_INDEX(SP_OTP_INDEX_SPI_CFG_OVERRIDE)))
    {
        if (NULL != pll_100)
        {
            pll_100->R = 0xFFFFFFFF;
        }
        if (NULL != pll_75)
        {
            pll_75->R = 0xFFFFFFFF;
        }
        if (NULL != pll_50)
        {
            pll_50->R = 0xFFFFFFFF;
        }
        if (NULL != pll_off)
        {
            pll_off->R = 0xFFFFFFFF;
        }
        return 0;
    }

    if ((NULL != pll_100) && (0 != sp_otp_read(SP_OTP_INDEX_SPI_CFG_OVERRIDE + 0, &(pll_100->R))))
    {
        pll_100->R = 0;
        goto READ_ERROR;
    }

    if ((NULL != pll_75) && (0 != sp_otp_read(SP_OTP_INDEX_SPI_CFG_OVERRIDE + 1, &(pll_75->R))))
    {
        pll_75->R = 0;
        goto READ_ERROR;
    }

    if ((NULL != pll_50) && (0 != sp_otp_read(SP_OTP_INDEX_SPI_CFG_OVERRIDE + 2, &(pll_50->R))))
    {
        pll_50->R = 0;
        goto READ_ERROR;
    }

    if ((NULL != pll_off) && (0 != sp_otp_read(SP_OTP_INDEX_SPI_CFG_OVERRIDE + 3, &(pll_off->R))))
    {
        pll_off->R = 0;
        goto READ_ERROR;
    }

    return 0;

READ_ERROR:
    return ERROR_SP_OTP_OTP_READ;
}

int sp_otp_get_flash_configuration_data(OTP_FLASH_CONFIGURATION_OVERRIDE_t *spi0,
                                        OTP_FLASH_CONFIGURATION_OVERRIDE_t *spi1)
{
    if (!gs_is_otp_available)
    {
        return ERROR_SP_OTP_OTP_NOT_AVAILABLE;
    }

    if (!otp_is_bank_locked(OTP_CALC_START_BANK_INDEX(SP_OTP_INDEX_FLASH_CFG_OVERRIDE)))
    {
        if (NULL != spi0)
        {
            spi0->dw0.R = 0xFFFFFFFF;
            spi0->dw1.R = 0xFFFFFFFF;
        }
        if (NULL != spi1)
        {
            spi1->dw0.R = 0xFFFFFFFF;
            spi1->dw1.R = 0xFFFFFFFF;
        }
        return 0;
    }

    if (NULL != spi0)
    {
        if (0 != sp_otp_read(SP_OTP_INDEX_FLASH_CFG_OVERRIDE + 0, &(spi0->dw0.R)))
        {
            goto READ_ERROR;
        }
        if (0 != sp_otp_read(SP_OTP_INDEX_FLASH_CFG_OVERRIDE + 1, &(spi0->dw1.R)))
        {
            goto READ_ERROR;
        }
    }
    if (NULL != spi1)
    {
        if (0 != sp_otp_read(SP_OTP_INDEX_FLASH_CFG_OVERRIDE + 2, &(spi1->dw0.R)))
        {
            goto READ_ERROR;
        }
        if (0 != sp_otp_read(SP_OTP_INDEX_FLASH_CFG_OVERRIDE + 3, &(spi1->dw1.R)))
        {
            goto READ_ERROR;
        }
    }

    return 0;

READ_ERROR:
    if (NULL != spi0)
    {
        spi0->dw0.R = 0;
        spi0->dw1.R = 0;
    }
    if (NULL != spi1)
    {
        spi1->dw0.R = 0;
        spi1->dw1.R = 0;
    }
    return ERROR_SP_OTP_OTP_READ;
}

static int get_whitelist_configuration_data(uint32_t flags,
                                            OTP_PCIE_WHITELIST_ENTRY_OVERRIDE_t *table,
                                            uint32_t table_size, uint32_t *count)
{
    uint32_t index;
    uint32_t wr_index;
    uint32_t tmp_index;
    uint32_t valid_count = 0;
    OTP_PCIE_WHITELIST_ENTRY_OVERRIDE_1_t entry_override;
    const uint32_t otp_pcie_bank_start_index =
        OTP_CALC_START_BANK_INDEX(SP_OTP_INDEX_PCIE_PHY_CFG_WHITEIST_OVERRIDE);
    const uint32_t otp_pcie_bank_end_index =
        OTP_CALC_END_BANK_INDEX(SP_OTP_INDEX_PCIE_PHY_CFG_WHITEIST_OVERRIDE,
                                SP_OTP_MAX_PCIE_CONFIG_ENTRIES_COUNT, sizeof(table[0]));

    if (!gs_is_otp_available)
    {
        return ERROR_SP_OTP_OTP_NOT_AVAILABLE;
    }

    if (!otp_is_bank_range_locked(otp_pcie_bank_start_index, otp_pcie_bank_end_index))
    {
        // Not all OTP PCIe banks are locked
        *count = 0;
        return 0;
    }

    for (index = 0; index < SP_OTP_MAX_PCIE_CONFIG_ENTRIES_COUNT; index++)
    {
        if (0 != sp_otp_read(SP_OTP_INDEX_PCIE_PHY_CFG_WHITEIST_OVERRIDE + 1 + 2 * index,
                             &(entry_override.R)))
        {
            return ERROR_SP_OTP_OTP_READ;
        }
        if (flags == entry_override.B.FLAGS)
        {
            valid_count++;
        }
    }

    if (NULL != count)
    {
        *count = valid_count;
    }

    if (NULL == table || table_size < valid_count)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    wr_index = 0;
    for (index = 0; index < SP_OTP_MAX_PCIE_CONFIG_ENTRIES_COUNT; index++)
    {
        if (0 != sp_otp_read(SP_OTP_INDEX_PCIE_PHY_CFG_WHITEIST_OVERRIDE + 1 + 2 * index,
                             &(entry_override.R)))
        {
            for (tmp_index = 0; tmp_index < wr_index; tmp_index++)
            {
                table[tmp_index].dw_0.R = 0;
                table[tmp_index].dw_1.R = 0;
            }
            return ERROR_SP_OTP_OTP_READ;
        }
        if (flags == entry_override.B.FLAGS)
        {
            if (0 != sp_otp_read(SP_OTP_INDEX_PCIE_PHY_CFG_WHITEIST_OVERRIDE + 2 * index,
                                 &(table[wr_index].dw_0.R)))
            {
                for (tmp_index = 0; tmp_index < wr_index; tmp_index++)
                {
                    table[tmp_index].dw_0.R = 0;
                    table[tmp_index].dw_1.R = 0;
                }
                return ERROR_SP_OTP_OTP_READ;
            }
            table[wr_index].dw_1.R = entry_override.R;
            wr_index++;
            if (wr_index == valid_count)
            {
                break;
            }
        }
    }

    return 0;
}

int sp_otp_get_pcie_whitelist_configuration_data(OTP_PCIE_WHITELIST_ENTRY_OVERRIDE_t *table,
                                                 uint32_t table_size, uint32_t *count)
{
    return get_whitelist_configuration_data(0x0, table, table_size, count);
}

int sp_otp_get_pre_configuration_data(OTP_PCIE_WHITELIST_ENTRY_OVERRIDE_t *table,
                                      uint32_t table_size, uint32_t *count)
{
    return get_whitelist_configuration_data(0x1, table, table_size, count);
}

int sp_otp_get_post_configuration_data(OTP_PCIE_WHITELIST_ENTRY_OVERRIDE_t *table,
                                       uint32_t table_size, uint32_t *count)
{
    return get_whitelist_configuration_data(0x2, table, table_size, count);
}

static int get_otp_counter(uint32_t index, uint32_t *counter)
{
    uint32_t counter_data;
    uint32_t count;

    if (NULL == counter)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!gs_is_otp_available)
    {
        return ERROR_SP_OTP_OTP_NOT_AVAILABLE;
    }

    if (otp_is_bank_locked(OTP_CALC_START_BANK_INDEX(index)))
    {
        return ERROR_SP_OTP_BANK_LOCKED;
    }

    if (0 != sp_otp_read(index, &counter_data))
    {
        return ERROR_SP_OTP_OTP_READ;
    }

    count = 0;
    for (uint32_t n = 0; n < 32; n++)
    {
        if (0 == (counter_data & 1))
        {
            count++;
        }
        counter_data = counter_data >> 1u;
    }

    *counter = count;
    return 0;
}

int sp_otp_get_sp_issuing_ca_certificate_monotonic_version_counter(uint32_t *counter)
{
    return get_otp_counter(SP_OTP_INDEX_SP_ISSUING_CA_CERTIFICATE_MONOTONIC_VERSION_COUNTER,
                           counter);
}

int sp_otp_get_pcie_cfg_data_certificate_monotonic_version_counter(uint32_t *counter)
{
    return get_otp_counter(
        SP_OTP_INDEX_SP_PCIE_PHY_CONFIG_DATA_CERTIFICATE_MONOTONIC_VERSION_COUNTER, counter);
}

int sp_otp_get_sp_bl1_certificate_monotonic_version_counter(uint32_t *counter)
{
    return get_otp_counter(SP_OTP_INDEX_SP_BL1_CERTIFICATE_MONOTONIC_VERSION_COUNTER, counter);
}

int sp_otp_get_vaultip_chicken_bit(bool *disable_vault)
{
    if (NULL == disable_vault)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!gs_is_otp_available)
    {
        *disable_vault = false;
        return ERROR_SP_OTP_OTP_NOT_AVAILABLE;
    }

    if (gs_chicken_bits.B.VaultIP_Chicken_Bit)
    {
        *disable_vault = true;
    }
    else
    {
        *disable_vault = false;
    }

    return 0;
}

int sp_otp_get_vaultip_plain_text_firmware_chicken_bit(bool *allow_plain_text_firmware)
{
    if (NULL == allow_plain_text_firmware)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!gs_is_otp_available)
    {
        *allow_plain_text_firmware = false;
        return ERROR_SP_OTP_OTP_NOT_AVAILABLE;
    }

    if (gs_chicken_bits.B.VaultIP_FWp_Allowed_Chicken_Bit)
    {
        *allow_plain_text_firmware = true;
    }
    else
    {
        *allow_plain_text_firmware = false;
    }

    return 0;
}

int sp_otp_get_vaultip_clock_switch_chicken_bit(bool *switch_clocks,
                                                uint32_t *clock_switch_input_token)
{
    int rv;

    if (NULL == switch_clocks || NULL == clock_switch_input_token)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!gs_is_otp_available)
    {
        rv = ERROR_SP_OTP_OTP_NOT_AVAILABLE;
        goto FAILURE;
    }

    if (gs_chicken_bits.B.VaultIP_FWp_Allowed_Chicken_Bit)
    {
        *switch_clocks = true;
        if (0 != sp_otp_read(SP_OTP_INDEX_VAULTIP_FIRMWARE_CLOCK_SWITCH_INPUT_TOKEN,
                             clock_switch_input_token))
        {
            rv = ERROR_SP_OTP_OTP_READ;
            goto FAILURE;
        }
    }
    else
    {
        *switch_clocks = false;
        *clock_switch_input_token = 0xFFFFFFFF;
    }

    return 0;

FAILURE:
    *switch_clocks = false;
    *clock_switch_input_token = 0xFFFFFFFF;
    return rv;
}

int sp_otp_get_vaultip_FIPS_mode(bool *use_FIPS_mode)
{
    if (NULL == use_FIPS_mode)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!gs_is_otp_available)
    {
        *use_FIPS_mode = false;
        return ERROR_SP_OTP_OTP_NOT_AVAILABLE;
    }

    if (0 == gs_misc_configuration.B.VaultIP_FIPS)
    {
        *use_FIPS_mode = true;
    }
    else
    {
        *use_FIPS_mode = false;
    }

    return 0;
}

int sp_otp_get_engineering_mode(bool *allow_engineering_keys)
{
    if (NULL == allow_engineering_keys)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!gs_is_otp_available)
    {
        *allow_engineering_keys = false;
        return ERROR_SP_OTP_OTP_NOT_AVAILABLE;
    }

    if (0 == gs_misc_configuration.B.ENG)
    {
        *allow_engineering_keys = false;
    }
    else
    {
        *allow_engineering_keys = true;
    }

    return 0;
}

int sp_otp_get_signatures_check_chicken_bit(bool *ignore_signatures)
{
    if (NULL == ignore_signatures)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!gs_is_otp_available)
    {
        *ignore_signatures = false;
        return ERROR_SP_OTP_OTP_NOT_AVAILABLE;
    }

    if (gs_chicken_bits.B.Signatures_Chicken_Bit)
    {
        *ignore_signatures = true;
    }
    else
    {
        *ignore_signatures = false;
    }

    return 0;
}

int sp_otp_get_pcie_cfg_white_list_check_chicken_bit(bool *ignore_white_list)
{
    if (NULL == ignore_white_list)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!gs_is_otp_available)
    {
        *ignore_white_list = false;
        return ERROR_SP_OTP_OTP_NOT_AVAILABLE;
    }

    if (gs_chicken_bits.B.PCIe_WhiteList_Chicken_Bit)
    {
        *ignore_white_list = true;
    }
    else
    {
        *ignore_white_list = false;
    }

    return 0;
}

int sp_otp_get_sp_l1_cache_chicken_bit(bool *enable_l1_cache)
{
    if (NULL == enable_l1_cache)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!gs_is_otp_available)
    {
        *enable_l1_cache = false;
        return ERROR_SP_OTP_OTP_NOT_AVAILABLE;
    }

    if (gs_chicken_bits.B.SP_L1_Cache_Chicken_Bit)
    {
        *enable_l1_cache = false;
    }
    else
    {
        *enable_l1_cache = true;
    }

    return 0;
}

int sp_otp_get_vaultip_firmware_check_start_timeout(bool *use_otp_timeout, uint32_t *timeout)
{
    int rv;

    if (NULL == use_otp_timeout || NULL == timeout)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!gs_is_otp_available)
    {
        rv = ERROR_SP_OTP_OTP_NOT_AVAILABLE;
        goto FAILURE;
    }

    if (0 == gs_misc_configuration.B.VaultIP_FCST)
    {
        *use_otp_timeout = true;
        if (0 != sp_otp_read(SP_OTP_INDEX_VAULTIP_FIRMWARE_CHECK_START_TIMEOUT, timeout))
        {
            rv = ERROR_SP_OTP_OTP_READ;
            goto FAILURE;
        }
    }
    else
    {
        *use_otp_timeout = false;
        *timeout = 0xFFFFFFFF;
    }

    return 0;

FAILURE:
    *use_otp_timeout = false;
    *timeout = 0xFFFFFFFF;
    return rv;
}

int sp_otp_get_vaultip_firmware_accepted_timeout(bool *use_otp_timeout, uint32_t *timeout)
{
    int rv;

    if (NULL == use_otp_timeout || NULL == timeout)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!gs_is_otp_available)
    {
        rv = ERROR_SP_OTP_OTP_NOT_AVAILABLE;
        goto FAILURE;
    }

    if (0 == gs_misc_configuration.B.VaultIP_FAT)
    {
        *use_otp_timeout = true;
        if (0 != sp_otp_read(SP_OTP_INDEX_VAULTIP_FIRMWARE_ACCEPTED_TIMEOUT, timeout))
        {
            rv = ERROR_SP_OTP_OTP_READ;
            goto FAILURE;
        }
    }
    else
    {
        *use_otp_timeout = false;
        *timeout = 0xFFFFFFFF;
    }

    return 0;

FAILURE:
    *use_otp_timeout = false;
    *timeout = 0xFFFFFFFF;
    return rv;
}

int sp_otp_get_vaultip_firmware_output_token_timeout_1(bool *use_otp_timeout, uint32_t *timeout)
{
    int rv;

    if (NULL == use_otp_timeout || NULL == timeout)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!gs_is_otp_available)
    {
        rv = ERROR_SP_OTP_OTP_NOT_AVAILABLE;
        goto FAILURE;
    }

    if (0 == gs_misc_configuration.B.VaultIP_ROTT1)
    {
        *use_otp_timeout = true;
        if (0 != sp_otp_read(SP_OTP_INDEX_VAULTIP_FIRMWARE_OUTPUT_TOKEN_TIMEOUT_1, timeout))
        {
            rv = ERROR_SP_OTP_OTP_READ;
            goto FAILURE;
        }
    }
    else
    {
        *use_otp_timeout = false;
        *timeout = 0xFFFFFFFF;
    }

    return 0;

FAILURE:
    *use_otp_timeout = false;
    *timeout = 0xFFFFFFFF;
    return rv;
}

int sp_otp_get_vaultip_firmware_output_token_timeout_2(bool *use_otp_timeout, uint32_t *timeout)
{
    int rv;

    if (NULL == use_otp_timeout || NULL == timeout)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!gs_is_otp_available)
    {
        rv = ERROR_SP_OTP_OTP_NOT_AVAILABLE;
        goto FAILURE;
    }

    if (0 == gs_misc_configuration.B.VaultIP_ROTT2)
    {
        *use_otp_timeout = true;
        if (0 != sp_otp_read(SP_OTP_INDEX_VAULTIP_FIRMWARE_OUTPUT_TOKEN_TIMEOUT_2, timeout))
        {
            rv = ERROR_SP_OTP_OTP_READ;
            goto FAILURE;
        }
    }
    else
    {
        *use_otp_timeout = false;
        *timeout = 0xFFFFFFFF;
    }

    return 0;

FAILURE:
    *use_otp_timeout = false;
    *timeout = 0xFFFFFFFF;
    return rv;
}

int sp_otp_get_vaultip_firmware_output_token_timeout_3(bool *use_otp_timeout, uint32_t *timeout)
{
    int rv;

    if (NULL == use_otp_timeout || NULL == timeout)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!gs_is_otp_available)
    {
        rv = ERROR_SP_OTP_OTP_NOT_AVAILABLE;
        goto FAILURE;
    }

    if (0 == gs_misc_configuration.B.VaultIP_ROTT3)
    {
        *use_otp_timeout = true;
        if (0 != sp_otp_read(SP_OTP_INDEX_VAULTIP_FIRMWARE_OUTPUT_TOKEN_TIMEOUT_3, timeout))
        {
            rv = ERROR_SP_OTP_OTP_READ;
            goto FAILURE;
        }
    }
    else
    {
        *use_otp_timeout = false;
        *timeout = 0xFFFFFFFF;
    }

    return 0;

FAILURE:
    *use_otp_timeout = false;
    *timeout = 0xFFFFFFFF;
    return rv;
}

int sp_otp_get_vaultip_firmware_output_token_timeout_4(bool *use_otp_timeout, uint32_t *timeout)
{
    int rv;

    if (NULL == use_otp_timeout || NULL == timeout)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!gs_is_otp_available)
    {
        rv = ERROR_SP_OTP_OTP_NOT_AVAILABLE;
        goto FAILURE;
    }

    if (0 == gs_misc_configuration.B.VaultIP_ROTT4)
    {
        *use_otp_timeout = true;
        if (0 != sp_otp_read(SP_OTP_INDEX_VAULTIP_FIRMWARE_OUTPUT_TOKEN_TIMEOUT_4, timeout))
        {
            rv = ERROR_SP_OTP_OTP_READ;
            goto FAILURE;
        }
    }
    else
    {
        *use_otp_timeout = false;
        *timeout = 0xFFFFFFFF;
    }

    return 0;

FAILURE:
    *use_otp_timeout = false;
    *timeout = 0xFFFFFFFF;
    return rv;
}

int sp_otp_get_vaultip_firmware_output_token_timeout_5(bool *use_otp_timeout, uint32_t *timeout)
{
    int rv;

    if (NULL == use_otp_timeout || NULL == timeout)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!gs_is_otp_available)
    {
        rv = ERROR_SP_OTP_OTP_NOT_AVAILABLE;
        goto FAILURE;
    }

    if (0 == gs_misc_configuration.B.VaultIP_ROTT5)
    {
        *use_otp_timeout = true;
        if (0 != sp_otp_read(SP_OTP_INDEX_VAULTIP_FIRMWARE_OUTPUT_TOKEN_TIMEOUT_5, timeout))
        {
            rv = ERROR_SP_OTP_OTP_READ;
            goto FAILURE;
        }
    }
    else
    {
        *use_otp_timeout = false;
        *timeout = 0xFFFFFFFF;
    }

    return 0;

FAILURE:
    *use_otp_timeout = false;
    *timeout = 0xFFFFFFFF;
    return rv;
}

int sp_otp_get_vaultip_clock_switch_token(bool *use_clock_switch, uint32_t *token)
{
    int rv;

    if (NULL == use_clock_switch || NULL == token)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!gs_is_otp_available)
    {
        rv = ERROR_SP_OTP_OTP_NOT_AVAILABLE;
        goto FAILURE;
    }

    if (0 == gs_misc_configuration.B.VaultIP_Clock_Switch)
    {
        *use_clock_switch = true;
        if (0 != sp_otp_read(SP_OTP_INDEX_VAULTIP_FIRMWARE_CLOCK_SWITCH_INPUT_TOKEN, token))
        {
            rv = ERROR_SP_OTP_OTP_READ;
            goto FAILURE;
        }
    }
    else
    {
        *use_clock_switch = false;
        *token = 0xFFFFFFFF;
    }

    return 0;

FAILURE:
    *use_clock_switch = false;
    *token = 0xFFFFFFFF;
    return rv;
}

int sp_otp_get_special_customer_designator(uint8_t *designator)
{
    int rv;
    OTP_CRITICAL_PAOTP_SPECIAL_CUSTOMER_DESIGNATOR_t temp;

    if (NULL == designator)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!gs_is_otp_available)
    {
        rv = ERROR_SP_OTP_OTP_NOT_AVAILABLE;
        goto FAILURE;
    }

    if (0 != sp_otp_read(SP_OTP_INDEX_SPECIAL_CUSTOMER_DESIGNATOR, &(temp.R)))
    {
        rv = ERROR_SP_OTP_OTP_READ;
        goto FAILURE;
    }
    *designator = (uint8_t)((~temp.B.special_customer_id) & 0xFF);

    return 0;

FAILURE:
    *designator = 0;
    return rv;
}

int sp_otp_get_critical_patch_data(uint32_t index, OTP_CRITICAL_PATCH_t *patch_data)
{
    int rv;
    uint32_t otp_patch_index;
    uint32_t bank_index;

    if (index >= OTP_MAX_CRITICAL_PATCH_COUNT || NULL == patch_data)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!gs_is_otp_available)
    {
        rv = ERROR_SP_OTP_OTP_NOT_AVAILABLE;
        goto FAILURE;
    }

    otp_patch_index = SP_OTP_INDEX_CRITICAL_PATCH_0_ADDRESS_HI + index * 4;
    bank_index = OTP_CALC_START_BANK_INDEX(otp_patch_index);

    // Critical patch data in OTP is only valid if the corresponding bank lock bit is set
    if (!otp_is_bank_locked(bank_index))
    {
        rv = ERROR_SP_OTP_BANK_NOT_LOCKED;
        goto FAILURE;
    }

    if (0 != sp_otp_read(otp_patch_index, &(patch_data->dw0.R)))
    {
        rv = ERROR_SP_OTP_OTP_READ;
        goto FAILURE;
    }
    if (0 != sp_otp_read(otp_patch_index + 1, &(patch_data->dw1.R)))
    {
        rv = ERROR_SP_OTP_OTP_READ;
        goto FAILURE;
    }
    if (0 != sp_otp_read(otp_patch_index + 2, &(patch_data->dw2.R)))
    {
        rv = ERROR_SP_OTP_OTP_READ;
        goto FAILURE;
    }
    if (0 != sp_otp_read(otp_patch_index + 3, &(patch_data->dw3.R)))
    {
        rv = ERROR_SP_OTP_OTP_READ;
        goto FAILURE;
    }

    return 0;

FAILURE:
    memset(patch_data, 0xFF, sizeof(OTP_CRITICAL_PATCH_t));
    return rv;
}

void sp_otp_diag(void)
{
#if 0
    MESSAGE_INFO("OTP: %03x %03x\n", 0xFFF & (gs_chicken_bits.R >> 20u), 0xFFF & (gs_misc_configuration.R >> 20u));
    uint32_t dw0, dw1, dw2, dw3;
    sp_otp_read(232, &dw0);
    sp_otp_read(233, &dw1);
    sp_otp_read(234, &dw2);
    sp_otp_read(235, &dw3);
    MESSAGE_INFO("SP_OTP words 232-235: %08x %08x %08x %08x\n", dw0, dw1, dw2, dw3);
#endif
}

int sp_otp_get_silicon_revision(OTP_SILICON_REVISION_t *si_revision)
{
    if (!gs_is_otp_available)
    {
        return ERROR_SP_OTP_OTP_NOT_AVAILABLE;
    }

    if (NULL == si_revision)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if (!otp_is_bank_locked(OTP_CALC_START_BANK_INDEX(SP_OTP_INDEX_SILICON_REVISION)))
    {
        si_revision->R = 0xFFFFFFFF;
        return 0;
    }

    if (0 != sp_otp_read(SP_OTP_INDEX_SILICON_REVISION, &(si_revision->R)))
    {
        si_revision->R = 0;
        return ERROR_SP_OTP_OTP_READ;
    }

    return 0;
}
