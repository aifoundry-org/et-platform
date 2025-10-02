#pragma once
#include "DW_apb_i2c.h"
#include <stdbool.h>


typedef struct ET_I2C_DEV {
  I2c* regs;
  bool isInitialized;
} ET_I2C_DEV_t;

typedef enum ET_I2C_CONTROLLER {
    ET_SPIO_I2C0 = 0,
    ET_SPIO_I2C1,
    ET_PU_I2C
} ET_I2C_CONTROLLER_t;

typedef enum ET_I2C_SPEED {
    ET_I2C_SPEED_100k = 0,
    ET_I2C_SPEED_400k 
} ET_I2C_SPEED_t;

enum ET_I2C_ERROR_CODES {

    ET_I2C_ERROR_DEV_ALREADY_INITIALIZED = -1000,
    ET_I2C_ERROR_DEV_NOT_INITIALIZED = -900,
    ET_I2C_ERROR_SPEED_BAD_VALUE = - 800,
    ET_I2C_OK = 0
};


ET_I2C_DEV_t* i2c_get_device(ET_I2C_CONTROLLER_t controllerId);
int i2c_init(ET_I2C_DEV_t* dev, ET_I2C_SPEED_t speed, uint8_t addr_slave);
int i2c_write(ET_I2C_DEV_t* dev, uint8_t regAddr, uint8_t* txDataBuff, uint8_t txDataCount);
int i2c_read(ET_I2C_DEV_t* dev, uint8_t regAddr, uint8_t* rxDataBuff, uint8_t rxDataCount);
int i2c_disable(ET_I2C_DEV_t* dev);


