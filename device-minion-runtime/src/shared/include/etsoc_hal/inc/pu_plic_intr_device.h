/**
 *  @Component      pu_plic
 *
 *  @Filename       $REPOROOT/dv/tests/ioshire/sw/inc/pu_plic_intr_device.h
 *
 *  @Description    Defines IDs for SPIO PLIC interrupt sources
 *
 *//*======================================================================== */



#ifndef __PU_PLIC_DEVICE_H
#define __PU_PLIC_DEVICE_H

#ifdef __cplusplus
extern "C"
{
#endif



#define PU_PLIC_NO_INTERRUPT_INTR_ID            0
#define PU_PLIC_I2C_INTR_ID                     1
#define PU_PLIC_SPI_INTR_ID                     2
#define PU_PLIC_UART0_INTR_ID                   3
#define PU_PLIC_GPIO_INTR_ID                    4
#define PU_PLIC_WDT_INTR_ID                     5
#define PU_PLIC_TIMER0_INTR_ID                  6
#define PU_PLIC_TIMER1_INTR_ID                  7
#define PU_PLIC_TIMER2_INTR_ID                  8
#define PU_PLIC_TIMER3_INTR_ID                  9
#define PU_PLIC_TIMER4_INTR_ID                 10
#define PU_PLIC_TIMER5_INTR_ID                 11
#define PU_PLIC_TIMER6_INTR_ID                 12
#define PU_PLIC_TIMER7_INTR_ID                 13
#define PU_PLIC_I3C_INTR_ID                    14
#define PU_PLIC_UART1_INTR_ID                  15
#define PU_PLIC_PCIE0_DMA_DONE0_INTR_ID        16
#define PU_PLIC_PCIE0_DMA_DONE1_INTR_ID        17
#define PU_PLIC_PCIE0_DMA_DONE2_INTR_ID        18
#define PU_PLIC_PCIE0_DMA_DONE3_INTR_ID        19
#define PU_PLIC_PCIE0_DMA_DONE4_INTR_ID        20
#define PU_PLIC_PCIE0_DMA_DONE5_INTR_ID        21
#define PU_PLIC_PCIE0_DMA_DONE6_INTR_ID        22
#define PU_PLIC_PCIE0_DMA_DONE7_INTR_ID        23
#define PU_PLIC_PCIE_MSI_INTR_ID               24
#define PU_PLIC_USB20_INTR_ID                  25
#define PU_PLIC_USB21_INTR_ID                  26
#define PU_PLIC_DMA_INTR_ID                    27
#define PU_PLIC_EMMC_INTR_ID                   28
#define PU_PLIC_PCIE_RADM_INTA_INTR_ID         29
#define PU_PLIC_PCIE_RADM_INTB_INTR_ID         30
#define PU_PLIC_PCIE_RADM_INTC_INTR_ID         31
#define PU_PLIC_PCIE_RADM_INTD_INTR_ID         32
#define PU_PLIC_PCIE_MESSAGE_INTR_ID           33
#define PU_PLIC_RESERVED0_INTR_ID              34
#define PU_PLIC_RESERVED1_INTR_ID              35
#define PU_PLIC_RESERVED2_INTR_ID              36
#define PU_PLIC_RESERVED3_INTR_ID              37
#define PU_PLIC_RESERVED4_INTR_ID              38
#define PU_PLIC_RESERVED5_INTR_ID              39
#define PU_PLIC_RESERVED6_INTR_ID              40
#define PU_PLIC_RESERVED7_INTR_ID              41

#define PU_PLIC_INTR_SRC_CNT                   42


#ifdef __cplusplus
}
#endif

#endif	/* __PU_PLIC_DEVICE_H*/



