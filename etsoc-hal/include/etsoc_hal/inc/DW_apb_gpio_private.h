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

#ifndef DW_APB_GPIO_PRIVATE_H
#define DW_APB_GPIO_PRIVATE_H

#ifdef __cplusplus
extern "C" {    // allow C++ to use these headers
#endif

/****id* gpio.macros/GPIO_COMMON_REQUIREMENTS
 * NAME
 *  Common DW_apb_gpio API Requirements
 * DESCRIPTION
 *  These are the common pre-conditions that must be met for all
 *  DW_apb_gpio Driver Kit functions.  Primarily, they check that a
 *  function has been passed a legitimate dw_device structure.
 * SOURCE
 */
#define GPIO_COMMON_REQUIREMENTS(dev)           \
do {                                            \
    DW_REQUIRE(dev != NULL);                    \
    DW_REQUIRE(dev->comp_param != NULL);        \
    DW_REQUIRE(dev->base_address != NULL);      \
    DW_REQUIRE(dev->comp_type == Dw_apb_gpio);  \
    DW_REQUIRE((dev->data_width == 32)          \
            || (dev->data_width == 16)          \
            || (dev->data_width == 8));         \
} while(0)
/*****/

/****id* gpio.macros/bit_definitions
 * DESCRIPTION
 *  Used in conjunction with DW_common_bitops.h to access register
 *  bitfields.  They are defined as bit offset/mask pairs for each gpio
 *  register bitfield.
 * NOTES
 *  bfo is the offset of the bitfield with respect to LSB;
 *  bfw is the width of the bitfield
 * SOURCE
 */
// For the following #define, X can be A, B, C or D (registers
// PORTA_CTL, PORTB_CTL, PORTC_CTL and PORTD_CTL respectively).
#define bfoGPIO_PORTX_CTL_DATA_SRC      ((uint32_t) 0)
#define bfwGPIO_PORTX_CTL_DATA_SRC      ((uint32_t) 1)
#define bfoGPIO_LS_SYNC_SYNCLEVEL       ((uint32_t) 0)
#define bfwGPIO_LS_SYNC_SYNCLEVEL       ((uint32_t) 1)
// Component parameters
#define bfoGPIO_PARAMS_1_APB_DATA_WIDTH     ((uint32_t) 0)
#define bfwGPIO_PARAMS_1_APB_DATA_WIDTH     ((uint32_t) 2)
#define bfoGPIO_PARAMS_1_NUM_PORTS          ((uint32_t) 2)
#define bfwGPIO_PARAMS_1_NUM_PORTS          ((uint32_t) 2)
#define bfoGPIO_PARAMS_1_PORTA_SINGLE_CTL   ((uint32_t) 4)
#define bfwGPIO_PARAMS_1_PORTA_SINGLE_CTL   ((uint32_t) 1)
#define bfoGPIO_PARAMS_1_PORTB_SINGLE_CTL   ((uint32_t) 5)
#define bfwGPIO_PARAMS_1_PORTB_SINGLE_CTL   ((uint32_t) 1)
#define bfoGPIO_PARAMS_1_PORTC_SINGLE_CTL   ((uint32_t) 6)
#define bfwGPIO_PARAMS_1_PORTC_SINGLE_CTL   ((uint32_t) 1)
#define bfoGPIO_PARAMS_1_PORTD_SINGLE_CTL   ((uint32_t) 7)
#define bfwGPIO_PARAMS_1_PORTD_SINGLE_CTL   ((uint32_t) 1)
#define bfoGPIO_PARAMS_1_HW_PORTA           ((uint32_t) 8)
#define bfwGPIO_PARAMS_1_HW_PORTA           ((uint32_t) 1)
#define bfoGPIO_PARAMS_1_HW_PORTB           ((uint32_t) 9)
#define bfwGPIO_PARAMS_1_HW_PORTB           ((uint32_t) 1)
#define bfoGPIO_PARAMS_1_HW_PORTC           ((uint32_t) 10)
#define bfwGPIO_PARAMS_1_HW_PORTC           ((uint32_t) 1)
#define bfoGPIO_PARAMS_1_HW_PORTD           ((uint32_t) 11)
#define bfwGPIO_PARAMS_1_HW_PORTD           ((uint32_t) 1)
#define bfoGPIO_PARAMS_1_PORTA_INTR         ((uint32_t) 12)
#define bfwGPIO_PARAMS_1_PORTA_INTR         ((uint32_t) 1)
#define bfoGPIO_PARAMS_1_DEBOUNCE           ((uint32_t) 13)
#define bfwGPIO_PARAMS_1_DEBOUNCE           ((uint32_t) 1)
#define bfoGPIO_PARAMS_1_ADD_ENCODED_PARAMS ((uint32_t) 14)
#define bfwGPIO_PARAMS_1_ADD_ENCODED_PARAMS ((uint32_t) 1)
#define bfoGPIO_PARAMS_1_ID                 ((uint32_t) 15)
#define bfwGPIO_PARAMS_1_ID                 ((uint32_t) 1)
#define bfoGPIO_PARAMS_1_ID_WIDTH           ((uint32_t) 16)
#define bfwGPIO_PARAMS_1_ID_WIDTH           ((uint32_t) 5)
#define bfoGPIO_PARAMS_2_PWIDTH_A           ((uint32_t) 0)
#define bfwGPIO_PARAMS_2_PWIDTH_A           ((uint32_t) 5)
#define bfoGPIO_PARAMS_2_PWIDTH_B           ((uint32_t) 5)
#define bfwGPIO_PARAMS_2_PWIDTH_B           ((uint32_t) 5)
#define bfoGPIO_PARAMS_2_PWIDTH_C           ((uint32_t) 10)
#define bfwGPIO_PARAMS_2_PWIDTH_C           ((uint32_t) 5)
#define bfoGPIO_PARAMS_2_PWIDTH_D           ((uint32_t) 15)
#define bfwGPIO_PARAMS_2_PWIDTH_D           ((uint32_t) 5)
/*****/

/****id* gpio.macros/APB_ACCESS
 * DESCRIPTION
 *  This macro is used to hardcode the APB data accesses, if the APB
 *  data width is the same for an entire system.  Simply defining
 *  APB_DATA_WIDTH at compile time will force all DW_apb_gpio memory map
 *  I/O accesses to be performed with the specifed data width.  By
 *  default, no I/O access is performed until the APB data width of a
 *  device is checked in the dw_device data structure.
 * SOURCE
 */
#ifdef APB_DATA_WIDTH
#if (APB_DATA_WIDTH == 32)
#define GPIO_INP    DW_IN32_32P
#define GPIO_OUTP   DW_OUT32_32P
#elif (APB_DATA_WIDTH == 16)
#define GPIO_INP    DW_IN32_16P
#define GPIO_OUTP   DW_OUT32_16P
#else
#define GPIO_INP    DW_IN32_8P
#define GPIO_OUTP   DW_OUT32_8P
#endif      // (APB_DATA_WIDTH == 32)
#else
#define GPIO_INP    DW_INP
#define GPIO_OUTP   DW_OUTP
#endif      // APB_DATA_WIDTH
/*****/

/****id* gpio.macros/DW_CC_DEFINE_GPIO_PARAMS
 * DESCRIPTION
 *  This macro is intended for use in initializing values for the
 *  dw_gpio_param structure (upon which it is dependent).  These
 *  values are obtained from DW_apb_gpio_defs.h (upon which this
 *  macro is also dependent).
 * ARGUMENTS
 *  prefix      -- prefix of peripheral (can be blank/empty)
 * NOTES
 *  The relevant DW_apb_gpio coreKit C header must be included before
 *  this macro can be used.
 * SEE ALSO
 *  dw_gpio_param
 * SOURCE
 */
#define DW_CC_DEFINE_GPIO_PARAMS(x) DW_CC_DEFINE_GPIO_PARAMS_2_03(x)

#define DW_CC_DEFINE_GPIO_PARAMS_2_03(prefix) {             \
    prefix ## CC_GPIO_DEBOUNCE,                             \
    prefix ## CC_GPIO_HW_PORTA,                             \
    prefix ## CC_GPIO_HW_PORTB,                             \
    prefix ## CC_GPIO_HW_PORTC,                             \
    prefix ## CC_GPIO_HW_PORTD,                             \
    prefix ## CC_GPIO_ID,                                   \
    prefix ## CC_GPIO_PORTA_INTR,                           \
    prefix ## CC_GPIO_PORTA_SINGLE_CTL,                     \
    prefix ## CC_GPIO_PORTB_SINGLE_CTL,                     \
    prefix ## CC_GPIO_PORTC_SINGLE_CTL,                     \
    prefix ## CC_GPIO_PORTD_SINGLE_CTL,                     \
    prefix ## CC_GPIO_ID_WIDTH,                             \
    prefix ## CC_GPIO_NUM_PORTS,                            \
    prefix ## CC_GPIO_PWIDTH_A,                             \
    prefix ## CC_GPIO_PWIDTH_B,                             \
    prefix ## CC_GPIO_PWIDTH_C,                             \
    prefix ## CC_GPIO_PWIDTH_D                              \
}
/*****/

/****is* gpio.data/dw_gpio_param
 * DESCRIPTION
 *  This structure comprises the gpio hardware parameters that affect
 *  the software driver.  This structure needs to be initialized with
 *  the correct values and be pointed to by the (struct
 *  dw_device).comp_param member of the relevant gpio device structure.
 * SEE ALSO
 *  DW_CC_DEFINE_GPIO_PARAMS
 * SOURCE
 */
struct dw_gpio_param {
    bool debounce;
    bool hw_porta;
    bool hw_portb;
    bool hw_portc;
    bool hw_portd;
    bool id;
    bool porta_intr;
    bool porta_single_ctl;
    bool portb_single_ctl;
    bool portc_single_ctl;
    bool portd_single_ctl;
    uint8_t id_width;
    uint8_t num_ports;
    uint8_t pwidth_a;
    uint8_t pwidth_b;
    uint8_t pwidth_c;
    uint8_t pwidth_d;
};
/*****/

/****is* gpio.data/dw_gpio_portmap
 * DESCRIPTION
 *  This is the structure used for accessing the DW_apb_gpio memory map.
 * SOURCE
 */
struct dw_gpio_portmap {
    volatile uint32_t swporta_dr;    // port A data              (0x00)
    volatile uint32_t swporta_ddr;   // port A data direction    (0x04)
    volatile uint32_t porta_ctl;     // port A data source       (0x08)
    volatile uint32_t swportb_dr;    // port B data              (0x0c)
    volatile uint32_t swportb_ddr;   // port B data direction    (0x10)
    volatile uint32_t portb_ctl;     // port B data source       (0x14)
    volatile uint32_t swportc_dr;    // port C data              (0x18)
    volatile uint32_t swportc_ddr;   // port C data direction    (0x1c)
    volatile uint32_t portc_ctl;     // port C data source       (0x20)
    volatile uint32_t swportd_dr;    // port D data              (0x24)
    volatile uint32_t swportd_ddr;   // port D data direction    (0x28)
    volatile uint32_t portd_ctl;     // port D data source       (0x2c)
    volatile uint32_t inten;         // interrupt enable         (0x30)
    volatile uint32_t intmask;       // interrupt mask           (0x34)
    volatile uint32_t inttype_level; // interrupt level          (0x38)
    volatile uint32_t int_polarity;  // interrupt polarity       (0x3c)
    volatile uint32_t intstatus;     // port A irq status        (0x40)
    volatile uint32_t rawintstatus;  // port A raw irq status    (0x44)
    volatile uint32_t debounce;      // debounce enable          (0x48)
    volatile uint32_t porta_eoi;     // port A interrupt clear   (0x4c)
    volatile uint32_t ext_porta;     // port A external port     (0x50)
    volatile uint32_t ext_portb;     // port B external port     (0x54)
    volatile uint32_t ext_portc;     // port C external port     (0x58)
    volatile uint32_t ext_portd;     // port D external port     (0x5c)
    volatile uint32_t ls_sync;       // level-sensitive          (0x60)
                                     // synchronization enable
    volatile uint32_t id_code;       // user-specified ID code   (0x64)
    volatile uint32_t reserved1;     // reserved                 (0x68)
    volatile uint32_t old_comp_version; // component version     (0x6c)
    volatile uint32_t reserved2[32]; // reserved          (0x70 - 0xec)
    volatile uint32_t comp_params_2; // component parameters 2   (0xf0)
    volatile uint32_t comp_params_1; // component parameters 1   (0xf4)
    volatile uint32_t comp_version;  // component version        (0xf8)
    volatile uint32_t comp_type;     // component type           (0xfc)
};
/*****/

/****is* gpio.data/dw_gpio_instance
 * DESCRIPTION
 *  This structure contains variables which relate to each individual
 *  DW_apb_gpio instance.  Cumulatively, they can be thought of as the
 *  "state/global variables" for each gpio instance.  For DW_apb_gpio,
 *  the only variable that needs to be tracked is the function pointer
 *  to the user listener function.  This is called whenever a interrupt
 *  on port A is triggered.
 * SOURCE
 */
struct dw_gpio_instance {
    dw_callback listener;       // user event listener
};
/*****/

/****if* gpio.functions/dw_gpio_autoCompParams
 * DESCRIPTION
 *  This function attempts to automatically discover the hardware
 *  component parameters, if this supported by the DW_apb_gpio supplied
 *  in the arguments.
 * ARGUMENTS
 *  dev         -- DW_apb_gpio device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -ENOSYS     -- function not supported
 * USES
 *  Accesses the following DW_apb_i2c register/bitfield(s):
 *   - gpio_comp_type
 *   - gpio_comp_version
 *   - gpiocomp_param_1
 *
 * NOTES
 *  This function does not allocate any memory.  An instance of
 *  dw_gpio_param must already be allocated and properly referenced from
 *  the relevant dw_device.comp_param structure member.
 * SEE ALSO
 *  dw_gpio_init
 * SOURCE
 */
int dw_gpio_autoCompParams(struct dw_device *dev);
/*****/

#ifdef __cplusplus
}
#endif

#endif  // DW_APB_GPIO_PRIVATE_H

