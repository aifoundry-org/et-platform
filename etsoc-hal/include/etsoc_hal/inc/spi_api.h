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
 *  @Component      SPI
 *
 *  @Filename       spi_api.h
 *
 *  @Description    The SPI component contains API interface to SPI
 *
 *//*======================================================================== */

#ifndef __SPI_API_H
#define __SPI_API_H

#ifdef __cplusplus
extern
{
#endif

/* =============================================================================
 * EXPORTED DEFINITIONS
 * =============================================================================
 */ 


/* =============================================================================
 * EXPORTED TYPES
 * =============================================================================
 */


/*-------------------------------------------------------------------------*//**
 * @TYPE         SPI_CFG_t
 *
 * @BRIEF        Spi configuration structure
 *
 * @DESCRIPTION  struct which defines Spi configuration parameters
 *
 *//*------------------------------------------------------------------------ */ 
typedef struct spi_cfg 
{
    uint32_t txThreshold;
    uint32_t rxThreshold;
    uint32_t clockDivider;
    uint32_t frameSize;
    uint32_t transferMode;
    uint32_t numberDataFrames;
    uint32_t slaveSelect;
} SPI_CFG_t;



/*-------------------------------------------------------------------------*//**
 * @TYPE         SPI_API_t
 *
 * @BRIEF        Spi API structure
 *
 * @DESCRIPTION  struct which defines Spi API interface
 *               Consists of two configuration structure and control interface
 *
 *//*------------------------------------------------------------------------ */ 
typedef struct spi_api
{
    volatile struct spi_regs* reg;   
    SPI_CFG_t cfg;
    uint32_t   irqNo;   // SPI have single interupt line, need additional checks
                        // over rxApi and txApi
    uint32_t   error;
    API_CFG_t  txApi;
    API_CFG_t  rxApi;
} SPI_API_t;



/* =============================================================================
 * EXPORTED VARIABLES
 * =============================================================================
 */ 
uint32_t irqCnt;
uint32_t rxFifoFull;
uint32_t rxFifoOver;
uint32_t rxFifoUnder;
uint32_t txFifoEmpty;
uint32_t txFifoOver;

 
 /* =============================================================================
 * EXPORTED FUNCTIONS
 * =============================================================================
 */ 

/* ------------------------------------------------------------------------*//**
 * @FUNCTION      spi_open
 *
 * @BRIEF         Open SPI API
 *
 * @param[in]     uint32_t spiId             -> Spi Id from soc.h
 * @param[in]     API_IP_PARAMS_t *pIpParams -> Pointer to API_IP_PARAMS_t 
 *                                              structure holding IP instantiation 
 *                                              parameters
 *
 * @RETURNS       et_handle_t
 *
 * @DESCRIPTION   Open SPI API and initialize its structures
 *
 *//*------------------------------------------------------------------------ */
extern et_handle_t spi_open( uint32_t spiId, API_IP_PARAMS_t *pIpParams );



/* ------------------------------------------------------------------------*//**
 * @FUNCTION      spi_close
 *
 * @BRIEF         Close SPI API
 *
 * @param[in]     et_handle_t handle    -> Handle to opened spi IP
 * 
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Close SPI API and initialize its structures
 *
 *//*------------------------------------------------------------------------ */                                                 
extern et_status_t spi_close( et_handle_t handle );



/* ------------------------------------------------------------------------*//**
 * @FUNCTION      spi_configure
 *
 * @BRIEF         Configure SPI API
 *
 * @param[in]     et_handle_t handle    -> Handle to opened spi IP
 * 
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Configure SPI API and initialize its structures
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t spi_configure( et_handle_t handle );



/* ------------------------------------------------------------------------*//**
 * @FUNCTION      spi_write
 *
 * @BRIEF         Write SPI API
 *
 * @param[in]     et_handle_t handle    -> Handle to opened spi IP
 * @param[in]     void *txBuff          -> Transmit Buffer
 * @param[in]     uint32_t txCount      -> Number of elements to transfer
 * @param[in]     uint32_t blocking     -> Block return until transmit completes ( == true )
 *                                         or return immediately ( == false )
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   Write SPI API and initialize its structures
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t spi_write( et_handle_t handle, void *txBuff, uint32_t txCount, uint32_t blocking, uint32_t target );



/* ------------------------------------------------------------------------*//**
 * @FUNCTION      spi_read
 *
 * @BRIEF         Read SPI API
 *
 * @param[in]     et_handle_t handle    -> Handle to opened spi IP
 * @param[in]     void *rxBuff          -> Receive Buffer
 * @param[in]     uint32_t rxCount      -> Number of elements to receive
 * @param[in]     uint32_t blocking     -> Block return until receive completes ( == true )
 *                                         or return immediately ( == false )
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   Read SPI API and initialize its structures
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t spi_read( et_handle_t handle, void *rxBuff, uint32_t rxCount, uint32_t blocking, uint32_t target );



/* ------------------------------------------------------------------------*//**
 * @FUNCTION      spi_slaveSelect
 *
 * @BRIEF         Enabling slave
 *
 * @param[in]     et_handle_t handle    -> Handle to opened spi IP
 * @param[in]     uint32_t slaveNo      -> Number of slave we want to enable
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Write in SER registar to enable sending to correspodenting slave
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t spi_slaveSelect( et_handle_t handle, uint32_t slaveNo);



/* ------------------------------------------------------------------------*//**
 * @FUNCTION      spi_enableSSI
 *
 * @BRIEF         Enabling SSI 
 *
 * @param[in]     et_handle_t handle    -> Handle to opened spi IP
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Write in SSIENNR registar to enable SSI
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t spi_enableSSI( et_handle_t handle);



/* ------------------------------------------------------------------------*//**
 * @FUNCTION      spi_disableSSI
 *
 * @BRIEF         disabling SSI 
 *
 * @param[in]     et_handle_t handle    -> Handle to opened spi IP
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Write in SSIENNR registar to disable SSI
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t spi_disableSSI( et_handle_t handle);



/* ------------------------------------------------------------------------*//**
 * @FUNCTION      spi_enableSSI
 *
 * @BRIEF         Enabling SSI 
 *
 * @param[in]     et_handle_t handle    -> Handle to opened spi IP
 * @param[in]     uint32_t_t length     -> Number of data frames we want to receive
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Write in CTRLR1 registar number of data we want to receive in 
 *                EEPROM read mode
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t spi_dataFrames( et_handle_t handle, uint32_t length);



/* ------------------------------------------------------------------------*//**
 * @FUNCTION      spi_transferMode
 *
 * @BRIEF         Selecting Transfer mode 
 *
 * @param[in]     et_handle_t handle    -> Handle to opened spi IP
 * @param[in]     uint32_t_t mode       -> Transfer Mode
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Write in CTRLR0 registar to select transfer mode
 *                0 - Transmite & Receive
 *                1 - Transmite only
 *                2 - Receive only
 *                3 - EEPROM read 
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t spi_transferMode( et_handle_t handle, uint32_t mode);



/* ------------------------------------------------------------------------*//**
 * @FUNCTION      spi_enableIntr
 *
 * @BRIEF         Enabling interrupt 
 *
 * @param[in]     et_handle_t handle    -> Handle to opened spi IP
 * @param[in]     uint32_t_t irqNo      -> Number of interrupt
 * @param[in]     uint32_t_t irqMask    -> Mask for innterupt we want to enable
 * @param[in]     uint32_t_t target     -> Target for which we enable interrupt
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Enabling interrupts for selected IRQ line and write in IMR
 *                Mask for selected line
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t spi_enableIntr( et_handle_t handle, /*uint32_t irqNo,*/ uint32_t irqMask, uint32_t target);



/* ------------------------------------------------------------------------*//**
 * @FUNCTION      spi_readISR
 *
 * @BRIEF         Reading ISR 
 *
 * @param[in]     et_handle_t handle    -> Handle to opened spi IP
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   Read ISR to get which interrupt need to be service
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t spi_readISR( et_handle_t handle);



/* ------------------------------------------------------------------------*//**
 * @FUNCTION      spi_writeThreshold
 *
 * @BRIEF         Write Threshold value 
 *
 * @param[in]     et_handle_t handle    -> Handle to opened spi IP
 * @param[in]     uint32_t_t threshold  -> Threshold value
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Write in RXFTLR threshold value
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t writeThreshold( SPI_API_t *pSpi, uint32_t threshold);



/* ------------------------------------------------------------------------*//**
 * @FUNCTION      spi_clockDivider
 *
 * @BRIEF         Getting BAUD rate 
 *
 * @param[in]     et_handle_t handle    -> Handle to opened spi IP
 * @param[in]     uint32_t_t ssi_clock_frequency  -> Frequency on SSI
 * @param[in]     uint32_t_t frequency  -> Frequency we want to get
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Calculating value that need to be written in BAUDR
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t spi_clockDivider(uint32_t ssi_clock_frequency, uint32_t frequency);



/* ------------------------------------------------------------------------*//**
 * @FUNCTION      spi_readTxflr
 *
 * @BRIEF         Reading current level of tx fifo 
 *
 * @param[in]     et_handle_t handle    -> Handle to opened spi IP
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   Read from TXFLR to get level of TX Fifo
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t spi_readTxflr( et_handle_t handle);



/* ------------------------------------------------------------------------*//**
 * @FUNCTION      spi_readRxflr
 *
 * @BRIEF         Reading current level of rx fifo 
 *
 * @param[in]     et_handle_t handle    -> Handle to opened spi IP
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   Read from RXFLR to get level of RX Fifo
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t spi_readRxflr( et_handle_t handle);

uint32_t spiWriteData(  SPI_API_t *pSpi);
et_status_t spiWriteIrq( SPI_API_t *pSpi, uint32_t target);
uint32_t spiReadData(  SPI_API_t *pSpi);
et_status_t spiReadIrq( SPI_API_t *pSpi, uint32_t target);
void spi_isr(uint64_t irqNo);
void spi_init (void);


#ifdef __cplusplus
}
#endif

#endif	/* __SPI_API_H */


/*     <EOF>     */

