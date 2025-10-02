/* --------------------------------------------------------------------
** 
** Synopsys DesignWare DW_apb_uart Software Driver Kit and
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

#include <string.h>                 // needed for memcpy()
#include "DW_common.h"              // common header for all devices
#include "DW_apb_uart_public.h"     // DW_apb_uart public header
#include "DW_apb_uart_private.h"    // DW_apb_uart private header

// This definition is used by the assetion macros to determine the
// current file name.  It is defined in the DW_common_dbc header.
DW_DEFINE_THIS_FILE;

/**********************************************************************/

int dw_uart_init(struct dw_device *dev)
{
    int retval;

    UART_COMMON_REQUIREMENTS(dev);

    // disable all uart interrupts
    dw_uart_disableIrq(dev, Uart_irq_all);

    // reset device FIFOs
    dw_uart_resetTxFifo(dev);
    dw_uart_resetRxFifo(dev);

    // initialize private variables
    dw_uart_resetInstance(dev);

    // attempt to determine hardware parameters
    retval = dw_uart_autoCompParams(dev);

    return retval;
}

/**********************************************************************/

int32_t dw_uart_isBusy(struct dw_device *dev)
{
    int32_t retval;
    uint32_t reg;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;

    // if fifo status registers are available (version 3.00 and above)
    if(dev->comp_version >= 0x3330302a) {
        // check busy bit of uart status register
        reg = UART_IN8P(portmap->usr);
        if(DW_BIT_GET(reg, UART_USR_BUSY) == Dw_set)
            retval = true;
        else
            retval = false;
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

int dw_uart_reset(struct dw_device *dev)
{
    int retval;
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;

    if(param->shadow == true) {
        reg = 0;
        retval = 0;
        DW_BIT_SET(reg, UART_SRR_UR, Dw_set);
        UART_OUT8P(reg, portmap->srr);
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

void dw_uart_resetTxFifo(struct dw_device *dev)
{
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;
    instance = (struct dw_uart_instance *) dev->instance;

    // The FIFO reset bits are self-clearing.
    if(param->shadow == true) {
        reg = 0;
        DW_BIT_SET(reg, UART_SRR_XFR, Dw_set);
        UART_OUT8P(reg, portmap->srr);
    }
    else {
        reg = instance->value_in_fcr;
        DW_BIT_SET(reg, UART_FCR_XMIT_FIFO_RESET, Dw_set);
        UART_OUT8P(reg, portmap->iir_fcr);
    }
}

/**********************************************************************/

void dw_uart_resetRxFifo(struct dw_device *dev)
{
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;
    instance = (struct dw_uart_instance *) dev->instance;

    // The FIFO reset bits are self-clearing.
    if(param->shadow == true) {
        reg = 0;
        DW_BIT_SET(reg, UART_SRR_RFR, Dw_set);
        UART_OUT8P(reg, portmap->srr);
    }
    else {
        reg = instance->value_in_fcr;
        DW_BIT_SET(reg, UART_FCR_RCVR_FIFO_RESET, Dw_set);
        UART_OUT8P(reg, portmap->iir_fcr);
    }
}

/**********************************************************************/

int dw_uart_setClockDivisor(struct dw_device *dev, uint16_t
        divisor)
{
    int retval;
    uint32_t reg;
    uint8_t dll, dlh;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;

    // For version 3, the divisor latch access bit (DLAB) of the line
    // control register (LCR) cannot be written while the UART is busy.
    // This does not affect version 2.
    if(dw_uart_isBusy(dev) == true)
        retval = -DW_EBUSY;
    else {
        retval = 0;
        // set DLAB to access DLL and DLH registers
        reg = UART_IN8P(portmap->lcr);
        DW_BIT_SET(reg, UART_LCR_DLAB, Dw_set);
        UART_OUT8P(reg, portmap->lcr);
        dll = divisor & 0x00ffu;
        dlh = (uint8_t) ((divisor & 0xff00u) >> 8);
        UART_OUT8P(dll, portmap->rbr_thr_dll);
        UART_OUT8P(dlh, portmap->ier_dlh);
        // clear DLAB
        DW_BIT_CLEAR(reg, UART_LCR_DLAB);
        UART_OUT8P(reg, portmap->lcr);
    }

    return retval;
}

/**********************************************************************/

uint16_t dw_uart_getClockDivisor(struct dw_device *dev)
{
    uint8_t dll, dlh;
    uint16_t retval;
    uint32_t reg;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;

    // set DLAB to access DLL and DLH registers
    reg = UART_IN8P(portmap->lcr);
    DW_BIT_SET(reg, UART_LCR_DLAB, Dw_set);
    UART_OUT8P(reg, portmap->lcr);
    dll = (uint8_t) UART_IN8P(portmap->rbr_thr_dll);
    dlh = (uint8_t) UART_IN8P(portmap->ier_dlh);
    // clear DLAB
    DW_BIT_CLEAR(reg, UART_LCR_DLAB);
    UART_OUT8P(reg, portmap->lcr);
    
    retval = (uint16_t) ((dlh << 8) | dll);

    return retval;
}

/**********************************************************************/

int32_t dw_uart_setLineControl(struct dw_device *dev, enum
        dw_uart_line_control setting)
{
    int32_t retval;
    uint8_t reg;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;

    // Note: it does not matter if the UART is busy when setting the
    // line control register on DW_apb_uart v2.00.  For uart v2.00,
    // the isBusy function returns -DW_ENOSYS (not supported).  This is
    // necessary, however, for UART v3.00.
    if(dw_uart_isBusy(dev) == true)
        retval = -DW_EBUSY;
    else {
        retval = 0;
        reg = (uint8_t) UART_IN8P(portmap->lcr);
        // avoid bus write is possible
        if(DW_BIT_GET(reg, UART_LCR_LINE) != setting) {
            DW_BIT_SET_8(reg, UART_LCR_LINE, setting);
            UART_OUT8P(reg, portmap->lcr);
        }
    }

    return retval;
}

/**********************************************************************/

enum dw_uart_line_control dw_uart_getLineControl(struct dw_device
        *dev)
{
    uint32_t reg;
    enum dw_uart_line_control retval;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;

    // get the value of the LCR, masking irrelevant bits
    reg = UART_IN8P(portmap->lcr);
    retval = (enum dw_uart_line_control) DW_BIT_GET(reg, UART_LCR_LINE);

    return retval;
}

/**********************************************************************/

void dw_uart_setDataBits(struct dw_device *dev, enum dw_uart_cls
        cls)
{
    uint32_t reg;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;

    reg = UART_IN8P(portmap->lcr);
    if(DW_BIT_GET(reg, UART_LCR_CLS) != cls) {
        DW_BIT_SET(reg, UART_LCR_CLS, cls);
        UART_OUT8P(reg, portmap->lcr);
    }
}

/**********************************************************************/

enum dw_uart_cls dw_uart_getDataBits(struct dw_device *dev)
{
    uint32_t reg;
    enum dw_uart_cls retval;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;

    reg = UART_IN8P(portmap->lcr);
    retval = (enum dw_uart_cls) DW_BIT_GET(reg, UART_LCR_CLS);

    return retval;
}

/**********************************************************************/

void dw_uart_setStopBits(struct dw_device *dev, enum
        dw_uart_stop_bits stop)
{
    uint32_t reg;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;

    reg = UART_IN8P(portmap->lcr);
    if(DW_BIT_GET(reg, UART_LCR_STOP) != stop) {
        DW_BIT_SET(reg, UART_LCR_STOP, stop);
        UART_OUT8P(reg, portmap->lcr);
    }
}

/**********************************************************************/

enum dw_uart_stop_bits dw_uart_getStopBits(struct dw_device *dev)
{
    uint32_t reg;
    enum dw_uart_stop_bits retval;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;

    reg = UART_IN8P(portmap->lcr);
    retval = (enum dw_uart_stop_bits) DW_BIT_GET(reg, UART_LCR_STOP);

    return retval;
}

/**********************************************************************/

void dw_uart_setParity(struct dw_device *dev, enum dw_uart_parity
        parity)
{
    uint32_t reg;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;

    reg = UART_IN8P(portmap->lcr);
    if(DW_BIT_GET(reg, UART_LCR_PARITY) != parity) {
        DW_BIT_SET(reg, UART_LCR_PARITY, parity);
        UART_OUT8P(reg, portmap->lcr);
    }
}

/**********************************************************************/

enum dw_uart_parity dw_uart_getParity(struct dw_device *dev)
{
    uint32_t reg;
    enum dw_uart_parity retval;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;

    reg = UART_IN8P(portmap->lcr);
    retval = (enum dw_uart_parity) DW_BIT_GET(reg, UART_LCR_PARITY);

    return retval;
}

/**********************************************************************/

int dw_uart_enableFifos(struct dw_device *dev)
{
    int retval;
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;
    instance = (struct dw_uart_instance *) dev->instance;

    // if FIFOs are available
    if(param->fifo_mode != 0) {
        retval = 0;
        // The status of the FIFOs is cached here as reading the IIR
        // register directly with dw_uart_areFifosEnabled() can
        // inadvertently clear any pending THRE interrupt.
        instance->fifos_enabled = true;
        // update stored FCR value
        DW_BIT_SET(instance->value_in_fcr, UART_FCR_FIFO_ENABLE, Dw_set);
        // if shadow registers are available
        if(param->shadow == true) {
            reg = 0;
            DW_BIT_SET(reg, UART_SFE_FE, Dw_set);
            UART_OUT8P(reg, portmap->sfe);
        }
        else {
            UART_OUT8P(instance->value_in_fcr, portmap->iir_fcr);
        }
    }
    else
        retval = -DW_ENOSYS;       // function not supported

    return retval;
}

/**********************************************************************/

int dw_uart_disableFifos(struct dw_device *dev)
{
    int retval;
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;
    instance = (struct dw_uart_instance *) dev->instance;

    // if FIFOs are available
    if(param->fifo_mode != 0) {
        retval = 0;
        // The status of the FIFOs is cached here as reading the IIR
        // register directly with dw_uart_areFifosEnabled() can
        // inadvertently clear any pending THRE interrupt.
        instance->fifos_enabled = false;
        // update stored FCR value
        DW_BIT_CLEAR(instance->value_in_fcr, UART_FCR_FIFO_ENABLE);
        // if shadow registers are available
        if(param->shadow == true) {
            reg = 0;
            DW_BIT_SET(reg, UART_SFE_FE, Dw_clear);
            UART_OUT8P(reg, portmap->sfe);
        }
        else {
            UART_OUT8P(instance->value_in_fcr, portmap->iir_fcr);
        }
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

bool dw_uart_areFifosEnabled(struct dw_device *dev)
{
    bool retval;
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;

    if(param->shadow == true) {
        reg = UART_IN8P(portmap->sfe);
        if(DW_BIT_GET(reg, UART_SFE_FE) == Dw_clear)
            retval = false;
        else
            retval = true;
    }
    else {
        reg = UART_IN8P(portmap->iir_fcr);
        if(DW_BIT_GET(reg, UART_IIR_FIFO_STATUS) == 0x3)
            retval = true;
        else
            retval = false;
    }

    return retval;
}

/**********************************************************************/

int dw_uart_isTxFifoFull(struct dw_device *dev)
{
    int retval;
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;

    // if FIFO status registers are available (UART version >= 3.00)
    if(param->fifo_stat == true) {
        reg = UART_IN8P(portmap->usr);
        if(DW_BIT_GET(reg, UART_USR_TFNF) == Dw_clear)
            retval = true;
        else
            retval = false;
    }
    else
        retval = -DW_ENOSYS;       // function not supported

    return retval;
}

/**********************************************************************/

int dw_uart_isTxFifoEmpty(struct dw_device *dev)
{
    int retval;
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;

    if(param->fifo_stat == true) {
        reg = UART_IN8P(portmap->usr);
        if(DW_BIT_GET(reg, UART_USR_TFE) == 1)
            retval = true;
        else
            retval = false;
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

int dw_uart_isRxFifoFull(struct dw_device *dev)
{
    int retval;
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;

    // If FIFO status registers are available (UART version >= 3.00)
    if(param->fifo_stat == true) {
        reg = UART_IN8P(portmap->usr);
        if(DW_BIT_GET(reg, UART_USR_RFF) == 1)
            retval = true;
        else
            retval = false;
    }
    else
        retval = -DW_ENOSYS;       // function not supported

    return retval;
}

/**********************************************************************/

int dw_uart_isRxFifoEmpty(struct dw_device *dev)
{
    int retval;
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;

    // if FIFO status registers are available (version 3.00 and above)
    if(param->fifo_stat == true) {
        reg = UART_IN8P(portmap->usr);
        // RFNE bit clear == Rx FIFO empty
        if(DW_BIT_GET(reg, UART_USR_RFNE) == Dw_clear)
            retval = true;
        else
            retval = false;
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

int dw_uart_getTxFifoLevel(struct dw_device *dev)
{
    int retval;
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;

    // It does not make sense to call this function if there are no
    // FIFOs.
    DW_REQUIRE(param->fifo_mode != 0);

    // If FIFO status registers are available (version 3.00 and above)
    if(param->fifo_stat == true) {
        reg = UART_INP(portmap->tfl);
        retval = DW_BIT_GET(reg, UART_TFL_LEVEL);
    }
    else
        retval = -DW_ENOSYS;       // function not supported

    return retval;
}

/**********************************************************************/

int dw_uart_getRxFifoLevel(struct dw_device *dev)
{
    int retval;
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;

    DW_REQUIRE(param->fifo_mode != 0);

    // if FIFO status registers are available (version 3.00 and above)
    if(param->fifo_stat == true) {
        reg = UART_INP(portmap->rfl);
        retval = DW_BIT_GET(reg, UART_RFL_LEVEL);
    }
    else
        retval = -DW_ENOSYS;       // function not supported

    return retval;
}

/**********************************************************************/

unsigned dw_uart_getFifoDepth(struct dw_device *dev)
{
    unsigned retval;
    struct dw_uart_param *param;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;

    retval = param->fifo_mode;

    return retval;
}

/**********************************************************************/

int dw_uart_enablePtime(struct dw_device *dev)
{
    int retval;
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;
    instance = (struct dw_uart_instance *) dev->instance;

    if(param->thre_mode == true) {
        retval = 0;
        reg = UART_IN8P(portmap->ier_dlh);
        // avoid bus write if possible
        if(DW_BIT_GET(reg, UART_IER_PTIME) != Dw_set) {
            DW_BIT_SET(reg, UART_IER_PTIME, Dw_set);
            // save IER value
            instance->ier_save = reg;
            // enable PTIME
            UART_OUT8P(reg, portmap->ier_dlh);
        }
    }
    else
        retval = -DW_ENOSYS;       // function not implemented

    return retval;
}

/**********************************************************************/

int dw_uart_disablePtime(struct dw_device *dev)
{
    int retval;
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;
    instance = (struct dw_uart_instance *) dev->instance;

    if(param->thre_mode == true) {
        retval = 0;
        reg = UART_IN8P(portmap->ier_dlh);
        // avoid bus write if possible
        if(DW_BIT_GET(reg, UART_IER_PTIME) != Dw_clear) {
            DW_BIT_SET(reg, UART_IER_PTIME, Dw_clear);
            // save IER value
            instance->ier_save = reg;
            // disable PTIME
            UART_OUT8P(reg, portmap->ier_dlh);
        }
    }
    else
        retval = -DW_ENOSYS;       // function not implemented

    return retval;
}

/**********************************************************************/

bool dw_uart_isPtimeEnabled(struct dw_device *dev)
{
    bool retval;
    uint32_t reg;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;

    reg = UART_IN8P(portmap->ier_dlh);
    if(DW_BIT_GET(reg, UART_IER_PTIME) == Dw_set)
        retval = true;
    else
        retval = false;

    return retval;
}

/**********************************************************************/

void dw_uart_setBreak(struct dw_device *dev, enum dw_state state)
{
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    DW_REQUIRE((state == Dw_clear) || (state == Dw_set));

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;

    if(param->shadow == true) {
        reg = 0;
        DW_BIT_SET(reg, UART_SBCR_BCR, state);
        UART_OUT8P(state, portmap->sbcr);
    }
    else {
        reg = UART_IN8P(portmap->lcr);
        // avoid bus write if possible
        if((enum dw_state) DW_BIT_GET(reg, UART_LCR_BREAK) != state) {
            DW_BIT_SET(reg, UART_LCR_BREAK, state);
            UART_OUT8P(reg, portmap->lcr);
        }
    }
}

/**********************************************************************/

enum dw_state dw_uart_getBreak(struct dw_device *dev)
{
    uint32_t reg;
    enum dw_state retval;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;

    if(param->shadow == true) {
        reg = UART_IN8P(portmap->sbcr);
        retval = (enum dw_state) DW_BIT_GET(reg, UART_SBCR_BCR);
    }
    else {
        reg = UART_IN8P(portmap->lcr);
        retval = (enum dw_state) DW_BIT_GET(reg, UART_LCR_BREAK);
    }

    return retval;
}

/**********************************************************************/

void dw_uart_setModemLine(struct dw_device *dev,
        enum dw_uart_modem_line lines)
{
    uint32_t reg;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;

    reg = UART_IN8P(portmap->mcr);
    reg |= lines;
    UART_OUT8P(reg, portmap->mcr);
}

/**********************************************************************/

void dw_uart_clearModemLine(struct dw_device *dev,
        enum dw_uart_modem_line lines)
{
    uint32_t reg;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;

    reg = UART_IN8P(portmap->mcr);
    reg &= ~lines;
    UART_OUT8P(reg, portmap->mcr);
}

/**********************************************************************/

enum dw_uart_modem_line dw_uart_getModemLine(struct dw_device
        *dev)
{
    uint32_t reg;
    enum dw_uart_modem_line retval;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;

    reg = UART_IN8P(portmap->mcr);
    retval = (enum dw_uart_modem_line) (reg & Uart_modem_line_all);

    return retval;
}

/**********************************************************************/

void dw_uart_enableLoopback(struct dw_device *dev)
{
    uint32_t reg;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;

    reg = UART_IN8P(portmap->mcr);
    if(DW_BIT_GET(reg, UART_MCR_LOOPBACK) != Dw_set) {
        DW_BIT_SET(reg, UART_MCR_LOOPBACK, Dw_set);
        UART_OUT8P(reg, portmap->mcr);
    }
}

/**********************************************************************/

void dw_uart_disableLoopback(struct dw_device *dev)
{
    uint32_t reg;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;

    reg = UART_IN8P(portmap->mcr);
    if(DW_BIT_GET(reg, UART_MCR_LOOPBACK) != Dw_clear) {
        DW_BIT_SET(reg, UART_MCR_LOOPBACK, Dw_clear);
        UART_OUT8P(reg, portmap->mcr);
    }
}

/**********************************************************************/

bool dw_uart_isLoopbackEnabled(struct dw_device *dev)
{
    bool retval;
    uint32_t reg;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;

    reg = UART_IN8P(portmap->mcr);
    retval = (bool) DW_BIT_GET(reg, UART_MCR_LOOPBACK);

    return retval;
}

/**********************************************************************/

int dw_uart_enableAfc(struct dw_device *dev)
{
    int retval;
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;

    if(param->afce_mode == true) {
        reg = UART_IN8P(portmap->mcr);
        DW_BIT_SET(reg, UART_MCR_AFCE, Dw_set);
        UART_OUT8P(reg, portmap->mcr);
        retval = 0;
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

int dw_uart_disableAfc(struct dw_device *dev)
{
    int retval;
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;

    if(param->afce_mode == true) {
        reg = UART_IN8P(portmap->mcr);
        DW_BIT_CLEAR(reg, UART_MCR_AFCE);
        UART_OUT8P(reg, portmap->mcr);
        retval = 0;
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

bool dw_uart_isAfcEnabled(struct dw_device *dev)
{
    bool retval;
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;

    if(param->afce_mode == true) {
        reg = UART_IN8P(portmap->mcr);
        retval = DW_BIT_GET(reg, UART_MCR_AFCE);
    }
    else
        retval = false;

    return retval;
}

/**********************************************************************/

int dw_uart_enableSir(struct dw_device *dev)
{
    int retval;
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;

    if(param->sir_mode == true) {
        reg = UART_IN8P(portmap->mcr);
        DW_BIT_SET(reg, UART_MCR_SIRE, Dw_set);
        UART_OUT8P(reg, portmap->mcr);
        retval = 0;
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

int dw_uart_disableSir(struct dw_device *dev)
{
    int retval;
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;

    if(param->sir_mode == true) {
        reg = UART_IN8P(portmap->mcr);
        DW_BIT_CLEAR(reg, UART_MCR_SIRE);
        UART_OUT8P(reg, portmap->mcr);
        retval = 0;
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

bool dw_uart_isSirEnabled(struct dw_device *dev)
{
    bool retval;
    uint32_t reg;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;

    reg = UART_IN8P(portmap->mcr);
    retval = DW_BIT_GET(reg, UART_MCR_SIRE);

    return retval;
}

/**********************************************************************/

enum dw_uart_line_status dw_uart_getLineStatus(struct dw_device
        *dev)
{
    uint8_t reg;
    enum dw_uart_line_status retval;
    struct dw_uart_portmap *portmap;
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;
    instance = (struct dw_uart_instance *) dev->instance;

    // read line status register
    reg = (uint8_t) UART_IN8P(portmap->lsr);
    // include saved line status errors
    retval = (enum dw_uart_line_status) (reg | instance->lsr_save);
    // reset saved errors
    instance->lsr_save = 0;

    return retval;
}

/**********************************************************************/

enum dw_uart_modem_status dw_uart_getModemStatus(struct dw_device
        *dev)
{
    uint8_t reg;
    enum dw_uart_modem_status retval;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;

    reg = (uint8_t) UART_IN8P(portmap->msr);
    retval = (enum dw_uart_modem_status) reg;

    return retval;
}

/**********************************************************************/

void dw_uart_setScratchpad(struct dw_device *dev, uint8_t value)
{
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;

    UART_OUT8P(value, portmap->scr);
}

/**********************************************************************/

uint8_t dw_uart_getScratchpad(struct dw_device *dev)
{
    uint8_t retval;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;

    retval = (uint8_t) UART_IN8P(portmap->scr);

    return retval;
}

/**********************************************************************/

void dw_uart_setDmaMode(struct dw_device *dev, enum
        dw_uart_dma_mode mode)
{
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;
    instance = (struct dw_uart_instance *) dev->instance;

    // update stored FCR value
    DW_BIT_SET(instance->value_in_fcr, UART_FCR_DMA_MODE, mode);
    // set dma mode
    if(param->shadow == true) {
        reg = 0;
        DW_BIT_SET(reg, UART_SDMAM_MODE, mode);
        UART_OUT8P(reg, portmap->sdmam);
    }
    else
        UART_OUT8P(instance->value_in_fcr, portmap->iir_fcr);
}

/**********************************************************************/

enum dw_uart_dma_mode dw_uart_getDmaMode(struct dw_device *dev)
{
    uint32_t reg;
    enum dw_uart_dma_mode retval;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;
    instance = (struct dw_uart_instance *) dev->instance;

    // get dma mode
    if(param->shadow == true) {
        reg = UART_IN8P(portmap->sdmam);
        retval = (enum dw_uart_dma_mode) DW_BIT_GET(reg,
                UART_SDMAM_MODE);
    }
    else {
        if(DW_BIT_GET(instance->value_in_fcr, UART_FCR_DMA_MODE) ==
                Dw_clear)
            retval = Uart_dma_single;
        else
            retval = Uart_dma_multi;
    }

    return retval;
}

/**********************************************************************/

int dw_uart_dmaSwAck(struct dw_device *dev)
{
    int retval;
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;

    if(param->dma_extra == true) {
        reg = 0;
        retval = 0;
        DW_BIT_SET(reg, UART_DMASA_ACK, Dw_set);
        UART_OUT8P(reg, portmap->dmasa);
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

uint8_t dw_uart_read(struct dw_device *dev)
{
    uint8_t retval;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;

    retval = (uint8_t) UART_IN8P(portmap->rbr_thr_dll);
    
    return retval;
}

/**********************************************************************/

void dw_uart_write(struct dw_device *dev, uint8_t character)
{
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;

    UART_OUT8P(character, portmap->rbr_thr_dll);
}

/**********************************************************************/

int dw_uart_burstRead(struct dw_device *dev, uint8_t *buffer,
        unsigned length)
{
    int i, index, retval;
    uint32_t tmpbuf[4];
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;
    DW_REQUIRE(length <= param->fifo_mode);

    if(param->shadow == true) {
        retval = i = 0;
        // if there are four or more bytes to be read
        while(((int) length - (i + 4)) >= 4) {
            // burst read from the shadow receive buffer registers
            memcpy(tmpbuf, &(portmap->srbr_sthr[0]), sizeof(uint32_t) *
                    4);
            // copy 4 bytes from temporary word buffer to user buffer
            for(index = 0; index < 4; i++, index++)
                buffer[i] = (uint8_t) tmpbuf[index];
        }
        // read any remaining characters
        while(i < (int)length) {
            buffer[i++] = (uint8_t) UART_IN8P(portmap->rbr_thr_dll);
        }
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

int dw_uart_burstWrite(struct dw_device *dev, uint8_t *buffer,
        unsigned length)
{
    int i, index, retval;
    uint32_t tmpbuf[4];
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;
    DW_REQUIRE(length <= param->fifo_mode);

    if(param->shadow == true) {
        retval = i = 0;
        while(((int) length - i) >= 4) {
            // copy 4 bytes from user buffer to temporary word buffer
            for(index = 0; index < 4; i++, index++)
                tmpbuf[index] = buffer[i];
            // burst write to the shadow transmit hold registers
            memcpy(&(portmap->srbr_sthr[0]), tmpbuf, sizeof(uint32_t) *
                    4);
        }
        // write any remaining characters
        while(i < (int)length) {
            UART_OUT8P(buffer[i++], portmap->rbr_thr_dll);
        }
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

void dw_uart_enableIrq(struct dw_device *dev, enum dw_uart_irq
        interrupts)
{
    uint32_t reg;
    struct dw_uart_portmap *portmap;
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;
    instance = (struct dw_uart_instance *) dev->instance;

    // get current interrupt enable settings
    reg = UART_IN8P(portmap->ier_dlh);
    // set interrupts to be enabled
    reg |= interrupts;
    // update copy of IER value (used when avoiding shared data issues)
    instance->ier_save = reg;
    // write new IER value
    UART_OUT8P(reg, portmap->ier_dlh);
}

/**********************************************************************/

void dw_uart_disableIrq(struct dw_device *dev, enum dw_uart_irq
        interrupts)
{
    uint32_t reg;
    struct dw_uart_portmap *portmap;
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;
    instance = (struct dw_uart_instance *) dev->instance;

    // get current interrupt enable settings
    reg = UART_IN8P(portmap->ier_dlh);
    // mask interrupts to be disabled
    reg = (uint32_t) reg & ~interrupts;
    // update copy of IER value (used when avoiding shared data issues)
    instance->ier_save = reg;
    // write new IER value
    UART_OUT8P(reg, portmap->ier_dlh);
}

/**********************************************************************/

bool dw_uart_isIrqEnabled(struct dw_device *dev, enum dw_uart_irq
        interrupt)
{
    bool retval;
    uint32_t reg;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);
    DW_REQUIRE((interrupt == Uart_irq_erbfi)
            || (interrupt == Uart_irq_etbei)
            || (interrupt == Uart_irq_elsi)
            || (interrupt == Uart_irq_edssi));

    portmap = (struct dw_uart_portmap *) dev->base_address;

    // get current interrupt enable settings
    reg = UART_IN8P(portmap->ier_dlh);
    // specified interrupt enabled?
    if((reg & interrupt) != 0)
        retval = true;
    else
        retval = false;

    return retval;
}

/**********************************************************************/

uint8_t dw_uart_getIrqMask(struct dw_device *dev)
{
    uint8_t retval;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;

    // get current interrupt enable settings
    retval = (uint8_t) UART_IN8P(portmap->ier_dlh);

    return retval;
}

/**********************************************************************/

enum dw_uart_event dw_uart_getActiveIrq(struct dw_device *dev)
{
    uint32_t reg;
    enum dw_uart_event retval;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;

    // get current interrupt ID
    reg = UART_IN8P(portmap->iir_fcr);
    retval = (enum dw_uart_event) DW_BIT_GET(reg,
            UART_IIR_INTERRUPT_ID);

    return retval;
}

/**********************************************************************/


void dw_uart_setTxTrigger(struct dw_device *dev, enum
        dw_uart_tx_trigger trigger)
{
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;
    instance = (struct dw_uart_instance *) dev->instance;

    // save user-specified Tx trigger
    instance->txTrigger = trigger;

    // update stored FCR value
    DW_BIT_SET(instance->value_in_fcr, UART_FCR_TX_EMPTY_TRIGGER,
            trigger);
    // set transmitter empty trigger
    if(param->shadow == true) {
        reg = 0;
        DW_BIT_SET(reg, UART_STET_TRIGGER, trigger);
        UART_OUTP(reg, portmap->stet);
    }
    else
        UART_OUTP(instance->value_in_fcr, portmap->iir_fcr);
}

/**********************************************************************/

enum dw_uart_tx_trigger dw_uart_getTxTrigger(struct dw_device
        *dev)
{
    uint32_t reg;
    enum dw_uart_tx_trigger retval;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;
    instance = (struct dw_uart_instance *) dev->instance;

    // get transmitter empty trigger
    if(param->shadow == true) {
        reg = UART_IN8P(portmap->stet);
        retval = (enum dw_uart_tx_trigger) DW_BIT_GET(reg,
                UART_STET_TRIGGER);
    }
    else
        retval = (enum dw_uart_tx_trigger)
            DW_BIT_GET(instance->value_in_fcr,
                    UART_FCR_TX_EMPTY_TRIGGER);

    return retval;
}

/**********************************************************************/

void dw_uart_setRxTrigger(struct dw_device *dev, enum
        dw_uart_rx_trigger trigger)
{
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;
    instance = (struct dw_uart_instance *) dev->instance;

    // update stored FCR value
    DW_BIT_SET(instance->value_in_fcr, UART_FCR_RCVR_TRIGGER, trigger);
    // set receiver trigger
    if(param->shadow == true) {
        reg = 0;
        DW_BIT_SET(reg, UART_SRT_TRIGGER, trigger);
        UART_OUTP(reg, portmap->srt);
    }
    else
        UART_OUTP(instance->value_in_fcr, portmap->iir_fcr);
}

/**********************************************************************/

enum dw_uart_rx_trigger dw_uart_getRxTrigger(struct dw_device
        *dev)
{
    uint32_t reg;
    enum dw_uart_rx_trigger retval;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;
    instance = (struct dw_uart_instance *) dev->instance;

    // get receiver trigger
    if(param->shadow == true) {
        reg = UART_IN8P(portmap->srt);
        retval = (enum dw_uart_rx_trigger) DW_BIT_GET(reg,
                UART_SRT_TRIGGER);
    }
    else
        retval = (enum dw_uart_rx_trigger)
            DW_BIT_GET(instance->value_in_fcr, UART_FCR_RCVR_TRIGGER);

    return retval;
}

/**********************************************************************/

void dw_uart_setListener(struct dw_device *dev, dw_callback
        userFunction)
{
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);
    DW_REQUIRE(userFunction != NULL);

    instance = (struct dw_uart_instance *) dev->instance;

    // set user listener function
    instance->listener = userFunction;

    // enable interrupts/events which should be handled by the user
    // listener function
    dw_uart_enableIrq(dev, (enum dw_uart_irq) (Uart_irq_elsi |
                Uart_irq_erbfi));
}

/**********************************************************************/

int dw_uart_transmit(struct dw_device *dev, void *buffer, unsigned
        length, dw_callback userFunction)
{
    int retval;
    unsigned numChars;
    uint8_t *tmp;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);
    DW_REQUIRE(buffer != NULL);
    DW_REQUIRE(length != 0);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;
    instance = (struct dw_uart_instance *) dev->instance;

    // disable UART interrupts
    UART_ENTER_CRITICAL_SECTION();
    
    if((instance->state == Uart_state_tx)
            || (instance->state == Uart_state_tx_rx))
        retval = -DW_EBUSY;
    else {
        retval = 0;

        // update state
        switch(instance->state) {
            case Uart_state_rx:
                instance->state = Uart_state_tx_rx;
                break;
            case Uart_state_idle:
                instance->state = Uart_state_tx;
                break;
            default:
                DW_ASSERT(false);
                break;
        }

        // reset number of characters sent in current/last transfer
        instance->txLength = length;
        instance->txRemain = length;
        instance->txCallback = userFunction;
        instance->txHold = 0;
        instance->txIdx = 0;

        // check for non word-aligned buffer as UART[_X]_FIFO_WRITE()
        // works efficiently on word reads from txHold
        tmp = (uint8_t *) buffer;
        while(((uint64_t) tmp) & 0x3ul) {
            instance->txHold |= ((*tmp++ & 0xffu) << (instance->txIdx *
                        8));
            instance->txIdx++;
        }
        instance->txBuffer = (uint32_t *) (uint64_t)tmp;

        if(instance->fifos_enabled == false) {
            UART_FIFO_WRITE();
        }
        else if(dw_uart_isTxFifoEmpty(dev) == true) {
            numChars = param->fifo_mode;
            UART_X_FIFO_WRITE((int)numChars);
        }
        else if(param->fifo_stat == true) {
            numChars = (unsigned) (param->fifo_mode - dw_uart_getTxFifoLevel(dev));
            UART_X_FIFO_WRITE((int)numChars);
        }
        else {
            UART_FIFO_WRITE();
        }
        
        // make sure THR/Tx FIFO empty interrupt is enabled
        instance->ier_save |= Uart_irq_etbei;
    }

    // restore UART interrupts
    UART_EXIT_CRITICAL_SECTION();

    return retval;
}

/**********************************************************************/

int dw_uart_receive(struct dw_device *dev, void *buffer, unsigned
        length, dw_callback userFunction)
{
    int retval;
    unsigned numChars;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;
    struct dw_uart_instance *instance;
    dw_callback userCallback;

    UART_COMMON_REQUIREMENTS(dev);
    DW_REQUIRE(buffer != NULL);
    DW_REQUIRE(length != 0);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;
    instance = (struct dw_uart_instance *) dev->instance;

    // disable UART interrupts
    UART_ENTER_CRITICAL_SECTION();

    if((instance->state == Uart_state_rx)
            || (instance->state == Uart_state_tx_rx))
        retval = -DW_EBUSY;
    else {
        retval = 0;

        switch(instance->state) {
            case Uart_state_tx:
            case Uart_state_tx_rx_req:
                instance->state = Uart_state_tx_rx;
                break;
            case Uart_state_idle:
            case Uart_state_rx_req:
                instance->state = Uart_state_rx;
                break;
            default:
                DW_ASSERT(false);
                break;
        }

        // check for buffer alignment
        if(((uint64_t) buffer & 0x3ul) == 0)
            instance->rxAlign = true;
        else
            instance->rxAlign = false;

        // update Rx buffer
        instance->rxBuffer = buffer;
        // reset number of characters received for current transfer
        instance->rxLength = length;
        instance->rxRemain = length;
        instance->rxCallback = userFunction;
        instance->rxHold = 0;
        instance->rxIdx = 4;

        // if the Rx buffer is 32-bit word-aligned
        if(instance->rxAlign == true) {
            if(instance->fifos_enabled == false) {
                UART_FIFO_READ();
            }
            else if(param->fifo_stat == true) {
                numChars = (unsigned) dw_uart_getRxFifoLevel(dev);
                UART_X_FIFO_READ((int)numChars);
            }
            else {
                UART_FIFO_READ();
            }
        }

        // if we've already filled the user Rx buffer
        if(instance->rxRemain == 0) {
            // flush any byte in the hold register
            dw_uart_flushRxHold(dev);
            // update state
            switch(instance->state) {
                case Uart_state_tx_rx:
                    instance->state = Uart_state_tx;
                    break;
                case Uart_state_rx:
                    instance->state = Uart_state_idle;
                    break;
                default:
                    DW_ASSERT(false);
                    break;
            }
            userCallback = instance->rxCallback;
            instance->rxBuffer = NULL;
            instance->rxCallback = NULL;
            // call callback functoin
            if(userCallback != NULL)
                (userCallback)(dev, (int32_t)instance->rxLength);
        }
        else  {
            // ensure receive is underway
            instance->ier_save |= Uart_irq_erbfi;
        }
    }

    // restore UART interrupts
    UART_EXIT_CRITICAL_SECTION();

    return retval;
}

/**********************************************************************/

void dw_uart_setCallbackMode(struct dw_device *dev, enum
        dw_uart_callback_mode mode)
{
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);

    instance = (struct dw_uart_instance *) dev->instance;

    instance->callbackMode = mode;
}

/**********************************************************************/

enum dw_uart_callback_mode dw_uart_getCallbackMode(struct dw_device
        *dev)
{
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);

    instance = (struct dw_uart_instance *) dev->instance;

    return instance->callbackMode;
}

/**********************************************************************/

int32_t dw_uart_terminateTx(struct dw_device *dev)
{
    int32_t retval;
    struct dw_uart_portmap *portmap;
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;
    instance = (struct dw_uart_instance *) dev->instance;

    // disable UART interrupts
    UART_ENTER_CRITICAL_SECTION();

    // disable Tx interrupt
    instance->ier_save &= ~(unsigned)Uart_irq_etbei;

    retval = 0;
    // terminate current Tx transfer
    switch(instance->state) {
        case Uart_state_tx_rx:
            instance->state = Uart_state_rx;
            break;
        case Uart_state_tx:
            instance->state = Uart_state_idle;
            break;
        default:
            retval = -DW_EPERM;
            break;
    }

    // Terminate Tx transfer if one is in progress.
    if(retval != -DW_EPERM) {
        instance->txCallback = NULL;
        instance->txBuffer = NULL;
        retval = (int32_t) (instance->txLength - instance->txRemain);
    }

    // restore UART interrupts
    UART_EXIT_CRITICAL_SECTION();

    return retval;
}

/**********************************************************************/

int32_t dw_uart_terminateRx(struct dw_device *dev)
{
    int32_t retval;
    struct dw_uart_portmap *portmap;
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;
    instance = (struct dw_uart_instance *) dev->instance;

    // disable UART interrupts
    UART_ENTER_CRITICAL_SECTION();

    retval = 0;
    // update state
    switch(instance->state) {
        case Uart_state_tx:
        case Uart_state_tx_rx:
            instance->state = Uart_state_tx;
            break;
        case Uart_state_rx:
            instance->state = Uart_state_idle;
            break;
        default:
            retval = -DW_EPERM;
            break;
    }

    // Terminate Rx transfer if one is in progress.
    if(retval != -DW_EPERM) {
        // Flush any remaining data in the Rx FIFO to the user Rx
        // buffer.
        dw_uart_flushRxFifo(dev);
        // Transfer terminated.
        instance->rxBuffer = NULL;
        instance->rxCallback = NULL;
        // Return the number of bytes stored in the user Rx buffer.
        retval = (int32_t) (instance->rxLength - instance->rxRemain);
    }

    // restore UART interrupts
    UART_EXIT_CRITICAL_SECTION();

    return retval;
}

/**********************************************************************/

void dw_uart_setDmaTxMode(struct dw_device *dev, enum dw_dma_mode mode)
{
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);

    instance = (struct dw_uart_instance *) dev->instance;

    if(mode == Dw_dma_sw_handshake) {
        DW_REQUIRE(instance->dmaTx.notifier != NULL);
    }

    instance->dmaTx.mode = mode;
}

/**********************************************************************/

enum dw_dma_mode dw_uart_getDmaTxMode(struct dw_device *dev)
{
    enum dw_dma_mode retval;
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);

    instance = (struct dw_uart_instance *) dev->instance;

    retval = instance->dmaTx.mode;

    return retval;
}

/**********************************************************************/

void dw_uart_setDmaRxMode(struct dw_device *dev, enum dw_dma_mode mode)
{
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);

    instance = (struct dw_uart_instance *) dev->instance;

    if(mode == Dw_dma_sw_handshake) {
        DW_REQUIRE(instance->dmaRx.notifier != NULL);
    }

    if(mode == Dw_dma_hw_handshake)
        dw_uart_disableIrq(dev, Uart_irq_erbfi);
    else
        dw_uart_enableIrq(dev, Uart_irq_erbfi);

    instance->dmaRx.mode = mode;
}

/**********************************************************************/

enum dw_dma_mode dw_uart_getDmaRxMode(struct dw_device *dev)
{
    enum dw_dma_mode retval;
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);

    instance = (struct dw_uart_instance *) dev->instance;

    retval = instance->dmaRx.mode;

    return retval;
}

/**********************************************************************/

void dw_uart_setNotifier_sourceReady(struct dw_device *dev,
        dw_dma_notifier_func funcptr, struct dw_device *dmac, unsigned
        channel)
{
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);
    DW_REQUIRE(funcptr != NULL);
    DW_REQUIRE(dmac != NULL);
    DW_REQUIRE(dmac->comp_type == Dw_ahb_dmac);

    instance = (struct dw_uart_instance *) dev->instance;

    instance->dmaRx.notifier = funcptr;
    instance->dmaRx.dmac = dmac;
    instance->dmaRx.channel = channel;
}

/**********************************************************************/

void dw_uart_setNotifier_destinationReady(struct dw_device *dev,
        dw_dma_notifier_func funcptr, struct dw_device *dmac, unsigned
        channel)
{
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);
    DW_REQUIRE(funcptr != NULL);
    DW_REQUIRE(dmac != NULL);
    DW_REQUIRE(dmac->comp_type == Dw_ahb_dmac);

    instance = (struct dw_uart_instance *) dev->instance;

    instance->dmaTx.notifier = funcptr;
    instance->dmaTx.dmac = dmac;
    instance->dmaTx.channel = channel;
}

/**********************************************************************/

// NOTE: This interrupt handler updates variables in the 'instance'
// structure.  Any other code which modifies these variables must be
// guarded against shared data issues.
int dw_uart_irqHandler(struct dw_device *dev)
{
    int retval;
    int callbackArg;
    bool ptime, fifos;
    unsigned numChars;
    unsigned chars_to_align;
    uint8_t *tmp;
    uint32_t reg;
    enum dw_uart_event event;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;
    struct dw_uart_instance *instance;
    dw_callback userCallback;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;
    instance = (struct dw_uart_instance *) dev->instance;

    // A listener function _must_ be setup before using interrupts.
    DW_REQUIRE(instance->listener != NULL);

    // Assume an interrupt will be processed.  This will be set to false
    // if there is no active interrupt found.
    retval = true;
    userCallback = NULL;
    callbackArg = 0;

    // what caused this interrupt
    reg = UART_IN8P(portmap->iir_fcr);
    event = (enum dw_uart_event) (reg & 0xf);
    // are FIFOs enabled?
    fifos = (DW_BIT_GET(reg, UART_IIR_FIFO_STATUS) == 0 ? false : true);
    // process event
    switch(event) {
        case Uart_event_none:
            // no pending interrupt
            retval = false;
            break;
        case Uart_event_modem:
            userCallback = instance->listener;
            callbackArg = Uart_event_modem;
            break;
        case Uart_event_thre:
            // The Tx empty interrupt should never be enabled when using
            // DMA with hardware handshaking.
            DW_REQUIRE(instance->dmaTx.mode != Dw_dma_hw_handshake);
            if(instance->dmaTx.mode == Dw_dma_sw_handshake) {
                // The user must have previously set a Tx notifier.
                DW_REQUIRE(instance->dmaTx.notifier != NULL);
                // Disable the Tx empty interrupt .. this is re-enabled
                // after the DMA has finished the current transfer (via
                // a callback set by the user).
                dw_uart_disableIrq(dev, Uart_irq_etbei);
                // Notify the dma that the Tx fifo is ready to receive
                // mode data.  This function and its arguments are set
                // with the dw_uart_setNotifier_destinationReady()
                // function.
                (instance->dmaTx.notifier)(instance->dmaTx.dmac,
                                           instance->dmaTx.channel,
                                           false, false);
            }
            else if(instance->txRemain == 0) {
                switch(instance->state) {
                    case Uart_state_tx:
                    case Uart_state_tx_rx:
                        // disable interrupt
                        dw_uart_disableIrq(dev, Uart_irq_etbei);
                        // restore user Tx trigger
                        DW_BIT_SET(instance->value_in_fcr,
                                UART_FCR_TX_EMPTY_TRIGGER,
                                instance->txTrigger);
                        UART_OUT8P(instance->value_in_fcr,
                                portmap->iir_fcr);
                        // inform user of end of transfer
                        userCallback = instance->txCallback;
                        // pass callback the number of bytes sent
                        callbackArg = (int) instance->txLength;
                        // update state
                        if(instance->state == Uart_state_tx_rx)
                            instance->state = Uart_state_rx;
                        else
                            instance->state = Uart_state_idle;
                        instance->txBuffer = NULL;
                        instance->txCallback = NULL;
                        break;
                    default:
                        // We should not get this interrupt in any other
                        // state.
                        DW_ASSERT(false);
                        break;
                }
            }
            else {
                // Is PTIME enabled?
                ptime = dw_uart_isPtimeEnabled(dev);
                switch(instance->state) {
                    case Uart_state_tx:
                    case Uart_state_tx_rx:
                        if(fifos == false) {
                            // Can only write one character if FIFOs are
                            // disabled.
                            numChars = 1;
                        }
                        else if(param->fifo_stat == true) {
                            // If the FIFO status registers are
                            // available, we can query how many
                            // characters are already in the Tx FIFO.
                            numChars = (unsigned) (param->fifo_mode -
                                dw_uart_getTxFifoLevel(dev));
                        }
                        else if(ptime == false) {
                            // If PTIME is disabled when a
                            // Uart_event_thre interrupt occurs, the Tx
                            // FIFO is completely empty.
                            numChars = param->fifo_mode;
                        }
                        else {
                            // How many characters we can write to the
                            // Tx FIFO depends on the trigger which
                            // caused this interrupt.
                            switch(dw_uart_getTxTrigger(dev)) {
                                case Uart_empty_fifo:
                                    numChars = param->fifo_mode;
                                    break;
                                case Uart_two_chars_in_fifo:
                                    numChars = (unsigned) (param->fifo_mode - 2);
                                    break;
                                case Uart_quarter_full_fifo:
                                    numChars = (unsigned) (param->fifo_mode * 3/4);
                                    break;
                                case Uart_half_full_fifo:
                                    numChars = (param->fifo_mode / 2);
                                    break;
                                default:
                                    DW_ASSERT(false);
                                    break;
                            }
                        }
                        // Write maximum number of bytes to the Tx
                        // FIFO with no risk of overflow.
                        UART_X_FIFO_WRITE((int)numChars);

                        if((ptime == true) && (fifos == true)) {
                            // Send more bytes if the Tx FIFO is still 
                            // not full.  Stops when LSR THRE bit (FIFO
                            // full) becomes Set (PTIME enabled!).
                            UART_FIFO_WRITE();
                        }
                        break;
                    default:
                        // We should not get this interrupt in any other
                        // state.
                        DW_ASSERT(false);
                        break;
                }
                // Ensure we get an interrupt when the last byte has
                // been sent.  The user callback function will be called
                // on the next thre interrupt.
                if(instance->txRemain == 0) {
                    DW_BIT_SET(instance->value_in_fcr,
                            UART_FCR_TX_EMPTY_TRIGGER, Uart_empty_fifo);
                    UART_OUT8P(instance->value_in_fcr,
                            portmap->iir_fcr);
                }
            }
            break;
        case Uart_event_timeout:
        case Uart_event_data:
            // The Rx full interrupt should never be enabled when using
            // a DMA with hardware handshaking.
            DW_REQUIRE(instance->dmaRx.mode != Dw_dma_hw_handshake);
            if(instance->dmaRx.mode == Dw_dma_sw_handshake) {
                // The user must have previously set an Rx notifier.
                DW_REQUIRE(instance->dmaRx.notifier != NULL);
                // If this is a timeout interrupt, ignore it as there is
                // not enough data in the Rx FIFO for the DMA to read.
                if(event == Uart_event_timeout)
                    break;
                // Disable the Rx full interrupt .. this is re-enabled
                // after the DMA has finished the current transfer (via
                // a callback set by the user).
                dw_uart_disableIrq(dev, Uart_irq_erbfi);
                // Notify the DMA that the Rx FIFO is ready to be read.
                // This function and its arguments are set by the
                // user via the dw_uart_setNotifier_sourceReady()
                // function.
                (instance->dmaRx.notifier)(instance->dmaRx.dmac,
                                           instance->dmaRx.channel,
                                           false, false);
            }
            else if((instance->state == Uart_state_rx)
                    || (instance->state == Uart_state_tx_rx)) {
                // Does the Rx buffer need to be 32-bit word-aligned?
                // Need to do this here as UART_[X_]FIFO_READ() works
                // efficiently on word writes from rxHold
                chars_to_align = 0;
                if(instance->rxAlign == false) {
                    tmp = (uint8_t *) instance->rxBuffer;
                    // need to store any errors that may occur so the
                    // user sees them on a subsequent call to
                    // dw_uart_getLineStatus().
                    reg = UART_IN8P(portmap->lsr);
                    // save any line status errors
                    instance->lsr_save = (reg & (Uart_line_oe |
                                Uart_line_pe | Uart_line_fe |
                                Uart_line_bi));
                    while((((uint64_t) tmp) & 0x3ul) &&
                            (instance->rxRemain > 0) &&
                            ((reg & Uart_line_dr) == Uart_line_dr)) {
                        *tmp++ = (uint8_t) UART_IN8P(portmap->rbr_thr_dll);
                        instance->rxRemain--;
                        // Record the number of characters read used to
                        // align the buffer pointer.
                        chars_to_align++;
                        // read the line status register
                        reg = UART_IN8P(portmap->lsr);
                        // save any line status errors
                        instance->lsr_save = (reg & (Uart_line_oe |
                                Uart_line_pe | Uart_line_fe |
                                Uart_line_bi));
                    }
                    instance->rxBuffer = (uint32_t *) (uint64_t) tmp;
                    if(((uint64_t) tmp & 0x3ul) == 0)
                        instance->rxAlign = true;
                }

                // number of characters to read from the RBR/Rx FIFO
                numChars = 0;
                // The following code only gets executed when the Rx
                // buffer is 32-bit word-aligned.
                if(instance->rxAlign == true) {
                    if(fifos == false) {
                        // There is only one character available if
                        // FIFOs are disabled.
                        if(chars_to_align == 0)
                            numChars = 1;
                    }
                    else if(param->fifo_stat == true) {
                        // If the FIFO status registers are available,
                        // we can simply query how many characters are
                        // in the Rx FIFO.
                        numChars = (unsigned) dw_uart_getRxFifoLevel(dev);
                    }
                    else if((event == Uart_event_data) &&
                            (dw_uart_isPtimeEnabled(dev) == true)) {
                        // If this interrupt was caused by received data
                        // reaching the Rx FIFO full trigger level.
                        switch(dw_uart_getRxTrigger(dev)) {
                            case Uart_one_char_in_fifo:
                                numChars = 1;
                                break;
                            case Uart_fifo_quarter_full:
                                numChars = param->fifo_mode / 4;
                                break;
                            case Uart_fifo_half_full:
                                numChars = param->fifo_mode / 2;
                                break;
                            case Uart_fifo_two_less_full:
                                numChars = param->fifo_mode - 0x2u;
                                break;
                            default:
                                DW_ASSERT(false);
                                break;
                        }
                        // Subtract any bytes already read when
                        // aligning the Rx buffer.
                        if(numChars > chars_to_align)
                            numChars -= chars_to_align;
                    }
                    // Read maximum known number of characters
                    // from the Rx FIFO without underflowing it.
                    UART_X_FIFO_READ((int)numChars);
                    // Read any remaining bytes in the Rx FIFO (until
                    // LSR Data Ready bit is Dw_clear).
                    UART_FIFO_READ();
                }       // instance->rxAlign == true

                // end of rxBuffer?
                if(instance->rxRemain == 0) {
                    // flush rxHold variable to user Rx buffer
                    dw_uart_flushRxHold(dev);
                    // inform user of end of transfer
                    userCallback = instance->rxCallback;
                    // pass callback the number of bytes received
                    callbackArg = (int) instance->rxLength;
                    // update state
                    if(instance->state == Uart_state_tx_rx)
                        instance->state = Uart_state_tx;
                    else
                        instance->state = Uart_state_idle;
                    instance->rxBuffer = NULL;
                    instance->rxCallback = NULL;
                }       // rxRemain == 0
                else if((event == Uart_event_timeout) &&
                        (instance->callbackMode == Uart_rx_timeout)) {
                    // Call the associated callback function if this
                    // interrupt was caused by a character timeout and
                    // the callback mode has been set to
                    // Uart_rx_timeout.
                    userCallback = instance->rxCallback;
                    callbackArg = (int) (instance->rxLength -
                        instance->rxRemain);
                }      // rxRemain == 0
            }       // Uart_state_rx
            else {
                // update state
                switch(instance->state) {
                    case Uart_state_tx:
                        instance->state = Uart_state_tx_rx_req;
                        break;
                    case Uart_state_idle:
                        instance->state = Uart_state_rx_req;
                        break;
                    default:
                        DW_ASSERT(false);
                        break;
                }
                userCallback = instance->listener;
                callbackArg = event;
            }
            break;
        case Uart_event_line:
            // inform user callback function
            userCallback = instance->listener;
            callbackArg = Uart_event_line;
            break;
        case Uart_event_busy:
            userCallback = instance->listener;
            callbackArg = Uart_event_busy;
            retval = -DW_EIO;
            break;
        default:
            // If we've reached this point, the value read from the
            // iir_fcr register is unrecognized.
            retval = -DW_EIO;
            break;
    }

    // If required, call the user listener or callback function
    if(userCallback != NULL) {
        userCallback(dev, callbackArg);
    }

    // The driver should never be in one of these states when the
    // interrupt handler is finished.  If it is, the user listener
    // function has not correctly dealt with an event/interrupt.
    DW_REQUIRE(instance->state != Uart_state_rx_req);
    DW_REQUIRE(instance->state != Uart_state_tx_rx_req);

    return retval;
}

/**********************************************************************/

int dw_uart_userIrqHandler(struct dw_device *dev)
{
    int retval;
    int callbackArg;
    uint32_t reg;
    enum dw_uart_event event;
    struct dw_uart_portmap *portmap;
    struct dw_uart_instance *instance;
    dw_callback userCallback;

    UART_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_uart_portmap *) dev->base_address;
    instance = (struct dw_uart_instance *) dev->instance;

    // A listener function _must_ be setup before using interrupts.
    DW_REQUIRE(instance->listener != NULL);

    userCallback = NULL;
    callbackArg = 0;

    // what caused this interrupt
    reg = UART_IN8P(portmap->iir_fcr);
    event = (enum dw_uart_event) (reg & 0xf);
    // assume an interrupt will be processed
    retval = true;
    
    // process event
    switch(event) {
        case Uart_event_none:
            // no interrupt has occurred
            retval = false;
            break;
        case Uart_event_modem:
            userCallback = instance->listener;
            callbackArg = Uart_event_modem;
            break;
        case Uart_event_thre:
            userCallback = instance->listener;
            callbackArg = Uart_event_thre;
            break;
        case Uart_event_timeout:
            userCallback = instance->listener;
            callbackArg = Uart_event_timeout;
            break;
        case Uart_event_data:
            userCallback = instance->listener;
            callbackArg = Uart_event_data;
            break;
        case Uart_event_line:
            userCallback = instance->listener;
            callbackArg = Uart_event_line;
            break;
        case Uart_event_busy:
            userCallback = instance->listener;
            callbackArg = Uart_event_busy;
            break;
        default:
            // If we've reached this point, the value read from the
            // iir_fcr register is unrecognized.
            retval = -DW_EIO;
            break;
    }

    // If an interrupt has occurred, pass it to the user listener
    // function.
    if(userCallback != NULL) {
        userCallback(dev, callbackArg);
    }

    return retval;
}

/**********************************************************************/

int dw_uart_enableFifoAccess(struct dw_device *dev)
{
    int retval;
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;

    if(param->fifo_access == true) {
        reg = 0;
        retval = 0;
        DW_BIT_SET(reg, UART_FAR_FIFO_ACCESS, Dw_set);
        UART_OUT8P(reg, portmap->far);
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

int dw_uart_disableFifoAccess(struct dw_device *dev)
{
    int retval;
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;

    if(param->fifo_access == true) {
        reg = 0;
        retval = 0;
        DW_BIT_SET(reg, UART_FAR_FIFO_ACCESS, Dw_clear);
        UART_OUT8P(reg, portmap->far);
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

bool dw_uart_isFifoAccessEnabled(struct dw_device *dev)
{
    bool retval;
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;

    if(param->fifo_access == true) {
        reg = UART_IN8P(portmap->far);
        if(DW_BIT_GET(reg, UART_FAR_FIFO_ACCESS) == Dw_set)
            retval = true;
        else
            retval = false;
    }
    else
        retval = false;

    return retval;
}

/**********************************************************************/

int dw_uart_writeRx(struct dw_device *dev, uint16_t character)
{
    int retval;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;

    if(param->fifo_access == true) {
        retval = 0;
        UART_OUTP(character, portmap->rfw);
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

int dw_uart_readTx(struct dw_device *dev)
{
    int32_t retval;
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;

    if(param->fifo_access == true) {
        reg = UART_IN8P(portmap->tfr);
        retval = reg & 0xff;
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

int dw_uart_haltTx(struct dw_device *dev)
{
    int retval;
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;

    if(param->fifo_access == true) {
        reg = 0;
        retval = 0;
        DW_BIT_SET(reg, UART_HTX_HALT, Dw_set);
        UART_OUT8P(reg, portmap->htx);
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

int dw_uart_resumeTx(struct dw_device *dev)
{
    int retval;
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;

    if(param->fifo_access == true) {
        reg = 0;
        retval = 0;
        DW_BIT_SET(reg, UART_HTX_HALT, Dw_clear);
        UART_OUT8P(reg, portmap->htx);
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

bool dw_uart_isTxHalted(struct dw_device *dev)
{
    bool retval;
    uint32_t reg;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;

    if(param->fifo_access == true) {
        reg = UART_IN8P(portmap->htx);
        retval = DW_BIT_GET(reg, UART_HTX_HALT);
    }
    else
        retval = false;

    return retval;
}

/**********************************************************************/
/***                    PRIVATE FUNCTIONS                           ***/
/**********************************************************************/

void dw_uart_resetInstance(struct dw_device *dev)
{
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);

    instance = (struct dw_uart_instance *) dev->instance;

    // initialize instance variables
    instance->state = Uart_state_idle;
    instance->value_in_fcr = 0;
    instance->lsr_save = 0;
    instance->ier_save = 0;
    instance->txCallback = NULL;
    instance->rxCallback = NULL;
    instance->callbackMode = Uart_buffer_full;
    instance->listener = NULL;
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
    instance->fifos_enabled = dw_uart_areFifosEnabled(dev);
    instance->txTrigger = dw_uart_getTxTrigger(dev);
    instance->dmaTx.mode = Dw_dma_none;
    instance->dmaRx.mode = Dw_dma_none;
    instance->dmaTx.notifier = NULL;
    instance->dmaRx.notifier = NULL;
}

/**********************************************************************/

int dw_uart_flushRxFifo(struct dw_device *dev)
{
    int retval;
    unsigned numChars;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;
    instance = (struct dw_uart_instance *) dev->instance;

    // if a user Rx buffer is available
    if(instance->rxBuffer != NULL) {
        retval = 0;

        if(instance->fifos_enabled == false) {
            UART_FIFO_READ();
        }
        else if(param->fifo_stat == true) {
            numChars = (unsigned) dw_uart_getRxFifoLevel(dev);
            UART_X_FIFO_READ((int)numChars);
        }
        else {
            UART_FIFO_READ();
        }

        // flush the Rx 'hold' variable
        dw_uart_flushRxHold(dev);
    }
    else
        retval = -DW_EPERM;     // permission denied (no Rx buffer set)

    return retval;
}

/**********************************************************************/

void dw_uart_flushRxHold(struct dw_device *dev)
{
    int i;
    uint8_t *tmp;
    uint32_t rxMask, c;
    struct dw_uart_instance *instance;

    UART_COMMON_REQUIREMENTS(dev);

    instance = (struct dw_uart_instance *) dev->instance;

    DW_REQUIRE(instance->rxIdx <= 4);
    // if there is data in the rxHold variable
    if(instance->rxIdx != 4) {
        // Flush read buffer of data if last read in
        // UART_[X_]FIFO_READ was not 4 complete bytes
        // if there is greater than 4 bytes remaining in the Rx buffer
        if(instance->rxRemain >= 4) {
            // safe to perform 32-bit writes
            rxMask = ((uint32_t) (0xffffffff) >> (8 *
                        instance->rxIdx));
            c = rxMask & (instance->rxHold >> (8 *
                        instance->rxIdx));
            *instance->rxBuffer = (*instance->rxBuffer & ~rxMask) | c;
        }
        else {
            // Need to write each byte individually to avoid writing
            // past the end of the Rx buffer.
            // tmp = next free location in Rx buffer
            tmp = (uint8_t *) instance->rxBuffer;
            // shift rxHold so that lsB contains valid data
            c = instance->rxHold >> (8 * instance->rxIdx);
            // write out valid character to Rx buffer
            for(i = (int)(4 - instance->rxIdx); i > 0; i--) {
                *tmp++ = (uint8_t) (c & 0xff);
                c >>= 8;
            }
        }
    }
}

/**********************************************************************/

int dw_uart_autoCompParams(struct dw_device *dev)
{
    int retval;
    uint32_t comp_param;
    struct dw_uart_param *param;
    struct dw_uart_portmap *portmap;

    UART_COMMON_REQUIREMENTS(dev);

    param = (struct dw_uart_param *) dev->comp_param;
    portmap = (struct dw_uart_portmap *) dev->base_address;

    // assume component parameters register is not available
    retval = -DW_ENOSYS;

    // only version 3.00 and greater support component identification
    // registers
    if(UART_INP(portmap->comp_type) == Dw_apb_uart) {
        dev->comp_version = UART_INP(portmap->comp_version);
        comp_param = UART_INP(portmap->comp_param_1);
        if(comp_param != 0x0) {
            retval = 0;
            param->afce_mode = DW_BIT_GET(comp_param,
                    UART_PARAM_AFCE_MODE);
            param->fifo_access = DW_BIT_GET(comp_param,
                    UART_PARAM_FIFO_ACCESS);
            param->fifo_mode = DW_BIT_GET(comp_param,
                    UART_PARAM_FIFO_MODE);
            param->fifo_mode = (uint8_t)(param->fifo_mode * (uint8_t) 0x10u);
            param->fifo_stat = DW_BIT_GET(comp_param,
                    UART_PARAM_FIFO_STAT);
            param->new_feat = DW_BIT_GET(comp_param,
                    UART_PARAM_NEW_FEAT);
            param->shadow = DW_BIT_GET(comp_param, UART_PARAM_SHADOW);
            param->sir_lp_mode = DW_BIT_GET(comp_param,
                    UART_PARAM_SIR_LP_MODE);
            param->sir_mode = DW_BIT_GET(comp_param,
                    UART_PARAM_SIR_MODE);
            param->thre_mode = DW_BIT_GET(comp_param,
                    UART_PARAM_THRE_MODE);
            param->dma_extra = DW_BIT_GET(comp_param,
                    UART_PARAM_DMA_EXTRA);
        }
    }

    return retval;
}

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/

