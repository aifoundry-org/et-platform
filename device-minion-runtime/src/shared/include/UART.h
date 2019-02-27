#ifndef SPIO_UART_H
#define SPIO_UART_H

#include <stdint.h>

typedef struct SPIO_UART_MCR_s
{
    union {
        uint32_t R;
        struct {
            uint32_t DTR:1;
            uint32_t RTS:1;
            uint32_t OUT1:1;
            uint32_t OUT2:1;
            uint32_t LOOPBACK:1;
            uint32_t AFCE:1;
            uint32_t SIRE:1;
            uint32_t RESERVED:25;
        } B;
    };
} SPIO_UART_MCR_t;

typedef struct SPIO_UART_LCR_s
{
    union {
        uint32_t R;
        struct {
            uint32_t DLS:2;
            uint32_t STOP:1;
            uint32_t PEN:1;
            uint32_t EPS:1;
            uint32_t SP:1;
            uint32_t BC:1;
            uint32_t DLAB:1;
            uint32_t RESERVED:24;
        } B;
    };
} SPIO_UART_LCR_t;

typedef struct SPIO_UART_RBR_THR_DLL_s
{
    union {
        uint32_t R;
        struct {
            uint32_t RBR_THR_DLL:8;
            uint32_t RESERVED:24;
        } B;
    };
} SPIO_UART_RBR_THR_DLL_t;


typedef struct SPIO_UART_IER_DLH_s
{
    union {
        uint32_t R;
        struct {
            uint32_t ERBFI:1;
            uint32_t ETBEI:1;
            uint32_t ELSI:1;
            uint32_t EDSSI:1;
            uint32_t ELCOLR:1;
            uint32_t RESERVED0:2;
            uint32_t PTIME:1;
            uint32_t RESERVED1:24;
        } B_IER;
        struct {
            uint32_t DLH:8;
            uint32_t RESERVED:24;
        } B_DLH;
    };
} SPIO_UART_IER_DLH_t;

typedef struct SPIO_UART_FCR_s
{
    union {
        uint32_t R;
        struct {
            uint32_t FIFOE:1;
            uint32_t RFIFOR:1;
            uint32_t XFIFOR:1;
            uint32_t DMAM:1;
            uint32_t TET:1;
            uint32_t RT:2;
            uint32_t RESERVED:25;
        } B;
    };
} SPIO_UART_FCR_t;

typedef struct SPIO_UART_LSR_s
{
    union {
        uint32_t R;
        struct {
            uint32_t DR:1;
            uint32_t OE:1;
            uint32_t PE:1;
            uint32_t FE:1;
            uint32_t BI:1;
            uint32_t THRE:1;
            uint32_t TEMT:1;
            uint32_t RFE:1;
            uint32_t ADDR_RCVD:1;
            uint32_t RESERVED:23;
        } B;
    };
} SPIO_UART_LSR_t;

typedef struct SPIO_UART_DLF_s
{
    union {
        uint32_t R;
        struct {
            uint32_t DLF:4;
            uint32_t RESERVED:28;
        } B;
    };
} SPIO_UART_DLF_t;

typedef struct SPIO_UART_s
{
    SPIO_UART_RBR_THR_DLL_t RBR_THR_DLL; // 0X0
    SPIO_UART_IER_DLH_t IER_DLH;         // 0X4
    SPIO_UART_FCR_t FCR;                 // 0X8
    SPIO_UART_LCR_t LCR;                 // 0XC
    SPIO_UART_MCR_t MCR;                 // 0X10
    SPIO_UART_LSR_t LSR;                 // 0X14
    uint32_t unused[42];                 // 0x18 - 0xBC
    SPIO_UART_DLF_t DLF;                 // 0xC0
} SPIO_UART_t;

#endif // SPIO_UART_H
