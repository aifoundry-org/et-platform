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

static volatile vqueue_info_intern_t vq_info[MM_VQ_COUNT] __attribute__((section(".data")));

static inline __attribute__((always_inline)) void
evict_data(enum cop_dest dest, const volatile void *const data_ptr, uint64_t size)
{
    asm volatile("fence");
    evict(dest, data_ptr, size);
    WAIT_CACHEOPS
}

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

    for (uint32_t i = 0; i < MM_VQ_COUNT; i++) {
        init_vqueue(i, (VQUEUE_ELEMENT_COUNT * VQUEUE_ELEMENT_SIZE));
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
        vq ? DEVICE_CQUEUE_BASE(vq_index, (VQUEUE_ELEMENT_COUNT * VQUEUE_ELEMENT_SIZE)) :
             DEVICE_SQUEUE_BASE(vq_index, (VQUEUE_ELEMENT_COUNT * VQUEUE_ELEMENT_SIZE));

    if (!vq_info[vq_index].is_ready) {
        return VQ_ERROR_NOT_READY;
    }

    // TODO: Disabled the locking until we move back to DRAM
    //acquire_vqueue_lock(&(vq_info[vq_index].producer_lock));

    uint32_t head = sq_cq_ptr->head;
    uint32_t tail = sq_cq_ptr->tail;
    void *const vq_buf_off = (void *)(vq_buf + (head * VQUEUE_ELEMENT_SIZE));

    if (CIRCBUFFER_free(head, tail) == 0) {
        release_vqueue_lock(&(vq_info[vq_index].producer_lock));
        return VQ_ERROR_VQ_FULL;
    }

    if (VQUEUE_BUFFER_HEADER_SIZE ==
        CIRCBUFFER_write(vq_buf_off, 0U, VQUEUE_ELEMENT_SIZE, &header, VQUEUE_BUFFER_HEADER_SIZE)) {
        if (length == CIRCBUFFER_write(vq_buf_off, VQUEUE_BUFFER_HEADER_SIZE, VQUEUE_ELEMENT_SIZE,
                                       buffer_ptr, length)) {
            // Update head index
            sq_cq_ptr->head = (uint16_t)((head + 1) % VQUEUE_ELEMENT_COUNT);
            // Make sure VQ data and head pointer is coherent in memory
            // TODO: Disabled the eviction until we move back to DRAM
            //evict_data(to_L3, vq_buf_off, VQUEUE_ELEMENT_SIZE);
            //evict_data(to_L3, &(sq_cq_ptr->head), sizeof(sq_cq_ptr->head));
            asm volatile("fence");
            pcie_interrupt_host(vq_info[vq_index].notify_int);
            rv = 0;
        } else {
            log_write(LOG_LEVEL_ERROR, "VQUEUE_push: Unable to write buffer data!\r\n");
        }
    } else {
        log_write(LOG_LEVEL_ERROR, "VQUEUE_push: Unable to write buffer header!\r\n");
    }

    // TODO: Disabled the locking until we move back to DRAM
    //release_vqueue_lock(&(vq_info[vq_index].producer_lock));

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
        vq ? DEVICE_CQUEUE_BASE(vq_index, (VQUEUE_ELEMENT_COUNT * VQUEUE_ELEMENT_SIZE)) :
             DEVICE_SQUEUE_BASE(vq_index, (VQUEUE_ELEMENT_COUNT * VQUEUE_ELEMENT_SIZE));

    if (!vq_info[vq_index].is_ready) {
        return VQ_ERROR_NOT_READY;
    }

    // TODO: Disabled the locking until we move back to DRAM
    //acquire_vqueue_lock(&(vq_info[vq_index].consumer_lock));

    uint32_t head = sq_cq_ptr->head;
    uint32_t tail = sq_cq_ptr->tail;
    void *const vq_buf_off = (void *)(vq_buf + (tail * VQUEUE_ELEMENT_SIZE));

    if (CIRCBUFFER_used(head, tail) == 0) {
        release_vqueue_lock(&(vq_info[vq_index].consumer_lock));
        return VQ_ERROR_VQ_EMPTY;
    }

    // TODO: Disabled the eviction until we move back to DRAM
    // Invalidate to read fresh header data
    //evict_data(to_L3, vq_buf_off, VQUEUE_BUFFER_HEADER_SIZE);

    if (VQUEUE_BUFFER_HEADER_SIZE ==
        CIRCBUFFER_read(vq_buf_off, 0U, VQUEUE_ELEMENT_SIZE, &header, VQUEUE_BUFFER_HEADER_SIZE)) {
        if ((header.length > 0) && (header.magic == VQUEUE_MAGIC)) {
            if (header.length <= buffer_size) {
                // TODO: Disabled the eviction until we move back to DRAM
                // Invalidate to read fresh vq data
                //evict_data(to_L3, (void *)((uint64_t)vq_buf_off + VQUEUE_BUFFER_HEADER_SIZE), header.length);
                rv = CIRCBUFFER_read(vq_buf_off, VQUEUE_BUFFER_HEADER_SIZE, VQUEUE_ELEMENT_SIZE,
                                     buffer_ptr, header.length);
            } else {
                log_write(LOG_LEVEL_ERROR,
                          "VQUEUE_pop: insufficient buffer, unable to pop message\r\n");
            }
        } else {
            log_write(LOG_LEVEL_ERROR, "VQUEUE_pop: invalid header\r\n");
        }
        // Update tail index (will also discard invalid data)
        sq_cq_ptr->tail = (uint16_t)((tail + 1) % VQUEUE_ELEMENT_COUNT);
        // Make sure VQ tail pointer is coherent in memory
        // TODO: Disabled the eviction until we move back to DRAM
        //evict_data(to_L3, &(sq_cq_ptr->tail), sizeof(sq_cq_ptr->tail));
    }

    // TODO: Disabled the locking until we move back to DRAM
    //release_vqueue_lock(&(vq_info[vq_index].consumer_lock));

    return rv;
}

void VQUEUE_update_status(uint32_t vq_index)
{
    // We are the master
    volatile struct vqueue_info *const vqueue_ptr = DEVICE_VQUEUE_BASE(vq_index);

    // Invalidate to read fresh slave_status
    // TODO: Disabled the eviction until we move back to DRAM
    //evict_data(to_L3, &(vqueue_ptr->slave_status), sizeof(vqueue_ptr->slave_status));

    switch (vqueue_ptr->slave_status) {
    case VQ_STATUS_NOT_READY:
        break;

    case VQ_STATUS_READY:
        if (vqueue_ptr->master_status == VQ_STATUS_WAITING) {
            log_write(LOG_LEVEL_INFO, "received slave ready, going master ready\r\n");
            vqueue_ptr->master_status = VQ_STATUS_READY;
            // Evict the dirty master_status to Memory
            // TODO: Disabled the eviction until we move back to DRAM
            //evict_data(to_L3, &(vqueue_ptr->master_status), sizeof(vqueue_ptr->master_status));
            pcie_interrupt_host(vq_info[vq_index].notify_int);
        }
        break;

    case VQ_STATUS_WAITING:
        // The slave has requested we reset the VQs.
        log_write(LOG_LEVEL_INFO, "received slave reset req\r\n");
        init_vqueue(vq_index, (VQUEUE_ELEMENT_COUNT * VQUEUE_ELEMENT_SIZE));
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
    vq_info[vq_index].notify_int = MM_DEV_INTF_GET_BASE->mm_vq.interrupt_vector[vq_index];

    CIRCBUFFER_init(&(vq->sq_header), sbuffer_addr, vq_size);
    CIRCBUFFER_init(&(vq->cq_header), cbuffer_addr, vq_size);

    vq->master_status = VQ_STATUS_WAITING;

    // TODO: Disabled the eviction until we move back to DRAM
    // Evict the submission queue data region
    //evict_data(to_L3, sbuffer_addr, vq_size);
    // Evict the completion queue data region
    //evict_data(to_L3, cbuffer_addr, vq_size);
    // Evict the shared vqueue information data region
    //evict_data(to_L3, vq, sizeof(struct vqueue_info));
    // Evict the internal vqueue information data region
    evict_data(to_L3, &(vq_info[vq_index]), sizeof(vqueue_info_intern_t));
}
