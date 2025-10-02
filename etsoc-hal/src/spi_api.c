/*-------------------------------------------------------------------------
* Copyright (C) 2018, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies or
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

/**
* @file $Id$
* @version $Release$
* @date $Date$
* @author
*
* @brief spi_api.c
*/
/**
 *  @Component    SPI
 *
 *  @Filename     spi_api.c
 *
 *  @Description  SPI Driver
 *
 *//*======================================================================== */



/* =============================================================================
 * STANDARD INCLUDE FILES
 * =============================================================================
 */
#include "cpu.h"
#include "soc.h"
#include "et.h"
#include "api.h"
#include "spi_api.h"
#include "spi_regs.h"
#include "inth.h"

/* =============================================================================
 * GLOBAL VARIABLES DECLARATIONS
 * =============================================================================
 */
#define NUM_SPI             3
#define TX_FIFO_DEPTH       0x100


SPI_API_t spiApi[NUM_SPI];
/* =============================================================================
 * LOCAL TYPES AND DEFINITIONS
 * =============================================================================
 */

/* =============================================================================
 * LOCAL VARIABLES DECLARATIONS
 * =============================================================================
 */

 /* ============================================================================
 * LOCAL FUNCTIONS PROTOTYPES
 * =============================================================================
 */

/*==================== Function Separator =============================*/
uint32_t spiWriteData(  SPI_API_t *pSpi)
{
    uint32_t txCount = 0;
    uint32_t *txBuff;
    uint32_t writeCnt;
    uint32_t txFifoLvl;

    txBuff = (uint32_t *)pSpi -> txApi.pBuff;

    txFifoLvl = pSpi -> reg -> txflr;
    writeCnt = TX_FIFO_DEPTH - txFifoLvl;
    while ((writeCnt --) && (pSpi -> txApi.transferCount < pSpi -> txApi.count)) {
        pSpi -> reg -> dr = txBuff[pSpi -> txApi.transferCount ++];
        txCount ++;
    }
    return txCount;
}   /*  spiWriteData()  */

/*==================== Function Separator =============================*/
et_status_t spiWriteIrq( SPI_API_t *pSpi, uint32_t target)
{
    et_status_t est;

    est = ET_OK;

    if (pSpi -> irqNo == 0) {
        est = ET_FAIL;
        return est;
    }

    est = inth_enableInterrupt( pSpi -> irqNo, target );

    pSpi -> reg -> imr |= 0x00000001;    //Enable TX Interrupts

    return est;
}   /*  spiWriteIrq()   */

/*==================== Function Separator =============================*/
uint32_t spiReadData(  SPI_API_t *pSpi)
{
    uint32_t rxCount = 0;
    uint32_t *rxBuff;
    uint32_t readCnt;

    rxBuff = (uint32_t *)pSpi -> rxApi.pBuff;

    readCnt = pSpi -> reg -> rxflr;
    while (readCnt --) {
        rxBuff[pSpi -> rxApi.transferCount ++] = pSpi -> reg -> dr;
        rxCount ++;
    }

    return rxCount;
}   /*  spiReadData()  */

/*==================== Function Separator =============================*/
et_status_t spiReadIrq( SPI_API_t *pSpi, uint32_t target)
{
    et_status_t est;

    est = ET_OK;

    if (pSpi -> irqNo == 0) {
        est = ET_FAIL;
        return est;
    }

    est = inth_enableInterrupt( pSpi -> irqNo, target );

    pSpi -> reg -> imr |= 0x00000014;    //Enable RX Interrupts

    return est;
}   /*  spiReadIrq()    */

/*==================== Function Separator =============================*/
void spi_isr(uint64_t irqNo)
{
    SPI_API_t *pSpi;
    uint32_t irqSrc;
    uint32_t th;

    pSpi = &spiApi[0];
    for (int i=0; i<NUM_SPI; i++) {
        if (pSpi -> irqNo == irqNo) {
            break;
        }
        pSpi++;
    }
    irqSrc = pSpi -> reg -> isr;

    if (irqSrc & 0x10) {
        rxFifoFull = 1;
        spiReadData( pSpi);
        if (pSpi -> reg -> rxflr <= pSpi -> reg -> rxftlr && ((pSpi -> rxApi.count - pSpi ->rxApi.transferCount) < 0x100)) {
            th = pSpi -> reg -> rxflr/2;
            writeThreshold(pSpi, th);
        }
        if( pSpi->rxApi.count == pSpi->rxApi.transferCount ) {
            pSpi -> rxApi.status = API_STATUS_COMPLETED;
            pSpi -> reg -> imr &= 0xFFFFFFEF;
            irqCnt++;
        }
    }
    else if (irqSrc & 0x1) {
        txFifoEmpty = 1;
        spiWriteData( pSpi );
        if( pSpi->txApi.count == pSpi->txApi.transferCount ) {
            pSpi -> reg -> imr &= 0xFFFFFFFE;
            pSpi -> txApi.status = API_STATUS_COMPLETED;
            irqCnt++;
        }
    }

    else {
        switch (irqSrc) {
            case 2:
                pSpi -> txApi.status = API_STATUS_COMPLETED;
                irqCnt++;
                txFifoOver = 1;
                break;
            case 4:
                pSpi -> txApi.status = API_STATUS_COMPLETED;
                irqCnt++;
                rxFifoUnder = 1;
                break;
            case 8:
                pSpi -> txApi.status = API_STATUS_COMPLETED;
                irqCnt++;
                rxFifoOver = 1;
                break;
        }
    }


}   /* spi_isr()    */

/* =============================================================================
 * FUNCTIONS
 * =============================================================================
 */

/*==================== Function Separator =============================*/
void spi_init (void)
{
    uint32_t i;

    for (i=0; i<NUM_SPI; i++) {
        spiApi[i].reg = NULL;
    }
}   /*  spi_init()  */

/*==================== Function Separator =============================*/
et_handle_t spi_open( uint32_t spiId, API_IP_PARAMS_t *pIpParams )
{
    et_status_t est;
    est = ET_OK;

    if (spiId > (NUM_SPI -1)) {
        return NULL;
    }

    if (spiApi[spiId].reg != NULL) {
        return NULL;
    }

    spiApi[spiId].reg           = (volatile struct spi_regs*) ((uint64_t)pIpParams -> baseAddress);

    spiApi[spiId].txApi.mode    = pIpParams -> txMode;
    spiApi[spiId].txApi.irq     = pIpParams -> txIrq;
    spiApi[spiId].txApi.status  = API_STATUS_IDLE;

    spiApi[spiId].rxApi.mode    = pIpParams -> rxMode;
    spiApi[spiId].rxApi.irq     = pIpParams -> rxIrq;
    spiApi[spiId].rxApi.status  = API_STATUS_IDLE;


    spiApi[spiId].error         = 0;

    if( spiApi[spiId].irqNo ) {
        est = inth_setupInterrupt( spiApi[spiId].irqNo, spi_isr, 5 );

        if (est != ET_OK) {
            return NULL;
        }
    }

    return  (et_handle_t)&spiApi[spiId];

}   /*  spi_open()  */

/*==================== Function Separator =============================*/
et_status_t spi_close( et_handle_t handle )
{
    SPI_API_t *pSpi;
    et_status_t est;

    est = ET_OK;
    pSpi = (SPI_API_t *)handle;

    pSpi -> reg -> ssiennr  = 0x0;

    pSpi -> reg = NULL;

    return est;

}   /*  spi_close()  */

/*==================== Function Separator =============================*/
et_status_t spi_configure( et_handle_t handle )
{
    SPI_API_t *pSpi;
    et_status_t est;

    est = ET_OK;
    pSpi = (SPI_API_t *)handle;

    if (pSpi->txApi.status != API_STATUS_IDLE) {
        est = ET_FAIL;
        return est;
    }

    pSpi -> reg -> ssiennr  = 0x0;
    pSpi -> reg -> baudr    = pSpi -> cfg.clockDivider;
    pSpi -> reg -> txftlr   = pSpi -> cfg.txThreshold;
    pSpi -> reg -> rxftlr   = pSpi -> cfg.rxThreshold;
    pSpi -> reg -> ctrlr0   = pSpi -> cfg.frameSize  << 16;
    pSpi -> reg -> ctrlr0   = pSpi -> cfg.transferMode << 8;
    pSpi -> reg -> ctrlr1   = pSpi -> cfg.numberDataFrames;
    pSpi -> reg -> ser      = pSpi -> cfg.slaveSelect;
    pSpi -> reg -> imr      = 0x0;
    pSpi -> reg -> ssiennr  = 0x1;

    return est;

}   /*  spi_configure()   */



/*==================== Function Separator =============================*/
uint32_t spi_write( et_handle_t handle, void *txBuff, uint32_t txCount, uint32_t blocking, uint32_t target )
{
    SPI_API_t *pSpi;
    uint32_t dataSent;

    pSpi = (SPI_API_t *) handle;

    pSpi -> txApi.pBuff             = (void *)txBuff;
    pSpi -> txApi.count             = txCount;
    pSpi -> txApi.status            = API_STATUS_RUN;
    pSpi -> txApi.transferCount     = 0;
    pSpi -> txApi.errorStatus       = 0;
    pSpi -> txApi.irqErrorStatus    = 0;

    switch (pSpi -> txApi.mode) {
        case API_MODE_POLING:
            dataSent = spiWriteData( pSpi);
            pSpi -> txApi.status = (dataSent == pSpi -> txApi.count) ? API_STATUS_COMPLETED : API_STATUS_ERROR;
            return (pSpi -> txApi.count);
        case API_MODE_IRQ:
            spiWriteIrq( pSpi, target);
            if (blocking) {
                while ((pSpi->reg->sr) & 0x01);
                return (pSpi -> txApi.transferCount);
            }
            return API_NON_BLOCKING;
    }
    return API_NON_BLOCKING;

}   /*  spi_write()     */

/*==================== Function Separator =============================*/
uint32_t spi_read( et_handle_t handle, void *rxBuff, uint32_t rxCount, uint32_t blocking, uint32_t target )
{
    SPI_API_t *pSpi;
    uint32_t dataReceive;

    pSpi = (SPI_API_t *) handle;

    pSpi -> rxApi.pBuff             = (void *)rxBuff;
    pSpi -> rxApi.count             = rxCount;
    pSpi -> rxApi.status            = API_STATUS_RUN;
    pSpi -> rxApi.transferCount     = 0;
    pSpi -> rxApi.errorStatus       = 0;
    pSpi -> rxApi.irqErrorStatus    = 0;

    switch (pSpi -> rxApi.mode) {
        case API_MODE_POLING:
            dataReceive = spiReadData( pSpi);
            pSpi -> rxApi.status = (dataReceive == pSpi -> rxApi.count) ? API_STATUS_COMPLETED : API_STATUS_ERROR;
            return (pSpi -> rxApi.count);
        case API_MODE_IRQ:
            spiReadIrq( pSpi, target);
            if (blocking) {
                while ((pSpi->reg->sr) & 0x01);
                return (pSpi -> rxApi.transferCount);
            }
            return API_NON_BLOCKING;
    }
    return API_NON_BLOCKING;
    
}   /*  spi_read()  */

/*==================== Function Separator =============================*/
uint32_t spi_clockDivider( uint32_t ssi_clock_frequency, uint32_t frequency)
{
    uint32_t baud;

    baud = ssi_clock_frequency / frequency;
    return baud;

}   /*  spi_clockDivider()   */

/*==================== Function Separator =============================*/
et_status_t writeThreshold( SPI_API_t *pSpi, uint32_t threshold)
{
    et_status_t est;

    est = ET_OK;

    pSpi -> reg -> rxftlr = threshold;

    return est;
}   /*  spiReadIrq()    */

/*==================== Function Separator =============================*/
et_status_t spi_slaveSelect( et_handle_t handle, uint32_t slaveNo)
{
    SPI_API_t *pSpi;
    et_status_t est;

    est = ET_OK;
    pSpi = (SPI_API_t *)handle;

    pSpi -> reg -> ser = slaveNo;

    return est;

}   /*  spi_slaveSelect()   */

/*==================== Function Separator =============================*/
et_status_t spi_disableSSI( et_handle_t handle)
{
    SPI_API_t *pSpi;
    et_status_t est;

    est = ET_OK;
    pSpi = (SPI_API_t *)handle;

    pSpi -> reg -> ssiennr = 0;

    return est;

}   /*  spi_disableSSI()   */

/*==================== Function Separator =============================*/
et_status_t spi_enableIntr( et_handle_t handle, /*uint32_t irqNo,*/ uint32_t irqMask, uint32_t target)
{
    SPI_API_t *pSpi;
    et_status_t est;
    est=ET_OK;
    pSpi = (SPI_API_t *)handle;

    if (pSpi -> irqNo == 0) {
        est = ET_FAIL;
        return est;
    }

    est = inth_enableInterrupt( pSpi -> irqNo, target );

    pSpi -> reg -> imr = irqMask;

    return est;

}   /*  spi_enableIntr()   */

uint32_t spi_readISR( et_handle_t handle)
{
    uint32_t status;
    SPI_API_t *pSpi;

    pSpi = (SPI_API_t *)handle;


    status = pSpi -> reg -> isr;

    return status;

}   /*  spi_readISR()   */

/*==================== Function Separator =============================*/
et_status_t spi_enableSSI( et_handle_t handle)
{
    SPI_API_t *pSpi;
    et_status_t est;

    est = ET_OK;
    pSpi = (SPI_API_t *)handle;

    pSpi -> reg -> ssiennr = 1;

    return est;

}   /*  spi_enableSSI()   */

/*==================== Function Separator =============================*/
et_status_t spi_dataFrames( et_handle_t handle, uint32_t dataFrames)
{
    SPI_API_t *pSpi;
    et_status_t est;

    est = ET_OK;
    pSpi = (SPI_API_t *)handle;

    pSpi -> reg -> ctrlr1 = dataFrames - 1;

    return est;

}   /*  spi_dataFrames()   */

/*==================== Function Separator =============================*/
uint32_t spi_readTxflr( et_handle_t handle)
{
    SPI_API_t *pSpi;
    uint32_t txflr;

    pSpi = (SPI_API_t *)handle;

    txflr = pSpi -> reg -> txflr;

    return txflr;

}   /*  spi_readTxflr()   */

/*==================== Function Separator =============================*/
uint32_t spi_readRxflr( et_handle_t handle)
{
    SPI_API_t *pSpi;
    uint32_t rxflr;

    pSpi = (SPI_API_t *)handle;

    rxflr = pSpi -> reg -> rxflr;

    return rxflr;

}   /*  spi_readRxflr()   */

/*==================== Function Separator =============================*/
et_status_t spi_transferMode( et_handle_t handle, uint32_t mode)
{
    SPI_API_t *pSpi;
    et_status_t est;

    est = ET_OK;
    pSpi = (SPI_API_t *)handle;

    pSpi -> reg -> ctrlr0 = mode << 8;

    return est;

}   /*  spi_transferMode()   */





/*****     < EOF >     *****/

