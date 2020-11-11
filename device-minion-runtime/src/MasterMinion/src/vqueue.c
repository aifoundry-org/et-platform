/*-------------------------------------------------------------------------
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include "hal_device.h"
#include "interrupt.h"
#include "log.h"
#include "pcie_isr.h"
#include "pcie_int.h"
#include "layout.h"
#include "vqueue.h"
#include "cacheops.h"
#include "atomic.h"

#include <inttypes.h>
#include <stddef.h>

// Data structures
typedef struct {
    bool is_ready;
    uint8_t producer_lock;
    uint8_t consumer_lock;
    uint16_t notify_int;
} vqueue_info_intern_t;

static void init_vqueue(uint32_t vq_index, uint64_t vq_size);

static struct vqueue_desc *const vq_desc_glob = (struct vqueue_desc *)DEVICE_MM_VQUEUE_BASE;
static vqueue_info_intern_t vq_info[VQUEUE_COUNT] = { 0 };

static inline void acquire_vqueue_lock(volatile uint8_t *lock)
{
    while (atomic_load_global_8(lock) != 0U) {
        asm volatile("fence\n" ::: "memory");
    }
    atomic_store_global_8(lock, 1U);
    asm volatile("fence\n" ::: "memory");
}

static inline void release_vqueue_lock(volatile uint8_t *lock)
{
    atomic_store_global_8(lock, 0U);
    asm volatile("fence\n" ::: "memory");
}

void VQUEUE_init(void)
{
    // First configure the PLIC to accept PCIe interrupts (note that External Interrupts are not enabled yet)
    INT_enableInterrupt(PU_PLIC_PCIE_MESSAGE_INTR, 1, pcie_isr);

    // Populate the VQ descriptor
    vq_desc_glob->queue_addr = VQUEUE_DATA_BASE;
    vq_desc_glob->queue_count = VQUEUE_COUNT;
    vq_desc_glob->queue_element_count = VQUEUE_ELEMENT_COUNT;
    vq_desc_glob->queue_element_size = VQUEUE_ELEMENT_SIZE;
    vq_desc_glob->device_ready = 1U;

    // Make sure writes to mem are synced
    asm volatile("fence");
    // Evict the dirty vqueue descriptor to memory
    // TODO: Remove the cache eviction once Device Interface Registers are available in SRAM.
    evict(to_Mem, vq_desc_glob, sizeof(vq_desc_glob));
    WAIT_CACHEOPS

    for (uint32_t i = 0; i < vq_desc_glob->queue_count; i++) {
        init_vqueue(
            i, (uint64_t)(vq_desc_glob->queue_element_count * vq_desc_glob->queue_element_size));
    }
}

int64_t VQUEUE_push(vq_e vq, uint32_t vq_index, const void *const buffer_ptr, uint32_t length)
{
    int64_t rv = VQ_ERROR_PUSH;
    const struct vqueue_buf_header header = { .length = (uint16_t)length, .magic = VQUEUE_MAGIC };
    volatile struct vqueue_info *const vqueue_ptr = DEVICE_VQUEUE_BASE(vq_index);
    volatile struct circ_buf_header *const sq_cq_ptr = vq ? &vqueue_ptr->cq_header :
                                                            &vqueue_ptr->sq_header;
    uint8_t *const vq_buf =
        vq ? DEVICE_CQUEUE_BASE(vq_index, (uint64_t)(vq_desc_glob->queue_element_count *
                                                     vq_desc_glob->queue_element_size)) :
             DEVICE_SQUEUE_BASE(vq_index, (uint64_t)(vq_desc_glob->queue_element_count *
                                                     vq_desc_glob->queue_element_size));

    if (!vq_info[vq_index].is_ready) {
        return VQ_ERROR_NOT_READY;
    }

    acquire_vqueue_lock(&(vq_info[vq_index].producer_lock));

    uint32_t head = sq_cq_ptr->head;
    uint32_t tail = sq_cq_ptr->tail;
    void *const vq_buf_off = (void *)(vq_buf + (head * vq_desc_glob->queue_element_size));

    if (CIRCBUFFER_free(head, tail) == 0) {
        release_vqueue_lock(&(vq_info[vq_index].producer_lock));
        return VQ_ERROR_VQ_FULL;
    }

    if (VQUEUE_BUFFER_HEADER_SIZE == CIRCBUFFER_write(vq_buf_off, 0U,
                                                      vq_desc_glob->queue_element_size, &header,
                                                      VQUEUE_BUFFER_HEADER_SIZE)) {
        if (length == CIRCBUFFER_write(vq_buf_off, VQUEUE_BUFFER_HEADER_SIZE,
                                       vq_desc_glob->queue_element_size, buffer_ptr, length)) {
            // Update head index
            sq_cq_ptr->head = (uint16_t)((head + 1) % vq_desc_glob->queue_element_count);
            // Make sure VQ data and head pointer is coherent in memory
            asm volatile("fence");
            evict(to_Mem, vq_buf_off, vq_desc_glob->queue_element_size);
            WAIT_CACHEOPS
            evict(to_Mem, &(sq_cq_ptr->head), sizeof(sq_cq_ptr->head));
            WAIT_CACHEOPS
            pcie_interrupt_host(vq_info[vq_index].notify_int);
            rv = 0;
        } else {
            log_write(LOG_LEVEL_ERROR, "VQUEUE_push: Unable to write buffer data!\r\n");
        }
    } else {
        log_write(LOG_LEVEL_ERROR, "VQUEUE_push: Unable to write buffer header!\r\n");
    }

    release_vqueue_lock(&(vq_info[vq_index].producer_lock));

    return rv;
}

int64_t VQUEUE_pop(vq_e vq, uint32_t vq_index, void *const buffer_ptr, size_t buffer_size)
{
    int64_t rv = VQ_ERROR_POP;
    struct vqueue_buf_header header;
    volatile struct vqueue_info *const vqueue_ptr = DEVICE_VQUEUE_BASE(vq_index);
    volatile struct circ_buf_header *const sq_cq_ptr = vq ? &vqueue_ptr->cq_header :
                                                            &vqueue_ptr->sq_header;
    uint8_t *const vq_buf =
        vq ? DEVICE_CQUEUE_BASE(vq_index, (uint64_t)(vq_desc_glob->queue_element_count *
                                                     vq_desc_glob->queue_element_size)) :
             DEVICE_SQUEUE_BASE(vq_index, (uint64_t)(vq_desc_glob->queue_element_count *
                                                     vq_desc_glob->queue_element_size));

    if (!vq_info[vq_index].is_ready) {
        return VQ_ERROR_NOT_READY;
    }

    acquire_vqueue_lock(&(vq_info[vq_index].consumer_lock));

    uint32_t head = sq_cq_ptr->head;
    uint32_t tail = sq_cq_ptr->tail;
    void *const vq_buf_off = (void *)(vq_buf + (tail * vq_desc_glob->queue_element_size));

    if (CIRCBUFFER_used(head, tail) == 0) {
        release_vqueue_lock(&(vq_info[vq_index].consumer_lock));
        return VQ_ERROR_VQ_EMPTY;
    }

    // Invalidate to read fresh data
    asm volatile("fence");
    evict(to_Mem, vq_buf_off, vq_desc_glob->queue_element_size);
    WAIT_CACHEOPS

    if (VQUEUE_BUFFER_HEADER_SIZE == CIRCBUFFER_read(vq_buf_off, 0U,
                                                     vq_desc_glob->queue_element_size, &header,
                                                     VQUEUE_BUFFER_HEADER_SIZE)) {
        if ((header.length > 0) && (header.magic == VQUEUE_MAGIC)) {
            if (header.length <= buffer_size) {
                rv = CIRCBUFFER_read(vq_buf_off, VQUEUE_BUFFER_HEADER_SIZE,
                                     vq_desc_glob->queue_element_size, buffer_ptr, header.length);
                // Update tail index
                sq_cq_ptr->tail = (uint16_t)((tail + 1) % vq_desc_glob->queue_element_count);
                // Make sure VQ tail pointer is coherent in memory
                asm volatile("fence");
                evict(to_Mem, &(sq_cq_ptr->tail), sizeof(sq_cq_ptr->tail));
                WAIT_CACHEOPS
            } else {
                log_write(LOG_LEVEL_ERROR,
                          "VQUEUE_pop: insufficient buffer, unable to pop message\r\n");
            }
        } else {
            log_write(LOG_LEVEL_ERROR, "VQUEUE_pop: invalid header\r\n");
        }
    }

    release_vqueue_lock(&(vq_info[vq_index].consumer_lock));

    return rv;
}

void VQUEUE_update_status(uint32_t vq_index)
{
    // We are the master
    volatile struct vqueue_info *const vqueue_ptr = DEVICE_VQUEUE_BASE(vq_index);

    // Invalidate to read fresh slave_status
    asm volatile("fence");
    evict(to_Mem, &(vqueue_ptr->slave_status), sizeof(vqueue_ptr->slave_status));
    WAIT_CACHEOPS

    switch (vqueue_ptr->slave_status) {
    case VQ_STATUS_NOT_READY:
        break;

    case VQ_STATUS_READY:
        if (vqueue_ptr->master_status == VQ_STATUS_WAITING) {
            log_write(LOG_LEVEL_CRITICAL, "received slave ready, going master ready\r\n");
            vqueue_ptr->master_status = VQ_STATUS_READY;
            // Evict the dirty master_status to Memory
            asm volatile("fence");
            evict(to_Mem, &(vqueue_ptr->master_status), sizeof(vqueue_ptr->master_status));
            WAIT_CACHEOPS
            pcie_interrupt_host(vq_info[vq_index].notify_int);
        }
        break;

    case VQ_STATUS_WAITING:
        // The slave has requested we reset the VQs.
        log_write(LOG_LEVEL_CRITICAL, "received slave reset req\r\n");
        init_vqueue(vq_index, (uint64_t)(vq_desc_glob->queue_element_count *
                                         vq_desc_glob->queue_element_size));
        pcie_interrupt_host(vq_info[vq_index].notify_int);
        break;

    case VQ_STATUS_ERROR:
        break;
    }

    // Update the VQ ready flag
    vq_info[vq_index].is_ready = (vqueue_ptr->slave_status == VQ_STATUS_READY) &&
                                 (vqueue_ptr->master_status == VQ_STATUS_READY);
}

bool VQUEUE_empty(vq_e vq, uint32_t vq_index)
{
    volatile struct vqueue_info *const vqueue_ptr = DEVICE_VQUEUE_BASE(vq_index);
    volatile struct circ_buf_header *const sq_cq_ptr = vq ? &vqueue_ptr->cq_header :
                                                            &vqueue_ptr->sq_header;

    return (CIRCBUFFER_used(sq_cq_ptr->head, sq_cq_ptr->tail) == 0);
}

bool VQUEUE_full(vq_e vq, uint32_t vq_index)
{
    volatile struct vqueue_info *const vqueue_ptr = DEVICE_VQUEUE_BASE(vq_index);
    volatile struct circ_buf_header *const sq_cq_ptr = vq ? &vqueue_ptr->cq_header :
                                                            &vqueue_ptr->sq_header;

    return (CIRCBUFFER_free(sq_cq_ptr->head, sq_cq_ptr->tail) == 0);
}

static void init_vqueue(uint32_t vq_index, uint64_t vq_size)
{
    // We are the master, init everything
    struct vqueue_info *vq = DEVICE_VQUEUE_BASE(vq_index);
    void *sbuffer_addr, *cbuffer_addr;

    // Submission Queue start address
    sbuffer_addr = DEVICE_SQUEUE_BASE(vq_index, vq_size);
    // Completion Queue start address
    cbuffer_addr = DEVICE_CQUEUE_BASE(vq_index, vq_size);

    vq->master_status = VQ_STATUS_NOT_READY;
    vq->slave_status = VQ_STATUS_NOT_READY;

    // Init the producer and consumer locks
    vq_info[vq_index].producer_lock = 0U;
    vq_info[vq_index].consumer_lock = 0U;

    // Init the MSI-X vector
    vq_info[vq_index].notify_int = (uint16_t)vq_index;

    CIRCBUFFER_init(&(vq->sq_header), sbuffer_addr, vq_size);
    CIRCBUFFER_init(&(vq->cq_header), cbuffer_addr, vq_size);

    vq->master_status = VQ_STATUS_WAITING;

    // Evict the dirty vqueue_info to Mem
    asm volatile("fence");
    evict(to_Mem, vq, sizeof(struct vqueue_info));
    WAIT_CACHEOPS
}
