/***********************************************************************
*
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/*! \file bl2_i2c_driver.h
    \brief A C header that defines the Firmware update service's
    public interfaces. These interfaces provide services using which
    the host can issue firmware update/status commands to device.
*/
/***********************************************************************/
#pragma once
#include "hwinc/sp_i2c0.h"
#include <stdbool.h>
#include "FreeRTOS.h"
#include "portmacro.h"
#include "semphr.h"

/*!
 * @struct struct ET_I2C_DEV
 * @brief I2C device control block
 */
typedef struct ET_I2C_DEV {
  I2c* regs;
  bool isInitialized;
  StaticSemaphore_t bus_lock;
  SemaphoreHandle_t bus_lock_handle;
} ET_I2C_DEV_t;

/*!
 * @enum ET_I2C_CONTROLLER
 * @brief enum for number of I2C controllers
 */
typedef enum ET_I2C_CONTROLLER {
    ET_SPIO_I2C0 = 0,
    ET_SPIO_I2C1,
    ET_PU_I2C
} ET_I2C_CONTROLLER_t;

/*!
 * @enum ET_I2C_SPEED
 * @brief enum for I2C baud rate definitions
 */
typedef enum ET_I2C_SPEED {
    ET_I2C_SPEED_100k = 0,
    ET_I2C_SPEED_400k
} ET_I2C_SPEED_t;

/*!
 * @enum ET_I2C_ERROR_CODES
 * @brief enum for I2C error codes
 */
enum ET_I2C_ERROR_CODES {

    ET_I2C_ERROR_DEV_ALREADY_INITIALIZED = -1000,
    ET_I2C_ERROR_DEV_NOT_INITIALIZED = -900,
    ET_I2C_ERROR_SPEED_BAD_VALUE = - 800,
    ET_I2C_ERROR_BUS_LOCK_INIT = -700,
    ET_I2C_ERROR_BUS_LOCK = -600,
    ET_I2C_OK = 0
};

/*! \fn int i2c_init(ET_I2C_DEV_t* dev, ET_I2C_SPEED_t speed, uint8_t addr_slave)
    \brief This function initializes I2C controller
    \param dev pointer to I2C device control block
    \param speed baud rate of I2C
    \param addr_slave slave address
    \return Status indicating success or negative error
*/
int i2c_init(ET_I2C_DEV_t* dev, ET_I2C_SPEED_t speed, uint8_t addr_slave);

/*! \fn int i2c_write(ET_I2C_DEV_t* dev, uint8_t regAddr, const uint8_t* txDataBuff, uint8_t txDataCount)
    \brief Interface to write data onto I2C slave
    \param dev pointer to I2C device control block
    \param regAddr register address
    \param txDataBuff buffer containing data to be written
    \param txDataCount data size
    \return Status indicating success or negative error
*/
int i2c_write(ET_I2C_DEV_t* dev, uint8_t regAddr, const uint8_t* txDataBuff, uint8_t txDataCount);

/*! \fn int i2c_read(ET_I2C_DEV_t* dev, uint8_t regAddr, uint8_t* rxDataBuff, uint8_t rxDataCount)
    \brief Interface to read data from I2C slave
    \param dev pointer to I2C device control block
    \param regAddr register address
    \param txDataBuff buffer for rx data
    \param txDataCount data size
    \return Status indicating success or negative error
*/
int i2c_read(ET_I2C_DEV_t* dev, uint8_t regAddr, uint8_t* rxDataBuff, uint8_t rxDataCount);

/*! \fn int i2c_disable(ET_I2C_DEV_t* dev)
    \brief this function disables I2C controller
    \param dev pointer to I2C device control block
    \return Status indicating success or negative error
*/
int i2c_disable(ET_I2C_DEV_t* dev);


