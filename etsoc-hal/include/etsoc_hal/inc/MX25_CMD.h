/*
 * COPYRIGHT (c) 2010-2011 MACRONIX INTERNATIONAL CO., LTD
 * SPI Flash Low Level Driver (LLD) Sample Code
 *
 * SPI interface command hex code, type definition and function prototype.
 *
 * $Id: MX25_CMD.h,v 1.9 2011/03/17 01:50:16 modelqa Exp $
 */
#ifndef    __MX25_CMD_H__
#define    __MX25_CMD_H__

#include    "MX25_DEF.h"


#include    "api.h"

/*** MX25 series command hex code definition ***/
//ID comands
#define    FLASH_CMD_RDID      0x9F    //RDID (Read Identification)
#define    FLASH_CMD_RES       0xAB    //RES (Read Electronic ID)
#define    FLASH_CMD_REMS      0x90    //REMS (Read Electronic & Device ID)
#define    FLASH_CMD_REMS2     0xEF    //REMS2 (Read ID for 2 x I/O mode)
#define    FLASH_CMD_REMS4     0xDF    //REMS4 (Read ID for 4 x I/O mode)

//Register comands
#define    FLASH_CMD_WRSR      0x01    //WRSR (Write Status Register)
#define    FLASH_CMD_RDSR      0x05    //RDSR (Read Status Register)
#define    FLASH_CMD_WRSCUR    0x2F    //WRSCUR (Write Security Register)
#define    FLASH_CMD_RDSCUR    0x2B    //RDSCUR (Read Security Register)

//READ comands
#define    FLASH_CMD_READ        0x03    //READ (1 x I/O)
#define    FLASH_CMD_2READ       0xBB    //2READ (2 x I/O)
#define    FLASH_CMD_4READ       0xEB    //4READ (4 x I/O)
#define    FLASH_CMD_FASTREAD    0x0B    //FAST READ (Fast read data)
#define    FLASH_CMD_RDSFDP      0x5A    //RDSFDP (Read SFDP)

//Program comands
#define    FLASH_CMD_WREN     0x06    //WREN (Write Enable)
#define    FLASH_CMD_WRDI     0x04    //WRDI (Write Disable)
#define    FLASH_CMD_PP       0x02    //PP (page program)
#define    FLASH_CMD_4PP      0x38    //4PP (Quad page program)

//Erase comands
#define    FLASH_CMD_SE       0x20    //SE (Sector Erase)
#define    FLASH_CMD_BE32K    0x52    //BE32K (Block Erase 32kb)
#define    FLASH_CMD_BE       0xD8    //BE (Block Erase)
#define    FLASH_CMD_CE       0x60    //CE (Chip Erase) hex code: 60 or C7

//Mode setting comands
#define    FLASH_CMD_DP       0xB9    //DP (Deep Power Down)
#define    FLASH_CMD_RDP      0xAB    //RDP (Release form Deep Power Down)
#define    FLASH_CMD_ENSO     0xB1    //ENSO (Enter Secured OTP)
#define    FLASH_CMD_EXSO     0xC1    //EXSO  (Exit Secured OTP)
#define    FLASH_CMD_WPSEL    0x68    //WPSEL (Enable block protect mode)

//Reset comands

//Security comands
#define    FLASH_CMD_SBLK       0x36    //SBLK (Single Block Lock)
#define    FLASH_CMD_SBULK      0x39    //SBULK(Single Block Unlock)
#define    FLASH_CMD_RDBLOCK    0x3C    //RDBLOCK (Block Protect Read)
#define    FLASH_CMD_GBLK       0x7E    //GBLK (Gang Block Lock)
#define    FLASH_CMD_GBULK      0x98    //GBULK (Gang Block Unlock)

//Suspend/Resume comands

// Return Message
typedef enum {
    FlashOperationSuccess,
    FlashWriteRegFailed,
    FlashTimeOut,
    FlashIsBusy,
    FlashQuadNotEnable,
    FlashAddressInvalid,
}ReturnMsg;

// Flash status structure define
struct sFlashStatus{
    /* Mode Register:
     * Bit  Description
     * -------------------------
     *  7   RYBY enable
     *  6   Reserved
     *  5   Reserved
     *  4   Reserved
     *  3   Reserved
     *  2   Reserved
     *  1   Parallel mode enable
     *  0   QPI mode enable
    */
    uint32_t    ModeReg;
    BOOL     ArrangeOpt;
};

typedef struct sFlashStatus FlashStatus;

/* Basic functions */
void CS_High(et_handle_t spi_handle);
void CS_Low(et_handle_t spi_handle, uint32_t slave);
void InsertDummyCycle( uint32_t dummy_cycle );
void SendByte( uint32_t byte_value, uint32_t transfer_type );
uint32_t GetByte( uint32_t transfer_type );

/* Utility functions */
void Wait_Flash_WarmUp( void );
et_handle_t Initial_Spi(uint32_t spiSelect, uint32_t slaveSelect, uint32_t target, API_IP_PARAMS_t pIpParams);
BOOL WaitFlashReady( et_handle_t spi_handle, uint32_t ExpectTime, uint32_t targ );
BOOL WaitRYBYReady( uint32_t ExpectTime );
BOOL IsFlashBusy( et_handle_t spi_handle, uint32_t targ );
BOOL IsFlashQIO( void );
BOOL IsFlash4Byte( void );
void SendFlashAddr( uint32_t flash_address, uint32_t io_mode, BOOL addr_4byte_mode );
void dataLength(et_handle_t spi_handle, uint32_t length);
void transferMode(et_handle_t spi_handle, uint32_t mode);

/* Flash commands */
ReturnMsg CMD_RDID( uint32_t *Identification );
ReturnMsg CMD_RES( et_handle_t spi_handle, uint32_t *ElectricIdentification, uint32_t targ );
ReturnMsg CMD_REMS( uint32_t *REMS_Identification, FlashStatus *fsptr );
ReturnMsg CMD_REMS2( uint32_t *REMS_Identification, FlashStatus *fsptr );
ReturnMsg CMD_REMS4( uint32_t *REMS_Identification, FlashStatus *fsptr );

ReturnMsg CMD_RDSR( et_handle_t spi_handle, uint32_t *StatusReg, uint32_t targ  );
ReturnMsg CMD_WRSR( uint32_t UpdateValue );
ReturnMsg CMD_RDSCUR( uint32_t *SecurityReg );
ReturnMsg CMD_WRSCUR( void );

ReturnMsg CMD_READ( et_handle_t spi_handle, uint32_t flash_address, uint32_t *target_address, uint32_t byte_length, uint32_t targ   );
ReturnMsg CMD_2READ( uint32_t flash_address, uint32_t *target_address, uint32_t byte_length );
ReturnMsg CMD_4READ( uint32_t flash_address, uint32_t *target_address, uint32_t byte_length );
ReturnMsg CMD_FASTREAD( uint32_t flash_address, uint32_t *target_address, uint32_t byte_length );
ReturnMsg CMD_RDSFDP( uint32_t flash_address, uint32_t *target_address, uint32_t byte_length );

ReturnMsg CMD_WREN( et_handle_t spi_handle, uint32_t targ  );
ReturnMsg CMD_WRDI( void );
ReturnMsg CMD_PP( et_handle_t spi_handle, uint32_t flash_address, uint32_t *source_address, uint32_t byte_length, uint32_t targ  );
ReturnMsg CMD_4PP( uint32_t flash_address, uint32_t *source_address, uint32_t byte_length );

ReturnMsg CMD_SE( et_handle_t spi_handle, uint32_t flash_address, uint32_t targ  );
ReturnMsg CMD_BE32K( uint32_t flash_address );
ReturnMsg CMD_BE( uint32_t flash_address );
ReturnMsg CMD_CE( et_handle_t spi_handle, uint32_t targ  );

ReturnMsg CMD_DP( void );
ReturnMsg CMD_RDP( void );
ReturnMsg CMD_ENSO( void );
ReturnMsg CMD_EXSO( void );
ReturnMsg CMD_WPSEL( void );


ReturnMsg CMD_SBLK( uint32_t flash_address );
ReturnMsg CMD_SBULK( uint32_t flash_address );
ReturnMsg CMD_RDBLOCK( uint32_t flash_address, BOOL *protect_flag );
ReturnMsg CMD_GBLK( void );
ReturnMsg CMD_GBULK( void );
#endif    /* __MX25_CMD_H__ */

