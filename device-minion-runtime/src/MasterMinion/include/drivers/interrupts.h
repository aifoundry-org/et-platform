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
************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       Header/Interface description for the Interrupts component
*
***********************************************************************/
#ifndef INTERRUPTS_DEFS_H
#define INTERRUPTS_DEFS_H

#include "common_defs.h"

#define WAIT_FOR_INTERRUPTS             asm volatile("wfi")
#define INTERRUPTS_DISABLE_SUPERVISOR   asm volatile("csrci sstatus, 0x2")
#define INTERRUPTS_ENABLE_SUPERVISOR    asm volatile("csrsi sstatus, 0x2");

typedef enum {
    PU_PLIC_NO_INTERRUPT_INTR = 0,
    PU_PLIC_I2C_INTR,
    PU_PLIC_SPI_INTR,
    PU_PLIC_UART0_INTR,
    PU_PLIC_GPIO_INTR,
    PU_PLIC_WDT_INTR,
    PU_PLIC_TIMER0_INTR,
    PU_PLIC_TIMER1_INTR,
    PU_PLIC_TIMER2_INTR,
    PU_PLIC_TIMER3_INTR,
    PU_PLIC_TIMER4_INTR,
    PU_PLIC_TIMER5_INTR,
    PU_PLIC_TIMER6_INTR,
    PU_PLIC_TIMER7_INTR,
    PU_PLIC_I3C_INTR,
    PU_PLIC_UART1_INTR,
    PU_PLIC_PCIE0_DMA_DONE0_INTR,
    PU_PLIC_PCIE0_DMA_DONE1_INTR,
    PU_PLIC_PCIE0_DMA_DONE2_INTR,
    PU_PLIC_PCIE0_DMA_DONE3_INTR,
    PU_PLIC_PCIE0_DMA_DONE4_INTR,
    PU_PLIC_PCIE0_DMA_DONE5_INTR,
    PU_PLIC_PCIE0_DMA_DONE6_INTR,
    PU_PLIC_PCIE0_DMA_DONE7_INTR,
    PU_PLIC_PCIE_MSI_INTR,
    PU_PLIC_USB20_INTR,
    PU_PLIC_USB21_INTR,
    PU_PLIC_DMA_INTR,
    PU_PLIC_EMMC_INTR,
    PU_PLIC_PCIE_RADM_INTA_INTR,
    PU_PLIC_PCIE_RADM_INTB_INTR,
    PU_PLIC_PCIE_RADM_INTC_INTR,
    PU_PLIC_PCIE_RADM_INTD_INTR,
    PU_PLIC_PCIE_MESSAGE_INTR,
    PU_PLIC_RESERVED0_INTR,
    PU_PLIC_RESERVED1_INTR,
    PU_PLIC_RESERVED2_INTR,
    PU_PLIC_RESERVED3_INTR,
    PU_PLIC_RESERVED4_INTR,
    PU_PLIC_RESERVED5_INTR,
    PU_PLIC_RESERVED6_INTR,
    PU_PLIC_RESERVED7_INTR,
    PU_PLIC_INTR_CNT
} interrupt_t;

/*! \fn void Interrupt_Init(void)
    \brief Initialize Interrupts
    \param None
*/
void Interrupt_Init(void);

/*! \fn void Interrupt_Enable(interrupt_t interrupt, uint32_t priority, void (*isr)(void))
    \brief Enable an Interrupt
    \param [in] Interrupt vector
    \param [in] Interrupt priority
    \param [in] Function pointer to interrupt service routine
*/
void Interrupt_Enable(interrupt_t interrupt, uint32_t priority, void (*isr)(void));

/*! \fn void Interrupt_Disable(interrupt_t interrupt)
    \brief Disable an Interrupt
    \param [in] Interrupt vector
*/
void Interrupt_Disable(interrupt_t interrupt);


#endif /* INTERRUPTS_DEFS_H */