/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------

*************************************************************************/
/*! \file gpio_controller.c
    \brief A C module that implements the GPIO access functions

    Public interfaces:
        gpio_config_interrupt
        gpio_enable_interrupt
        gpio_disable_interrupt

*/
/***********************************************************************/

#include <stdio.h>

#include "io.h"
#include "bl2_gpio_controller.h"
#include "bl_error_code.h"
#include "bl2_main.h"

#include "hwinc/sp_gpio.h"
#include "hwinc/hal_device.h"

/************************************************************************
*
*   FUNCTION
*
*       get_gpio_registers
*
*   DESCRIPTION
*
*       This function returns base address of GPIO registers
*
*   INPUTS
*
*       id     GPIO controller id
*
*   OUTPUTS
*
*       return   pointer to base address of GPIO registers
*
***********************************************************************/
static uintptr_t get_gpio_registers(GPIO_CONTROLLER_ID_t id)
{
    switch (id)
    {
        case GPIO_CONTROLLER_ID_SPIO:
            return R_SP_GPIO_BASEADDR;
        case GPIO_CONTROLLER_ID_PU:
            return R_PU_GPIO_BASEADDR;
        case GPIO_CONTROLLER_ID_INVALID:
            return 0;
        default:
            return 0;
    }
}

/************************************************************************
*
*   FUNCTION
*
*       gpio_config_interrupt
*
*   DESCRIPTION
*
*       This function configures GPIO interrupt:
*           - sensitivity (edge/level)
*           - polarity
*           - debounce
*
*   INPUTS
*
*       id             GPIO controller id
*       pin_number     GPIO pin number
*       sensitivity    setting 0-level sensitive, 1-edge sensitive
*       polarity       setting 0-falling-edge or active-low sensitive,
*                      1-rising-edge or active-high sensitive
*       debounce       setting 0-no debounce, 1-en debounce
*
*   OUTPUTS
*
*       none
*
***********************************************************************/
int gpio_config_interrupt(GPIO_CONTROLLER_ID_t id, uint8_t pin_number, GPIO_INT_SENSITIVITY_t sensitivity,
                          GPIO_INT_POLARITY_t polarity, GPIO_INT_DEBOUNCE_t debounce)
{
    uint32_t reg_value;

    uintptr_t gpio_regs = get_gpio_registers(id);
    if (0 == gpio_regs)
    {
        MESSAGE_ERROR("Invalid GPIO ID!");
        return ERROR_GPIO_INVALID_ID;
    }

    if(pin_number > 31)
    {
        MESSAGE_ERROR("Invalid pin number!");
        return ERROR_GPIO_INVALID_ARGUMENTS;
    }

    reg_value = ioread32(gpio_regs + SPIO_GPIO_GPIO_INTTYPE_LEVEL_ADDRESS);
    if(sensitivity == 0)
    {
        reg_value = reg_value & (uint32_t)(~(1 << pin_number));
    }
    else
    {
        reg_value = reg_value | (uint32_t)(1 << pin_number);
    }
    iowrite32(gpio_regs + SPIO_GPIO_GPIO_INTTYPE_LEVEL_ADDRESS, reg_value);

    reg_value = ioread32(gpio_regs + SPIO_GPIO_GPIO_INT_POLARITY_ADDRESS);
    if(polarity == 0)
    {
        reg_value = reg_value & (uint32_t)(~(1 << pin_number));
    }
    else
    {
        reg_value = reg_value | (uint32_t)(1 << pin_number);
    }
    iowrite32(gpio_regs + SPIO_GPIO_GPIO_INT_POLARITY_ADDRESS, reg_value);

    reg_value = ioread32(gpio_regs + SPIO_GPIO_GPIO_DEBOUNCE_ADDRESS);
    if(debounce == 0)
    {
        reg_value = reg_value & (uint32_t)(~(1 << pin_number));
    }
    else
    {
        reg_value = reg_value | (uint32_t)(1 << pin_number);
    }
    iowrite32(gpio_regs + SPIO_GPIO_GPIO_DEBOUNCE_ADDRESS, reg_value);

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       gpio_enable_interrupt
*
*   DESCRIPTION
*
*       This function unmask and enables GPIO interrupt
*
*   INPUTS
*
*       id           GPIO controller id
*       pin_number   GPIO pin number
*
*   OUTPUTS
*
*       none
*
***********************************************************************/
int gpio_enable_interrupt(GPIO_CONTROLLER_ID_t id, uint8_t pin_number)
{
    uint32_t reg_value;

    uintptr_t gpio_regs = get_gpio_registers(id);
    if (0 == gpio_regs)
    {
        MESSAGE_ERROR("Invalid GPIO ID!");
        return ERROR_GPIO_INVALID_ID;
    }

    if(pin_number > 31)
    {
        MESSAGE_ERROR("Invalid pin number!");
        return ERROR_GPIO_INVALID_ARGUMENTS;
    }

    reg_value = ioread32(gpio_regs + SPIO_GPIO_GPIO_INTMASK_ADDRESS);
    reg_value = reg_value & (uint32_t)(~(1 << pin_number));
    iowrite32(gpio_regs + SPIO_GPIO_GPIO_INTMASK_ADDRESS, reg_value);

    reg_value = ioread32(gpio_regs + SPIO_GPIO_GPIO_INTEN_ADDRESS);
    reg_value = reg_value | (uint32_t)(1 << pin_number);
    iowrite32(gpio_regs + SPIO_GPIO_GPIO_INTEN_ADDRESS, reg_value);

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       gpio_disable_interrupt
*
*   DESCRIPTION
*
*       This function mask and disables GPIO interrupt
*
*   INPUTS
*
*       id           GPIO controller id
*       pin_number   GPIO pin number
*
*   OUTPUTS
*
*       none
*
***********************************************************************/
int gpio_disable_interrupt(GPIO_CONTROLLER_ID_t id, uint8_t pin_number)
{
    uint32_t reg_value;

    uintptr_t gpio_regs = get_gpio_registers(id);
    if (0 == gpio_regs)
    {
        MESSAGE_ERROR("Invalid GPIO ID!");
        return ERROR_GPIO_INVALID_ID;
    }

    if(pin_number > 31)
    {
        MESSAGE_ERROR("Invalid pin number!");
        return ERROR_GPIO_INVALID_ARGUMENTS;
    }

    reg_value = ioread32(gpio_regs + SPIO_GPIO_GPIO_INTMASK_ADDRESS);
    reg_value = reg_value | (uint32_t)(1 << pin_number);
    iowrite32(gpio_regs + SPIO_GPIO_GPIO_INTMASK_ADDRESS, reg_value);

    reg_value = ioread32(gpio_regs + SPIO_GPIO_GPIO_INTEN_ADDRESS);
    reg_value = reg_value & (uint32_t)(~(1 << pin_number));
    iowrite32(gpio_regs + SPIO_GPIO_GPIO_INTEN_ADDRESS, reg_value);

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       gpio_clear_interrupt
*
*   DESCRIPTION
*
*       This function clears GPIO interrupt
*
*   INPUTS
*
*       id           GPIO controller id
*       pin_number   GPIO pin number
*
*   OUTPUTS
*
*       none
*
***********************************************************************/
int gpio_clear_interrupt(GPIO_CONTROLLER_ID_t id, uint8_t pin_number)
{
    uint32_t reg_value = 0;

    uintptr_t gpio_regs = get_gpio_registers(id);
    if (0 == gpio_regs)
    {
        MESSAGE_ERROR("Invalid GPIO ID!");
        return ERROR_GPIO_INVALID_ID;
    }

    if(pin_number > 31)
    {
        MESSAGE_ERROR("Invalid pin number!");
        return ERROR_GPIO_INVALID_ARGUMENTS;
    }

    reg_value = reg_value | (uint32_t)(1 << pin_number);
    iowrite32(gpio_regs + SPIO_GPIO_GPIO_PORTA_EOI_ADDRESS, reg_value);

    return 0;
}
