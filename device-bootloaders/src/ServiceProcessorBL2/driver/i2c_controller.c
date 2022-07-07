/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies or
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
************************************************************************/
/*! \file i2c_controller.c
    \brief A C module that implements the I2C controller services. It
    provides functionality of I2C read/write.

    Public interfaces:
        i2c_init
        i2c_write
        i2c_read
        i2c_disable
*/
/***********************************************************************/
/***
 *
 * @author nikola.rajovic@esperantotech.com
 *
 *
 */
#include <stdint.h>
#include "hwinc/hal_device.h"
#include "bl2_i2c_driver.h"
#include "interrupt.h"
#include "delays.h"

static void i2c_clock_init(ET_I2C_DEV_t *dev)
{
    /* #define sdaHoldTime 0x50005 */

#define ET_SS_MIN_SCL_HIGH       4400
#define ET_SS_MIN_SCL_LOW        5200
#define ET_FS_MIN_SCL_HIGH       400
#define ET_FS_MIN_SCL_LOW        1700
#define ET_HS_MIN_SCL_HIGH_100PF 60
#define ET_HS_MIN_SCL_LOW_100PF  120

#define ET_CLOCK_SPEED_DIVIDER 1000
#define ET_IC_CLK_MHZ          25

    uint16_t val_ss_scl_hcnt, val_ss_scl_lcnt;
    uint16_t val_fs_scl_hcnt, val_fs_scl_lcnt;

    val_ss_scl_hcnt =
        ((uint16_t)(((ET_SS_MIN_SCL_HIGH * ET_IC_CLK_MHZ) / ET_CLOCK_SPEED_DIVIDER) + 1));
    val_ss_scl_lcnt =
        ((uint16_t)(((ET_SS_MIN_SCL_LOW * ET_IC_CLK_MHZ) / ET_CLOCK_SPEED_DIVIDER) + 1));
    val_fs_scl_hcnt =
        ((uint16_t)(((ET_FS_MIN_SCL_HIGH * ET_IC_CLK_MHZ) / ET_CLOCK_SPEED_DIVIDER) + 1));
    val_fs_scl_lcnt =
        ((uint16_t)(((ET_FS_MIN_SCL_LOW * ET_IC_CLK_MHZ) / ET_CLOCK_SPEED_DIVIDER) + 1));

    dev->regs->IC_SS_SCL_HCNT = I2C_IC_SS_SCL_HCNT_IC_SS_SCL_HCNT_SET(val_ss_scl_hcnt);
    dev->regs->IC_SS_SCL_LCNT = I2C_IC_SS_SCL_LCNT_IC_SS_SCL_LCNT_SET(val_ss_scl_lcnt);
    dev->regs->IC_FS_SCL_HCNT = I2C_IC_FS_SCL_HCNT_IC_FS_SCL_HCNT_SET(val_fs_scl_hcnt);
    dev->regs->IC_FS_SCL_LCNT = I2C_IC_FS_SCL_LCNT_IC_FS_SCL_LCNT_SET(val_fs_scl_lcnt);
}

static int i2c_set_connection_params(ET_I2C_DEV_t *dev, ET_I2C_SPEED_t speed)
{
    /* set speed, address format and enable restart on the bus and enable master FSM, */
    const uint32_t et_ic_con_mask =
        I2C_IC_CON_IC_SLAVE_DISABLE_SET(
            I2C_IC_CON_IC_SLAVE_DISABLE_IC_SLAVE_DISABLE_SLAVE_DISABLED) |
        I2C_IC_CON_IC_RESTART_EN_SET(I2C_IC_CON_IC_RESTART_EN_IC_RESTART_EN_ENABLED) |
        I2C_IC_CON_IC_10BITADDR_MASTER_SET(
            I2C_IC_CON_IC_10BITADDR_MASTER_IC_10BITADDR_MASTER_ADDR_7BITS) |
        I2C_IC_CON_SPEED_SET(I2C_IC_CON_SPEED_SPEED_FAST) |
        I2C_IC_CON_MASTER_MODE_SET(I2C_IC_CON_MASTER_MODE_MASTER_MODE_ENABLED);

    switch (speed)
    {
        case ET_I2C_SPEED_100k:
            dev->regs->IC_CON = et_ic_con_mask |
                                I2C_IC_CON_SPEED_SET(I2C_IC_CON_SPEED_SPEED_STANDARD);
            return ET_I2C_OK;
        case ET_I2C_SPEED_400k:
            dev->regs->IC_CON = et_ic_con_mask | I2C_IC_CON_SPEED_SET(I2C_IC_CON_SPEED_SPEED_FAST);
            return ET_I2C_OK;
        default: /* should never happen, but I do not know flag waivers in advance */
            return ET_I2C_ERROR_SPEED_BAD_VALUE;
    }
}

int i2c_init(ET_I2C_DEV_t *dev, ET_I2C_SPEED_t speed, uint8_t addr_slave)
{
    if (dev->isInitialized == true)
        return ET_I2C_ERROR_DEV_ALREADY_INITIALIZED;

    /* Init the i2c bus lock to released state */
    dev->bus_lock_handle = xSemaphoreCreateMutexStatic(&dev->bus_lock);

    if (!dev->bus_lock_handle)
    {
        return ET_I2C_ERROR_BUS_LOCK_INIT;
    }

    /* disable */
    dev->regs->IC_ENABLE = I2C_IC_ENABLE_ENABLE_SET(I2C_IC_ENABLE_ENABLE_ENABLE_DISABLED);

#define I2C_IRQ_ALL 0xFFF

    /* mask all interrupts */
    dev->regs->IC_INTR_MASK = I2C_IRQ_ALL;

    /* clear all interrupts */
    volatile uint32_t dummyRegRead;
    dummyRegRead = dev->regs->IC_CLR_INTR;
    (void)dummyRegRead;

    i2c_clock_init(dev);

    /* Set IC_SMBUS_THIGH_MAX_IDLE_COUNT  to 'hF to short idle time after reset */
    dev->regs->IC_SMBUS_THIGH_MAX_IDLE_COUNT = 0x0000000F;

    /* set SDA hold time to ic_clk*valueInReg for receive and for transmit */
    if (speed == ET_I2C_SPEED_100k)
        dev->regs->IC_SDA_HOLD =
            0x50005; /* this is a magic number, I do not know where it comes from */

    i2c_set_connection_params(dev, speed);

    /* set TX mode using slave/target address and
        set slave/target address */
    dev->regs->IC_TAR = I2C_IC_TAR_SPECIAL_SET(I2C_IC_TAR_SPECIAL_SPECIAL_DISABLED) |
                        I2C_IC_TAR_IC_TAR_SET(addr_slave);

    dev->regs->IC_ENABLE = I2C_IC_ENABLE_RESET_VALUE |
                           I2C_IC_ENABLE_ENABLE_SET(I2C_IC_ENABLE_ENABLE_ENABLE_ENABLED);

    dev->isInitialized = true;

    return ET_I2C_OK;
}

int i2c_write(ET_I2C_DEV_t *dev, uint8_t regAddr, const uint8_t *txDataBuff, uint8_t txDataCount)
{
    if (dev->isInitialized == false)
        return ET_I2C_ERROR_DEV_NOT_INITIALIZED;

    if (!INT_Is_Trap_Context() && (xSemaphoreTake(dev->bus_lock_handle, portMAX_DELAY) == pdTRUE))
    {
        /* "write" command byte  is sent throguh MasterFSM, we only need to hint it
            in the very first data byte we explicitly write to I2C */
        dev->regs->IC_DATA_CMD = (uint32_t)(I2C_IC_DATA_CMD_RESET_VALUE |
                                            I2C_IC_DATA_CMD_CMD_SET(I2C_IC_DATA_CMD_CMD_CMD_WRITE) |
                                            I2C_IC_DATA_CMD_DAT_SET(regAddr));
        /* send register address */

        /* write all but last byte, since it contains the stop condition generation request */
        for (uint8_t n = 0; n < txDataCount - 1; n++)
        {
            dev->regs->IC_DATA_CMD =
                (uint32_t)(I2C_IC_DATA_CMD_RESET_VALUE |
                           I2C_IC_DATA_CMD_CMD_SET(I2C_IC_DATA_CMD_CMD_CMD_WRITE) |
                           I2C_IC_DATA_CMD_DAT_SET(txDataBuff[n]));
            /*value for above register */
        }
        dev->regs->IC_DATA_CMD = (uint32_t)(
            I2C_IC_DATA_CMD_RESET_VALUE | I2C_IC_DATA_CMD_CMD_SET(I2C_IC_DATA_CMD_CMD_CMD_WRITE) |
            I2C_IC_DATA_CMD_STOP_SET(I2C_IC_DATA_CMD_STOP_STOP_ENABLE) |
            I2C_IC_DATA_CMD_DAT_SET(txDataBuff[txDataCount - 1]));
        /* value for above register */

        /* wait for transfer to finish */
        while (I2C_IC_STATUS_MST_ACTIVITY_GET(dev->regs->IC_STATUS) ==
               I2C_IC_STATUS_MST_ACTIVITY_MST_ACTIVITY_ACTIVE)
            ;
        /* TODO: Fix I2C driver to use ack instead of adding delays after each operation 
        delays are added to prevent SP from hanging with back to back I2C read/writes. */
        msdelay(1);
        xSemaphoreGive(dev->bus_lock_handle);
    }
    else
    {
        return ET_I2C_ERROR_BUS_LOCK;
    }

    return ET_I2C_OK;
}

int i2c_read(ET_I2C_DEV_t *dev, uint8_t regAddr, uint8_t *rxDataBuff, uint8_t rxDataCount)
{
    if (dev->isInitialized == false)
        return ET_I2C_ERROR_DEV_NOT_INITIALIZED;

    if (!INT_Is_Trap_Context() && xSemaphoreTake(dev->bus_lock_handle, portMAX_DELAY) == pdTRUE)
    {
        /* Now read what we have written previously */
        dev->regs->IC_DATA_CMD = (uint32_t)(
            I2C_IC_DATA_CMD_RESET_VALUE | I2C_IC_DATA_CMD_CMD_SET(I2C_IC_DATA_CMD_CMD_CMD_WRITE) |
            I2C_IC_DATA_CMD_RESTART_SET(I2C_IC_DATA_CMD_RESTART_RESTART_ENABLE) |
            I2C_IC_DATA_CMD_DAT_SET(regAddr));
        /* send register address */

        /* this is where the actual read request(s) happens */
        for (uint8_t n = 0; n < rxDataCount - 1; n++)
        {
            dev->regs->IC_DATA_CMD =
                (uint32_t)(I2C_IC_DATA_CMD_RESET_VALUE |
                           I2C_IC_DATA_CMD_CMD_SET(I2C_IC_DATA_CMD_CMD_CMD_READ));
            /* this is what will init an actual read */
        }
        dev->regs->IC_DATA_CMD = (uint32_t)(
            I2C_IC_DATA_CMD_RESET_VALUE | I2C_IC_DATA_CMD_CMD_SET(I2C_IC_DATA_CMD_CMD_CMD_READ) |

            /* this is what will init an actual read */
            I2C_IC_DATA_CMD_STOP_SET(I2C_IC_DATA_CMD_STOP_STOP_ENABLE));

        /* check RX fifo for received bytes */
        /* wait until we get the bytes we want in the RX fifo. */
        while (I2C_IC_RXFLR_RXFLR_GET(dev->regs->IC_RXFLR) < rxDataCount)
            ;

        for (uint8_t n = 0; n < rxDataCount; n++)
        {
            rxDataBuff[n] = I2C_IC_DATA_CMD_DAT_GET(dev->regs->IC_DATA_CMD);
        }
        /* TODO: Fix I2C driver to use ack instead of adding delays after each operation 
        delays are added to prevent SP from hanging with back to back I2C read/writes. */
        msdelay(1);
        xSemaphoreGive(dev->bus_lock_handle);
    }
    else
    {
        return ET_I2C_ERROR_BUS_LOCK;
    }

    return ET_I2C_OK;
}

int i2c_disable(ET_I2C_DEV_t *dev)
{
    if (dev->isInitialized == false)
        return ET_I2C_ERROR_DEV_NOT_INITIALIZED;

    if (!INT_Is_Trap_Context() && xSemaphoreTake(dev->bus_lock_handle, portMAX_DELAY) == pdTRUE)
    {
        dev->regs->IC_ENABLE = I2C_IC_ENABLE_RESET_VALUE |
                               I2C_IC_ENABLE_ENABLE_SET(I2C_IC_ENABLE_ENABLE_ENABLE_DISABLED);

        dev->isInitialized = false;
        xSemaphoreGive(dev->bus_lock_handle);
    }
    else
    {
        return ET_I2C_ERROR_BUS_LOCK;
    }

    return ET_I2C_OK;
}
