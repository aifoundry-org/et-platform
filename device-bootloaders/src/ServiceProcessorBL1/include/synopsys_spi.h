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

#ifndef __SYNOPSYS_SPI_H__
#define __SYNOPSYS_SPI_H__

#include <stdint.h>

#define SPI_SSI_NUM_SLAVES 2
#define SPI_TX_FIFO_MAX_DEPTH 128
#define SPI_RX_FIFO_MAX_DEPTH 128
#define TX_ABW 8

#define SPI_CTRLR0_FRAME_08BITS 0x07

#define SPI_CTRLR0_FRF_MOTOROLA_SPI 0x00
#define SPI_CTRLR0_FRF_TEXAS_SSP    0x01
#define SPI_CTRLR0_FRF_NS_MICROWIRE 0x02

#define SPI_CTRLR0_SCPH_MIDDLE  0x0
#define SPI_CTRLR0_SCPH_START   0x1

#define SPI_CTRLR0_SCPOL_SCLK_LOW  0x0
#define SPI_CTRLR0_SCPOL_SCLK_HIGH 0x1

#define SPI_CTRLR0_SRL_NORMAL_MODE  0x0
#define SPI_CTRLR0_SRL_TESTING_MODE 0x1

#define SPI_CTRLR0_TMOD_TX_AND_RX 0x0
#define SPI_CTRLR0_TMOD_TX_ONLY   0x1
#define SPI_CTRLR0_TMOD_RX_ONLY   0x2

#define SPI_CGTRLR0_SPI_PERF_STD    0x0
#define SPI_CGTRLR0_SPI_PERF_DUAL   0x1
#define SPI_CGTRLR0_SPI_PERF_QUAD   0x2
#define SPI_CGTRLR0_SPI_PERF_OCTAL  0x3


typedef struct SPI_CTRLR0_s {
    union {
        uint32_t R;
        struct {
            uint32_t DFS : 4;
            uint32_t FRF : 2;
            uint32_t SCPH : 1;
            uint32_t SCPOL : 1;
            uint32_t TMOD : 2;
            uint32_t SLV_OE : 1;
            uint32_t SRL : 1;
            uint32_t CFS : 4;
            uint32_t DFS_32 : 5;
            uint32_t SPI_FRF : 2;
            uint32_t RSVD_CTRLR0_23 : 1;
            uint32_t SSTE : 1;
            uint32_t RSVD_CTRLR0 : 7;
        } B;
    };
} SPI_CTRLR0_t;

typedef struct SPI_CTRLR1_s {
    union {
        uint32_t R;
        struct {
            uint32_t NDF : 16;
            uint32_t RSVD_CTRLR1 : 16;
        } B;
    };
} SPI_CTRLR1_t;

typedef struct SPI_SSIENR_s {
    union {
        uint32_t R;
        struct {
            uint32_t SSI_EN : 1;
            uint32_t RSVD_SSIENR : 31;
        } B;
    };
} SPI_SSIENR_t;

typedef struct SPI_MWCR_s {
    union {
        uint32_t R;
        struct {
            uint32_t MWMOD : 1;
            uint32_t MOD : 1;
            uint32_t MHS : 1;
            uint32_t RSVD_MWCR : 29;
        } B;
    };
} SPI_MWCR_t;

typedef struct SPI_SER_s {
    union {
        uint32_t R;
        struct {
            uint32_t SER : SPI_SSI_NUM_SLAVES;
            uint32_t RSVD_SER : (32 - SPI_SSI_NUM_SLAVES);
        } B;
    };
} SPI_SER_t;

typedef struct SPI_BAUDR_s {
    union {
        uint32_t R;
        struct {
            uint32_t SCKDV : 16;
            uint32_t RSVD_BAUDR : 16;
        } B;
    };
} SPI_BAUDR_t;

typedef struct SPI_TXFTLR_s {
    union {
        uint32_t R;
        struct {
            uint32_t TFT : (TX_ABW + 1);
            uint32_t RSVD_TFXTLR : (32 - TX_ABW - 1);
        } B;
    };
} SPI_TXFTLR_t;

typedef struct SPI_RXFTLR_s {
    union {
        uint32_t R;
        struct {
            uint32_t RFT : (TX_ABW + 1);
            uint32_t RSVD_RXFTLR : (32 - TX_ABW - 1);
        } B;
    };
} SPI_RXFTLR_t;

typedef struct SPI_TXFLR_s {
    union {
        uint32_t R;
        struct {
            uint32_t TXTFL : (TX_ABW + 1);
            uint32_t RSVD_TFXTLR : (32 - TX_ABW - 1);
        } B;
    };
} SPI_TXFLR_t;

typedef struct SPI_RXFLR_s {
    union {
        uint32_t R;
        struct {
            uint32_t RXTFL : (TX_ABW + 1);
            uint32_t RSVD_RXFTLR : (32 - TX_ABW - 1);
        } B;
    };
} SPI_RXFLR_t;

typedef struct SPI_SR_s {
    union {
        uint32_t R;
        struct {
            uint32_t BUSY : 1;
            uint32_t TFNF : 1;
            uint32_t TFE : 1;
            uint32_t RFNE : 1;
            uint32_t RFF : 1;
            uint32_t TXE : 1;
            uint32_t DCOL : 1;
            uint32_t RSVD_SR : 25;
        } B;
    };
} SPI_SR_t;

typedef struct SPI_IMR_s {
    union {
        uint32_t R;
        struct {
            uint32_t TXEIM : 1;
            uint32_t TXOIM : 1;
            uint32_t RXUIM : 1;
            uint32_t RXOIM : 1;
            uint32_t RXFIM : 1;
            uint32_t MSTIM : 1;
            uint32_t RSVD_IMR : 26;
        } B;
    };
} SPI_IMR_t;

typedef struct SPI_ISR_s {
    union {
        uint32_t R;
        struct {
            uint32_t TXEIS : 1;
            uint32_t TXOIS : 1;
            uint32_t RXUIS : 1;
            uint32_t RXOIS : 1;
            uint32_t RXFIS : 1;
            uint32_t MSTIS : 1;
            uint32_t RSVD_ISR : 26;
        } B;
    };
} SPI_ISR_t;

typedef struct SPI_RISR_s {
    union {
        uint32_t R;
        struct {
            uint32_t TXEIR : 1;
            uint32_t TXOIR : 1;
            uint32_t RXUIR : 1;
            uint32_t RXOIR : 1;
            uint32_t RXFIR : 1;
            uint32_t MSTIR : 1;
            uint32_t RSVD_RISR : 26;
        } B;
    };
} SPI_RISR_t;

typedef struct SPI_TXOICR_s {
    union {
        uint32_t R;
        struct {
            uint32_t TXOICR : 1;
            uint32_t RSVD_TXOICR : 31;
        } B;
    };
} SPI_TXOICR_t;

typedef struct SPI_RXOICR_s {
    union {
        uint32_t R;
        struct {
            uint32_t RXOICR : 1;
            uint32_t RSVD_RXOICR : 31;
        } B;
    };
} SPI_RXOICR_t;

typedef struct SPI_RXUICR_s {
    union {
        uint32_t R;
        struct {
            uint32_t RXUICR : 1;
            uint32_t RSVD_RXUICR : 31;
        } B;
    };
} SPI_RXUICR_t;

typedef struct SPI_MSTICR_s {
    union {
        uint32_t R;
        struct {
            uint32_t MSTICR : 1;
            uint32_t RSVD_MSTICR : 31;
        } B;
    };
} SPI_MSTICR_t;

typedef struct SPI_ICR_s {
    union {
        uint32_t R;
        struct {
            uint32_t ICR : 1;
            uint32_t RSVD_ICR : 31;
        } B;
    };
} SPI_ICR_t;
/*
typedef struct SPI_DMACR_s {
    union {
        uint32_t R;
        struct {
            uint32_t RDMAE : 1;
            uint32_t TDMAE : 1;
            uint32_t RSVD_DMACR : 30;
        } B;
    };
} SPI_DMACR_t;
*/
typedef struct SPI_s {
    SPI_CTRLR0_t CTRLR0;        // 00
    SPI_CTRLR1_t CTRLR1;        // 04
    SPI_SSIENR_t SSIENR;        // 08
    SPI_MWCR_t MWCR;            // 0C
    SPI_SER_t SER;              // 10
    SPI_BAUDR_t BAUDR;          // 14
    SPI_TXFTLR_t TXFTLR;        // 18
    SPI_RXFTLR_t RXFTLR;        // 1C
    SPI_TXFLR_t TXFLR;          // 20
    SPI_RXFLR_t RXFLR;          // 24
    SPI_SR_t SR;                // 28
    SPI_IMR_t IMR;              // 2C
    SPI_ISR_t ISR;              // 30
    SPI_RISR_t RISR;            // 34
    SPI_TXOICR_t TXOICR;        // 38
    SPI_RXOICR_t RXOICR;        // 3C
    SPI_RXUICR_t RXUICR;        // 40
    SPI_MSTICR_t MSTICR;        // 44
    SPI_ICR_t ICR;              // 48
    uint32_t DMACR;             // 4C
    uint32_t DMATDLR;           // 50
    uint32_t DMARDLR;           // 54
    uint32_t IDR;               // 58
    uint32_t SSI_VERSION_ID;    // 5C
    uint32_t DRx[36];           // 60..EC
    uint32_t RX_SAMPLE_DLY;     // F0
    uint32_t SPI_CTRLR0;        // F4
    uint32_t TXD_DRIVE_EDGE;    // F8
    uint32_t RSVD;              // FC
} SPI_t;

#endif
