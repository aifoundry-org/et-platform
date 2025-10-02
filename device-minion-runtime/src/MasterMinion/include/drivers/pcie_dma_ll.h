#ifndef PCIE_DMA_LL_H
#define PCIE_DMA_LL_H

#include <stdint.h>
#include <assert.h>

#include "system/layout.h"

typedef struct {
    union {
        struct {
            /* Description:                                                   */
            /**
             *    Cycle Bit (CB). Used in linked list mode only. It is used to
             *    synchronize the producer (software) and the consumer (DMA).
             *    For more details, see "PCS-CCS-CB-TCB Producer-Consumer
             *    Synchronization".
             *    The DMA loads this field with the CB of the linked list
             *    element.
            */
            uint32_t CB : 1; /* bit 0; R/W; 0x0 */

            /* Description:                                                   */
            /**
             *    Toggle Cycle Bit (TCB). Indicates to the DMA to toggle its
             *    interpretation of the CB. Used in linked list mode only. It is
             *     used to synchronize the producer (software) and the consumer
             *    (DMA). For more details, see "PCS-CCS-CB-TCB Producer-Consumer
             *     Synchronization".
             *    The DMA loads this field with the TCB of the linked list
             *    element.
             *    this field is not defined in a data LL element.
            */
            uint32_t TCB : 1; /* bit 1; R/W; 0x0 */

            /* Description:                                                   */
            /**
             *    Load Link Pointer (LLP). Used in linked list mode only.
             *    Indicates that this linked list element is a link element, and
             *     its LL element pointer dwords are pointing to the next
             *    (non-contiguous) element.
             *    The DMA loads this field with the LLP of the linked list
             *    element.
            */
            uint32_t LLP : 1; /* bit 2; R/W; 0x0 */

            /* Source filename: DWC_pcie_dbi_cpcie_usp_4x8.csr, line: 110126  */
            /* Description:                                                   */
            /**
             *    Local Interrupt Enable (LIE). You must set this bit to enable
             *    the generation of the Done or Abort Local interrupts. For more
             *     details, see "Interrupts and Error Handling".
             *    In LL mode, the DMA overwrites this with the LIE of the LL
             *    element. The LIE bit in a LL element only enables the Done
             *    interrupt. In non-LL mode, the LIE bit enables the Done and
             *    Abort interrupts.
             *    This field is not defined in a link LL element.
         */
            uint32_t LIE : 1; /* bit 3; R/W; 0x0 */

            /* Source filename: DWC_pcie_dbi_cpcie_usp_4x8.csr, line: 110147  */
            /* Description:                                                   */
            /**
             *    Remote Interrupt Enable (RIE). You must set this bit to enable
             *     the generation of the Done or Abort Remote interrupts. For
             *    more details, see "Interrupts and Error Handling".
             *    In LL mode, the DMA overwrites this with the RIE of the LL
             *    element. The RIE bit in a LL element only enables the Done
             *    interrupt. In non-LL mode, the RIE bit enables the Done and
             *    Abort interrupts.
             *    This field is not defined in a link LL element.
             */
            uint32_t RIE : 1; /* bit 4; R/W; 0x0 */
            uint32_t reserved : 27;
        } B;
        uint32_t R;
    };
} __attribute__((__packed__)) transfer_list_ctrl_t;

static_assert(sizeof(transfer_list_ctrl_t) == 4, "invalid size");

typedef struct {
    transfer_list_ctrl_t ctrl;
    uint32_t size;
    uint64_t sar;
    uint64_t dar;
} data_elem_t;

static_assert(sizeof(data_elem_t) == DMA_PER_ENTRY_SIZE, "invalid size");

typedef struct {
    transfer_list_ctrl_t ctrl;
    uint32_t reserved0;
    uint64_t ptr;
    uint64_t reserved1;
} link_elem_t;

static_assert(sizeof(link_elem_t) == DMA_PER_ENTRY_SIZE, "invalid size");

typedef union {
    data_elem_t data;
    link_elem_t link;
} transfer_list_elem_t;

static_assert(sizeof(transfer_list_elem_t) == DMA_PER_ENTRY_SIZE, "invalid size");

#endif