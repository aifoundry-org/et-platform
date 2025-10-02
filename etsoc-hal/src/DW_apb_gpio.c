/* --------------------------------------------------------------------
** 
** Synopsys DesignWare DW_apb_gpio Software Driver Kit and
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

#include "DW_common.h"              // common definitions
#include "DW_apb_gpio_public.h"     // DW_apb_gpio public header
#include "DW_apb_gpio_private.h"    // DW_apb_gpio private header

DW_DEFINE_THIS_FILE;

/**********************************************************************/

int dw_gpio_init(struct dw_device *dev)
{
    int retval;
    struct dw_gpio_instance *instance;

    GPIO_COMMON_REQUIREMENTS(dev);

    // reset the listener function pointer variable
    instance = (struct dw_gpio_instance *) dev->instance;
    instance->listener = NULL;

    // attempt to determine hardware parameters
    retval = dw_gpio_autoCompParams(dev);

    return retval;
}

/**********************************************************************/

uint32_t dw_gpio_getIdCode(struct dw_device *dev)
{
    uint32_t retval;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    if(param->id == true)
        retval = GPIO_INP(portmap->id_code);
    else
        retval = 0;

    return retval;
}

/**********************************************************************/

uint32_t dw_gpio_getRtlVersion(struct dw_device *dev)
{
    uint32_t retval;

    GPIO_COMMON_REQUIREMENTS(dev);

    retval = dev->comp_version;

    return retval;
}

/**********************************************************************/

unsigned dw_gpio_getNumPorts(struct dw_device *dev)
{
    unsigned retval;
    struct dw_gpio_param *param;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;

    retval = param->num_ports;

    return retval;
}

/**********************************************************************/

int dw_gpio_getPortWidth(struct dw_device *dev, enum dw_gpio_port port)
{
    int retval;
    struct dw_gpio_param *param;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;

    DW_REQUIRE(port < param->num_ports);

    retval = 0;
    switch(port) {
        case Gpio_port_a:
            retval = param->pwidth_a;
            break;
        case Gpio_port_b:
            if(param->num_ports >= 2)
                retval = param->pwidth_b;
            break;
        case Gpio_port_c:
            if(param->num_ports >= 3)
                retval = param->pwidth_c;
            break;
        case Gpio_port_d:
            if(param->num_ports == 4)
                retval = param->pwidth_d;
            break;
    }

    return retval;
}

/**********************************************************************/

int dw_gpio_setPort(struct dw_device *dev, enum dw_gpio_port port,
        uint32_t data)
{
    int retval;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    DW_REQUIRE(port < param->num_ports);

    retval = -DW_ENOSYS;
    switch(port) {
        case Gpio_port_a:
            retval = 0;
            GPIO_OUTP(data, portmap->swporta_dr);
            break;
        case Gpio_port_b:
            if(param->num_ports >= 2) {
                retval = 0;
                GPIO_OUTP(data, portmap->swportb_dr);
            }
            break;
        case Gpio_port_c:
            if(param->num_ports >= 3) {
                retval = 0;
                GPIO_OUTP(data, portmap->swportc_dr);
            }
            break;
        case Gpio_port_d:
            if(param->num_ports == 4) {
                retval = 0;
                GPIO_OUTP(data, portmap->swportd_dr);
            }
            break;
    }

    return retval;
}

/**********************************************************************/

uint32_t dw_gpio_getPort(struct dw_device *dev, enum dw_gpio_port port)
{
    uint32_t retval;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    DW_REQUIRE(port < param->num_ports);

    retval = 0;
    switch(port) {
        case Gpio_port_a:
            retval = GPIO_INP(portmap->swporta_dr);
            break;
        case Gpio_port_b:
            if(param->num_ports >= 2) {
                retval = GPIO_INP(portmap->swportb_dr);
            }
            break;
        case Gpio_port_c:
            if(param->num_ports >= 3) {
                retval = GPIO_INP(portmap->swportc_dr);
            }
            break;
        case Gpio_port_d:
            if(param->num_ports == 4) {
                retval = GPIO_INP(portmap->swportd_dr);
            }
            break;
    }

    return retval;
}

/**********************************************************************/

uint32_t dw_gpio_getExtPort(struct dw_device *dev, enum dw_gpio_port
        port)
{
    uint32_t retval;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    //DW_REQUIRE(port < param->num_ports);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    DW_REQUIRE(port < param->num_ports);

    retval = 0;
    switch(port) {
        case Gpio_port_a:
            retval = GPIO_INP(portmap->ext_porta);
            break;
        case Gpio_port_b:
            if(param->num_ports >= 2) {
                retval = GPIO_INP(portmap->ext_portb);
            }
            break;
        case Gpio_port_c:
            if(param->num_ports >= 3) {
                retval = GPIO_INP(portmap->ext_portc);
            }
            break;
        case Gpio_port_d:
            if(param->num_ports == 4) {
                retval = GPIO_INP(portmap->ext_portd);
            }
            break;
    }

    return retval;
}

/**********************************************************************/

int dw_gpio_setBit(struct dw_device *dev, enum dw_gpio_port port,
        enum dw_state value, uint32_t bits)
{
    int retval;
    uint32_t data_in;
    uint32_t data_out;
    volatile uint32_t *ptr;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    DW_REQUIRE(port < param->num_ports);

    retval = -DW_ENOSYS;
    switch(port) {
        case Gpio_port_a:
            retval = 0;
            ptr = &portmap->swporta_dr;
            break;
        case Gpio_port_b:
            if(param->num_ports >= 2) {
                retval = 0;
                ptr = &portmap->swportb_dr;
            }
            break;
        case Gpio_port_c:
            if(param->num_ports >= 3) {
                retval = 0;
                ptr = &portmap->swportc_dr;
            }
            break;
        case Gpio_port_d:
            if(param->num_ports == 4) {
                retval = 0;
                ptr = &portmap->swportd_dr;
            }
            break;
    }

    // if a legal port has been specified
    if(retval == 0) {
        data_in = GPIO_INP(*ptr);
        switch(value) {
            case Dw_clear:
                data_out = data_in & ~bits;
                break;
            case Dw_set:
                data_out = data_in | bits;
                break;
            default:
                retval = -DW_EINVAL;
                break;
        }
        // avoid bus write if possible
        if((data_in != data_out) && (retval == 0))
            GPIO_OUTP(data_out, *ptr);
    }

    return retval;
}

/**********************************************************************/

enum dw_state dw_gpio_getBit(struct dw_device *dev, enum dw_gpio_port
        port, enum dw_gpio_bit bit)
{
    enum dw_state retval;
    uint32_t reg;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    DW_REQUIRE(port < param->num_ports);

    retval = Dw_err;
    switch(port) {
        case Gpio_port_a:
            if(bit < param->pwidth_a) {
                reg = GPIO_INP(portmap->swporta_dr);
                retval = (enum dw_state) ((reg & (0x1u << bit)) >> bit);
            }
            break;
        case Gpio_port_b:
            if((param->num_ports >= 2) && (bit < param->pwidth_b)) {
                reg = GPIO_INP(portmap->swportb_dr);
                retval = (enum dw_state) ((reg & (0x1u << bit)) >> bit);
            }
            break;
        case Gpio_port_c:
            if((param->num_ports >= 3) && (bit < param->pwidth_c)) {
                reg = GPIO_INP(portmap->swportc_dr);
                retval = (enum dw_state) ((reg & (0x1u << bit)) >> bit);
            }
            break;
        case Gpio_port_d:
            if((param->num_ports == 4) && (bit < param->pwidth_d)) {
                reg = GPIO_INP(portmap->swportd_dr);
                retval = (enum dw_state) ((reg & (0x1u << bit)) >> bit);
            }
            break;
    }

    return retval;
}

/**********************************************************************/

int dw_gpio_setDirection(struct dw_device *dev, enum dw_gpio_port port,
        uint32_t data)
{
    int retval;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    DW_REQUIRE(port < param->num_ports);

    retval = -DW_ENOSYS;
    switch(port) {
        case Gpio_port_a:
            retval = 0;
            GPIO_OUTP(data, portmap->swporta_ddr);
            break;
        case Gpio_port_b:
            if(param->num_ports >= 2) {
                retval = 0;
                GPIO_OUTP(data, portmap->swportb_ddr);
            }
            break;
        case Gpio_port_c:
            if(param->num_ports >= 3) {
                retval = 0;
                GPIO_OUTP(data, portmap->swportc_ddr);
            }
            break;
        case Gpio_port_d:
            if(param->num_ports == 4) {
                retval = 0;
                GPIO_OUTP(data, portmap->swportd_ddr);
            }
            break;
    }

    return retval;
}

/**********************************************************************/

uint32_t dw_gpio_getDirection(struct dw_device *dev, enum dw_gpio_port
        port)
{
    uint32_t retval;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    DW_REQUIRE(port < param->num_ports);

    retval = 0;
    switch(port) {
        case Gpio_port_a:
            retval = GPIO_INP(portmap->swporta_ddr);
            break;
        case Gpio_port_b:
            if(param->num_ports >= 2) {
                retval = GPIO_INP(portmap->swportb_ddr);
            }
            break;
        case Gpio_port_c:
            if(param->num_ports >= 3) {
                retval = GPIO_INP(portmap->swportc_ddr);
            }
            break;
        case Gpio_port_d:
            if(param->num_ports == 4) {
                retval = GPIO_INP(portmap->swportd_ddr);
            }
            break;
    }

    return retval;
}

/**********************************************************************/

int dw_gpio_setBitDirection(struct dw_device *dev, enum dw_gpio_port
        port, enum dw_gpio_data_direction direction, uint32_t
        bits)
{
    int retval;
    uint32_t data_in;
    uint32_t data_out;
    volatile uint32_t *ptr;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);
    DW_REQUIRE(direction != Gpio_no_direction);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    DW_REQUIRE(port < param->num_ports);

    retval = -DW_ENOSYS;
    switch(port) {
        case Gpio_port_a:
            retval = 0;
            ptr = &portmap->swporta_ddr;
            break;
        case Gpio_port_b:
            if(param->num_ports >= 2) {
                retval = 0;
                ptr = &portmap->swportb_ddr;
            }
            break;
        case Gpio_port_c:
            if(param->num_ports >= 3) {
                retval = 0;
                ptr = &portmap->swportc_ddr;
            }
            break;
        case Gpio_port_d:
            if(param->num_ports == 4) {
                retval = 0;
                ptr = &portmap->swportd_ddr;
            }
            break;
    }

    // if a legal port has been specified
    if(retval == 0) {
        data_in = GPIO_INP(*ptr);
        switch(direction) {
            case Gpio_input:
                data_out = data_in & ~bits;
                break;
            case Gpio_output:
                data_out = data_in | bits;
                break;
            default:
                retval = -DW_EINVAL;
                break;
        }
        // avoid bus write if possible
        if((data_in != data_out) && (retval == 0))
            GPIO_OUTP(data_out, *ptr);
    }

    return retval;
}

/**********************************************************************/

enum dw_gpio_data_direction dw_gpio_getBitDirection(struct dw_device
        *dev, enum dw_gpio_port port, enum dw_gpio_bit bit)
{
    enum dw_gpio_data_direction retval;
    uint32_t reg;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    DW_REQUIRE(port < param->num_ports);

    retval = Gpio_no_direction;
    switch(port) {
        case Gpio_port_a:
            if(bit < param->pwidth_a) {
                reg = GPIO_INP(portmap->swporta_ddr);
                retval = (enum dw_gpio_data_direction) ((reg & (0x1u <<
                            bit)) >> bit);
            }
            break;
        case Gpio_port_b:
            if((param->num_ports >= 2) && (bit < param->pwidth_b)) {
                reg = GPIO_INP(portmap->swportb_ddr);
                retval = (enum dw_gpio_data_direction) ((reg & (0x1u <<
                                bit)) >> bit);
            }
            break;
        case Gpio_port_c:
            if((param->num_ports >= 3) && (bit < param->pwidth_c)) {
                reg = GPIO_INP(portmap->swportc_ddr);
                retval = (enum dw_gpio_data_direction) ((reg & (0x1u <<
                                bit)) >> bit);
            }
            break;
        case Gpio_port_d:
            if((param->num_ports == 4) && (bit < param->pwidth_d)) {
                reg = GPIO_INP(portmap->swportd_ddr);
                retval = (enum dw_gpio_data_direction) ((reg & (0x1u <<
                                bit)) >> bit);
            }
            break;
    }

    return retval;
}

/**********************************************************************/

int dw_gpio_setSource(struct dw_device *dev, enum dw_gpio_port port,
        uint32_t data)
{
    int retval;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    DW_REQUIRE(port < param->num_ports);

    retval = -DW_ENOSYS;
    switch(port) {
        case Gpio_port_a:
            if(param->hw_porta == true) {
                retval = 0;
                GPIO_OUTP(data, portmap->porta_ctl);
            }
            break;
        case Gpio_port_b:
            if((param->num_ports >= 2) && (param->hw_portb == true)) {
                retval = 0;
                GPIO_OUTP(data, portmap->portb_ctl);
            }
            break;
        case Gpio_port_c:
            if((param->num_ports >= 3) && (param->hw_portc == true)) {
                retval = 0;
                GPIO_OUTP(data, portmap->portc_ctl);
            }
            break;
        case Gpio_port_d:
            if((param->num_ports == 4) && (param->hw_portd == true)) {
                retval = 0;
                GPIO_OUTP(data, portmap->portd_ctl);
            }
            break;
    }

    return retval;
}

/**********************************************************************/

uint32_t dw_gpio_getSource(struct dw_device *dev, enum dw_gpio_port
        port)
{
    uint32_t retval;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    DW_REQUIRE(port < param->num_ports);

    retval = 0x0;
    switch(port) {
        case Gpio_port_a:
            if(param->hw_porta == true) {
                retval = GPIO_INP(portmap->porta_ctl);
            }
            break;
        case Gpio_port_b:
            if((param->num_ports >= 2) && (param->hw_portb)) {
                retval = GPIO_INP(portmap->portb_ctl);
            }
            break;
        case Gpio_port_c:
            if((param->num_ports >= 3) && (param->hw_portc)) {
                retval = GPIO_INP(portmap->portc_ctl);
            }
            break;
        case Gpio_port_d:
            if((param->num_ports == 4) && (param->hw_portd)) {
                retval = GPIO_INP(portmap->portd_ctl);
            }
            break;
    }

    return retval;
}

/**********************************************************************/

int dw_gpio_setBitSource(struct dw_device *dev, enum dw_gpio_port port,
        enum dw_gpio_data_source source, uint32_t bits)
{
    int retval;
    uint32_t data_in;
    uint32_t data_out;
    volatile uint32_t *ptr;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);
    DW_REQUIRE(source != Gpio_no_source);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    DW_REQUIRE(port < param->num_ports);

    retval = -DW_ENOSYS;
    switch(port) {
        case Gpio_port_a:
            if((param->hw_porta == true)
                && (param->porta_single_ctl == true)) {
                retval = 0;
                ptr = &portmap->porta_ctl;
            }
            break;
        case Gpio_port_b:
            if((param->num_ports >= 2)
                    && (param->hw_portb == true)
                    && (param->portb_single_ctl == true)) {
                retval = 0;
                ptr = &portmap->portb_ctl;
            }
            break;
        case Gpio_port_c:
            if((param->num_ports >= 3)
                    && (param->hw_portc == true)
                    && (param->portc_single_ctl == true)) {
                retval = 0;
                ptr = &portmap->portc_ctl;
            }
            break;
        case Gpio_port_d:
            if((param->num_ports == 4)
                    && (param->hw_portd == true)
                    && (param->portd_single_ctl == true)) {
                retval = 0;
                ptr = &portmap->portd_ctl;
            }
            break;
    }

    // if a legal port has been specified
    if(retval == 0) {
        data_in = GPIO_INP(*ptr);
        switch(source) {
            case Gpio_software_mode:
                data_out = data_in & ~bits;
                break;
            case Gpio_hardware_mode:
                data_out = data_in | bits;
                break;
            default:
                retval = -DW_EINVAL;
                break;
        }
        // avoid bus write if possible
        if((data_in != data_out) && (retval == 0))
            GPIO_OUTP(data_out, *ptr);
    }

    return retval;
}

/**********************************************************************/

enum dw_gpio_data_source dw_gpio_getBitSource(struct dw_device *dev,
        enum dw_gpio_port port, enum dw_gpio_bit bit)
{
    enum dw_gpio_data_source retval;
    uint32_t reg = 0;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    DW_REQUIRE(port < param->num_ports);

    retval = Gpio_no_source;
    switch(port) {
        case Gpio_port_a:
            if((bit < param->pwidth_a) && (param->hw_porta == true)) {
                reg = GPIO_INP(portmap->porta_ctl);
                if(param->porta_single_ctl == true) {
                    retval = (enum dw_gpio_data_source) ((reg & (0x1u <<
                                bit)) >> bit);
                }
                else {
                    retval = (enum dw_gpio_data_source) (reg & 0x1);
                }
            }
            break;
        case Gpio_port_b:
            if((param->num_ports >= 2)
                    && (bit < param->pwidth_b)
                    && (param->hw_portb == true)) {
                reg = GPIO_INP(portmap->portb_ctl);
                if(param->portb_single_ctl == true) {
                    retval = (enum dw_gpio_data_source) ((reg & (0x1u <<
                                bit)) >> bit);
                }
                else {
                    retval = (enum dw_gpio_data_source) (reg & 0x1);
                }
            }
            break;
        case Gpio_port_c:
            if((param->num_ports >= 3)
                    && (bit < param->pwidth_c)
                    && (param->hw_portc == true)) {
                if(param->portc_single_ctl == true) {
                    reg = GPIO_INP(portmap->portc_ctl);
                    retval = (enum dw_gpio_data_source) ((reg & (0x1u <<
                                bit)) >> bit);
                }
                else {
                    retval = (enum dw_gpio_data_source) (reg & 0x1);
                }
            }
            break;
        case Gpio_port_d:
            if((param->num_ports == 4)
                    && (bit < param->pwidth_d)
                    && (param->hw_portd == true)) {
                reg = GPIO_INP(portmap->portd_ctl);
                if(param->portd_single_ctl == true) {
                    retval = (enum dw_gpio_data_source) ((reg & (0x1u <<
                                bit)) >> bit);
                }
                else {
                    retval = (enum dw_gpio_data_source) (reg & 0x1);
                }
            }
            break;
    }

    return retval;
}

/**********************************************************************/

int dw_gpio_enableIrq(struct dw_device *dev, uint32_t
        interrupts)
{
    int retval;
    uint32_t data_in;
    uint32_t data_out;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    if(param->porta_intr == true) {
        retval = 0;
        data_in = GPIO_INP(portmap->inten);
        data_out = data_in | interrupts;
        // avoid bus write if possible
        if(data_in != data_out)
            GPIO_OUTP(data_out, portmap->inten);
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

int dw_gpio_disableIrq(struct dw_device *dev, uint32_t
        interrupts)
{
    int retval;
    uint32_t data_in;
    uint32_t data_out;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    if(param->porta_intr == true) {
        retval = 0;
        data_in = GPIO_INP(portmap->inten);
        data_out = data_in & ~interrupts;
        // avoid bus write if possible
        if(data_in != data_out)
            GPIO_OUTP(data_out, portmap->inten);
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

bool dw_gpio_isIrqEnabled(struct dw_device *dev, enum dw_gpio_bit
        interrupt)
{
    bool retval;
    uint32_t reg;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    retval = false;
    if((param->porta_intr == true) && (interrupt < param->pwidth_a)) {
        reg = GPIO_INP(portmap->inten);
        if((reg & (0x1u << interrupt)) != 0x0)
            retval = true;
    }

    return retval;
}

/**********************************************************************/

uint32_t dw_gpio_getEnabledIrq(struct dw_device *dev)
{
    uint32_t retval;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    if(param->porta_intr == true) {
        retval = GPIO_INP(portmap->inten);
    }
    else
        retval = 0x0;

    return retval;
}

/**********************************************************************/

int dw_gpio_maskIrq(struct dw_device *dev, uint32_t
        interrupts)
{
    int retval;
    uint32_t data_in;
    uint32_t data_out;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    if(param->porta_intr == true) {
        retval = 0;
        data_in = GPIO_INP(portmap->intmask);
        data_out = data_in | interrupts;
        // avoid bus write if possible
        if(data_in != data_out)
            GPIO_OUTP(data_out, portmap->intmask);
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

int dw_gpio_unmaskIrq(struct dw_device *dev, uint32_t
        interrupts)
{
    int retval;
    uint32_t data_in;
    uint32_t data_out;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    if(param->porta_intr == true) {
        retval = 0;
        data_in = GPIO_INP(portmap->intmask);
        data_out = data_in & ~interrupts;
        // avoid bus write if possible
        if(data_in != data_out)
            GPIO_OUTP(data_out, portmap->intmask);
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

bool dw_gpio_isIrqMasked(struct dw_device *dev, enum dw_gpio_bit
        interrupt)
{
    bool retval;
    uint32_t reg;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    retval = false;
    if((param->porta_intr == true) && (interrupt < param->pwidth_a)) {
        reg = GPIO_INP(portmap->intmask);
        if((reg & (0x1u << interrupt)) != 0x0)
            retval = true;
    }

    return retval;
}

/**********************************************************************/

uint32_t dw_gpio_getIrqMask(struct dw_device *dev)
{
    uint32_t retval;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    if(param->porta_intr == true) {
        retval = GPIO_INP(portmap->intmask);
    }
    else
        retval = 0x0;

    return retval;
}

/**********************************************************************/

bool dw_gpio_isIrqActive(struct dw_device *dev, enum dw_gpio_bit
        interrupt)
{
    bool retval;
    uint32_t reg;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    retval = false;
    if((param->porta_intr == true) && (interrupt < param->pwidth_a)) {
        reg = GPIO_INP(portmap->intstatus);
        if((reg & (0x1u << interrupt)) != 0x0)
            retval = true;
    }

    return retval;
}

/**********************************************************************/

uint32_t dw_gpio_getActiveIrq(struct dw_device *dev)
{
    uint32_t retval;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    if(param->porta_intr == true) {
        retval = GPIO_INP(portmap->intstatus);
    }
    else
        retval = 0x0;

    return retval;
}

/**********************************************************************/

bool dw_gpio_isRawIrqActive(struct dw_device *dev, enum dw_gpio_bit
        interrupt)
{
    bool retval;
    uint32_t reg;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    DW_REQUIRE(interrupt < param->pwidth_a);

    retval = false;
    if((param->porta_intr == true) && (interrupt < param->pwidth_a)) {
        reg = GPIO_INP(portmap->rawintstatus);
        if((reg & (0x1u << interrupt)) != 0x0)
            retval = true;
    }

    return retval;
}

/**********************************************************************/

uint32_t dw_gpio_getRawIrq(struct dw_device *dev)
{
    uint32_t retval;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    if(param->porta_intr == true) {
        retval = GPIO_INP(portmap->rawintstatus);
    }
    else
        retval = 0x0;

    return retval;
}

/**********************************************************************/

int dw_gpio_clearIrq(struct dw_device *dev, uint32_t
        interrupts)
{
    int retval;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    if(param->porta_intr == true) {
        retval = 0;
        GPIO_OUTP(interrupts, portmap->porta_eoi);
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

int dw_gpio_setIrqType(struct dw_device *dev, enum dw_gpio_irq_type
        type, uint32_t interrupts)
{
    int retval;
    uint32_t data_in;
    uint32_t data_out;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);
    DW_REQUIRE(type != Gpio_no_type);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    if(param->porta_intr == true) {
        retval = 0;
        data_in = GPIO_INP(portmap->inttype_level);
        switch(type) {
            case Gpio_level_sensitive:
                data_out = data_in & ~interrupts;
                break;
            case Gpio_edge_sensitive:
                data_out = data_in | interrupts;
                break;
            default:
                retval = -DW_EINVAL;
                break;
        }
        // avoid bus write if possible
        if((data_in != data_out) && (retval == 0))
            GPIO_OUTP(data_out, portmap->inttype_level);
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

enum dw_gpio_irq_type dw_gpio_getIrqType(struct dw_device *dev, enum
        dw_gpio_bit interrupt)
{
    enum dw_gpio_irq_type retval;
    uint32_t reg;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    if((param->porta_intr == true) && (interrupt < param->pwidth_a)) {
        reg = GPIO_INP(portmap->inttype_level);
        retval = (enum dw_gpio_irq_type) ((reg & (0x1u << interrupt)) >>
                    interrupt);
    }
    else
        retval = Gpio_no_type;

    return retval;
}

/**********************************************************************/

int dw_gpio_setIrqPolarity(struct dw_device *dev, enum
        dw_gpio_irq_polarity polarity, uint32_t interrupts)
{
    int retval;
    uint32_t data_in;
    uint32_t data_out;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    if(param->porta_intr == true) {
        retval = 0;
        data_in = GPIO_INP(portmap->int_polarity);
        switch(polarity) {
            case Gpio_active_low:
                data_out = data_in & ~interrupts;
                break;
            case Gpio_active_high:
                data_out = data_in | interrupts;
                break;
            default:
                retval = -DW_EINVAL;
                break;
        }
        // avoid bus write if possible
        if((data_in != data_out) && (retval == 0))
            GPIO_OUTP(data_out, portmap->int_polarity);
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

enum dw_gpio_irq_polarity dw_gpio_getIrqPolarity(struct dw_device *dev,
        enum dw_gpio_bit interrupt)
{
    enum dw_gpio_irq_polarity retval;
    uint32_t reg;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    if((param->porta_intr == true) && (interrupt < param->pwidth_a)) {
        reg = GPIO_INP(portmap->int_polarity);
        retval = (enum dw_gpio_irq_polarity) ((reg & (0x1u <<
                        interrupt)) >> interrupt);
    }
    else
        retval = Gpio_no_polarity;

    return retval;
}

/**********************************************************************/

int dw_gpio_enableDebounce(struct dw_device *dev, uint32_t
        interrupts)
{
    int retval;
    uint32_t data_in;
    uint32_t data_out;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    if((param->porta_intr == true) && (param->debounce == true)) {
        retval = 0;
        data_in = GPIO_INP(portmap->debounce);
        data_out = data_in | interrupts;
        // avoid bus write if possible
        if(data_in != data_out)
            GPIO_OUTP(data_out, portmap->debounce);
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

int dw_gpio_disableDebounce(struct dw_device *dev, uint32_t interrupts)
{
    int retval;
    uint32_t data_in;
    uint32_t data_out;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    if((param->porta_intr == true) && (param->debounce == true)) {
        retval = 0;
        data_in = GPIO_INP(portmap->debounce);
        data_out = data_in & ~interrupts;
        // avoid bus write if possible
        if(data_in != data_out)
            GPIO_OUTP(data_out, portmap->debounce);
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

bool dw_gpio_isDebounceEnabled(struct dw_device *dev, enum dw_gpio_bit
        interrupt)
{
    bool retval;
    uint32_t reg;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    retval = false;
    if((param->porta_intr == true) && (param->debounce == true) &&
            (interrupt < param->pwidth_a)) {
        reg = GPIO_INP(portmap->debounce);
        if((reg & (0x1u << interrupt)) != 0x0)
            retval = true;
    }

    return retval;
}

/**********************************************************************/

int dw_gpio_enableSync(struct dw_device *dev)
{
    int retval;
    uint32_t reg;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    if(param->porta_intr == true) {
        retval = 0;
        reg = 0x0;
        DW_BIT_SET(reg, GPIO_LS_SYNC_SYNCLEVEL, 0x1);
        GPIO_OUTP(reg, portmap->ls_sync);
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

int dw_gpio_disableSync(struct dw_device *dev)
{
    int retval;
    uint32_t reg;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    if(param->porta_intr == true) {
        retval = 0;
        reg = 0x0;
        DW_BIT_SET(reg, GPIO_LS_SYNC_SYNCLEVEL, 0x0);
        GPIO_OUTP(reg, portmap->ls_sync);
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

bool dw_gpio_isSyncEnabled(struct dw_device *dev)
{
    bool retval;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    retval = false;
    if(param->porta_intr == true) {
        if(GPIO_INP(portmap->ls_sync) != 0x0)
            retval = true;
    }

    return retval;
}

/**********************************************************************/

int dw_gpio_setListener(struct dw_device *dev, dw_callback
        userFunction)
{
    int retval;
    struct dw_gpio_param *param;
    //struct dw_gpio_portmap *portmap;
    struct dw_gpio_instance *instance;

    GPIO_COMMON_REQUIREMENTS(dev);
    DW_REQUIRE(userFunction != NULL);
    DW_REQUIRE(dev->instance != NULL);

    param = (struct dw_gpio_param *) dev->comp_param;
    //portmap = (struct dw_gpio_portmap *) dev->base_address;
    instance = (struct dw_gpio_instance *) dev->instance;

    if(param->porta_intr == true) {
        retval = 0;
        instance->listener = userFunction;
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/

int dw_gpio_irqHandler(struct dw_device *dev)
{
    int i;
    int retval;
    uint32_t reg;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;
    struct dw_gpio_instance *instance;
    dw_callback userListener;

    GPIO_COMMON_REQUIREMENTS(dev);
    DW_REQUIRE(dev->instance != NULL);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;
    instance = (struct dw_gpio_instance *) dev->instance;

    // Assume no interrupt will be processed.  This will be set to true
    // if no active interrupt is found.
    retval = false;

    userListener = instance->listener;

    reg = GPIO_INP(portmap->intstatus);
    for(i = 0; i < param->pwidth_a; i++) {
        if((reg & 0x1) != 0x0) {
            retval = true;
            userListener(dev, i);
        }
        // shift reg one bit to the right
        reg >>= 1;
    }

    return retval;
}

/**********************************************************************/
/***                    PRIVATE FUNCTIONS                           ***/
/**********************************************************************/

int dw_gpio_autoCompParams(struct dw_device *dev)
{
    int retval;
    unsigned data_width;
    uint32_t comp_params_1;
    uint32_t comp_params_2;
    struct dw_gpio_param *param;
    struct dw_gpio_portmap *portmap;

    GPIO_COMMON_REQUIREMENTS(dev);

    param = (struct dw_gpio_param *) dev->comp_param;
    portmap = (struct dw_gpio_portmap *) dev->base_address;

    dev->comp_version = DW_INP(portmap->comp_version);

    // only version 2.04 and greater support identification registers
    if((GPIO_INP(portmap->comp_type) == Dw_apb_gpio) &&
            (dev->comp_version >= 0x3230342a)) {
        retval = 0;
        comp_params_1 = DW_INP(portmap->comp_params_1);
        comp_params_2 = DW_INP(portmap->comp_params_2);

        data_width = DW_BIT_GET(comp_params_1,
                GPIO_PARAMS_1_APB_DATA_WIDTH);
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

        param->debounce = DW_BIT_GET(comp_params_1,
                GPIO_PARAMS_1_DEBOUNCE);
        param->hw_porta = DW_BIT_GET(comp_params_1,
                GPIO_PARAMS_1_HW_PORTA);
        param->hw_portb = DW_BIT_GET(comp_params_1,
                GPIO_PARAMS_1_HW_PORTB);
        param->hw_portc = DW_BIT_GET(comp_params_1,
                GPIO_PARAMS_1_HW_PORTC);
        param->hw_portd = DW_BIT_GET(comp_params_1,
                GPIO_PARAMS_1_HW_PORTD);
        param->id = DW_BIT_GET(comp_params_1, GPIO_PARAMS_1_ID);
        param->porta_intr = DW_BIT_GET(comp_params_1,
                GPIO_PARAMS_1_PORTA_INTR);
        param->porta_single_ctl = DW_BIT_GET(comp_params_1,
                GPIO_PARAMS_1_PORTA_SINGLE_CTL);
        param->portb_single_ctl = DW_BIT_GET(comp_params_1,
                GPIO_PARAMS_1_PORTB_SINGLE_CTL);
        param->portc_single_ctl = DW_BIT_GET(comp_params_1,
                GPIO_PARAMS_1_PORTC_SINGLE_CTL);
        param->portd_single_ctl = DW_BIT_GET(comp_params_1,
                GPIO_PARAMS_1_PORTD_SINGLE_CTL);
        param->id_width = (uint8_t) (DW_BIT_GET(comp_params_1,
                GPIO_PARAMS_1_ID_WIDTH) + 0x1u);
        param->num_ports = (uint8_t) (DW_BIT_GET(comp_params_1,
                GPIO_PARAMS_1_NUM_PORTS) + 0x1u);
        param->pwidth_a = (uint8_t) (DW_BIT_GET(comp_params_2,
                GPIO_PARAMS_2_PWIDTH_A) + 0x1u);
        param->pwidth_b = (uint8_t) (DW_BIT_GET(comp_params_2,
                GPIO_PARAMS_2_PWIDTH_B) + 0x1u);
        param->pwidth_c = (uint8_t) (DW_BIT_GET(comp_params_2,
                GPIO_PARAMS_2_PWIDTH_C) + 0x1u);
        param->pwidth_d = (uint8_t) (DW_BIT_GET(comp_params_2,
                GPIO_PARAMS_2_PWIDTH_D) + 0x1u);
    }
    else
        retval = -DW_ENOSYS;

    return retval;
}

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/

