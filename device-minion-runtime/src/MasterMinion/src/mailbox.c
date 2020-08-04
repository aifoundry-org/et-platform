#include "mailbox.h"
#include "hal_device.h"
#include "interrupt.h"
#include "log.h"
#include "pcie_isr.h"
#include "pcie_int.h"
#include "ringbuffer.h"

#include <inttypes.h>
#include <stddef.h>

static void init_mbox(mbox_e mbox);
static void reset_mbox(mbox_e mbox);
static void send_interrupt(mbox_e mbox);

static volatile mbox_t* const mbox_hw[MBOX_COUNT] =
{
    (mbox_t*)R_PU_MBOX_MM_SP_BASEADDR,
    (mbox_t*)R_PU_MBOX_PC_MM_BASEADDR
};

static const bool mbox_master[MBOX_COUNT] = {false, true};

void MBOX_init(void)
{
    // First configure the PLIC to accept PCIe interrupts (note that External Interrupts are now enabled yet)
    INT_enableInterrupt(PU_PLIC_PCIE_MESSAGE_INTR, 1, pcie_isr);

    init_mbox(MBOX_SP);
    init_mbox(MBOX_PCIE);
}

int64_t MBOX_send(mbox_e mbox, const void* const buffer_ptr, uint32_t length)
{
    volatile ringbuffer_t* const ringbuffer_ptr = mbox_master[mbox] ? &(mbox_hw[mbox]->tx_ring_buffer) : &(mbox_hw[mbox]->rx_ring_buffer);

    if (MBOX_ready(mbox) && ((length + MBOX_HEADER_LENGTH) <= RINGBUFFER_free(ringbuffer_ptr)))
    {
        const mbox_header_t header = {.length = (uint16_t)length, .magic = MBOX_MAGIC};

        if (MBOX_HEADER_LENGTH == RINGBUFFER_write(ringbuffer_ptr, &header, MBOX_HEADER_LENGTH))
        {
            if (length == RINGBUFFER_write(ringbuffer_ptr, buffer_ptr, length))
            {
                asm volatile ("fence");
                send_interrupt(mbox);
                return 0;
            }
        }
    }

    return -1;
}

// returns the number of bytes read into the buffer, or an error code
int64_t MBOX_receive(mbox_e mbox, void* const buffer_ptr, size_t buffer_size)
{
    volatile ringbuffer_t* const ringbuffer_ptr = mbox_master[mbox] ? &(mbox_hw[mbox]->rx_ring_buffer) : &(mbox_hw[mbox]->tx_ring_buffer);

    if (MBOX_ready(mbox))
    {
        mbox_header_t header;

        if (MBOX_HEADER_LENGTH == RINGBUFFER_read(ringbuffer_ptr, &header, MBOX_HEADER_LENGTH))
        {
            if ((header.length > 0) && (header.magic == MBOX_MAGIC))
            {
                if (header.length <= buffer_size)
                {
                    return RINGBUFFER_read(ringbuffer_ptr, buffer_ptr, header.length);
                }
                else
                {
                    log_write(LOG_LEVEL_ERROR, "MBOX_receive: insufficient buffer, dropping message\r\n");
                    return RINGBUFFER_read(ringbuffer_ptr, NULL, header.length);
                }
            }
            else
            {
                log_write(LOG_LEVEL_ERROR, "MBOX_receive: invalid header\r\n");
            }
        }
    }

    return -1;
}

void MBOX_update_status(mbox_e mbox)
{
    if (mbox_master[mbox])
    {
        // We are the master
        switch (mbox_hw[mbox]->slave_status)
        {
            case MBOX_STATUS_NOT_READY:
            break;

            case MBOX_STATUS_READY:
                if (mbox_hw[mbox]->master_status == MBOX_STATUS_WAITING)
                {
                    log_write(LOG_LEVEL_INFO, "received slave ready, going master ready\r\n");
                    mbox_hw[mbox]->master_status = MBOX_STATUS_READY;
                    send_interrupt(mbox);
                }
            break;

            case MBOX_STATUS_WAITING:
                // The slave has requested we reset the mailbox interface.
                log_write(LOG_LEVEL_DEBUG, "received slave reset req\r\n");
                reset_mbox(mbox);
            break;

            case MBOX_STATUS_ERROR:
            break;
        }
    }
    else
    {
        // We are the slave
        switch (mbox_hw[mbox]->master_status)
        {
            case MBOX_STATUS_NOT_READY:
            break;

            case MBOX_STATUS_READY:
                if ((mbox_hw[mbox]->slave_status != MBOX_STATUS_READY) &&
                    (mbox_hw[mbox]->slave_status != MBOX_STATUS_WAITING))
                {
                    log_write(LOG_LEVEL_DEBUG, "received master ready, going slave ready\r\n");
                    mbox_hw[mbox]->slave_status = MBOX_STATUS_READY;
                    send_interrupt(mbox);
                }
            break;

            case MBOX_STATUS_WAITING:
                if ((mbox_hw[mbox]->slave_status != MBOX_STATUS_READY) &&
                    (mbox_hw[mbox]->slave_status != MBOX_STATUS_WAITING))
                {
                    log_write(LOG_LEVEL_DEBUG, "received master waiting, going slave ready\r\n");
                    mbox_hw[mbox]->slave_status = MBOX_STATUS_READY;
                    send_interrupt(mbox);
                }
            break;

            case MBOX_STATUS_ERROR:
            break;
        }
    }
}

bool MBOX_ready(mbox_e mbox)
{
    return ((mbox_hw[mbox]->master_status == MBOX_STATUS_READY) && (mbox_hw[mbox]->slave_status == MBOX_STATUS_READY));
}

bool MBOX_empty(mbox_e mbox)
{
    volatile ringbuffer_t* const ringbuffer_ptr = mbox_master[mbox] ? &(mbox_hw[mbox]->rx_ring_buffer) : &(mbox_hw[mbox]->tx_ring_buffer);

    return (!MBOX_ready(mbox) || RINGBUFFER_empty(ringbuffer_ptr));
}

static void init_mbox(mbox_e mbox)
{
    if (mbox_master[mbox])
    {
        // We are the master, init everything
        mbox_hw[mbox]->master_status = MBOX_STATUS_NOT_READY;
        mbox_hw[mbox]->slave_status = MBOX_STATUS_NOT_READY;

        RINGBUFFER_init(&(mbox_hw[mbox]->tx_ring_buffer));
        RINGBUFFER_init(&(mbox_hw[mbox]->rx_ring_buffer));

        mbox_hw[mbox]->master_status = MBOX_STATUS_WAITING;
    }
    else
    {
        // We are the slave, just set status
        mbox_hw[mbox]->slave_status = MBOX_STATUS_NOT_READY;
    }

    // Notify the other side that we've changed status
    send_interrupt(mbox);
}

static void reset_mbox(mbox_e mbox)
{
    if (mbox_master[mbox])
    {
        // We're the master, so initialize the mailbox
        init_mbox(mbox);
    }
    else
    {
        // We're the slave, so ask the master to reset the mailbox.
        mbox_hw[mbox]->slave_status = MBOX_STATUS_WAITING;
        send_interrupt(mbox);
    }
}

static void send_interrupt(mbox_e mbox)
{
    switch (mbox)
    {
        case MBOX_SP:
        {
            volatile uint32_t* const ipi_trigger = (volatile uint32_t*)(R_PU_TRG_MMIN_BASEADDR);
            *ipi_trigger = 1U;
            break;
        }
        case MBOX_PCIE:
        {
            pcie_interrupt_host(0);
            break;
        }
    }
}
