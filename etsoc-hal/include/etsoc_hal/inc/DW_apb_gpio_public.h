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

#ifndef DW_APB_GPIO_PUBLIC_H
#define DW_APB_GPIO_PUBLIC_H

#ifdef __cplusplus
extern "C" {    // allow C++ to use these headers
#endif

/****h* drivers.gpio/gpio.api
 * NAME
 *  DW_apb_gpio Driver Kit API overview
 * DESCRIPTION
 *  This section gives an overview of the DW_apb_gpio software Driver
 *  Kit Application Programming Interface (API).
 * SEE ALSO
 *  gpio.data, gpio.functions
 ***/

/****h* drivers.gpio/gpio.data
 * NAME
 *  DW_apb_gpio Driver Kit data types and definitions
 * DESCRIPTION
 *  This section details all the public data types and definitions used
 *  with the DW_apb_gpio software Driver Kit.
 * SEE ALSO
 *  gpio.api, gpio.functions
 ***/

/****h* drivers.gpio/gpio.functions
 * NAME
 *  DW_apb_gpio API functions
 * DESCRIPTION
 *  This section details all the public functions available for use with
 *  the DW_apb_gpio software Driver Kit.  The API is split into four
 *  categories:
 *
 *  - Command           -- functions which cause some functional
 *                         behaviour to occur with DW_apb_gpio (e.g.
 *                         setting the power mode)
 *  - Configuration     -- functions which are used to configure various
 *                         settings within DW_apb_gpio (e.g. setting up
 *                         the various clock cycle delays)
 *  - Status            -- functions which return status information
 *                         about DW_apb_gpio (e.g. getting the data
 *                         source or direction of a port bit)
 *  - Interrupt         -- functions which relate to interrupt-driven
 *                         operations (e.g. setting a listener function)
 *
 * SEE ALSO
 *  gpio.api, gpio.data
 ***/

/****h* gpio.api/gpio.data_types
 * NAME
 *  DW_apb_gpio Driver Kit data types and definitions
 * DESCRIPTION
 *  enum dw_gpio_bit            -- a single bit of a gpio port
 *  enum dw_gpio_port           -- gpio port (a, b, c or d)
 *  enum dw_gpio_irq_type       -- interrupt type (level/edge-sensitive)
 *  enum dw_gpio_irq_polarity   -- interrupt polarity (active-low/high)
 *  enum dw_gpio_data_source    -- the data source for port bits
 *  enum dw_gpio_data_direction -- the data direction for port bits
 *
 * SEE ALSO
 *  gpio.configuration, gpio.command, gpio.status, gpio.interrupt
 ***/

/****h* gpio.api/gpio.configuration
 * NAME
 *  DW_apb_gpio Driver Kit configuration functions
 * DESCRIPTION
 *  dw_gpio_enableIrq       -- enable an interrupt
 *  dw_gpio_disableIrq      -- disable an interrupt
 *  dw_gpio_maskIrq         -- mask an interrupt
 *  dw_gpio_unmaskIrq       -- unmask an interrupt
 *  dw_gpio_setIrqType      -- set the interrupt type
 *  dw_gpio_setIrqPolarity  -- set the interrupt polarity
 *  dw_gpio_enableDebounce  -- enable interrupt debouncing
 *  dw_gpio_disableDebounce -- disable interrupt debouncing
 *  dw_gpio_enableSync      -- enable interrupt synchronization
 *  dw_gpio_disableSync     -- disable interrupt synchronization
 *
 * SEE ALSO
 *  gpio.data_types, gpio.command, gpio.status, gpio.interrupt
 ***/

/****h* gpio.api/gpio.command
 * NAME
 *  DW_apb_gpio Driver Kit command functions
 * DESCRIPTION
 *  dw_gpio_init                -- initialize the device driver
 *  dw_gpio_setPort             -- set the data value of a port
 *  dw_gpio_setBit              -- set the value of one or more port
 *                                 bits
 *  dw_gpio_setDirection        -- set the data direction of a port
 *  dw_gpio_setBitDirection     -- set the data direction of one or more
 *                                 port bits
 *  dw_gpio_setSource           -- set the data source of a port
 *  dw_gpio_setBitSource        -- set the data source of one or more
 *                                 port bits
 *  dw_gpio_clearIrq            -- clear an edge-sensitive interrupt
 * SEE ALSO
 *  gpio.data_types, gpio.configuration, gpio.status, gpio.interrupt
 ***/

/****h* gpio.api/gpio.status
 * NAME
 *  DW_apb_gpio Driver Kit status functions
 * DESCRIPTION
 *  dw_gpio_getPort             -- get the data value of a port
 *  dw_gpio_getBit              -- get the value of a port bit
 *  dw_gpio_getDirection        -- get the data direction of a port
 *  dw_gpio_getBitDirection     -- get the direction of a port bit
 *  dw_gpio_getSource           -- get the data source of a port
 *  dw_gpio_getBitSource        -- get the source of a port bit
 *  dw_gpio_isIrqEnabled        -- is interrupt enable?
 *  dw_gpio_getEnabledIrq       -- get the data value of the interrupt
 *                                 enable register
 *  dw_gpio_isIrqMasked         -- is interrupt masked?
 *  dw_gpio_getIrqMask          -- get the data value of the interrupt
 *                                 mask register
 *  dw_gpio_getIrqType          -- get interrupt type
 *  dw_gpio_getIrqPolarity      -- get interrupt polarity
 *  dw_gpio_isIrqActive         -- is interrupt active?
 *  dw_gpio_isRawIrqActive      -- is raw interrupt active?
 *  dw_gpio_getRawIrq           -- get the data value of the raw
 *                                 interrupt status register
 *  dw_gpio_isDebounceEnabled   -- is interrupt debounce enabled?
 *  dw_gpio_getExtPort          -- get external port data register value
 *  dw_gpio_isSyncEnabled       -- is interrupt synchronization enabled?
 *  dw_gpio_getIdCode           -- get the ID code of DW_apb_gpio
 *  dw_gpio_getRtlVersion       -- get the RTL version of DW_apb_gpio
 *  dw_gpio_getPortWidth        -- get the width of a specified port
 *  dw_gpio_getNumPorts         -- get the number of ports available
 *
 * SEE ALSO
 *  gpio.data_types, gpio.configuration, gpio.command, gpio.interrupt
 ***/

/****h* gpio.api/gpio.interrupt
 * NAME
 *  DW_apb_gpio Driver Kit interrupt functions
 * DESCRIPTION
 *  dw_gpio_setListener     -- set the user listener function
 *  dw_gpio_irqHandler      -- process DW_apb_gpio interrupt(s)
 *
 * SEE ALSO
 *  gpio.data_types, gpio.configuration, gpio.command, gpio.status
 ***/

/****d* gpio.data/dw_gpio_port
 * DESCRIPTION
 *  This is the data type used for specifying a gpio port.  Most
 *  functions take this as one of their arguments.  Some obvious
 *  exceptions are interrupt-related functions which all work on port A.
 * SOURCE
 */
enum dw_gpio_port {
    Gpio_port_a = 0x0,
    Gpio_port_b = 0x1,
    Gpio_port_c = 0x2,
    Gpio_port_d = 0x3
};
/*****/

/****d* gpio.data/dw_gpio_bit
 * DESCRIPTION
 *  This is the data type used for specifying a single bit (of a gpio
 *  port).  Functions which utilise this data type allow only a single
 *  bit of a port to be specified pre invocation.
 * SEE ALSO
 *  dw_gpio_getBit, dw_gpio_getBitDirection, dw_gpio_getBitSource,
 *  dw_gpio_isIrqEnabled, dw_gpio_isIrqMasked, dw_gpio_isIrqActive,
 *  dw_gpio_isRawIrqActive, dw_gpio_getIrqType,
 *  dw_gpio_isDebounceEnabled
 * SOURCE
 */
enum dw_gpio_bit {
    Gpio_bit_0 = 0,
    Gpio_bit_1 = 1,
    Gpio_bit_2 = 2,
    Gpio_bit_3 = 3,
    Gpio_bit_4 = 4,
    Gpio_bit_5 = 5,
    Gpio_bit_6 = 6,
    Gpio_bit_7 = 7,
    Gpio_bit_8 = 8,
    Gpio_bit_9 = 9,
    Gpio_bit_10 = 10,
    Gpio_bit_11 = 11,
    Gpio_bit_12 = 12,
    Gpio_bit_13 = 13,
    Gpio_bit_14 = 14,
    Gpio_bit_15 = 15,
    Gpio_bit_16 = 16,
    Gpio_bit_17 = 17,
    Gpio_bit_18 = 18,
    Gpio_bit_19 = 19,
    Gpio_bit_20 = 20,
    Gpio_bit_21 = 21,
    Gpio_bit_22 = 22,
    Gpio_bit_23 = 23,
    Gpio_bit_24 = 24,
    Gpio_bit_25 = 25,
    Gpio_bit_26 = 26,
    Gpio_bit_27 = 27,
    Gpio_bit_28 = 28,
    Gpio_bit_29 = 29,
    Gpio_bit_30 = 30,
    Gpio_bit_31 = 31
};
/*****/

/****d* gpio.data/dw_gpio_data_source
 * DESCRIPTION
 *  This is the data type used for specifying the data source.
 * NOTES
 *  This data type relates to the following register bit-field(s):
 *   - gpio_porta_ctl/all bits
 *   - gpio_portb_ctl/all bits
 *   - gpio_portc_ctl/all bits
 *   - gpio_portd_ctl/all bits
 * SEE ALSO
 *  dw_gpio_setSource, dw_gpio_getSource, dw_gpio_setBitSource,
 *  dw_gpio_getBitSource
 * SOURCE
 */
enum dw_gpio_data_source {
    Gpio_no_source = -1,
    Gpio_software_mode = 0x0,
    Gpio_hardware_mode = 0x1
};
/*****/

/****d* gpio.data/dw_gpio_data_direction
 * DESCRIPTION
 *  This is the data type used for specifying the data direction.
 * NOTES
 *  This data type relates to the following register bit-field(s):
 *   - gpio_swporta_ddr/all bits
 *   - gpio_swportb_ddr/all bits
 *   - gpio_swportc_ddr/all bits
 *   - gpio_swportd_ddr/all bits
 * SEE ALSO
 *  dw_gpio_setDirection, dw_gpio_getDirection, dw_gpio_setBitDirection,
 *  dw_gpio_getBitDirection
 * SOURCE
 */
enum dw_gpio_data_direction {
    Gpio_no_direction = -1,
    Gpio_input = 0x0,
    Gpio_output = 0x1
};
/*****/

/****d* gpio.data/dw_gpio_irq_type
 * DESCRIPTION
 *  This is the data type used for specifying the interrupt type.
 * NOTES
 *  This data type relates to the following register bit-field(s):
 *   - gpio_inttype_level/all bits
 * SEE ALSO
 *  dw_gpio_setIrqType, dw_gpio_getIrqType
 * SOURCE
 */
enum dw_gpio_irq_type {
    Gpio_no_type = -1,
    Gpio_level_sensitive = 0x0,
    Gpio_edge_sensitive = 0x1
};
/*****/

/****d* gpio.data/dw_gpio_irq_polarity
 * DESCRIPTION
 *  This is the data type used for specifying the interrupt polarity.
 * NOTES
 *  This data type relates to the following register bit-field(s):
 *   - gpio_int_polarity/all bits
 * SEE ALSO
 *  dw_gpio_setIrqPolarity, dw_gpio_getIrqPolarity
 * SOURCE
 */
enum dw_gpio_irq_polarity {
    Gpio_no_polarity = -1,
    Gpio_active_low = 0x0,
    Gpio_falling_edge = 0x0,
    Gpio_active_high = 0x1,
    Gpio_rising_edge = 0x1
};
/*****/

/****f* gpio.functions/dw_gpio_init
 * DESCRIPTION
 *  This function initializes the DW_apb_gpio Driver Kit.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 * RETURN VALUE
 *  0           -- if successful (RTL version >= 2.04)
 *  -DW_ENOSYS  -- if hardware parameters for the device could not be
 *                 automatically determined (RTL version < 2.04)
 * NOTES
 *  This function is affected by the RTL version.  If this is less than
 *  2.04, it is necessary for the user to create an appropriate
 *  dw_gpio_param structure as part of the dw_device structure.  If
 *  the RTL version is 2.04 or greater, dw_gpio_init() will
 *  automatically initialize this structure (space for which must have
 *  been already allocated).
 *
 *  FUTURE:  (Raijko don't think this is necessary):
 *  If DW_apb_gpio is independently reset, the Driver Kit must be
 *  re-initialized by calling this function, in order to keep the device
 *  and driver in sync.
 * SOURCE
 */
int dw_gpio_init(struct dw_device *dev);
/*****/

/****f* gpio.functions/dw_gpio_getIdCode
 * DESCRIPTION
 *  This function returns the ID code of a DW_apb_gpio.  This is a
 *  read-only user-defined value in the memory map, which is set when
 *  the DW_apb_gpio is generated.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 * RETURN VALUE
 *  The ID code of DW_apb_gpio.
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_id_code
 *
 *  This function is affected by the GPIO_ID_NUM hardware parameter.
 * SOURCE
 */
uint32_t dw_gpio_getIdCode(struct dw_device *dev);
/*****/

/****f* gpio.functions/dw_gpio_getRtlVersion
 * DESCRIPTION
 *  This function returns the RTL version of the DW_apb_gpio.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 * RETURN VALUE
 *  The RTL version of DW_apb_gpio.
 * SOURCE
 */
uint32_t dw_gpio_getRtlVersion(struct dw_device *dev);
/*****/

/****f* gpio.functions/dw_gpio_getNumPorts
 * DESCRIPTION
 *  This function returns the number of ports present in DW_apb_gpio.
 *  There can be between one and four ports available.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 * RETURN VALUE
 *  The number of ports in DW_apb_gpio.
 * NOTES
 *  This function is affected by the GPIO_NUM_PORTS hardware parameter.
 * SOURCE
 */
unsigned dw_gpio_getNumPorts(struct dw_device *dev);
/*****/

/****f* gpio.functions/dw_gpio_getPortWidth
 * DESCRIPTION
 *  This function returns the width of a specified port.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 *  port        -- relevant gpio port
 * RETURN VALUE
 *  0           -- if the specified port is not available
 *  Otherwise, the relevant port width is returned.
 * NOTES
 *  This function is affected by the GPIO_PWIDTH_A, GPIO_PWIDTH_B,
 *  GPIO_PWIDTH_C, GPIO_PWIDTH_D and GPIO_NUM_PORTS hardware parameters.
 * SOURCE
 */
int dw_gpio_getPortWidth(struct dw_device *dev, enum dw_gpio_port port);
/*****/

/****f* gpio.functions/dw_gpio_setPort
 * DESCRIPTION
 *  This function sets the data register of a specified port.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 *  port        -- relevant gpio port
 *  data        -- value to write to the port data register
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- if the specified port is not available
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_swporta_dr
 *   - gpio_swportb_dr
 *   - gpio_swportc_dr
 *   - gpio_swportd_dr
 *
 *  This function is affected by the GPIO_NUM_PORTS hardware parameter.
 * SEE ALSO
 *  dw_gpio_getPort, dw_gpio_setBit
 * SOURCE
 */
int dw_gpio_setPort(struct dw_device *dev, enum dw_gpio_port port,
        uint32_t data);
/*****/

/****f* gpio.functions/dw_gpio_getPort
 * DESCRIPTION
 *  This function returns the data register value of a specified port.
 *  A value of '0' is returned when the specified port is not available.
 *  Note, though, that this could also be a legitimate return value of a
 *  port data register.  Because of this, one should call
 *  dw_gpio_getNumPorts() to verify that the desired gpio port is
 *  available before specifying it as an argument to this function.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 *  port        -- relevant gpio port
 * RETURN VALUE
 *  The port data register value.
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_swporta_dr
 *   - gpio_swportb_dr
 *   - gpio_swportc_dr
 *   - gpio_swportd_dr
 *
 *  This function is affected by the GPIO_NUM_PORTS hardware parameter.
 * SEE ALSO
 *  dw_gpio_setPort, dw_gpio_setBit, dw_gpio_getBit
 * SOURCE
 */
uint32_t dw_gpio_getPort(struct dw_device *dev, enum dw_gpio_port port);
/*****/

/****f* gpio.functions/dw_gpio_getExtPort
 * DESCRIPTION
 *  This function returns the value of the specified external port data
 *  register.  A value of '0' is returned when the specified port is not
 *  available.  Note, though, that this could also be a legitimate
 *  return value of an external port data register.  Because of this,
 *  one should call dw_gpio_getNumPorts() to verify that the desired
 *  gpio port is available before specifying it as an argument to
 *  this function.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 *  port        -- relevant gpio port
 * RETURN VALUE
 *  The external port register value.
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_ext_porta
 *   - gpio_ext_portb
 *   - gpio_ext_portc
 *   - gpio_ext_portd
 *
 *  This function is affected by the GPIO_NUM_PORTS hardware parameter.
 * SEE ALSO
 *  dw_gpio_getPort, dw_gpio_getDirection
 * SOURCE
 */
uint32_t dw_gpio_getExtPort(struct dw_device *dev, enum dw_gpio_port
        port);
/*****/

/****f* gpio.functions/dw_gpio_setBit
 * DESCRIPTION
 *  This function sets the specified bits of a port data register to the
 *  value specified.  Multiple bits of a single port can be specified in
 *  the function arguments, using the bitwise 'OR' operator.  The
 *  allowable values that a port bit can be set to are Dw_set and
 *  Dw_clear.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 *  port        -- relevant gpio port
 *  value       -- value to set (Dw_set or Dw_clear)
 *  bits        -- port bits to change
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- if the specified port is not available
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_swporta_dr
 *   - gpio_swportb_dr
 *   - gpio_swportc_dr
 *   - gpio_swportd_dr
 *
 *  This function is affected by the GPIO_NUM_PORTS hardware parameter.
 * SEE ALSO
 *  dw_gpio_getBit, dw_gpio_getPort
 * SOURCE
 */
int dw_gpio_setBit(struct dw_device *dev, enum dw_gpio_port port,
        enum dw_state value, uint32_t bits);
/*****/

/****f* gpio.functions/dw_gpio_getBit
 * DESCRIPTION
 *  This function returns the value of a port data register bit.  Only
 *  one bit may be specified per invocation of this function.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 *  port        -- relevant gpio port
 *  bit         -- bit to return the value of
 * RETURN VALUE
 *  Dw_set      -- if the specified port bit is set (0x1)
 *  Dw_clear    -- if the specified port bit is clear (0x0)
 *  Dw_err      -- if the specified port bit is not available
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_swporta_dr
 *   - gpio_swportb_dr
 *   - gpio_swportc_dr
 *   - gpio_swportd_dr
 *
 *  This function is affected by the GPIO_NUM_PORTS, GPIO_PWIDTH_A,
 *  GPIO_PWIDTH_B, GPIO_PWIDTH_C and GPIO_PWIDTH_D hardware parameters.
 * SEE ALSO
 *  dw_gpio_setBit, dw_gpio_setPort
 * SOURCE
 */
enum dw_state dw_gpio_getBit(struct dw_device *dev, enum
        dw_gpio_port port, enum dw_gpio_bit bit);
/*****/

/****f* gpio.functions/dw_gpio_setDirection
 * DESCRIPTION
 *  This function sets the data direction register of the specified
 *  port.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 *  port        -- relevant gpio port
 *  data        -- value to write to the port data direction register
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- if the specified port is not available
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_swporta_ddr
 *   - gpio_swportb_ddr
 *   - gpio_swportc_ddr
 *   - gpio_swportd_ddr
 *
 *  This function is affected by the GPIO_NUM_PORTS hardware parameter.
 * SEE ALSO
 *  dw_gpio_getDirection, dw_gpio_setBitDirection
 * SOURCE
 */
int dw_gpio_setDirection(struct dw_device *dev, enum dw_gpio_port port,
        uint32_t data);
/*****/

/****f* gpio.functions/dw_gpio_getDirection
 * DESCRIPTION
 *  This function returns the value of the specified port data direction
 *  register.  A value of '0' is returned when the specified port is not
 *  available.  Note, though, that this could also be a legitimate
 *  return value of a port data direction register.  Because of this,
 *  one should call dw_gpio_getNumPorts() to verify that the desired
 *  gpio port is available before specifying it as an argument to
 *  this function.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 *  port        -- relevant gpio port
 * RETURN VALUE
 *  The port data direction register value.
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_swporta_ddr
 *   - gpio_swportb_ddr
 *   - gpio_swportc_ddr
 *   - gpio_swportd_ddr
 *
 *  This function is affected by the GPIO_NUM_PORTS hardware parameter.
 * SEE ALSO
 *  dw_gpio_setDirection, dw_gpio_getBitDirection
 * SOURCE
 */
uint32_t dw_gpio_getDirection(struct dw_device *dev, enum dw_gpio_port
        port);
/*****/

/****f* gpio.functions/dw_gpio_setBitDirection
 * DESCRIPTION
 *  This function sets the specified bits of a port data direction
 *  register to the value specified.  Multiple bits of a single port can
 *  be specified in the function arguments, using the bitwise 'OR'
 *  operator.  The allowable values that a port data direction bit can
 *  be set to are Gpio_input and Gpio_output.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 *  port        -- relevant gpio port
 *  direction   -- direction to set port bit(s)
 *  bits        -- port bit(s) to set as input/output
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- if the specified port is not available
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_swporta_ddr
 *   - gpio_swportb_ddr
 *   - gpio_swportc_ddr
 *   - gpio_swportd_ddr
 *
 *  This function is affected by the GPIO_NUM_PORTS hardware parameter.
 * SEE ALSO
 *  dw_gpio_getBitDirection, dw_gpio_setDirection
 * SOURCE
 */
int dw_gpio_setBitDirection(struct dw_device *dev, enum dw_gpio_port
        port, enum dw_gpio_data_direction direction, uint32_t bits);
/*****/

/****f* gpio.functions/dw_gpio_getBitDirection
 * DESCRIPTION
 *  This function returns the value of a port data direction bit.  Only
 *  one bit may be specified per invocation of this function.
 * ARGUMENTS
 *  dev                 -- DW_apb_gpio device handle
 *  port                -- relevant gpio port
 *  bit                 -- port bit to check
 * RETURN VALUE
 *  Gpio_input          -- if the port bit is an input
 *  Gpio_output         -- if the port bit is an output
 *  Gpio_no_direction   -- if the specified port bit is not available
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_swporta_ddr
 *   - gpio_swportb_ddr
 *   - gpio_swportc_ddr
 *   - gpio_swportd_ddr
 *
 *  This function is affected by the GPIO_NUM_PORTS, GPIO_PWIDTH_A,
 *  GPIO_PWIDTH_B, GPIO_PWIDTH_C and GPIO_PWIDTH_D hardware parameters.
 * SEE ALSO
 *  dw_gpio_setBitDirection, dw_gpio_getDirection
 * SOURCE
 */
enum dw_gpio_data_direction dw_gpio_getBitDirection(struct dw_device
        *dev, enum dw_gpio_port port, enum dw_gpio_bit bit);
/*****/

/****f* gpio.functions/dw_gpio_setSource
 * DESCRIPTION
 *  This function sets the data source register of the specified port.
 *
 *  This function is not available if the corresponding GPIO_HW_PORT*
 *  hardware parameter is set to false.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 *  port        -- relevant gpio port
 *  data        -- value to write to the port data source register
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- if the specified port is not available
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_porta_ctl
 *   - gpio_portb_ctl
 *   - gpio_portc_ctl
 *   - gpio_portd_ctl
 *
 *  This function is affected by the GPIO_NUM_PORTS, GPIO_HW_PORTA,
 *  GPIO_HW_PORTB, GPIO_HW_PORTC and GPIO_HW_PORTD hardware parameters.
 * SEE ALSO
 *  dw_gpio_getSource, dw_gpio_setBitSource
 * SOURCE
 */
int dw_gpio_setSource(struct dw_device *dev, enum dw_gpio_port port,
        uint32_t data);
/*****/

/****f* gpio.functions/dw_gpio_getSource
 * DESCRIPTION
 *  This function returns the value of a specified port data source
 *  register.  A value of '0' is returned when the specified port is not
 *  available.  Note, though, that this could also be a legitimate
 *  return value of a port data source register.  Because of this, one
 *  should call dw_gpio_getNumPorts() to verify that the desired gpio
 *  port is available before specifying it as an argument to this
 *  function.
 *
 *  The data source register is not available if the corresponding
 *  GPIO_HW_PORT* hardware parameter is set to false.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 *  port        -- relevant gpio port
 * RETURN VALUE
 *  The port data source register value.
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_porta_ctl
 *   - gpio_portb_ctl
 *   - gpio_portc_ctl
 *   - gpio_portd_ctl
 *
 *  This function is affected by the GPIO_NUM_PORTS, GPIO_HW_PORTA,
 *  GPIO_HW_PORTB, GPIO_HW_PORTC and GPIO_HW_PORTD hardware parameters.
 * SEE ALSO
 *  dw_gpio_setSource, dw_gpio_getBitSource
 * SOURCE
 */
uint32_t dw_gpio_getSource(struct dw_device *dev, enum dw_gpio_port
        port);
/*****/

/****f* gpio.functions/dw_gpio_setBitSource
 * DESCRIPTION
 *  This function sets the specified bit(s) of a port data source
 *  register to the value specified.  Multiple bits of a single port can
 *  be specified in the function arguments, using the bitwise 'OR'
 *  operator.  The allowable values that a port data source bit can be
 *  set to are Gpio_software and Gpio_hardware.
 *
 *  This function can only be used if the corresponding
 *  GPIO_PORT*_SINGLE_CTL hardware parameter is set to true.  Otherwise,
 *  port data source bits are not individually controllable and this
 *  function will return an error.
 *
 *  The data source register is not available if the corresponding
 *  GPIO_HW_PORT* hardware parameter is set to false.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 *  port        -- relevant gpio port
 *  source      -- data source to set port bit(s) to (hardware/software)
 *  bits        -- port bit(s) to set
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- if the specified port is unavailable or the port
 *                 data source register is either not individually
 *                 controllable or not available
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_porta_ctl
 *   - gpio_portb_ctl
 *   - gpio_portc_ctl
 *   - gpio_portd_ctl
 *
 *  This function is affected by the GPIO_NUM_PORTS,
 *  GPIO_PORTA_SINGLE_CTL, GPIO_PORTB_SINGLE_CTL, GPIO_PORTC_SINGLE_CTL,
 *  GPIO_PORTD_SINGLE_CTL, GPIO_HW_PORTA, GPIO_HW_PORTB, GPIO_HW_PORTC
 *  and GPIO_HW_PORTD hardware parameters.
 * SEE ALSO
 *  dw_gpio_getBitSource, dw_gpio_setSource
 * SOURCE
 */
int dw_gpio_setBitSource(struct dw_device *dev, enum dw_gpio_port port,
        enum dw_gpio_data_source source, uint32_t bits);
/*****/

/****f* gpio.functions/dw_gpio_getBitSource
 * DESCRIPTION
 *  This function returns the data source for the specified port
 *  register bit.  Only one bit may be specified per invocation of this
 *  function.
 *
 *  This function is not available if the corresponding GPIO_HW_PORT*
 *  hardware parameter is set to false.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 *  port        -- relevant gpio port
 *  bit         -- port bit to check
 * RETURN VALUE
 *  Gpio_hardware   -- if bit is configured to have a hardware source
 *  Gpio_software   -- if bit is configured to have a software source
 *  Gpio_no_source  -- if the specified port bit is unavailable
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_porta_ctl
 *   - gpio_portb_ctl
 *   - gpio_portc_ctl
 *   - gpio_portd_ctl
 *
 *  This function is affected by the GPIO_NUM_PORTS, GPIO_PWIDTH_A,
 *  GPIO_PWIDTH_B, GPIO_PWIDTH_C, GPIO_PWIDTH_D, GPIO_PORTA_SINGLE_CTL,
 *  GPIO_PORTB_SINGLE_CTL, GPIO_PORTC_SINGLE_CTL, GPIO_PORTD_SINGLE_CTL,
 *  GPIO_HW_PORTA, GPIO_HW_PORTB, GPIO_HW_PORTC and GPIO_HW_PORTD
 *  hardware parameters.
 * SEE ALSO
 *  dw_gpio_setBitSource, dw_gpio_getSource
 * SOURCE
 */
enum dw_gpio_data_source dw_gpio_getBitSource(struct dw_device *dev,
        enum dw_gpio_port port, enum dw_gpio_bit bit);
/*****/

/****f* gpio.functions/dw_gpio_enableIrq
 * DESCRIPTION
 *  This function enables interrupts for the specified bit(s) of port A.
 *  Multiple bits can be specified in the function arguments, using the
 *  bitwise 'OR' operator.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 *  interrupts  -- interrupt bit(s) to enable
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- if interrupts are not supported
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_inten
 *
 *  This function is affected by the GPIO_PORTA_INTR hardware parameter.
 * SEE ALSO
 *  dw_gpio_disableIrq, dw_gpio_isIrqEnabled, dw_gpio_getEnabledIrq
 * SOURCE
 */
int dw_gpio_enableIrq(struct dw_device *dev, uint32_t interrupts);
/*****/

/****f* gpio.functions/dw_gpio_disableIrq
 * DESCRIPTION
 *  This function disables interrupts for the specified bit(s) of port
 *  A.  Multiple bits can be specified in the function arguments, using
 *  the bitwise 'OR' operator.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 *  interrupts  -- interrupt bit(s) to disable
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- if interrupts are not supported
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_inten
 *
 *  This function is affected by the GPIO_PORTA_INTR hardware parameter.
 * SEE ALSO
 *  dw_gpio_enableIrq, dw_gpio_isIrqEnabled, dw_gpio_getEnabledIrq
 * SOURCE
 */
int dw_gpio_disableIrq(struct dw_device *dev, uint32_t interrupts);
/*****/

/****f* gpio.functions/dw_gpio_isIrqEnabled
 * DESCRIPTION
 *  This function returns whether interrupts are enabled for a
 *  particular bit of port A or not.  Only one bit may be specified per
 *  invocation of this function.  A value of 'false' is always returned
 *  if interrupts are not supported, or if the specified bit is not
 *  available.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 *  interrupt   -- bit of port A to check
 * RETURN VALUE
 *  true        -- if interrupts are enabled for the specified bit
 *  false       -- if interrupts are disabled for the specified bit
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_inten
 *
 *  This function is affected by the GPIO_PORTA_INTR and GPIO_PWIDTH_A
 *  hardware parameters.
 * SEE ALSO
 *  dw_gpio_enableIrq, dw_gpio_disableIrq, dw_gpio_getEnabledIrq
 * SOURCE
 */
bool dw_gpio_isIrqEnabled(struct dw_device *dev, enum dw_gpio_bit
        interrupt);
/*****/

/****f* gpio.functions/dw_gpio_getEnabledIrq
 * DESCRIPTION
 *  This function returns the value of the interrupt enable register.
 *  A value of 0x0 is always returned if interrupts are not supported.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 * RETURN VALUE
 *  The value of the interrupt enable register.
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_inten
 *
 *  This function is affected by the GPIO_PORTA_INTR hardware parameter.
 * SEE ALSO
 *  dw_gpio_enableIrq, dw_gpio_disableIrq, dw_gpio_isIrqEnabled
 * SOURCE
 */
uint32_t dw_gpio_getEnabledIrq(struct dw_device *dev);
/*****/

/****f* gpio.functions/dw_gpio_maskIrq
 * DESCRIPTION
 *  This function masks interrupts for the specified bit(s) of port A.
 *  Multiple bits can be specified in the function arguments, using the
 *  bitwise 'OR' operator.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 *  interrupts  -- interrupt bit(s) to mask
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- if interrupts are not supported
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_intmask
 *
 *  This function is affected by the GPIO_PORTA_INTR hardware parameter.
 * SEE ALSO
 *  dw_gpio_unmaskIrq, dw_gpio_isIrqMasked, dw_gpio_getIrqMask
 * SOURCE
 */
int dw_gpio_maskIrq(struct dw_device *dev, uint32_t interrupts);
/*****/

/****f* gpio.functions/dw_gpio_unmaskIrq
 * DESCRIPTION
 *  This function unmasks interrupts for the specified bit(s) of port A.
 *  Multiple bits can be specified in the function arguments, using the
 *  bitwise 'OR' operator.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 *  interrupts  -- interrupt bit(s) to unmask
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- if interrupts are not supported
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_intmask
 *
 *  This function is affected by the GPIO_PORTA_INTR hardware parameter.
 * SEE ALSO
 *  dw_gpio_maskIrq, dw_gpio_isIrqMasked, dw_gpio_getIrqMask
 * SOURCE
 */
int dw_gpio_unmaskIrq(struct dw_device *dev, uint32_t interrupts);
/*****/

/****f* gpio.functions/dw_gpio_isIrqMasked
 * DESCRIPTION
 *  This function returns whether interrupts are masked for a
 *  particular bit of port A or not.  Only one bit may be specified per
 *  invocation of this function.  A value of 'false' is always returned
 *  if interrupts are not supported, or if the specified bit is not
 *  available.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 *  interrupt   -- bit of port A to check
 * RETURN VALUE
 *  true        -- if interrupts are masked for the specified bit
 *  false       -- if interrupts are not masked for the specified bit
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_intmask
 *
 *  This function is affected by the GPIO_PORTA_INTR and GPIO_PWIDTH_A
 *  hardware parameters.
 * SEE ALSO
 *  dw_gpio_maskIrq, dw_gpio_unmaskIrq, dw_gpio_getIrqMask
 * SOURCE
 */
bool dw_gpio_isIrqMasked(struct dw_device *dev, enum dw_gpio_bit
        interrupt);
/*****/

/****f* gpio.functions/dw_gpio_getIrqMask
 * DESCRIPTION
 *  This function returns the value of the interrupt mask register.  A
 *  value of 0x0 is always returned if interrupts are not supported.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 * RETURN VALUE
 *  The interrupt mask register value.
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_intmask
 *
 *  This function is affected by the GPIO_PORTA_INTR hardware parameter.
 * SEE ALSO
 *  dw_gpio_maskIrq, dw_gpio_unmaskIrq, dw_gpio_isIrqMasked
 * SOURCE
 */
uint32_t dw_gpio_getIrqMask(struct dw_device *dev);
/*****/

/****f* gpio.functions/dw_gpio_isIrqActive
 * DESCRIPTION
 *  This function returns whether an interrupt is active or not.  In
 *  order to be active, the interrupt for a bit of port A must be both
 *  enabled and unmasked.  Only one bit may be specified per invocation
 *  of this function.  A value of 'false' is always returned when
 *  interrupts are not supported.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 *  interrupt   -- bit of port A to check
 * RETURN VALUE
 *  true        -- if the interrupt bit is active
 *  false       -- if the interrupt bit is inactive
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_intstatus
 *
 *  This function is affected by the GPIO_PORTA_INTR and GPIO_PWIDTH_A
 *  hardware parameters.
 * SEE ALSO
 *  dw_gpio_enableIrq, dw_gpio_maskIrq, dw_gpio_getActiveIrq
 * SOURCE
 */
bool dw_gpio_isIrqActive(struct dw_device *dev, enum dw_gpio_bit
        interrupt);
/*****/

/****f* gpio.functions/dw_gpio_getActiveIrq
 * DESCRIPTION
 *  This function returns the value of the interrupt status register. A
 *  value of 0x0 is always returned if interrupts are not supported.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 * RETURN VALUE
 *  The interrupt status register value.
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_intstatus
 *
 *  This function is affected by the GPIO_PORTA_INTR hardware parameter.
 * SEE ALSO
 *  dw_gpio_enableIrq, dw_gpio_maskIrq, dw_gpio_isIrqActive
 * SOURCE
 */
uint32_t dw_gpio_getActiveIrq(struct dw_device *dev);
/*****/

/****f* gpio.functions/dw_gpio_isRawIrqActive
 * DESCRIPTION
 *  This function returns whether a raw interrupt is active or not.  In
 *  order to be active, the interrupt for a bit of port A must be
 *  enabled.  Only one bit may be specified per invocation of this
 *  function.  A value of 'false' is always returned when interrupts are
 *  not supported, or if the specified bit is not available.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 *  interrupt   -- bit of port A to check
 * RETURN VALUE
 *  true        -- if the raw interrupt bit is active
 *  false       -- if the raw interrupt bit is not active
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_rawintstatus
 *
 *  This function is affected by the GPIO_PORTA_INTR and GPIO_PWIDTH_A
 *  hardware parameters.
 * SEE ALSO
 *  dw_gpio_enableIrq, dw_gpio_getRawIrq
 * SOURCE
 */
bool dw_gpio_isRawIrqActive(struct dw_device *dev, enum dw_gpio_bit
        interrupt);
/*****/

/****f* gpio.functions/dw_gpio_getRawIrq
 * DESCRIPTION
 *  This function returns the value of the raw interrupt status
 *  register. A value of 0x0 is always returned if interrupts are not
 *  supported.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 * RETURN VALUE
 *  The raw interrupt status register value.
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_rawintstatus
 *
 *  This function is affected by the GPIO_PORTA_INTR hardware parameter.
 * SEE ALSO
 *  dw_gpio_enableIrq, dw_gpio_isRawIrqActive
 * SOURCE
 */
uint32_t dw_gpio_getRawIrq(struct dw_device *dev);
/*****/

/****f* gpio.functions/dw_gpio_clearIrq
 * DESCRIPTION
 *  This function is used to clear edge-sensitive interrupts of port A.
 *  Multiple bits can be specified in the function arguments, using the
 *  bitwise 'OR' operator.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 *  interrupts  -- interrupt bit(s) to clear
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- if interrupts are not supported
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_porta_eoi
 *
 *  This function is affected by the GPIO_PORTA_INTR hardware parameter.
 * SEE ALSO
 *  dw_gpio_enableIrq, dw_gpio_setIrqType
 * SOURCE
 */
int dw_gpio_clearIrq(struct dw_device *dev, uint32_t interrupts);
/*****/

/****f* gpio.functions/dw_gpio_setIrqType
 * DESCRIPTION
 *  This function sets the interrupt type for the specified bit(s) of
 *  port A.  There are two types of interrupts available,
 *  level-sensitive and edge-sensitive.  The polarity of these interrupt
 *  types is set using dw_gpio_setIrqPolarity().  Multiple bits can be
 *  specified in the function arguments, using the bitwise 'OR'
 *  operator.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 *  type        -- interrupt type to set
 *  interrupts  -- interrupt bit(s) to set
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- if interrupts are not supported
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_inttype_level
 *
 *  This function is affected by the GPIO_PORTA_INTR hardware parameter.
 * SEE ALSO
 *  dw_gpio_getIrqType, dw_gpio_setIrqPolarity
 * SOURCE
 */
int dw_gpio_setIrqType(struct dw_device *dev, enum dw_gpio_irq_type
        type, uint32_t interrupts);
/*****/

/****f* gpio.functions/dw_gpio_getIrqType
 * DESCRIPTION
 *  This function returns the interrupt type for a specified but of
 *  port A.  Only one bit may be specified per invocation of this
 *  function.
 * ARGUMENTS
 *  dev                     -- DW_apb_gpio device handle
 *  interrupt               -- bit of port A to check
 * RETURN VALUE
 *  Gpio_level_sensitive    -- if the interrupt bit is level-sensitive
 *  Gpio_edge_sensitive     -- if the interrupt bit is edge-sensitive
 *  Gpio_no_type            -- if interrupts are not supported or the
 *                             specified bit is not available
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_inttype_level
 *
 *  This function is affected by the GPIO_PORTA_INTR hardware parameter.
 * SEE ALSO
 *  dw_gpio_setIrqType, dw_gpio_getIrqPolarity
 * SOURCE
 */
enum dw_gpio_irq_type dw_gpio_getIrqType(struct dw_device *dev, enum
        dw_gpio_bit interrupt);
/*****/

/****f* gpio.functions/dw_gpio_setIrqPolarity
 * DESCRIPTION
 *  This function sets the interrupt polarity for the specified bit(s)
 *  of port A.  The polarity can be either active-low or active-high.
 *  For edge-sensitive interrupts, active-low corresponds to a
 *  falling-edge interrupt while active-high corresponds to a
 *  rising-edge interrupt.  The interrupt type is set using
 *  dw_gpio_setIrqType().  Multiple bits can be specified in the
 *  function arguments, using the bitwise 'OR' operator.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 *  polarity    -- interrupt polarity to set
 *  interrupts  -- interrupt bit(s) to set
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- if interrupts are not supported
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_int_polarity
 *
 *  This function is affected by the GPIO_PORTA_INTR hardware parameter.
 * SEE ALSO
 *  dw_gpio_getIrqPolarity, dw_gpio_setIrqType
 * SOURCE
 */
int dw_gpio_setIrqPolarity(struct dw_device *dev, enum
        dw_gpio_irq_polarity polarity, uint32_t interrupts);
/*****/

/****f* gpio.functions/dw_gpio_getIrqPolarity
 * DESCRIPTION
 *  This function returns the interrupt polarity of a specified bit of
 *  port A.  Only one bit may be specified per invocation of this
 *  function.
 * ARGUMENTS
 *  dev                 -- DW_apb_gpio device handle
 *  interrupt           -- bit of port A to check
 * RETURN VALUE
 *  Gpio_active_low     -- if interrupt bit is active-low/falling-edge
 *  Gpio_active_high    -- if interrupt bit is active-high/rising-edge
 *  Gpio_no_polarity    -- if interrupts are not supported
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_int_polarity
 *
 *  This function is affected by the GPIO_PORTA_INTR and GPIO_PWIDTH_A
 *  hardware parameters.
 * SEE ALSO
 *  dw_gpio_setIrqPolarity, dw_gpio_getIrqType
 * SOURCE
 */
enum dw_gpio_irq_polarity dw_gpio_getIrqPolarity(struct dw_device *dev,
        enum dw_gpio_bit interrupt);
/*****/

/****f* gpio.functions/dw_gpio_enableDebounce
 * DESCRIPTION
 *  This function enables debounce logic for the specified bit(s) of
 *  port A.  When enabled, a signal must be valid for two periods of the
 *  external debounce clock before it is internally processed.  See the
 *  DW_apb_gpio databook for more details.  Multiple bits can be
 *  specified in the function arguments, using the bitwise 'OR'
 *  operator.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 *  interrupts  -- interrupt bit(s) to set
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- if interrupts are not supported
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_debounce
 *
 *  This function is affected by the GPIO_PORTA_INTR and GPIO_DEBOUNCE
 *  hardware parameters.
 * SEE ALSO
 *  dw_gpio_disableDebounce, dw_gpio_isDebounceEnabled
 * SOURCE
 */
int dw_gpio_enableDebounce(struct dw_device *dev, uint32_t interrupts);
/*****/

/****f* gpio.functions/dw_gpio_disableDebounce
 * DESCRIPTION
 *  This function disables debounce logic for the specified bit(s) of
 *  port A.  Multiple bits can be specified in the function arguments,
 *  using the bitwise 'OR' operator.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 *  interrupts  -- interrupt bit(s) to set
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- if interrupts are not supported
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_debounce
 *
 *  This function is affected by the GPIO_PORTA_INTR and GPIO_DEBOUNCE
 *  hardware parameters.
 * SEE ALSO
 *  dw_gpio_enableDebounce, dw_gpio_isDebounceEnabled
 * SOURCE
 */
int dw_gpio_disableDebounce(struct dw_device *dev, uint32_t interrupts);
/*****/

/****f* gpio.functions/dw_gpio_isDebounceEnabled
 * DESCRIPTION
 *  This function returns whether debounce is enabled for the specified
 *  it or port A or not.  Only one bit may be specified per invocation
 *  of this function.  A value of 'false' is always returned if
 *  interrupts are not supported, or if the specified bit is not
 *  available.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 *  interrupt   -- bit of port A to check
 * RETURN VALUE
 *  true        -- if debounce is enabled for interrupt bit
 *  false       -- if debounce is disabled for interrupt bit
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_debounce
 *
 *  This function is affected by the GPIO_PORTA_INTR, GPIO_PWIDTH_A and
 *  GPIO_DEBOUNCE hardware parameters.
 * SEE ALSO
 *  dw_gpio_enableDebounce, dw_gpio_disableDebounce
 * SOURCE
 */
bool dw_gpio_isDebounceEnabled(struct dw_device *dev, enum dw_gpio_bit
        interrupt);
/*****/

/****f* gpio.functions/dw_gpio_enableSync
 * DESCRIPTION
 *  This function enables interrupt synchronization.  When enabled, all
 *  level-sensitive interrupts are synchronized to pclk.  See the
 *  DW_apb_gpio databook for more details.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- if interrupts are not supported
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_ls_sync
 *
 *  This function is affected by the GPIO_PORTA_INTR hardware parameter.
 * SEE ALSO
 *  dw_gpio_disableSync, dw_gpio_isSyncEnabled
 * SOURCE
 */
int dw_gpio_enableSync(struct dw_device *dev);
/*****/

/****f* gpio.functions/dw_gpio_disableSync
 * DESCRIPTION
 *  This function disables synchronization for level-sensitive
 *  interrupts.  See the DW_apb_gpio databook for more details.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- if interrupts are not supported
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_ls_sync
 *
 *  This function is affected by the GPIO_PORTA_INTR hardware parameter.
 * SEE ALSO
 *  dw_gpio_enableSync, dw_gpio_isSyncEnabled
 * SOURCE
 */
int dw_gpio_disableSync(struct dw_device *dev);
/*****/

/****f* gpio.functions/dw_gpio_isSyncEnabled
 * DESCRIPTION
 *  This function returns whether synchronization is enabled for
 *  level-sensitive interrupts or not.  A value of 'false' is always
 *  returned when interrupts are not supported.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 * RETURN VALUE
 *  true        -- if interrupt synchronization is enabled
 *  false       -- if interrupt synchronization is not enabled
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_ls_sync
 *
 *  This function is affected by the GPIO_PORTA_INTR hardware parameter.
 * SEE ALSO
 *  dw_gpio_enableSync, dw_gpio_disableSync
 * SOURCE
 */
bool dw_gpio_isSyncEnabled(struct dw_device *dev);
/*****/

/****f* gpio.functions/dw_gpio_setListener
 * DESCRIPTION
 *  This function is used to set a user listener function.  The listener
 *  function is responsible for handling all DW_apb_gpio interrupts.  It
 *  is also incumbent upon the listener to clear any edge-sensitive
 *  interrupts.  The listener function is passed two arguments; a
 *  dw_device structure and an integer indicating which bit of port A
 *  caused the interrupt (i.e. 0 for bit 0 to 31 for bit 31).
 * ARGUMENTS
 *  dev             -- DW_apb_gpio device handle
 *  userFunction    -- user listener function to set
 * RETURN VALUE
 *  0               -- if successful
 *  -DW_ENOSYS      -- if interrupts are not supported
 * NOTES
 *  This function is affected by the GPIO_PORTA_INTR hardware parameter.
 * SEE ALSO
 *  dw_gpio_irqHandler
 * SOURCE
 */
int dw_gpio_setListener(struct dw_device *dev, dw_callback
        userFunction);
/*****/

/****f* gpio.functions/dw_gpio_irqHandler
 * DESCRIPTION
 *  This function reads the interrupt status register and calls the user
 *  listener function for all active interrupts.  Interrupts are
 *  processed in priority from bit 0 to bit 31 of port A.  If you wish
 *  to use a different priority scheme, you must use a custom interrupt
 *  handler other than this one.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 * RETURN VALUE
 *  true        -- if an interrupt was processed
 *  false       -- if no interrupt was processed
 * NOTES
 *  Accesses the following DW_apb_gpio register(s)/bit-field(s):
 *   - gpio_intstatus
 *
 * SEE ALSO
 *  dw_gpio_setListener
 * SOURCE
 */
int dw_gpio_irqHandler(struct dw_device *dev);
/*****/


#ifdef __cplusplus
}
#endif

#endif  // DW_APB_GPIO_PUBLIC_H

