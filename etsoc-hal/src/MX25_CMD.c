/*
 * COPYRIGHT (c) 2010-2011 MACRONIX INTERNATIONAL CO., LTD
 * SPI Flash Low Level Driver (LLD) Sample Code
 *
 * SPI interface command set
 *
 * $Id: MX25U1633F_CMD.c,v 1.10 2011/07/01 07:34:17 modelqa Exp $
 */

#include "cpu.h" 
#include "soc.h" 
#include "macros.h" 
#include "ioshire_config.h" 

#include "tb.h"
#include "api.h"
#include "spi_api.h"
#include "spi_regs.h"
#include "inth.h"
#include "MX25_CMD.h"

/*
 --Common functions
 */

/*
 * Function:       Wait_Flash_WarmUp
 * Arguments:      None.
 * Description:    Wait some time until flash read / write enable.
 * Return Message: None.
 */
void Wait_Flash_WarmUp( void )
{
    uint32_t time_cnt = FlashFullAccessTime;
    while( time_cnt > 0 )
    {
        time_cnt--;
    }
}



BOOL WaitFlashReady( et_handle_t spi_handle, uint32_t ExpectTime, uint32_t targ  )
{

#ifndef NON_SYNCHRONOUS_IO
    uint32_t temp = 0;
    while( IsFlashBusy(spi_handle, targ ) ){
        if( temp > ExpectTime )
            return FALSE;

        temp = temp + 1;
    }

    return TRUE;
#else
    return TRUE;
#endif
}


/*
 * Function:       IsFlashBusy
 * Arguments:      None.
 * Description:    Check status register WIP bit.
 *                 If  WIP bit = 1: return TRUE ( Busy )
 *                             = 0: return FALSE ( Ready ).
 * Return Message: TRUE, FALSE
 */
BOOL IsFlashBusy( et_handle_t spi_handle, uint32_t targ )
{
    uint32_t  gDataBuffer;

    CMD_RDSR( spi_handle, &gDataBuffer, targ  );
    if( (gDataBuffer & FLASH_WIP_MASK)  == FLASH_WIP_MASK )
        return TRUE;
    else
        return FALSE;
}

ReturnMsg CMD_RES(et_handle_t spi_handle, uint32_t *ElectricIdentification, uint32_t targ)
{ 
    SPI_API_t *pSpi;
    pSpi = (SPI_API_t *)spi_handle;

    pSpi->reg->ssiennr  = 0x0;         // disable SSI
    pSpi->reg->ctrlr0   = 0x3 << 8;    // Setting EEPROM transfer mode
    pSpi->reg->ctrlr1   = 0;           // expecting one byte data
    pSpi->reg->rxftlr   = 0;           // seting rx threshold 	
    pSpi->reg->ssiennr  = 0x1;         // enable SSI


    uint32_t txBuff[4];
    uint32_t rxBuff[1];

    txBuff[0] = FLASH_CMD_RES;

    spi_write(spi_handle, &txBuff, 4, 1, targ);

    spi_read(spi_handle, &rxBuff, 1, 1, targ);

    *ElectricIdentification = rxBuff[0];


    return FlashOperationSuccess;
}



/*
 * Register  Command
 */

/*
 * Function:       CMD_RDSR
 * Arguments:      StatusReg, 8 bit buffer to store status register value
 * Description:    The RDSR instruction is for reading Status Register Bits.
 * Return Message: FlashOperationSuccess
 */
ReturnMsg CMD_RDSR(et_handle_t spi_handle, uint32_t *StatusReg, uint32_t targ)
{
    SPI_API_t *pSpi;
    pSpi = (SPI_API_t *)spi_handle;

    pSpi->reg->ssiennr  = 0x0;         // disable SSI
    pSpi->reg->ctrlr0   = 0x3 << 8;    // Setting EEPROM transfer mode
    pSpi->reg->ctrlr1   = 0;           // expecting one byte data
    pSpi->reg->rxftlr   = 0;           // seting rx threshold 	
    pSpi->reg->ssiennr  = 0x1;         // enable SSI

    uint32_t txBuff[1];
    uint32_t rxBuff[1];

    txBuff[0] = FLASH_CMD_RDSR;

    spi_write(spi_handle, &txBuff, 1, 1, targ);

    spi_read(spi_handle, &rxBuff, 1, 1, targ);

    *StatusReg = rxBuff[0];

    return FlashOperationSuccess;

}

/*
 * Read Command
 */

/*
 * Function:       CMD_READ
 * Arguments:      flash_address, 32 bit flash memory address
 *                 target_address, buffer address to store returned data
 *                 byte_length, length of returned data in byte unit
 * Description:    The READ instruction is for reading data out.
 * Return Message: FlashAddressInvalid, FlashOperationSuccess
 */
ReturnMsg CMD_READ(et_handle_t spi_handle, uint32_t flash_address, uint32_t *target_address, uint32_t byte_length, uint32_t targ )
{
    SPI_API_t *pSpi;
    pSpi = (SPI_API_t *)spi_handle;

    pSpi->reg->ssiennr  = 0x0;             // disable SSI
    pSpi->reg->ctrlr0   = 0x3 << 8;        // Setting EEPROM transfer mode
    pSpi->reg->ctrlr1   = byte_length -1;  // number of expexting data
    pSpi->reg->rxftlr   = byte_length/2;   // seting rx threshold 	
    pSpi->reg->ssiennr  = 0x1;             // enable SSI


    uint32_t txBuff[4];
    uint32_t rxBuff[byte_length];


    txBuff[0] = FLASH_CMD_READ;
    txBuff[1] = flash_address >> 16;
    txBuff[2] = flash_address >> 8;
    txBuff[3] = flash_address;

    spi_write(spi_handle, &txBuff, 4, 1, targ);


    spi_read(spi_handle, &rxBuff, byte_length, 1, targ);

    for (uint32_t i=0; i<byte_length; i++) {
        *(target_address + i) = rxBuff[i];
    }


    return FlashOperationSuccess;
}


/*
 * Program Command
 */

/*
 * Function:       CMD_WREN
 * Arguments:      None.
 * Description:    The WREN instruction is for setting
 *                 Write Enable Latch (WEL) bit.
 * Return Message: FlashOperationSuccess
 */
ReturnMsg CMD_WREN( et_handle_t spi_handle, uint32_t targ )
{
    SPI_API_t *pSpi;
    pSpi = (SPI_API_t *)spi_handle;

    pSpi->reg->ssiennr  = 0x0;             // disable SSI
    pSpi->reg->ctrlr0   = 0x0 << 8;        // Setting Transmite and Receive mode
    pSpi->reg->ssiennr  = 0x1;             // enable SSI


    uint32_t txBuff[1];
    txBuff[0] = FLASH_CMD_WREN;

    // Write Enable command = 0x06, Setting Write Enable Latch Bit
    spi_write(spi_handle, &txBuff, 1, 1, targ);


    return FlashOperationSuccess;
}



/*
 * Function:       CMD_PP
 * Arguments:      flash_address, 32 bit flash memory address
 *                 source_address, buffer address of source data to program
 *                 byte_length, byte length of data to programm
 * Description:    The PP instruction is for programming
 *                 the memory to be "0".
 *                 The device only accept the last 256 byte ( or 32 byte ) to program.
 *                 If the page address ( flash_address[7:0] ) reach 0xFF, it will
 *                 program next at 0x00 of the same page.
 *                 Some products have smaller page size ( 32 byte )
 * Return Message: FlashAddressInvalid, FlashIsBusy, FlashOperationSuccess,
 *                 FlashTimeOut
 */
ReturnMsg CMD_PP(et_handle_t spi_handle, uint32_t flash_address, uint32_t *source_address, uint32_t byte_length, uint32_t targ )
{
 
 
    uint32_t txBuff[4+byte_length];
    //uint32_t dataBuff[byte_length];

    txBuff[0] = FLASH_CMD_PP;
    txBuff[1] = flash_address >> 16;
    txBuff[2] = flash_address >> 8;
    txBuff[3] = flash_address;

    CMD_WREN(spi_handle, targ);

    for (uint32_t i=0; i<byte_length; i++) {
        txBuff[i + 4] = *(source_address + i);
    }

    spi_write(spi_handle, &txBuff, byte_length + 4, 1, targ);
    
    while(spi_readTxflr(spi_handle) != 0) {
        for (int i=0;i < 0x100; i++) {}    // Wait some time before check txflr buffer again
    };

    if (WaitFlashReady(spi_handle, PageProgramCycleTime, targ))
        return FlashOperationSuccess;
    else
        return FlashTimeOut;
}


/*
 * Erase Command
 */

/*
 * Function:       CMD_SE
 * Arguments:      flash_address, 32 bit flash memory address
 * Description:    The SE instruction is for erasing the data
 *                 of the chosen sector (4KB) to be "1".
 * Return Message: FlashAddressInvalid, FlashIsBusy, FlashOperationSuccess,
 *                 FlashTimeOut
 */
ReturnMsg CMD_SE(et_handle_t spi_handle, uint32_t flash_address, uint32_t targ )
{
    uint32_t txBuff[4];

    CMD_WREN(spi_handle, targ);

    txBuff[0] = FLASH_CMD_SE;
    txBuff[1] = flash_address >> 16;
    txBuff[2] = flash_address >> 8;
    txBuff[3] = flash_address;

    spi_write(spi_handle, &txBuff, 4, 1, targ);


    if (WaitFlashReady(spi_handle, SectorEraseCycleTime, targ))
        return FlashOperationSuccess;
    else
        return FlashTimeOut;

}


/*
 * Function:       CMD_CE
 * Arguments:      None.
 * Description:    The CE instruction is for erasing the data
 *                 of the whole chip to be "1".
 * Return Message: FlashIsBusy, FlashOperationSuccess, FlashTimeOut
 */
ReturnMsg CMD_CE( et_handle_t spi_handle, uint32_t targ )
{

    uint32_t txBuff[1];

    CMD_WREN(spi_handle, targ);

    txBuff[0] = FLASH_CMD_CE;

    spi_write(spi_handle, &txBuff, 1, 1, targ);


    if( WaitFlashReady( spi_handle, ChipEraseCycleTime, targ ) )
        return FlashOperationSuccess;
    else
        return FlashTimeOut;
}




