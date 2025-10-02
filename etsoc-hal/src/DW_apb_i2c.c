/* --------------------------------------------------------------------
** 
** Synopsys DesignWare DW_apb_i2c Software Driver Kit and
** documentation (hereinafter, "Software") is an Unsupported
** proprietary work of Synopsys, Inc. unless otherwise expressly
** agreed to in writing between Synopsys and you.
** 
** The Software IS NOT an item of Licensed Software or Licensed
** Product under any End User Software License Agreement or Agreement
** for Licensed Product with Synopsys or any supplement thereto. You
** are permitted to use and redistribute this Software in source and
** binary forms, with or without modification, provided that
** redistributions of source code must retain this notice. You may not
** view, use, disclose, copy or distribute this file or any information
** contained herein except pursuant to this license grant from Synopsys.
** If you do not agree with this notice, including the disclaimer
** below, then you are not authorized to use the Software.
** 
** THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS"
** BASIS AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
** FOR A PARTICULAR PURPOSE ARE HEREBY DISCLAIMED. IN NO EVENT SHALL
** SYNOPSYS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
** OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
** USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
** DAMAGE.
** 
** --------------------------------------------------------------------
*/

#include "DW_common.h"          // common header for all drivers
#include "DW_apb_i2c_public.h"  // DW_apb_i2c public header
#include "DW_apb_i2c_private.h" // DW_apb_i2c private header

// This definition is used by the assetion macros to determine the
// current file name.  It is defined in the DW_common_dbc.h header.
DW_DEFINE_THIS_FILE;

/**********************************************************************/

int dw_i2c_init(struct dw_device *dev)
{
    int retval;

    I2C_COMMON_REQUIREMENTS(dev);

    // disable device
    retval = dw_i2c_disable(dev);
    // if device is not busy (i.e. it is now disabled)
    if(retval == 0) {
        // disable all interrupts
        dw_i2c_maskIrq(dev, I2c_irq_all);
        dw_i2c_clearIrq(dev, I2c_irq_all);

        // reset instance variables
        dw_i2c_resetInstance(dev);

        // autoConfigure component parameters if possible
        retval = dw_i2c_autoCompParams(dev);
    }

    return retval;
}

/**********************************************************************/

void dw_i2c_enable(struct dw_device *dev)
{
    uint32_t reg;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    reg = 0;
    DW_BIT_SET(reg, I2C_ENABLE_ENABLE, 0x1);
    I2C_OUTP(reg, portmap->enable);
}

/**********************************************************************/

int dw_i2c_disable(struct dw_device *dev)
{
    int retval;
    uint32_t reg;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    if(dw_i2c_isBusy(dev) == false) {
        reg = 0;
        retval = 0;
        DW_BIT_SET(reg, I2C_ENABLE_ENABLE, 0);
        I2C_OUTP(reg, portmap->enable);
    }
    else
        retval = -DW_EBUSY;

    return retval;
}

/**********************************************************************/

bool dw_i2c_isEnabled(struct dw_device *dev)
{
    bool retval;
    uint32_t reg;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    reg = I2C_INP(portmap->enable);
    retval = (bool) DW_BIT_GET(reg, I2C_ENABLE_ENABLE);

    return retval;
}

/**********************************************************************/

bool dw_i2c_isBusy(struct dw_device *dev)
{
    bool retval;
    uint32_t reg;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    reg = I2C_INP(portmap->status);
    if(DW_BIT_GET(reg, I2C_STATUS_ACTIVITY) == 0x0)
        retval = false;
    else
        retval = true;

    return retval;
}

/**********************************************************************/

int dw_i2c_setSpeedMode(struct dw_device *dev, enum dw_i2c_speed_mode
        mode)
{
    int retval;
    uint32_t reg;
    struct dw_i2c_param *param;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    param = (struct dw_i2c_param *) dev->comp_param;
    portmap = (struct dw_i2c_portmap *) dev->base_address;

    if(mode > param->max_speed_mode)
        retval = -DW_ENOSYS;
    else if(dw_i2c_isEnabled(dev) == false) {
        retval = 0;
        reg = I2C_INP(portmap->con);
        // avoid bus write if possible
        if(DW_BIT_GET(reg, I2C_CON_SPEED) != mode) {
            DW_BIT_SET(reg, I2C_CON_SPEED, mode);
            I2C_OUTP(reg, portmap->con);
        }
    }
    else
        retval = -DW_EPERM;

    return retval;
}

/**********************************************************************/

enum dw_i2c_speed_mode dw_i2c_getSpeedMode(struct dw_device *dev)
{
    uint32_t reg;
    enum dw_i2c_speed_mode retval;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    reg = I2C_INP(portmap->con);
    retval = (enum dw_i2c_speed_mode) DW_BIT_GET(reg, I2C_CON_SPEED);

    return retval;
}

/**********************************************************************/

int dw_i2c_setMasterAddressMode(struct dw_device *dev, enum
        dw_i2c_address_mode mode)
{
    int retval;
    uint32_t reg;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    if(dw_i2c_isEnabled(dev) == false) {
        retval = 0;
        reg = I2C_INP(portmap->con);
        if(DW_BIT_GET(reg, I2C_CON_10BITADDR_MASTER) != mode) {
            DW_BIT_SET(reg, I2C_CON_10BITADDR_MASTER, mode);
            I2C_OUTP(reg, portmap->con);
        }
    }
    else
        retval = -DW_EPERM;

    return retval;
}

/**********************************************************************/

enum dw_i2c_address_mode dw_i2c_getMasterAddressMode(struct dw_device
    *dev)
{
    uint32_t reg;
    enum dw_i2c_address_mode retval;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    reg = I2C_INP(portmap->con);
    retval = (enum dw_i2c_address_mode) DW_BIT_GET(reg,
            I2C_CON_10BITADDR_MASTER);

    return retval;
}

/**********************************************************************/

int dw_i2c_setSlaveAddressMode(struct dw_device *dev, enum
        dw_i2c_address_mode mode)
{
    int retval;
    uint32_t reg;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    if(dw_i2c_isEnabled(dev) == false) {
        retval = 0;
        reg = I2C_INP(portmap->con);
        // avoid bus write if possible
        if(DW_BIT_GET(reg, I2C_CON_10BITADDR_SLAVE) != mode) {
            DW_BIT_SET(reg, I2C_CON_10BITADDR_SLAVE, mode);
            I2C_OUTP(reg, portmap->con);
        }
    }
    else
        retval = -DW_EPERM;

    return retval;
}

/**********************************************************************/

enum dw_i2c_address_mode dw_i2c_getSlaveAddressMode(struct dw_device
    *dev)
{
    uint32_t reg;
    enum dw_i2c_address_mode retval;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    reg = I2C_INP(portmap->con);
    retval = (enum dw_i2c_address_mode) DW_BIT_GET(reg,
            I2C_CON_10BITADDR_SLAVE);

    return retval;
}

/**********************************************************************/

int dw_i2c_enableSlave(struct dw_device *dev)
{
    int retval;
    uint32_t reg;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    if(dw_i2c_isEnabled(dev) == false) {
        retval = 0;
        reg = I2C_INP(portmap->con);
        // avoid bus write if possible
        if(DW_BIT_GET(reg, I2C_CON_SLAVE_DISABLE) != 0x0) {
            DW_BIT_SET(reg, I2C_CON_SLAVE_DISABLE, 0x0);
            I2C_OUTP(reg, portmap->con);
        }
    }
    else
        retval = -DW_EPERM;

    return retval;
}

/**********************************************************************/

int dw_i2c_disableSlave(struct dw_device *dev)
{
    int retval;
    uint32_t reg;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    if(dw_i2c_isEnabled(dev) == false) {
        retval = 0;
        reg = I2C_INP(portmap->con);
        // avoid bus write if possible
        if(DW_BIT_GET(reg, I2C_CON_SLAVE_DISABLE) != 0x1) {
            DW_BIT_SET(reg, I2C_CON_SLAVE_DISABLE, 0x1);
            I2C_OUTP(reg, portmap->con);
        }
    }
    else
        retval = -DW_EPERM;

    return retval;
}

/**********************************************************************/

bool dw_i2c_isSlaveEnabled(struct dw_device *dev)
{
    bool retval;
    uint32_t reg;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    reg = I2C_INP(portmap->con);
    if(DW_BIT_GET(reg, I2C_CON_SLAVE_DISABLE) == 0x0)
        retval = true;
    else
        retval = false;

    return retval;
}

/**********************************************************************/

int dw_i2c_enableMaster(struct dw_device *dev)
{
    int retval;
    uint32_t reg;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    if(dw_i2c_isEnabled(dev) == false) {
        retval = 0;
        reg = I2C_INP(portmap->con);
        // avoid bus write if possible
        if(DW_BIT_GET(reg, I2C_CON_MASTER_MODE) != 0x1) {
            DW_BIT_SET(reg, I2C_CON_MASTER_MODE, 0x1);
            I2C_OUTP(reg, portmap->con);
        }
    }
    else
        retval = -DW_EPERM;

    return retval;
}

/**********************************************************************/

int dw_i2c_disableMaster(struct dw_device *dev)
{
    int retval;
    uint32_t reg;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    if(dw_i2c_isEnabled(dev) == false) {
        retval = 0;
        reg = I2C_INP(portmap->con);
        // avoid bus write if possible
        if(DW_BIT_GET(reg, I2C_CON_MASTER_MODE) != 0x0) {
            DW_BIT_SET(reg, I2C_CON_MASTER_MODE, 0x0);
            I2C_OUTP(reg, portmap->con);
        }
    }
    else
        retval = -DW_EPERM;

    return retval;
}

/**********************************************************************/

bool dw_i2c_isMasterEnabled(struct dw_device *dev)
{
    uint32_t reg;
    bool retval;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    reg = I2C_INP(portmap->con);
    if(DW_BIT_GET(reg, I2C_CON_MASTER_MODE) == 0x1)
        retval = true;
    else
        retval = false;

    return retval;
}

/**********************************************************************/

int dw_i2c_enableRestart(struct dw_device *dev)
{
    int retval;
    uint32_t reg;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    if(dw_i2c_isEnabled(dev) == false) {
        retval = 0;
        reg = I2C_INP(portmap->con);
        // avoid bus write if possible
        if(DW_BIT_GET(reg, I2C_CON_RESTART_EN) != 0x1) {
            DW_BIT_SET(reg, I2C_CON_RESTART_EN, 0x1);
            I2C_OUTP(reg, portmap->con);
        }
    }
    else
        retval = -DW_EPERM;

    return retval;
}

/**********************************************************************/

int dw_i2c_disableRestart(struct dw_device *dev)
{
    int retval;
    uint32_t reg;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    if(dw_i2c_isEnabled(dev) == false) {
        retval = 0;
        reg = I2C_INP(portmap->con);
        // avoid bus write if possible
        if(DW_BIT_GET(reg, I2C_CON_RESTART_EN) != 0x0) {
            DW_BIT_SET(reg, I2C_CON_RESTART_EN, 0x0);
            I2C_OUTP(reg, portmap->con);
        }
    }
    else
        retval = -DW_EPERM;

    return retval;
}

/**********************************************************************/

bool dw_i2c_isRestartEnabled(struct dw_device *dev)
{
    uint32_t reg;
    bool retval;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    reg = I2C_INP(portmap->con);
    if(DW_BIT_GET(reg, I2C_CON_RESTART_EN) == 0x1)
        retval = true;
    else
        retval = false;

    return retval;
}

/**********************************************************************/

int dw_i2c_setTargetAddress(struct dw_device *dev, uint16_t address)
{
    int retval;
    uint32_t reg;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);
    // address should be no more than 10 bits long
    DW_REQUIRE((address & 0xfc00) == 0);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    if(dw_i2c_isEnabled(dev) == false) {
        retval = 0;
        reg = I2C_INP(portmap->tar);
        // avoid bus write if possible
        if(DW_BIT_GET(reg, I2C_TAR_ADDR) != address) {
            DW_BIT_SET(reg, I2C_TAR_ADDR, address);
            I2C_OUTP(reg, portmap->tar);
        }
    }
    else
        retval = -DW_EPERM;

    return retval;
}

/**********************************************************************/

uint16_t dw_i2c_getTargetAddress(struct dw_device *dev)
{
    uint32_t reg;
    uint16_t retval;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    reg = I2C_INP(portmap->tar);
    retval = DW_BIT_GET(reg, I2C_TAR_ADDR);

    return retval;
}

/**********************************************************************/

int dw_i2c_setSlaveAddress(struct dw_device *dev, uint16_t address)
{
    int retval;
    uint32_t reg;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);
    // address should be no more than 10 bits long
    DW_REQUIRE((address & 0xfc00) == 0);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    if(dw_i2c_isEnabled(dev) == false) {
        reg = 0;
        retval = 0;
        DW_BIT_SET(reg, I2C_SAR_ADDR, address);
        I2C_OUTP(reg, portmap->sar);
    }
    else
        retval = -DW_EPERM;

    return retval;
}

/**********************************************************************/

uint16_t dw_i2c_getSlaveAddress(struct dw_device *dev)
{
    uint32_t reg;
    uint16_t retval;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    reg = I2C_INP(portmap->sar);
    retval = DW_BIT_GET(reg, I2C_SAR_ADDR);

    return retval;
}

/**********************************************************************/

int dw_i2c_setTxMode(struct dw_device *dev, enum dw_i2c_tx_mode mode)
{
    int retval;
    uint32_t reg;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    if(dw_i2c_isEnabled(dev) == false) {
        retval = 0;
        reg = I2C_INP(portmap->tar);
        // avoid bus write if possible
        if(DW_BIT_GET(reg, I2C_TAR_TX_MODE) != mode) {
            DW_BIT_SET(reg, I2C_TAR_TX_MODE, mode);
            I2C_OUTP(reg, portmap->tar);
        }
    }
    else
        retval = -DW_EPERM;

    return retval;
}

/**********************************************************************/

enum dw_i2c_tx_mode dw_i2c_getTxMode(struct dw_device *dev)
{
    uint32_t reg;
    enum dw_i2c_tx_mode retval;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    reg = I2C_INP(portmap->tar);
    retval = (enum dw_i2c_tx_mode) DW_BIT_GET(reg, I2C_TAR_TX_MODE);

    return retval;
}

/**********************************************************************/

int dw_i2c_setMasterCode(struct dw_device *dev, uint8_t code)
{
    int retval;
    uint32_t reg;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);
    // code should be no more than 3 bits wide
    DW_REQUIRE((code & 0xf8) == 0);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    if(dw_i2c_isEnabled(dev) == false) {
        reg = 0;
        retval = 0;
        DW_BIT_SET(reg, I2C_HS_MADDR_HS_MAR, code);
        I2C_OUTP(reg, portmap->hs_maddr);
    }
    else
        retval = -DW_EPERM;

    return retval;
}

/**********************************************************************/

uint8_t dw_i2c_getMasterCode(struct dw_device *dev)
{
    uint32_t reg;
    uint8_t retval;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    reg = I2C_INP(portmap->hs_maddr);
    retval = (uint8_t) DW_BIT_GET(reg, I2C_HS_MADDR_HS_MAR);

    return retval;
}

/**********************************************************************/

int dw_i2c_setSclCount(struct dw_device *dev, enum dw_i2c_speed_mode
        mode, enum dw_i2c_scl_phase phase, uint16_t count)
{
    int retval;
    uint32_t reg;
    struct dw_i2c_param *param;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;
    param = (struct dw_i2c_param *) dev->comp_param;

    if(param->hc_count_values == true)
        retval = -DW_ENOSYS;
    else if(dw_i2c_isEnabled(dev) == false) {
        reg = 0;
        retval = 0;
        DW_BIT_SET(reg, I2C_SCL_COUNT, count);
        if(mode == I2c_speed_high) {
            if(phase == I2c_scl_low)
                I2C_OUTP(reg, portmap->hs_scl_lcnt);
            else
                I2C_OUTP(reg, portmap->hs_scl_hcnt);
        }
        else if(mode == I2c_speed_fast) {
            if(phase == I2c_scl_low)
                I2C_OUTP(reg, portmap->fs_scl_lcnt);
            else
                I2C_OUTP(reg, portmap->fs_scl_hcnt);
        }
        else if(mode == I2c_speed_standard) {
            if(phase == I2c_scl_low)
                I2C_OUTP(reg, portmap->ss_scl_lcnt);
            else
                I2C_OUTP(reg, portmap->ss_scl_hcnt);
        }
    }
    else
        retval = -DW_EPERM;

    return retval;
}

/**********************************************************************/

uint16_t dw_i2c_getSclCount(struct dw_device *dev, enum
        dw_i2c_speed_mode mode, enum dw_i2c_scl_phase phase)
{
    uint32_t reg;
    uint16_t retval;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    if(mode == I2c_speed_high) {
        if(phase == I2c_scl_low)
            reg = I2C_INP(portmap->hs_scl_lcnt);
        else
            reg = I2C_INP(portmap->hs_scl_hcnt);
    }
    else if(mode == I2c_speed_fast) {
        if(phase == I2c_scl_low)
            reg = I2C_INP(portmap->fs_scl_lcnt);
        else
            reg = I2C_INP(portmap->fs_scl_hcnt);
    }
    else if(mode == I2c_speed_standard) {
        if(phase == I2c_scl_low)
            reg = I2C_INP(portmap->ss_scl_lcnt);
        else
            reg = I2C_INP(portmap->ss_scl_hcnt);
    }

    retval = (uint16_t) DW_BIT_GET(reg, I2C_SCL_COUNT);

    return retval;
}

/**********************************************************************/

uint8_t dw_i2c_read(struct dw_device *dev)
{
    uint8_t retval;
    uint32_t reg;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    // read a byte from the DATA_CMD register
    reg = DW_IN32P(portmap->data_cmd);
    retval = (reg & 0xff);

    return retval;
}

/**********************************************************************/

void dw_i2c_write(struct dw_device *dev, uint32_t character)
{
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    // write a byte to the DATA_CMD register
    DW_OUT32P(character, portmap->data_cmd);
}

/**********************************************************************/

void dw_i2c_issueRead(struct dw_device *dev)
{
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    DW_OUT32P(0x100, portmap->data_cmd);
}

/**********************************************************************/

enum dw_i2c_tx_abort dw_i2c_getTxAbortSource(struct dw_device *dev)
{
    uint32_t reg;
    enum dw_i2c_tx_abort retval;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    reg = I2C_INP(portmap->tx_abrt_source);
    retval = (enum dw_i2c_tx_abort) DW_BIT_GET(reg, I2C_TX_ABRT_SRC_ALL);

    return retval;
}

/**********************************************************************/

uint16_t dw_i2c_getTxFifoDepth(struct dw_device *dev)
{
    unsigned retval;
    struct dw_i2c_param *param;

    I2C_COMMON_REQUIREMENTS(dev);

    param = (struct dw_i2c_param *) dev->comp_param;

    retval = param->tx_buffer_depth;

    return (uint16_t) retval;
}

/**********************************************************************/

uint16_t dw_i2c_getRxFifoDepth(struct dw_device *dev)
{
    unsigned retval;
    struct dw_i2c_param *param;

    I2C_COMMON_REQUIREMENTS(dev);

    param = (struct dw_i2c_param *) dev->comp_param;

    retval = param->rx_buffer_depth;

    return (uint16_t) retval;
}

/**********************************************************************/

bool dw_i2c_isTxFifoFull(struct dw_device *dev)
{
    uint32_t reg;
    bool retval;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    reg = I2C_INP(portmap->status);
    if(DW_BIT_GET(reg, I2C_STATUS_TFNF) == 0)
        retval = true;
    else
        retval = false;

    return retval;
}

/**********************************************************************/

bool dw_i2c_isTxFifoEmpty(struct dw_device *dev)
{
    bool retval;
    uint32_t reg;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    reg = I2C_INP(portmap->status);
    if(DW_BIT_GET(reg, I2C_STATUS_TFE) == 1)
        retval = true;
    else
        retval = false;

    return retval;
}

/**********************************************************************/

bool dw_i2c_isRxFifoFull(struct dw_device *dev)
{
    uint32_t reg;
    bool retval;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    reg = I2C_INP(portmap->status);
    if(DW_BIT_GET(reg, I2C_STATUS_RFF) == 1)
        retval = true;
    else
        retval = false;

    return retval;
}

/**********************************************************************/

bool dw_i2c_isRxFifoEmpty(struct dw_device *dev)
{
    uint32_t reg;
    bool retval;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    reg = I2C_INP(portmap->status);
    if(DW_BIT_GET(reg, I2C_STATUS_RFNE) == 0)
        retval = true;
    else
        retval = false;

    return retval;
}

/**********************************************************************/

uint16_t dw_i2c_getTxFifoLevel(struct dw_device *dev)
{
    uint32_t reg;
    uint16_t retval;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    reg = I2C_INP(portmap->txflr);
    retval = (uint16_t) DW_BIT_GET(reg, I2C_TXFLR_TXFL);

    return retval;
}

/**********************************************************************/

uint16_t dw_i2c_getRxFifoLevel(struct dw_device *dev)
{
    uint32_t reg;
    uint16_t retval;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    reg = I2C_INP(portmap->rxflr);
    retval = (uint16_t) DW_BIT_GET(reg, I2C_RXFLR_RXFL);

    return retval;
}

/**********************************************************************/

int dw_i2c_setTxThreshold(struct dw_device *dev, uint8_t level)
{
    int retval;
    uint32_t reg;
    struct dw_i2c_param *param;
    struct dw_i2c_portmap *portmap;
    struct dw_i2c_instance *instance;

    I2C_COMMON_REQUIREMENTS(dev);

    param = (struct dw_i2c_param *) dev->comp_param;
    portmap = (struct dw_i2c_portmap *) dev->base_address;
    instance = (struct dw_i2c_instance *) dev->instance;

    // We need to be careful here not to overwrite the tx threshold if
    // the interrupt handler has altered it to trigger on the last byte
    // of the current transfer (in order to call the user callback
    // function at the appropriate time).  When the driver returns to
    // the idle state, it will update the tx threshold with the
    // user-specified value.
    if(level > param->tx_buffer_depth)
        retval = -DW_EINVAL;
    else {
        // store user tx threshold value
        instance->txThreshold = level;

        if(instance->state == I2c_state_idle) {
            reg = 0;
            retval = 0;
            DW_BIT_SET(reg, I2C_TX_TL_TX_TL, level);
            I2C_OUTP(reg, portmap->tx_tl);
        }
        else
            retval = -DW_EBUSY;
    }

    return retval;
}

/**********************************************************************/

uint8_t dw_i2c_getTxThreshold(struct dw_device *dev)
{
    uint32_t reg;
    uint8_t retval;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    reg = I2C_INP(portmap->tx_tl);
    retval = DW_BIT_GET(reg, I2C_TX_TL_TX_TL);

    return retval;
}

/**********************************************************************/

int dw_i2c_setRxThreshold(struct dw_device *dev, uint8_t level)
{
    int retval;
    uint32_t reg;
    struct dw_i2c_param *param;
    struct dw_i2c_portmap *portmap;
    struct dw_i2c_instance *instance;

    I2C_COMMON_REQUIREMENTS(dev);

    param = (struct dw_i2c_param *) dev->comp_param;
    portmap = (struct dw_i2c_portmap *) dev->base_address;
    instance = (struct dw_i2c_instance *) dev->instance;

    // We need to be careful here not to overwrite the rx threshold if
    // the interrupt handler has altered it to trigger on the last byte
    // of the current transfer (in order to call the user callback
    // function at the appropriate time).  When the driver returns to
    // the idle state, it will update the rx threshold with the
    // user-specified value.
    if(level > param->rx_buffer_depth)
        retval = -DW_EINVAL;
    else {
        // store user rx threshold value
        instance->rxThreshold = level;

        if(instance->state == I2c_state_idle) {
            reg = 0;
            retval = 0;
            DW_BIT_SET(reg, I2C_RX_TL_RX_TL, level);
            I2C_OUTP(reg, portmap->rx_tl);
        }
        else
            retval = -DW_EBUSY;
    }

    return retval;
}

/**********************************************************************/

uint8_t dw_i2c_getRxThreshold(struct dw_device *dev)
{
    uint32_t reg;
    uint8_t retval;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    reg = I2C_INP(portmap->rx_tl);
    retval = DW_BIT_GET(reg, I2C_RX_TL_RX_TL);

    return retval;
}

/**********************************************************************/

void dw_i2c_setListener(struct dw_device *dev, dw_callback userFunction)
{
    struct dw_i2c_portmap *portmap;
    struct dw_i2c_instance *instance;

    I2C_COMMON_REQUIREMENTS(dev);
    DW_REQUIRE(userFunction != NULL);

    portmap = (struct dw_i2c_portmap *) dev->base_address;
    instance = (struct dw_i2c_instance *) dev->instance;

    // Critical section of code.  Shared data needs to be protected.
    // This macro disables all DW_apb_i2c interrupts.
    I2C_ENTER_CRITICAL_SECTION();

    instance->listener = userFunction;

    instance->intr_mask_save = (enum dw_i2c_irq) (I2c_irq_rx_under |
            I2c_irq_rx_over | I2c_irq_tx_over | I2c_irq_rd_req |
            I2c_irq_tx_abrt | I2c_irq_rx_done | I2c_irq_gen_call);
    // don't enable Rx FIFO full if DMA hardware handshaking is in use
    if(instance->dmaRx.mode != Dw_dma_hw_handshake)
        instance->intr_mask_save |= I2c_irq_rx_full;

    // End of critical section of code. This macros restores DW_apb_i2c
    // interrupts.
    I2C_EXIT_CRITICAL_SECTION();
}

/**********************************************************************/

// master transmit function
int dw_i2c_masterBack2Back(struct dw_device *dev, uint16_t *txBuffer,
        unsigned txLength, uint8_t *rxBuffer, unsigned rxLength,
        dw_callback userFunction)
{
    int retval;
    uint8_t *tmp;
    uint32_t reg;
    struct dw_i2c_portmap *portmap;
    struct dw_i2c_instance *instance;

    I2C_COMMON_REQUIREMENTS(dev);
    DW_REQUIRE(txBuffer != NULL);
    DW_REQUIRE(txLength != 0);
    DW_REQUIRE(rxBuffer != NULL);
    DW_REQUIRE(rxLength != 0);
    DW_REQUIRE(rxLength <= txLength);

    instance = (struct dw_i2c_instance *) dev->instance;
    portmap = (struct dw_i2c_portmap *) dev->base_address;

    DW_REQUIRE(instance->listener != NULL);

    // Critical section of code.  Shared data needs to be protected.
    // This macro disables DW_apb_i2c interrupts.
    I2C_ENTER_CRITICAL_SECTION();

    if(instance->state == I2c_state_idle) {
        retval = 0;
        // master back2back transfer mode
        instance->state = I2c_state_back2back;
        instance->rxBuffer = rxBuffer;
        instance->rxLength = rxLength;
        instance->rxRemain = rxLength;
        instance->rxCallback = userFunction;
        instance->rxHold = 0;
        instance->rxIdx = 4;
        instance->b2bBuffer = txBuffer;
        instance->txLength = txLength;
        instance->txRemain = txLength;
        instance->txCallback = userFunction;
        instance->txHold = 0;
        instance->txIdx = 0;

        // check if rx buffer is word-aligned
        if(((uint64_t) rxBuffer & 0x3ul) == 0)
            instance->rxAlign = true;
        else
            instance->rxAlign = false;

        // support non-word aligned 16-bit buffers
        tmp = (uint8_t *) txBuffer;
        while(((((uint64_t) tmp) & 0x3ul) != 0x0) && ((instance->txLength
                        - instance->txIdx) > 0)) {
            instance->txHold |= ((*tmp++ & 0xffu) << (instance->txIdx *
                        8));
            instance->txIdx++;
        }
        instance->txBuffer = tmp;

        // set rx fifo threshold if necessary
        if(rxLength <= instance->rxThreshold) {
            reg = 0;
            DW_BIT_SET(reg, I2C_RX_TL_RX_TL, (rxLength - 1));
            I2C_OUTP(reg, portmap->rx_tl);
        }

        // ensure transfer is underway
        instance->intr_mask_save |= (I2c_irq_tx_empty | I2c_irq_tx_abrt
                | I2c_irq_rx_full);
    }
    else
        retval = -DW_EBUSY;

    // End of critical section of code. This macros restores DW_apb_i2c
    // interrupts.
    I2C_EXIT_CRITICAL_SECTION();

    return retval;
}

/**********************************************************************/

// master transmit function
int dw_i2c_masterTransmit(struct dw_device *dev, uint8_t *buffer,
        unsigned length, dw_callback userFunction)
{
    int retval;
    uint8_t *tmp;
    struct dw_i2c_portmap *portmap;
    struct dw_i2c_instance *instance;

    I2C_COMMON_REQUIREMENTS(dev);
    DW_REQUIRE(buffer != NULL);

    DW_REQUIRE(length != 0);

    instance = (struct dw_i2c_instance *) dev->instance;
    portmap = (struct dw_i2c_portmap *) dev->base_address;

    DW_REQUIRE(instance->listener != NULL);

    // Critical section of code.  Shared data needs to be protected.
    // This macro disables DW_apb_i2c interrupts.
    I2C_ENTER_CRITICAL_SECTION();

    if(instance->state == I2c_state_idle) {
        // master-transmitter
        retval = 0;

        instance->state = I2c_state_master_tx;
        instance->txCallback = userFunction;
        instance->txLength = length;
        instance->txRemain = length;
        instance->txHold = 0;
        instance->txIdx = 0;
        // check for non word-aligned buffer as I2C_FIFO_WRITE() works
        // efficiently on words reads from instance->txHold.
        tmp = buffer;
        while(((((uint64_t) tmp) & 0x3ul) != 0x0) && ((instance->txLength
                        - instance->txIdx) > 0)) {
            instance->txHold |= ((*tmp++ & 0xffu) << (instance->txIdx *
                        8));
            instance->txIdx++;
        }
        instance->txBuffer = tmp;

        // ensure transfer is underway
        instance->intr_mask_save |= (I2c_irq_tx_empty |
                I2c_irq_tx_abrt);
    }
    else
        retval = -DW_EBUSY;

    // End of critical section of code. This macros restores DW_apb_i2c
    // interrupts.
    I2C_EXIT_CRITICAL_SECTION();

    return retval;
}

/**********************************************************************/

// slave transmit function
int dw_i2c_slaveTransmit(struct dw_device *dev, uint8_t *buffer,
        unsigned length, dw_callback userFunction)
{
    uint8_t *tmp;
    int retval;
    struct dw_i2c_portmap *portmap;
    struct dw_i2c_instance *instance;

    I2C_COMMON_REQUIREMENTS(dev);
    DW_REQUIRE(buffer != NULL);
    DW_REQUIRE(length != 0);

    portmap = (struct dw_i2c_portmap *) dev->base_address;
    instance = (struct dw_i2c_instance *) dev->instance;

    DW_REQUIRE(instance->listener != NULL);

    // Critical section of code.  Shared data needs to be protected.
    // This macro disables DW_apb_i2c interrupts.
    I2C_ENTER_CRITICAL_SECTION();

    if((instance->state == I2c_state_rd_req) || (instance->state ==
                I2c_state_slave_rx_rd_req)) {
        // slave-transmitter
        retval = 0;

        instance->txCallback = userFunction;
        instance->txLength = length;
        instance->txRemain = length;
        instance->txHold = 0;
        instance->txIdx = 0;
        // check for non word-aligned buffer as I2C_FIFO_WRITE() works
        // efficiently on words reads from instance->txHold.
        tmp = (uint8_t *) buffer;
        while(((((uint64_t) tmp) & 0x3ul) != 0x0) && ((instance->txLength
                        - instance->txIdx) > 0)) {
            instance->txHold |= ((*tmp++ & 0xffu) << (instance->txIdx *
                        8));
            instance->txIdx++;
        }
        // buffer is now word-aligned
        instance->txBuffer = tmp;
        // write only one byte of data to the slave tx fifo
        I2C_FIFO_WRITE(1);
        switch(instance->state) {
            case I2c_state_rd_req:
                instance->state = I2c_state_slave_tx;
                break;
            case I2c_state_slave_rx_rd_req:
                instance->state = I2c_state_slave_tx_rx;
                break;
            default:
                DW_ASSERT(false);
                break;
        }
        // Note: tx_empty is not enabled here as rd_req is the signal
        // used to write the next byte of data to the tx fifo.
    }
    else
        retval = -DW_EPROTO;

    // End of critical section of code. This macros restores DW_apb_i2c
    // interrupts.
    I2C_EXIT_CRITICAL_SECTION();

    return retval;
}
/**********************************************************************/

// slave bulk transmit function
int dw_i2c_slaveBulkTransmit(struct dw_device *dev, uint8_t *buffer,
        unsigned length, dw_callback userFunction)
{
    uint8_t *tmp;
    int retval, maxBytes;
    struct dw_i2c_param *param;
    struct dw_i2c_portmap *portmap;
    struct dw_i2c_instance *instance;

    I2C_COMMON_REQUIREMENTS(dev);
    DW_REQUIRE(buffer != NULL);
    DW_REQUIRE(length != 0);

    param = (struct dw_i2c_param *) dev->comp_param;
    portmap = (struct dw_i2c_portmap *) dev->base_address;
    instance = (struct dw_i2c_instance *) dev->instance;

    DW_REQUIRE(instance->listener != NULL);

    // Critical section of code.  Shared data needs to be protected.
    // This macro disables DW_apb_i2c interrupts.
    I2C_ENTER_CRITICAL_SECTION();

    if((instance->state == I2c_state_rd_req) || (instance->state ==
                I2c_state_slave_rx_rd_req)) {
        // slave-transmitter
        retval = 0;

        instance->txCallback = userFunction;
        instance->txLength = length;
        instance->txRemain = length;
        instance->txHold = 0;
        instance->txIdx = 0;
        // check for non word-aligned buffer as I2C_FIFO_WRITE() works
        // efficiently on words reads from instance->txHold.
        tmp = (uint8_t *) buffer;
        while(((((uint64_t) tmp) & 0x3ul) != 0x0) && ((instance->txLength
                        - instance->txIdx) > 0)) {
            instance->txHold |= ((*tmp++ & 0xffu) << (instance->txIdx *
                        8));
            instance->txIdx++;
        }
        // buffer is now word-aligned
        instance->txBuffer = tmp;
        // maximum available space in the tx fifo
        maxBytes = param->tx_buffer_depth - dw_i2c_getTxFifoLevel(dev);
        I2C_FIFO_WRITE(maxBytes);
        switch(instance->state) {
            case I2c_state_rd_req:
                instance->state = I2c_state_slave_bulk_tx;
                break;
            case I2c_state_slave_rx_rd_req:
                instance->state = I2c_state_slave_bulk_tx_rx;
                break;
            default:
                DW_ASSERT(false);
                break;
        }
        // ensure transfer is underway
        instance->intr_mask_save |= (I2c_irq_tx_empty |
                I2c_irq_tx_abrt);
    }
    else
        retval = -DW_EPROTO;

    // End of critical section of code. This macros restores DW_apb_i2c
    // interrupts.
    I2C_EXIT_CRITICAL_SECTION();

    return retval;
}

/**********************************************************************/

int dw_i2c_masterReceive(struct dw_device *dev, uint8_t *buffer,
        unsigned length, dw_callback userFunction)
{
    int retval;
    uint32_t reg;
    struct dw_i2c_portmap *portmap;
    struct dw_i2c_instance *instance;

    I2C_COMMON_REQUIREMENTS(dev);
    DW_REQUIRE(buffer != NULL);
    DW_REQUIRE(length != 0);

    instance = (struct dw_i2c_instance *) dev->instance;
    portmap = (struct dw_i2c_portmap *) dev->base_address;

    DW_REQUIRE(instance->listener != NULL);

    // Critical section of code.  Shared data needs to be protected.
    // This macro disables DW_apb_i2c interrupts.
    I2C_ENTER_CRITICAL_SECTION();

    if(instance->state == I2c_state_idle) {
        retval = 0;
        instance->state = I2c_state_master_rx;
        // check if rx buffer is not word-aligned
        if(((uint64_t) buffer & 0x3ul) == 0)
            instance->rxAlign = true;
        else
            instance->rxAlign = false;

        // set rx fifo threshold if necessary
        if(length <= instance->rxThreshold) {
            reg = 0;
            DW_BIT_SET(reg, I2C_RX_TL_RX_TL, (length - 1));
            I2C_OUTP(reg, portmap->rx_tl);
        }

        // set transfer variables
        instance->rxBuffer = buffer;
        instance->rxLength = length;
        instance->rxRemain = length;
        instance->txRemain = length;
        instance->rxCallback = userFunction;
        instance->rxIdx = 4;
        // restore interrupts and ensure master-receive is underway
        instance->intr_mask_save |= (I2c_irq_rx_full | I2c_irq_tx_empty
                | I2c_irq_tx_abrt);
    }
    else
        retval = -DW_EBUSY;

    // End of critical section of code. This macros restores DW_apb_i2c
    // interrupts.
    I2C_EXIT_CRITICAL_SECTION();

    return retval;
}

/**********************************************************************/

int dw_i2c_slaveReceive(struct dw_device *dev, uint8_t *buffer, unsigned
        length, dw_callback userFunction)
{
    int retval;
    uint32_t reg;
    struct dw_i2c_portmap *portmap;
    struct dw_i2c_instance *instance;

    I2C_COMMON_REQUIREMENTS(dev);
    DW_REQUIRE(buffer != NULL);
    DW_REQUIRE(length != 0);

    instance = (struct dw_i2c_instance *) dev->instance;
    portmap = (struct dw_i2c_portmap *) dev->base_address;

    DW_REQUIRE(instance->listener != NULL);

    // Critical section of code.  Shared data needs to be protected.
    // This macro disables DW_apb_i2c interrupts.
    I2C_ENTER_CRITICAL_SECTION();

    if((instance->state == I2c_state_rx_req)
            || (instance->state == I2c_state_idle)
            || (instance->state == I2c_state_slave_tx)
            || (instance->state == I2c_state_slave_bulk_tx)
            || (instance->state == I2c_state_slave_tx_rx_req)
            || (instance->state == I2c_state_slave_bulk_tx_rx_req)
            || (instance->state == I2c_state_master_tx_gen_call)) {
        retval = 0;
        // in case the state was idle
        switch(instance->state) {
            case I2c_state_idle:
            case I2c_state_rx_req:
                instance->state = I2c_state_slave_rx;
                break;
            case I2c_state_slave_tx:
            case I2c_state_slave_tx_rx_req:
                instance->state = I2c_state_slave_tx_rx;
                break;
            case I2c_state_slave_bulk_tx:
            case I2c_state_slave_bulk_tx_rx_req:
                instance->state = I2c_state_slave_bulk_tx_rx;
                break;
            case I2c_state_master_tx_gen_call:
                instance->state = I2c_state_master_tx_slave_rx;
                break;
            default:
                DW_ASSERT(false);
                break;
        }
        if(((uint64_t) buffer & 0x3ul) == 0)
            instance->rxAlign = true;
        else
            instance->rxAlign = false;

        // set rx fifo threshold if necessary
        if(length <= instance->rxThreshold) {
            reg = 0;
            DW_BIT_SET(reg, I2C_RX_TL_RX_TL, (length - 1));
            I2C_OUTP(reg, portmap->rx_tl);
        }

        // set transfer variables
        instance->rxBuffer = buffer;
        instance->rxLength = length;
        instance->rxRemain = length;
        instance->rxCallback = userFunction;
        instance->rxIdx = 4;

        // ensure receive is underway
        instance->intr_mask_save |= I2c_irq_rx_full;
    }
    else
        retval = -DW_EBUSY;

    // End of critical section of code. This macros restores DW_apb_i2c
    // interrupts.
    I2C_EXIT_CRITICAL_SECTION();

    return retval;
}

/**********************************************************************/

int dw_i2c_terminate(struct dw_device *dev)
{
    uint32_t reg;
    int retval, maxBytes;
    struct dw_i2c_portmap *portmap;
    struct dw_i2c_instance *instance;

    I2C_COMMON_REQUIREMENTS(dev);

    instance = (struct dw_i2c_instance *) dev->instance;
    portmap = (struct dw_i2c_portmap *) dev->base_address;

    DW_REQUIRE(instance->listener != NULL);

    // Critical section of code.  Shared data needs to be protected.
    // This macro disables DW_apb_i2c interrupts.
    I2C_ENTER_CRITICAL_SECTION();

    // disable tx interrupt
    if((instance->state == I2c_state_master_tx)
            || (instance->state == I2c_state_back2back)
            || (instance->state == I2c_state_slave_tx)
            || (instance->state == I2c_state_slave_tx_rx)
            || (instance->state == I2c_state_slave_bulk_tx)
            || (instance->state == I2c_state_slave_bulk_tx_rx)
            || (instance->state == I2c_state_master_tx_slave_rx)) {
        // ensure tx empty is not re-enabled when interrupts are
        // restored
        instance->intr_mask_save &= ~((unsigned)I2c_irq_tx_empty);
    }

    // flush rx fifo if necessary
    if((instance->state == I2c_state_master_rx)
            || (instance->state == I2c_state_slave_rx)
            || (instance->state == I2c_state_slave_tx_rx)
            || (instance->state == I2c_state_slave_bulk_tx_rx)
            || (instance->state == I2c_state_master_tx_slave_rx)
            || (instance->state == I2c_state_back2back)) {
        maxBytes = dw_i2c_getRxFifoLevel(dev);
        I2C_FIFO_READ(maxBytes);
        dw_i2c_flushRxHold(dev);
        // number of bytes that were received during the last transfer
        retval = (int)(instance->rxLength - instance->rxRemain);
    }
    else {
        // number of bytes that were sent during the last transfer
        retval = (int)(instance->txLength - instance->txRemain);
    }
    // sanity check .. retval should never be less than zero
    DW_ASSERT(retval >= 0);

    // terminate current transfer
    instance->state = I2c_state_idle;
    instance->txCallback = NULL;
    instance->txBuffer = NULL;
    instance->rxCallback = NULL;
    instance->rxBuffer = NULL;

    // restore user-specified tx/rx fifo threshold
    reg = 0;
    DW_BIT_SET(reg, I2C_TX_TL_TX_TL, instance->txThreshold);
    I2C_OUTP(reg, portmap->tx_tl);
    reg = 0;
    DW_BIT_SET(reg, I2C_RX_TL_RX_TL, instance->rxThreshold);
    I2C_OUTP(reg, portmap->rx_tl);
    
    // End of critical section of code. This macros restores DW_apb_i2c
    // interrupts.
    I2C_EXIT_CRITICAL_SECTION();

    // sanity check
    DW_ASSERT(dw_i2c_isIrqMasked(dev, I2c_irq_tx_empty) == true);

    return retval;
}

/**********************************************************************/

void dw_i2c_unmaskIrq(struct dw_device *dev, enum dw_i2c_irq interrupts)
{
    uint32_t reg;
    struct dw_i2c_portmap *portmap;
    struct dw_i2c_instance *instance;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;
    instance = (struct dw_i2c_instance *) dev->instance;

    reg = I2C_INP(portmap->intr_mask);
    // avoid bus write if irq already enabled
    if((interrupts & reg) != interrupts) {
        reg |= interrupts;
        // save current value of interrupt mask register
        instance->intr_mask_save = reg;
        I2C_OUTP(reg, portmap->intr_mask);
    }
}

/**********************************************************************/

void dw_i2c_maskIrq(struct dw_device *dev, enum dw_i2c_irq
        interrupts)
{
    uint32_t reg;
    struct dw_i2c_portmap *portmap;
    struct dw_i2c_instance *instance;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;
    instance = (struct dw_i2c_instance *) dev->instance;

    reg = I2C_INP(portmap->intr_mask);
    // avoid bus write if interrupt(s) already disabled
    if((interrupts & reg) != 0) {
        reg &= ~interrupts;
        // save current value of interrupt mask register
        instance->intr_mask_save = reg;
        I2C_OUTP(reg, portmap->intr_mask);
    }
}

/**********************************************************************/

void dw_i2c_clearIrq(struct dw_device *dev, enum dw_i2c_irq interrupts)
{
    volatile uint32_t reg;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    if(interrupts == I2c_irq_all)
        reg = I2C_INP(portmap->clr_intr);
    else {
        if((interrupts & I2c_irq_rx_under) != 0)
            reg = I2C_INP(portmap->clr_rx_under);
        if((interrupts & I2c_irq_rx_over) != 0)
            reg = I2C_INP(portmap->clr_rx_over);
        if((interrupts & I2c_irq_tx_over) != 0)
            reg = I2C_INP(portmap->clr_tx_over);
        if((interrupts & I2c_irq_rd_req) != 0)
            reg = I2C_INP(portmap->clr_rd_req);
        if((interrupts & I2c_irq_tx_abrt) != 0)
            reg = I2C_INP(portmap->clr_tx_abrt);
        if((interrupts & I2c_irq_rx_done) != 0)
            reg = I2C_INP(portmap->clr_rx_done);
        if((interrupts & I2c_irq_activity) != 0)
            reg = I2C_INP(portmap->clr_activity);
        if((interrupts & I2c_irq_stop_det) != 0)
            reg = I2C_INP(portmap->clr_stop_det);
        if((interrupts & I2c_irq_start_det) != 0)
            reg = I2C_INP(portmap->clr_start_det);
        if((interrupts & I2c_irq_gen_call) != 0)
            reg = I2C_INP(portmap->clr_gen_call);
    }

    if(reg == 0) {}
}

/**********************************************************************/

bool dw_i2c_isIrqMasked(struct dw_device *dev, enum dw_i2c_irq
        interrupt)
{
    bool retval;
    uint32_t reg;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);
    DW_REQUIRE((interrupt == I2c_irq_rx_under)
            || (interrupt == I2c_irq_rx_over)
            || (interrupt == I2c_irq_rx_full)
            || (interrupt == I2c_irq_tx_over)
            || (interrupt == I2c_irq_tx_empty)
            || (interrupt == I2c_irq_rd_req)
            || (interrupt == I2c_irq_tx_abrt)
            || (interrupt == I2c_irq_rx_done)
            || (interrupt == I2c_irq_activity)
            || (interrupt == I2c_irq_stop_det)
            || (interrupt == I2c_irq_start_det)
            || (interrupt == I2c_irq_gen_call));

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    reg = I2C_INP(portmap->intr_mask);
    reg &= interrupt;

    if(reg == 0)
        retval = true;
    else
        retval = false;

    return retval;
}

/**********************************************************************/

uint32_t dw_i2c_getIrqMask(struct dw_device *dev)
{
    uint32_t retval;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    retval = I2C_INP(portmap->intr_mask);

    return retval;
}

/**********************************************************************/

bool dw_i2c_isIrqActive(struct dw_device *dev, enum dw_i2c_irq
        interrupt)
{
    bool retval;
    uint32_t reg;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);
    DW_REQUIRE((interrupt == I2c_irq_rx_under)
            || (interrupt == I2c_irq_rx_over)
            || (interrupt == I2c_irq_rx_full)
            || (interrupt == I2c_irq_tx_over)
            || (interrupt == I2c_irq_tx_empty)
            || (interrupt == I2c_irq_rd_req)
            || (interrupt == I2c_irq_tx_abrt)
            || (interrupt == I2c_irq_rx_done)
            || (interrupt == I2c_irq_activity)
            || (interrupt == I2c_irq_stop_det)
            || (interrupt == I2c_irq_start_det)
            || (interrupt == I2c_irq_gen_call));

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    reg = I2C_INP(portmap->intr_stat);
    reg &= interrupt;

    if(reg == 0)
        retval = false;
    else
        retval = true;

    return retval;
}

/**********************************************************************/

bool dw_i2c_isRawIrqActive(struct dw_device *dev, enum dw_i2c_irq
        interrupt)
{
    bool retval;
    uint32_t reg;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);
    DW_REQUIRE((interrupt == I2c_irq_rx_under)
            || (interrupt == I2c_irq_rx_over)
            || (interrupt == I2c_irq_rx_full)
            || (interrupt == I2c_irq_tx_over)
            || (interrupt == I2c_irq_tx_empty)
            || (interrupt == I2c_irq_rd_req)
            || (interrupt == I2c_irq_tx_abrt)
            || (interrupt == I2c_irq_rx_done)
            || (interrupt == I2c_irq_activity)
            || (interrupt == I2c_irq_stop_det)
            || (interrupt == I2c_irq_start_det)
            || (interrupt == I2c_irq_gen_call));

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    reg = I2C_INP(portmap->raw_intr_stat);
    reg &= interrupt;

    if(reg == 0)
        retval = false;
    else
        retval = true;

    return retval;
}

/**********************************************************************/

int dw_i2c_setDmaTxMode(struct dw_device *dev, enum dw_dma_mode mode)
{
    int retval;
    uint32_t reg;
    struct dw_i2c_param *param;
    struct dw_i2c_portmap *portmap;
    struct dw_i2c_instance *instance;

    I2C_COMMON_REQUIREMENTS(dev);

    param = (struct dw_i2c_param *) dev->comp_param;
    portmap = (struct dw_i2c_portmap *) dev->base_address;
    instance = (struct dw_i2c_instance *) dev->instance;

    if(param->has_dma == true) {
        retval = 0;
        if(instance->dmaTx.mode != mode) {
            reg = I2C_INP(portmap->dma_cr);
            instance->dmaTx.mode = mode;
            if(mode == Dw_dma_hw_handshake)
                DW_BIT_SET(reg, I2C_DMA_CR_TDMAE, 0x1);
            else
                DW_BIT_SET(reg, I2C_DMA_CR_TDMAE, 0x0);
            I2C_OUTP(reg, portmap->dma_cr);
        }
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

enum dw_dma_mode dw_i2c_getDmaTxMode(struct dw_device *dev)
{
    enum dw_dma_mode retval;
    struct dw_i2c_instance *instance;

    I2C_COMMON_REQUIREMENTS(dev);

    instance = (struct dw_i2c_instance *) dev->instance;

    retval = instance->dmaTx.mode;

    return retval;
}

/**********************************************************************/

int dw_i2c_setDmaRxMode(struct dw_device *dev, enum dw_dma_mode mode)
{
    int retval;
    uint32_t reg;
    struct dw_i2c_param *param;
    struct dw_i2c_portmap *portmap;
    struct dw_i2c_instance *instance;

    I2C_COMMON_REQUIREMENTS(dev);

    param = (struct dw_i2c_param *) dev->comp_param;
    portmap = (struct dw_i2c_portmap *) dev->base_address;
    instance = (struct dw_i2c_instance *) dev->instance;

    if(param->has_dma == true) {
        retval = 0;
        if(instance->dmaRx.mode != mode) {
            reg = I2C_INP(portmap->dma_cr);
            instance->dmaRx.mode = mode;
            if(mode == Dw_dma_hw_handshake) {
                DW_BIT_SET(reg, I2C_DMA_CR_RDMAE, 0x1);
                // mask rx full interrupt
                dw_i2c_maskIrq(dev, I2c_irq_rx_full);
            }
            else {
                DW_BIT_SET(reg, I2C_DMA_CR_RDMAE, 0x0);
                // unmask rx full interrupt
                dw_i2c_unmaskIrq(dev, I2c_irq_rx_full);
            }
            I2C_OUTP(reg, portmap->dma_cr);
        }
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

enum dw_dma_mode dw_i2c_getDmaRxMode(struct dw_device *dev)
{
    enum dw_dma_mode retval;
    struct dw_i2c_instance *instance;

    I2C_COMMON_REQUIREMENTS(dev);

    instance = (struct dw_i2c_instance *) dev->instance;

    retval = instance->dmaRx.mode;

    return retval;
}

/**********************************************************************/

int dw_i2c_setDmaTxLevel(struct dw_device *dev, uint8_t level)
{
    int retval;
    uint32_t reg;
    struct dw_i2c_param *param;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    param = (struct dw_i2c_param *) dev->comp_param;
    portmap = (struct dw_i2c_portmap *) dev->base_address;

    if(param->has_dma == true) {
        reg = 0;
        retval = 0;
        DW_BIT_SET(reg, I2C_DMA_TDLR_DMATDL, level);
        I2C_OUTP(reg, portmap->dma_tdlr);
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

uint8_t dw_i2c_getDmaTxLevel(struct dw_device *dev)
{
    uint32_t reg;
    uint32_t retval;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    reg = I2C_INP(portmap->dma_tdlr);
    retval = DW_BIT_GET(reg, I2C_DMA_TDLR_DMATDL);

    return (uint8_t) retval;
}

/**********************************************************************/

int dw_i2c_setDmaRxLevel(struct dw_device *dev, uint8_t level)
{
    int retval;
    uint32_t reg;
    struct dw_i2c_param *param;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    param = (struct dw_i2c_param *) dev->comp_param;
    portmap = (struct dw_i2c_portmap *) dev->base_address;

    if(param->has_dma == true) {
        retval = 0;
        reg = 0;
        DW_BIT_SET(reg, I2C_DMA_RDLR_DMARDL, level);
        I2C_OUTP(reg, portmap->dma_rdlr);
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

uint8_t dw_i2c_getDmaRxLevel(struct dw_device *dev)
{
    uint32_t reg;
    uint32_t retval;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;

    reg = I2C_INP(portmap->dma_rdlr);
    retval = DW_BIT_GET(reg, I2C_DMA_RDLR_DMARDL);

    return (uint8_t) retval;
}

/**********************************************************************/

int dw_i2c_setNotifier_destinationReady(struct dw_device *dev,
        dw_dma_notifier_func funcptr, struct dw_device *dmac, unsigned
        channel)
{
    int retval;
    struct dw_i2c_param *param;
    struct dw_i2c_instance *instance;

    I2C_COMMON_REQUIREMENTS(dev);
    DW_REQUIRE(funcptr != NULL);
    DW_REQUIRE(dmac != NULL);
    DW_REQUIRE(dmac->comp_type == Dw_ahb_dmac);

    param = (struct dw_i2c_param *) dev->comp_param;
    instance = (struct dw_i2c_instance *) dev->instance;

    if(param->has_dma == true) {
        retval = 0;
        instance->dmaTx.notifier = funcptr;
        instance->dmaTx.dmac = dmac;
        instance->dmaTx.channel = channel;
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

int dw_i2c_setNotifier_sourceReady(struct dw_device *dev,
        dw_dma_notifier_func funcptr, struct dw_device *dmac, unsigned
        channel)
{
    int retval;
    struct dw_i2c_param *param;
    struct dw_i2c_instance *instance;

    I2C_COMMON_REQUIREMENTS(dev);
    DW_REQUIRE(funcptr != NULL);
    DW_REQUIRE(dmac != NULL);
    DW_REQUIRE(dmac->comp_type == Dw_ahb_dmac);

    param = (struct dw_i2c_param *) dev->comp_param;
    instance = (struct dw_i2c_instance *) dev->instance;

    if(param->has_dma == true) {
        retval = 0;
        instance->dmaRx.notifier = funcptr;
        instance->dmaRx.dmac = dmac;
        instance->dmaRx.channel = channel;
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

int dw_i2c_irqHandler(struct dw_device *dev)
{
    int retval;
    uint8_t *tmp;
    uint32_t reg;
    int i, maxBytes;
    int32_t callbackArg;
    dw_callback userCallback;
    enum dw_i2c_irq clearIrqMask;
    struct dw_i2c_param *param;
    struct dw_i2c_portmap *portmap;
    struct dw_i2c_instance *instance;

    I2C_COMMON_REQUIREMENTS(dev);

    param = (struct dw_i2c_param *) dev->comp_param;
    portmap = (struct dw_i2c_portmap *) dev->base_address;
    instance = (struct dw_i2c_instance *) dev->instance;

    userCallback = NULL;
    callbackArg = 0;
    clearIrqMask = I2c_irq_none;
    // Assume an interrupt will be processed.  This will be set to false
    // if no active interrupt is found.
    retval = true;

    // what caused the interrupt?
    reg = I2C_INP(portmap->intr_stat);

    // if an error has occurred
    if((reg & (I2c_irq_tx_abrt | I2c_irq_rx_over | I2c_irq_rx_under |
                    I2c_irq_tx_over)) != 0) {
        instance->state = I2c_state_error;
        userCallback = instance->listener;
        dw_i2c_maskIrq(dev, I2c_irq_tx_empty);
        // if a tx transfer was aborted
        if((reg & I2c_irq_tx_abrt) != 0) {
            callbackArg = I2c_irq_tx_abrt;
            clearIrqMask = I2c_irq_tx_abrt;
        }
        // rx fifo overflow
        else if((reg & I2c_irq_rx_over) != 0) {
            callbackArg = I2c_irq_rx_over;
            clearIrqMask = I2c_irq_rx_over;
        }
        // rx fifo underflow
        else if((reg & I2c_irq_rx_under) != 0) {
            callbackArg = I2c_irq_rx_under;
            clearIrqMask = I2c_irq_rx_under;
        }
        // tx fifo overflow
        else if((reg & I2c_irq_tx_over) != 0) {
            callbackArg = I2c_irq_tx_over;
            clearIrqMask = I2c_irq_tx_over;
        }
    }
    // a general call was detected
    else if((reg & I2c_irq_gen_call) != 0) {
        userCallback = instance->listener;
        callbackArg = I2c_irq_gen_call;
        clearIrqMask = I2c_irq_gen_call;
        // update state -- awaiting user to start a slave rx transfer
        switch(instance->state) {
            case I2c_state_idle:
                instance->state = I2c_state_rx_req;
                break;
            case I2c_state_master_tx:
                instance->state = I2c_state_master_tx_gen_call;
                break;
            case I2c_state_slave_rx:
                // leave state unchanged; user already has an rx buffer
                // set up to receive data.
                break;
            default:
                // should never reach this clause
                DW_ASSERT(false);
                break;
        }
    }
    // rx fifo level at or above threshold
    else if((reg & I2c_irq_rx_full) != 0) {
        // The rx full interrupt should not be unmasked when a DMA
        // interface with hardware handshaking is being used.
        DW_REQUIRE(instance->dmaRx.mode != Dw_dma_hw_handshake);
        if(instance->dmaRx.mode == Dw_dma_sw_handshake) {
            // The user must have previously set an rx notifier via
            // dw_i2c_setDmaRxNotifier.
            DW_REQUIRE(instance->dmaRx.notifier != NULL);
            // Disable the Rx full interrupt .. this is re-enabled
            // after the DMA has finished the current transfer (via a
            // callback set in the DMA driver by the user).
            dw_i2c_maskIrq(dev, I2c_irq_rx_full);
            // Notify the DMA that the Rx FIFO has data to be read.
            // This function and its arguments are set by the user via
            // the dw_i2c_setNotifier_sourceReady() function.
            (instance->dmaRx.notifier)(instance->dmaRx.dmac,
                                       instance->dmaRx.channel,
                                       false, false);
        }
        else {
            if((instance->state == I2c_state_idle) ||
               (instance->state == I2c_state_slave_tx) ||
               (instance->state == I2c_state_slave_bulk_tx)) {
                // sanity check: rxBuffer should be NULL in these states
                DW_ASSERT(instance->rxBuffer == NULL);
                // inform the user listener function of the event
                userCallback = instance->listener;
                callbackArg = I2c_irq_rx_full;
                switch(instance->state) {
                    case I2c_state_idle:
                        instance->state = I2c_state_rx_req;
                        break;
                    case I2c_state_slave_tx:
                        instance->state = I2c_state_slave_tx_rx_req;
                        break;
                    case I2c_state_slave_bulk_tx:
                        instance->state =
                            I2c_state_slave_bulk_tx_rx_req;
                        break;
                    default: break;
                }
            }
            else {
                DW_ASSERT(instance->rxBuffer != NULL);
                // does the rx buffer need to be word-aligned?
                if(instance->rxAlign == false) {
                    // align buffer:
                    tmp = (uint8_t *) instance->rxBuffer;
                    // repeat until either the buffer is aligned, there
                    // is no more space in the rx buffer or there is no
                    // more data to read from the rx fifo
                    while((((uint64_t) tmp) & 0x3ul) &&
                            (instance->rxRemain > 0) &&
                            (dw_i2c_isRxFifoEmpty(dev) == false)) {
                        *tmp++ = (uint8_t) DW_IN32P(portmap->data_cmd);
                        instance->rxRemain--;
                    }
                    instance->rxBuffer = tmp;
                    if(((uint64_t) tmp & 0x3ul) == 0)
                        instance->rxAlign = true;
                }       // instance->rxAlign == false
                // This code is only executed when the rx buffer is
                // word-aligned as I2C_FIFO_READ works efficiently with
                // a word-aligned buffer.
                if(instance->rxAlign == true) {
                    maxBytes = dw_i2c_getRxFifoLevel(dev);
                    I2C_FIFO_READ(maxBytes);
                }       // instance->rxAlign == true
                // if the rx buffer is full
                if(instance->rxRemain == 0) {
                    // Prepare to call the user callback function to
                    // notify it that the current transfer has finished.
                    // For an rx or back-to-back transfer, the number of
                    // bytes received is passed as an argument to the
                    // listener function.
                    userCallback = instance->rxCallback;
                    callbackArg = (int32_t) instance->rxLength;
                    // Flush the instance->rxHold regsiter to the rx
                    // buffer.
                    dw_i2c_flushRxHold(dev);
                    // transfer complete
                    instance->rxBuffer = NULL;
                    instance->rxCallback = NULL;
                    // restore rx threshold to user-specified value
                    I2C_OUTP(instance->rxThreshold, portmap->rx_tl);
                    // update state
                    switch(instance->state) {
                        case I2c_state_master_rx:
                            // End of master-receiver transfer.  Ensure
                            // that the tx empty interrupt is disabled.
                            dw_i2c_maskIrq(dev, I2c_irq_tx_empty);
                            // return to idle state
                            instance->state = I2c_state_idle;
                            break;
                        case I2c_state_slave_rx:
                            // return to idle state
                            instance->state = I2c_state_idle;
                            break;
                        case I2c_state_back2back:
                            // Back-to-back transfer is complete if
                            // there is no more data to send.  Else, the
                            // callback function is not called until all
                            // bytes have been transmitted.  Note that
                            // txCallback is cleared when all bytes have
                            // been sent and/or received.
                            if(instance->txCallback == NULL) {
                                instance->state = I2c_state_idle;
                                DW_ASSERT(dw_i2c_isIrqMasked(dev,
                                        I2c_irq_tx_empty) == true);
                            }
                            else
                                userCallback = NULL;
                            break;
                        case I2c_state_slave_tx_rx:
                            instance->state = I2c_state_slave_tx;
                            break;
                        case I2c_state_slave_bulk_tx_rx:
                            instance->state = I2c_state_slave_bulk_tx;
                            break;
                        case I2c_state_master_tx_slave_rx:
                            instance->state = I2c_state_master_tx;
                            break;
                        default:
                            // this clause should never be reached
                            DW_ASSERT(false);
                            break;
                    }
                }       // remain == 0
                else if(instance->rxRemain < (instance->rxThreshold +
                            0x1u)) {
                    reg = 0;
                    DW_BIT_SET(reg, I2C_RX_TL_RX_TL, (instance->rxRemain
                                - 1));
                    I2C_OUTP(reg, portmap->rx_tl);
                }
            }       // instance->rxBuffer != NULL
        }
    }       // (reg & I2c_irq_rx_full) != 0
    // read-request transfer completed (tx fifo may still contain data)
    else if((reg & I2c_irq_rx_done) != 0) {
        clearIrqMask = I2c_irq_rx_done;
        switch(instance->state) {
            case I2c_state_slave_tx:
                // return to idle state if tx transfer finished
                if(instance->txRemain == 0) {
                    instance->state = I2c_state_idle;
                    DW_ASSERT(dw_i2c_isIrqMasked(dev,
                            I2c_irq_tx_empty) == true);
                    callbackArg = 0;
                    // call user tx callback function
                    userCallback = instance->txCallback;
                    // clear tx buffer and callback function pointers
                    instance->txBuffer = NULL;
                    instance->txCallback = NULL;
                }
                break;
            case I2c_state_slave_tx_rx:
                // return to slave-rx state if tx transfer finished
                if(instance->txRemain == 0) {
                    instance->state = I2c_state_slave_rx;
                    callbackArg = 0;
                    // call user tx callback function
                    userCallback = instance->txCallback;
                    // clear tx buffer and callback function pointers
                    instance->txBuffer = NULL;
                    instance->txCallback = NULL;
                }
                break;
            case I2c_state_slave_bulk_tx:
                // mask tx empty interrupt
                dw_i2c_maskIrq(dev, I2c_irq_tx_empty);
                // return to idle state
                instance->state = I2c_state_idle;
                // call user tx callback function
                userCallback = instance->txCallback;
                // number of bytes left unsent
                callbackArg = (int32_t) (instance->txRemain +
                    dw_i2c_getTxFifoLevel(dev));
                // clear tx buffer and callback function pointers
                instance->txBuffer = NULL;
                instance->txCallback = NULL;
                break;
            case I2c_state_slave_bulk_tx_rx:
                // mask tx empty interrupt
                dw_i2c_maskIrq(dev, I2c_irq_tx_empty);
                // return to slave rx state
                instance->state = I2c_state_slave_rx;
                // call user tx callback function
                userCallback = instance->txCallback;
                // number of bytes left unsent
                callbackArg = (int32_t) (instance->txRemain +
                    dw_i2c_getTxFifoLevel(dev));
                // clear tx buffer and callback function pointers
                instance->txBuffer = NULL;
                instance->txCallback = NULL;
                break;
            default:
                // should not get rx_done in any other driver state
                DW_ASSERT(false);
                break;
        }
    }
    // read request received
    else if((reg & I2c_irq_rd_req) != 0) {
        switch(instance->state) {
            case I2c_state_idle:
                clearIrqMask = I2c_irq_rd_req;
                instance->state = I2c_state_rd_req;
                userCallback = instance->listener;
                callbackArg = I2c_irq_rd_req;
                break;
            case I2c_state_slave_rx:
                clearIrqMask = I2c_irq_rd_req;
                instance->state = I2c_state_slave_rx_rd_req;
                userCallback = instance->listener;
                callbackArg = I2c_irq_rd_req;
                break;
            case I2c_state_slave_tx:
            case I2c_state_slave_tx_rx:
                clearIrqMask = I2c_irq_rd_req;
                // remain in the current state and write the next byte
                // from the tx buffer to the tx fifo
                I2C_FIFO_WRITE(1);
                break;
            case I2c_state_slave_bulk_tx_rx:
                // A read request has occurred because, even though we
                // are performing a slave bulk transfer, the system did
                // not keep the tx FIFO from emptying.  This interrupt
                // is therefore treated the same as a tx empty
                // interrupt.
                maxBytes = MIN(param->tx_buffer_depth -
                    dw_i2c_getTxFifoLevel(dev), (int32_t) instance->txRemain);
                // buffer should be word-aligned (done by
                // dw_i2c_slaveBulkTransmit)
                if(instance->txRemain > 0) {
                    clearIrqMask = I2c_irq_rd_req;
                    I2C_FIFO_WRITE(maxBytes);
                }
                else {
                    // tx buffer has all been sent in bulk mode yet the
                    // master is still requesting more data.  We need to
                    // call the tx callback function first and then pass
                    // read request to the user listener.
                    // update state
                    instance->state = I2c_state_slave_rx;
                    // mask tx empty interrupt
                    dw_i2c_maskIrq(dev, I2c_irq_tx_empty);
                    // call user callback function with no bytes left to
                    // send
                    userCallback = instance->txCallback;
                    callbackArg = 0;
                    instance->txBuffer = NULL;
                    instance->txCallback = NULL;
                }
                break;
            case I2c_state_slave_bulk_tx:
                // A read request has occurred because, even though we
                // are performing a slave bulk transfer, the system did
                // not keep the tx FIFO from emptying.  This interrupt
                // is therefore treated the same as a tx empty
                // interrupt.
                maxBytes = MIN(param->tx_buffer_depth -
                    dw_i2c_getTxFifoLevel(dev), (int32_t) instance->txRemain);
                // buffer should be word-aligned (done by
                // dw_i2c_slaveBulkTransmit)
                if(instance->txRemain > 0) {
                    clearIrqMask = I2c_irq_rd_req;
                    I2C_FIFO_WRITE(maxBytes);
                }
                else {
                    // tx buffer has all been sent in bulk mode yet the
                    // master is still requesting more data.  We need to
                    // call the tx callback function first and then pass
                    // read request to the user listener.
                    // update state
                    instance->state = I2c_state_idle;
                    // mask tx empty interrupt
                    dw_i2c_maskIrq(dev, I2c_irq_tx_empty);
                    DW_ASSERT(dw_i2c_isIrqMasked(dev,
                            I2c_irq_tx_empty) == true);
                    // call user callback function with no bytes left to
                    // send
                    userCallback = instance->txCallback;
                    callbackArg = 0;
                    instance->txBuffer = NULL;
                    instance->txCallback = NULL;
                }
                break;
            default:
                // should not get rd_req in any other driver state
                DW_ASSERT(false);
                break;
        }
    }
    // tx fifo level at or below threshold
    else if((reg & I2c_irq_tx_empty) != 0) {
        // The tx empty interrupt should never be unmasked when we are
        // using DMA with hardware handshaking.
        DW_REQUIRE(instance->dmaTx.mode != Dw_dma_hw_handshake);
        if(instance->dmaTx.mode == Dw_dma_sw_handshake) {
            // The user must have previously set a tx notifier.
            DW_REQUIRE(instance->dmaTx.notifier != NULL);
            // Disable the tx empty interrupt .. this is re-enabled
            // after the DMA has finished the current transfer (via a
            // callback set by the user).
            dw_i2c_maskIrq(dev, I2c_irq_tx_empty);
            // Notify the dma that the tx fifo is ready to receive mode
            // data.  This function and its arguments are set by the
            // user via the dw_i2c_setNotifier_destinationReady()
            // function.
            (instance->dmaTx.notifier)(instance->dmaTx.dmac,
                                       instance->dmaTx.channel,
                                       false, false);
        }
        else if(instance->txRemain == 0) {
            // default: call callback function with zero as argument (no
            // bytes left to send)
            // tx callback function
            userCallback = instance->txCallback;
            // number of bytes left to transmit
            callbackArg = 0;
            dw_i2c_maskIrq(dev, I2c_irq_tx_empty);
            // restore user-specfied tx threshold value
            I2C_OUTP(instance->txThreshold, portmap->tx_tl);
            // update driver state
            switch(instance->state) {
                case I2c_state_master_tx:
                    // return to idle state at end of tx transfer
                    instance->state = I2c_state_idle;
                    DW_ASSERT(dw_i2c_isIrqMasked(dev,
                            I2c_irq_tx_empty) == true);
                    // transfer complete
                    instance->txBuffer = NULL;
                    instance->txCallback = NULL;
                    break;
                case I2c_state_master_tx_slave_rx:
                    // return to slave-rx state if slave rx transfer
                    // is still in progress
                    instance->state = I2c_state_slave_rx;
                    // transfer complete
                    instance->txBuffer = NULL;
                    instance->txCallback = NULL;
                    break;
                case I2c_state_master_rx:
                    // Reset tx buffer and callback function pointers
                    instance->txBuffer = NULL;
                    instance->txCallback = NULL;
                    // For a master-rx transfer, the callback is not
                    // called until the last byte has been received.
                    userCallback = NULL;
                    callbackArg = 0;
                    break;
                case I2c_state_slave_bulk_tx:
                case I2c_state_slave_bulk_tx_rx:
                    // For slave bulk transfers, the callback is not
                    // called until rx_done has been received.
                    userCallback = NULL;
                    callbackArg = 0;
                    break;
                case I2c_state_back2back:
                    if(instance->rxCallback == NULL) {
                        // If there is no more data to receive either,
                        // return to the idle state and call callback
                        // with the number of bytes received.
                        instance->state = I2c_state_idle;
                        DW_ASSERT(dw_i2c_isIrqMasked(dev,
                            I2c_irq_tx_empty) == true);
                        callbackArg = (int32_t) instance->rxLength;
                    }
                    else {
                        // Otherwise, if there is still data to receive,
                        // do not call the user callback function.
                        userCallback = NULL;
                        callbackArg = 0;
                    }
                    // Reset tx buffer and callback function pointers
                    instance->txBuffer = NULL;
                    instance->txCallback = NULL;
                    break;
                default:
                    // we shouldn't get a tx_empty interrupt in any
                    // other driver state.
                    DW_ASSERT(false);
                    break;
            }
        }
        else {
            switch(instance->state) {
                case I2c_state_master_tx:
                case I2c_state_slave_bulk_tx:
                case I2c_state_slave_bulk_tx_rx:
                case I2c_state_master_tx_slave_rx:
                    // slave-transmitter or master-transmitter
                    maxBytes = param->tx_buffer_depth -
                        dw_i2c_getTxFifoLevel(dev);
                    // buffer should already be word-aligned
                    I2C_FIFO_WRITE(maxBytes);
                    break;
                case I2c_state_master_rx:
                    maxBytes = MIN((param->tx_buffer_depth -
                        dw_i2c_getTxFifoLevel(dev)),
                            (int32_t) instance->txRemain);
                    for(i = 0; i < maxBytes; i++)
                        DW_OUT32P(0x100, portmap->data_cmd);
                    instance->txRemain -= (uint32_t) maxBytes;
                    break;
                case I2c_state_back2back:
                    maxBytes = MIN((param->tx_buffer_depth -
                                dw_i2c_getTxFifoLevel(dev)),
                            (int32_t) instance->txRemain);
                    I2C_FIFO_WRITE(maxBytes);
                    instance->txRemain -= (unsigned) maxBytes;
                    break;
                default:
                    // we shouldn't get a tx_empty interrupt in any
                    // other driver state.
                    DW_ASSERT(false);
                    break;
            }
            // If the tx buffer is empty, set the tx threshold to no
            // bytes in fifo.  This is to ensure the the tx callback
            // function, if any, is only called when the current
            // transfer has been completed by the DW_apb_i2c device.
            if(instance->txRemain == 0)
                I2C_OUTP(0x0, portmap->tx_tl);
        }
    }
    // start condition detected
    else if((reg & I2c_irq_start_det) != 0) {
        userCallback = instance->listener;
        callbackArg = I2c_irq_start_det;
        clearIrqMask = I2c_irq_start_det;
    }
    // stop condition detected
    else if((reg & I2c_irq_stop_det) != 0) {
        userCallback = instance->listener;
        callbackArg = I2c_irq_stop_det;
        clearIrqMask = I2c_irq_stop_det;
    }
    // i2c bus activity
    else if((reg & I2c_irq_activity) != 0) {
        userCallback = instance->listener;
        callbackArg = I2c_irq_activity;
        clearIrqMask = I2c_irq_activity;
    }
    else {
        // If we've reached this point, either the enabling and
        // disabling of I2C interrupts is not being handled properly or
        // this function is being called unnecessarily.
        retval = false;
    }

    // call the user listener function, if it has been set
    if(userCallback != NULL)
        userCallback(dev, callbackArg);

    // If the driver is still in one of these states, the user listener
    // function has not correctly handled a device event/interrupt.
    DW_REQUIRE(instance->state != I2c_state_rx_req);
    DW_REQUIRE(instance->state != I2c_state_rd_req);
    DW_REQUIRE(instance->state != I2c_state_slave_tx_rx_req);
    DW_REQUIRE(instance->state != I2c_state_slave_rx_rd_req);
    DW_REQUIRE(instance->state != I2c_state_master_tx_gen_call);
    DW_REQUIRE(instance->state != I2c_state_error);

    // clear the serviced interrupt
    if(clearIrqMask != 0)
        dw_i2c_clearIrq(dev, clearIrqMask);

    return retval;
}

/**********************************************************************/

int dw_i2c_userIrqHandler(struct dw_device *dev)
{
    bool retval;
    uint32_t reg;
    int32_t callbackArg;
    dw_callback userCallback;
    enum dw_i2c_irq clearIrqMask;
    struct dw_i2c_portmap *portmap;
    struct dw_i2c_instance *instance;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;
    instance = (struct dw_i2c_instance *) dev->instance;

    // Assume an interrupt will be processed.  userCallback will be set
    // to NULL and retval to false if no active interrupt is found.
    retval = true;
    userCallback = instance->listener;
    callbackArg = 0;
    clearIrqMask = I2c_irq_none;

    // what caused the interrupt?
    reg = I2C_INP(portmap->intr_stat);

    // if a tx transfer was aborted
    if((reg & I2c_irq_tx_abrt) != 0) {
        callbackArg = I2c_irq_tx_abrt;
        clearIrqMask = I2c_irq_tx_abrt;
    }
    // rx fifo overflow
    else if((reg & I2c_irq_rx_over) != 0) {
        callbackArg = I2c_irq_rx_over;
        clearIrqMask = I2c_irq_rx_over;
    }
    // rx fifo underflow
    else if((reg & I2c_irq_rx_under) != 0) {
        callbackArg = I2c_irq_rx_under;
        clearIrqMask = I2c_irq_rx_under;
    }
    // tx fifo overflow
    else if((reg & I2c_irq_tx_over) != 0) {
        callbackArg = I2c_irq_tx_over;
        clearIrqMask = I2c_irq_tx_over;
    }
    // a general call was detected
    else if((reg & I2c_irq_gen_call) != 0) {
        callbackArg = I2c_irq_gen_call;
        clearIrqMask = I2c_irq_gen_call;
    }
    // rx fifo level at or above threshold
    else if((reg & I2c_irq_rx_full) != 0) {
        callbackArg = I2c_irq_rx_full;
    }
    // read-request transfer completed (Tx FIFO may still contain data)
    else if((reg & I2c_irq_rx_done) != 0) {
        callbackArg = I2c_irq_rx_done;
        clearIrqMask = I2c_irq_rx_done;
    }
    // read request received
    else if((reg & I2c_irq_rd_req) != 0) {
        callbackArg = I2c_irq_rd_req;
        clearIrqMask = I2c_irq_rd_req;
    }
    // tx fifo level at or below threshold
    else if((reg & I2c_irq_tx_empty) != 0) {
        callbackArg = I2c_irq_tx_empty;
    }
    // start condition detected
    else if((reg & I2c_irq_start_det) != 0) {
        callbackArg = I2c_irq_start_det;
        clearIrqMask = I2c_irq_start_det;
    }
    // stop condition detected
    else if((reg & I2c_irq_stop_det) != 0) {
        callbackArg = I2c_irq_stop_det;
        clearIrqMask = I2c_irq_stop_det;
    }
    // i2c bus activity
    else if((reg & I2c_irq_activity) != 0) {
        callbackArg = I2c_irq_activity;
        clearIrqMask = I2c_irq_activity;
    }
    else {
        // no active interrupt was found
        retval = false;
        userCallback = NULL;
    }

    // call the user listener function, if there was an active interrupt
    if(userCallback != NULL)
        userCallback(dev, callbackArg);

    // clear any serviced interrupt
    if(clearIrqMask != 0)
        dw_i2c_clearIrq(dev, clearIrqMask);

    return retval;
}

/**********************************************************************/
/***                    PRIVATE FUNCTIONS                           ***/
/**********************************************************************/

/***
 * The following functions are all private and as such are not part of
 * the driver's public API.
 ***/

int dw_i2c_flushRxHold(struct dw_device *dev)
{
    int i, retval;
    uint8_t *tmp;
    uint32_t c, mask;
    uint32_t *buf;
    struct dw_i2c_instance *instance;

    I2C_COMMON_REQUIREMENTS(dev);

    instance = (struct dw_i2c_instance *) dev->instance;

    DW_REQUIRE(instance->rxBuffer != NULL);

    // sanity check .. idx should never be greater than four
    DW_ASSERT(instance->rxIdx <= 4);
    retval = 0;
    if(instance->rxIdx != 4) {
        // need to handle the case where there is less
        // than four bytes remaining in the rx buffer
        if(instance->rxRemain >= 4) {
            buf = (uint32_t *) ((uint64_t) instance->rxBuffer);
            mask = ((uint32_t) (0xffffffff) >> (8 *
                        instance->rxIdx));
            c = mask & (instance->rxHold >> (8 * instance->rxIdx));
            *buf = (*buf & ~mask) | c;
        }
        else {
            // tmp = next free location in rx buffer
            tmp = (uint8_t *) instance->rxBuffer;
            // shift hold so that the least
            // significant byte contains valid data
            c = instance->rxHold >> (8 * instance->rxIdx);
            // write out valid character to rx buffer
            for(i = (int32_t)(4 - instance->rxIdx); i > 0; i--) {
                *tmp++ = (uint8_t) (c & 0xff);
                c >>= 8;
            }
        }       // instance->rxRemain <= 4
    }       // instance->rxIdx != 4

    return retval;
}

/**********************************************************************/

int dw_i2c_autoCompParams(struct dw_device *dev)
{
    int retval;
    unsigned data_width;
    struct dw_i2c_param *param;
    struct dw_i2c_portmap *portmap;

    I2C_COMMON_REQUIREMENTS(dev);

    param = (struct dw_i2c_param *) dev->comp_param;
    portmap = (struct dw_i2c_portmap *) dev->base_address;

    dev->comp_version = DW_INP(portmap->comp_version);

    // only version 1.03 and greater support identification registers
    if((DW_INP(portmap->comp_type) == Dw_apb_i2c) &&
            (DW_BIT_GET(DW_INP(portmap->comp_param_1),
                I2C_PARAM_ADD_ENCODED_PARAMS) == true)) {
        retval = 0;
        param->hc_count_values =
            DW_BIT_GET(DW_INP(portmap->comp_param_1),
                I2C_PARAM_HC_COUNT_VALUES);
        param->has_dma = DW_BIT_GET(DW_INP(portmap->comp_param_1),
                I2C_PARAM_HAS_DMA);
        data_width = DW_BIT_GET(DW_INP(portmap->comp_param_1),
                I2C_PARAM_DATA_WIDTH);
        switch(data_width) {
            case 2:
                dev->data_width = 32;
                break;
            case 1:
                dev->data_width = 16;
                break;
            default:
                dev->data_width = 8;
                break;
        }
        param->max_speed_mode = (enum dw_i2c_speed_mode)
            DW_BIT_GET(DW_INP(portmap->comp_param_1),
                    I2C_PARAM_MAX_SPEED_MODE);
        param->rx_buffer_depth =
            DW_BIT_GET(DW_INP(portmap->comp_param_1),
                I2C_PARAM_RX_BUFFER_DEPTH);
        param->rx_buffer_depth++;
        param->tx_buffer_depth =
            DW_BIT_GET(DW_INP(portmap->comp_param_1),
                I2C_PARAM_TX_BUFFER_DEPTH);
        param->tx_buffer_depth++;
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

void dw_i2c_resetInstance(struct dw_device *dev)
{
    struct dw_i2c_portmap *portmap;
    struct dw_i2c_instance *instance;

    I2C_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_i2c_portmap *) dev->base_address;
    instance = (struct dw_i2c_instance *) dev->instance;

    instance->state = I2c_state_idle;
    instance->intr_mask_save = I2C_INP(portmap->intr_mask);
    instance->txThreshold = dw_i2c_getTxThreshold(dev);
    instance->rxThreshold = dw_i2c_getRxThreshold(dev);
    instance->listener = NULL;
    instance->txCallback = NULL;
    instance->rxCallback = NULL;
    instance->b2bBuffer = NULL;
    instance->txBuffer = NULL;
    instance->txHold = 0;
    instance->txIdx = 0;
    instance->txLength = 0;
    instance->txRemain = 0;
    instance->rxBuffer = NULL;
    instance->rxHold = 0;
    instance->rxIdx = 0;
    instance->rxLength = 0;
    instance->rxRemain = 0;
    instance->rxAlign = false;
    instance->dmaTx.notifier = NULL;
    instance->dmaRx.notifier = NULL;
    instance->dmaTx.mode = Dw_dma_none;
    instance->dmaRx.mode = Dw_dma_none;
}

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/

