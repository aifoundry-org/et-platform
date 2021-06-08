/*-------------------------------------------------------------------------
* Copyright (C) 2018, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

#ifndef __BL2_GPIO_CONTROLLER_H__
#define __BL2_GPIO_CONTROLLER_H__

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    GPIO_CONTROLLER_ID_INVALID = 0,
    GPIO_CONTROLLER_ID_SPIO,
    GPIO_CONTROLLER_ID_PU
} GPIO_CONTROLLER_ID_t;

typedef enum {
    GPIO_INT_LEVEL = 0,
    GPIO_INT_EDGE
} GPIO_INT_SENSITIVITY_t;

typedef enum {
    GPIO_INT_LOW = 0,
    GPIO_INT_HIGH
} GPIO_INT_POLARITY_t;

typedef enum {
    GPIO_INT_DEBOUNCE_OFF = 0,
    GPIO_INT_DEBOUNCE_ON
} GPIO_INT_DEBOUNCE_t;


/*! \fn int gpio_config_interrupt(GPIO_CONTROLLER_ID_t id, uint8_t pin_number, GPIO_INT_SENSITIVITY_t sensitivity,
*                          GPIO_INT_POLARITY_t polarity, GPIO_INT_DEBOUNCE_t debounce)
    \brief This function configures GPIO interrupt:
*           - sensitivity (edge/level)
*           - polarity
*           - debounce
    \param id - GPIO controller id (SPIO/PU).
    \param pin_number - GPIO pin number
    \param sensitivity - setting 0-level sensitive, 1-edge sensitive
    \param polarity - setting 0-falling-edge or active-low sensitive,
*                      1-rising-edge or active-high sensitive
    \param debounce - setting 0-no debounce, 1-en debounce
    \return The function call status, pass/fail.
*/

int gpio_config_interrupt(GPIO_CONTROLLER_ID_t id, uint8_t pin_number, GPIO_INT_SENSITIVITY_t sensitivity,
                          GPIO_INT_POLARITY_t polarity, GPIO_INT_DEBOUNCE_t debounce);

/*! \fn int gpio_enable_interrupt(GPIO_CONTROLLER_ID_t id, uint8_t pin_number)
    \brief This function unmask and enables GPIO interrupt
    \param id - GPIO controller id (SPIO/PU).
    \param pin_number - GPIO pin number
    \return The function call status, pass/fail.
*/

int gpio_enable_interrupt(GPIO_CONTROLLER_ID_t id, uint8_t pin_number);

/*! \fn int gpio_disable_interrupt(GPIO_CONTROLLER_ID_t id, uint8_t pin_number)
    \brief This function mask and disables GPIO interrupt
    \param id - GPIO controller id (SPIO/PU).
    \param pin_number - GPIO pin number
    \return The function call status, pass/fail.
*/

int gpio_disable_interrupt(GPIO_CONTROLLER_ID_t id, uint8_t pin_number);

/*! \fn int gpio_clear_interrupt(GPIO_CONTROLLER_ID_t id, uint8_t pin_number)
    \brief This function clears GPIO interrupt
    \param id - GPIO controller id (SPIO/PU).
    \param pin_number - GPIO pin number
    \return The function call status, pass/fail.
*/

int gpio_clear_interrupt(GPIO_CONTROLLER_ID_t id, uint8_t pin_number);

#endif
