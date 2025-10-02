#ifndef __REG_SPI_REGS_H__
#define __REG_SPI_REGS_H__

#include "spi_regs_macro.h"

struct spi_regs {
    volatile uint32_t ctrlr0;
    volatile uint32_t ctrlr1;
    volatile uint32_t ssiennr;
    volatile uint32_t mwcr;
    volatile uint32_t ser;
    volatile uint32_t baudr;
    volatile uint32_t txftlr;
    volatile uint32_t rxftlr;
    volatile uint32_t txflr;
    volatile uint32_t rxflr;
    volatile uint32_t sr;
    volatile uint32_t imr;
    volatile uint32_t isr;
    volatile uint32_t risr;
    volatile uint32_t txoicr;
    volatile uint32_t rxoicr;
    volatile uint32_t rxuicr;
    volatile uint32_t msticr;
    volatile uint32_t icr;
    volatile uint32_t dmacr;
    volatile uint32_t dmatdlr;
    volatile uint32_t dmardlr;
    volatile uint32_t idr;
    volatile uint32_t ssi_version_id;
    volatile uint32_t dr;
    volatile uint32_t rx_sample_dly;
    volatile uint32_t spi_ctrlr0;
    volatile uint32_t txd_drive_edge;
    volatile uint32_t rsvdg;
};


#endif /* __REG_SPI_REGS_H__*/
