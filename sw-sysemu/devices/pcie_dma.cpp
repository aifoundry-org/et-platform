/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include "agent.h"
#include "emu.h"
#include "emu_gio.h"
#include "pcie_dma.h"
#include "memory/main_memory.h"

#define CH_CONTROL1_OFF_CB_GET(x)  ((x) & 0x00000001ul)
#define CH_CONTROL1_OFF_TCB_GET(x) (((x) & 0x00000002ul) >> 1)
#define CH_CONTROL1_OFF_CCS_GET(x) (((x) & 0x00000100ul) >> 8)

#define CH_CONTROL1_OFF_CB_MODIFY(r, x)  (((x) & 0x00000001ul) | ((r) & 0xfffffffeul))
#define CH_CONTROL1_OFF_TCB_MODIFY(r, x) ((((x) << 1) & 0x00000002ul) | ((r) & 0xfffffffdul))
#define CH_CONTROL1_OFF_CCS_MODIFY(r, x) ((((x) << 8) & 0x00000100ul) | ((r) & 0xfffffefful))

typedef struct {
    union {
        struct {
            uint32_t CB : 1; /* bit 0; R/W; 0x0 */
            uint32_t TCB : 1; /* bit 1; R/W; 0x0 */
            uint32_t LLP : 1; /* bit 2; R/W; 0x0 */
            uint32_t LIE : 1; /* bit 3; R/W; 0x0 */
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
} __attribute__((__packed__)) data_elem_t;

static_assert(sizeof(data_elem_t) == 24, "invalid size");

typedef struct {
    transfer_list_ctrl_t ctrl;
    uint32_t reserved0;
    uint64_t ptr;
    uint64_t reserved1;
} __attribute__((__packed__)) link_elem_t;

static_assert(sizeof(link_elem_t) == 24, "invalid size");

typedef union {
    data_elem_t data;
    link_elem_t link;
} transfer_list_elem_t;

static_assert(sizeof(transfer_list_elem_t) == 24, "invalid size");

namespace bemu {

template<bool wrch>
void PcieDma<wrch>::go()
{
    LOG_NOTHREAD(DEBUG, "PcieDma%d<%s>::go()", chan_id, wrch ? "Write" : "Read");

    // Element pointer
    uint64_t elem_ptr = (uint64_t)llp_high << 32 | llp_low;

    // TODO: Check dma_{read/write}_engine_en, check handshake mode, check linked-list mode (LLE)

    do {
        transfer_list_elem_t elem;

        // Read element
        bemu::memory.read(Noagent{}, elem_ptr, sizeof(elem), &elem);

        LOG_NOTHREAD(DEBUG, "Elem ptr: 0x%" PRIx64 "\n", elem_ptr);
        LOG_NOTHREAD(DEBUG, "elem.ctrl: 0x%" PRIx32 "\n", elem.link.ctrl.R);

        uint32_t llp = elem.link.ctrl.B.LLP;                 // Link element
        uint32_t cb = elem.link.ctrl.B.CB;                   // Cycle Bit
        uint32_t tcb = CH_CONTROL1_OFF_TCB_GET(ch_control1); // Toggle Cycle Bit
        uint32_t ccs = CH_CONTROL1_OFF_CCS_GET(ch_control1); // Consumer Cycle State

        // Transfer list empty condition
        if (!llp && (cb != ccs)) {
            goto dma_done;
        }

        // Load Element into Channel Context
        /// TODO

        if (elem.link.ctrl.B.LLP) { // Link element. Also completes DMA process
            link_elem_t le = elem.link;
            LOG_NOTHREAD(DEBUG, "LE: {ptr: 0x%" PRIx64 "}\n", le.ptr);

            // Toggle Cycle Bit
            if (tcb == 1) {
                ch_control1 = CH_CONTROL1_OFF_CCS_MODIFY(ch_control1, !ccs);
            }

            // Check if Consumer Cycle Bit matches with Cycle Bit
            if (cb != ccs) {
                goto dma_done;
            }
        } else { // Data element
            data_elem_t de = elem.data;
            LOG_NOTHREAD(DEBUG, "DE: {size: 0x%" PRIx32 ", sar: 0x%" PRIx64 ", dar: 0x%" PRIx64 "}\n", de.size, de.sar, de.dar);

            // Trigger interrupt if it was pending
            if (liep) {
                liep = false; // Clear LIEP
                trigger_done_int();
            }

            // Transfer block of data
#ifdef SYS_EMU
            api_communicate *api_comm = sys_emu::get_api_communicate();
            if (api_comm) {
                void *buff = new uint8_t[de.size];
                if (wrch) {
                    bemu::memory.read(Noagent{}, de.sar, de.size, buff);
                    api_comm->host_memory_write(de.dar, de.size, buff);
                } else {
                    api_comm->host_memory_read(de.sar, de.size, buff);
                    bemu::memory.write(Noagent{}, de.dar, de.size, buff);
                }
            } else {
                LOG_NOTHREAD(WARN, "%s", "API Communicate is NULL!");
            }
#endif

            // Local interrupt
            if (de.ctrl.B.LIE) {
                liep = true;
            }
        }

        // Point to the next element of the TL
        elem_ptr += sizeof(transfer_list_elem_t);

        continue;
dma_done:
        /// Set Channel Status = STOPPED
        // Trigger interrupt if it was pending
        if (liep) {
            liep = false; // Clear LIEP
            trigger_done_int();
        }
        // Terminate the complete DMA process
        break;
    } while (1);

    LOG_NOTHREAD(DEBUG, "PcieDma%d<%s>::go() FINISHED", chan_id, wrch ? "Write" : "Read");
}

template<bool wrch>
void PcieDma<wrch>::trigger_done_int()
{
    LOG_NOTHREAD(DEBUG, "PcieDma%d<%s>::trigger_done_int()", chan_id, wrch ? "Write" : "Read");

#ifdef SYS_EMU
    bemu::memory.pcie_space.pcie0_dbi_slv.trigger_done_int(wrch, chan_id);
#endif
}

template struct PcieDma<true>;
template struct PcieDma<false>;

}
