#include "mailbox.h"
#include "mailbox_id.h"
#include "esr_defines.h"
#include "hal_device.h"
#include "interrupt.h"
#include "pcie_int.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#define MBOX_TASK_PRIORITY 2
#define MBOX_STACK_SIZE 256

static TaskHandle_t taskHandles[MBOX_COUNT];
static StackType_t stacks[MBOX_COUNT][MBOX_STACK_SIZE];
static StaticTask_t staticTasks[MBOX_COUNT];
static SemaphoreHandle_t semaphoreHandles[MBOX_COUNT];
static StaticSemaphore_t staticSemaphores[MBOX_COUNT];

static volatile mbox_t* const mbox_hw[MBOX_COUNT] =
{
    (mbox_t*)R_PU_MBOX_MM_SP_BASEADDR,
    (mbox_t*)R_PU_MBOX_MX_SP_BASEADDR,
    (mbox_t*)R_PU_MBOX_PC_SP_BASEADDR
};

static const bool mbox_master[MBOX_COUNT] = {true, true, true};

static void init_task(mbox_e mbox, const char* const mbox_task_name_ptr);
static void init_mbox(mbox_e mbox);
static void mbox_task(void* pvParameters);
static void update_mbox_status(mbox_e mbox);
static bool mbox_ready(mbox_e mbox);
static void reset_mbox(mbox_e mbox);
static int64_t mbox_receive(mbox_e mbox, void* const data_ptr, size_t buffer_size);
static void send_interrupt(mbox_e mbox);
static void mbox_master_minion_isr(void);
static void mbox_maxion_isr(void);
static void mbox_pcie_isr(void);
static inline void* mbox_ptr_to_void_ptr(volatile const mbox_t* const mbox_ptr);

void MBOX_init_pcie(void)
{
    init_task(MBOX_PCIE, "mbox_pcie");
    INT_enableInterrupt(SPIO_PLIC_MBOX_HOST_INTR, 1, mbox_pcie_isr);
    init_mbox(MBOX_PCIE);
}

void MBOX_init_mm(void)
{
    init_task(MBOX_MASTER_MINION, "mbox_mm");
    INT_enableInterrupt(SPIO_PLIC_MBOX_MMIN_INTR, 1, mbox_master_minion_isr);
    init_mbox(MBOX_MASTER_MINION);
}

void MBOX_init_max(void)
{
    init_task(MBOX_MAXION, "mbox_max");
    INT_enableInterrupt(SPIO_PLIC_MBOX_MXN_INTR,  1, mbox_maxion_isr);
    init_mbox(MBOX_MAXION);
}

int64_t MBOX_send(mbox_e mbox, const void* const buffer_ptr, uint32_t length)
{
    int64_t rv = -1;

    volatile ringbuffer_t* const ringbuffer_ptr = mbox_master[mbox] ? &(mbox_hw[mbox]->tx_ring_buffer) : &(mbox_hw[mbox]->rx_ring_buffer);

    if (pdTRUE == xSemaphoreTake(semaphoreHandles[mbox], portMAX_DELAY))
    {
        if (mbox_ready(mbox) && ((length + MBOX_HEADER_LENGTH) <= RINGBUFFER_free(ringbuffer_ptr)))
        {
            const mbox_header_t header = {.length = (uint16_t)length, .magic = MBOX_MAGIC};

            if (MBOX_HEADER_LENGTH == RINGBUFFER_write(ringbuffer_ptr, &header, MBOX_HEADER_LENGTH))
            {
                if (length == RINGBUFFER_write(ringbuffer_ptr, buffer_ptr, length))
                {
                    asm volatile ("fence");
                    send_interrupt(mbox);
                    rv = 0;
                }
            }
        }

        xSemaphoreGive(semaphoreHandles[mbox]);
    }

    return rv;
}

static void init_task(mbox_e mbox, const char* const mbox_task_name_ptr)
{
    // master minion mailbox
    taskHandles[mbox] = xTaskCreateStatic(mbox_task,
                                          mbox_task_name_ptr,
                                          MBOX_STACK_SIZE,
                                          (void*)mbox,
                                          MBOX_TASK_PRIORITY,
                                          stacks[mbox],
                                          &staticTasks[mbox]);

    semaphoreHandles[mbox] = xSemaphoreCreateMutexStatic(&staticSemaphores[mbox]);
    xSemaphoreGive(semaphoreHandles[mbox]);
}

static void init_mbox(mbox_e mbox)
{
    if (mbox_master[mbox])
    {
        // MBOX_send() might be accessing the mbox, use mutex
        xSemaphoreTake(semaphoreHandles[mbox], portMAX_DELAY);

        // We are the master, init everything
        mbox_hw[mbox]->master_status = MBOX_STATUS_NOT_READY;
        mbox_hw[mbox]->slave_status = MBOX_STATUS_NOT_READY;

        RINGBUFFER_init(&(mbox_hw[mbox]->tx_ring_buffer));
        RINGBUFFER_init(&(mbox_hw[mbox]->rx_ring_buffer));

        mbox_hw[mbox]->master_status = MBOX_STATUS_WAITING;

        xSemaphoreGive(semaphoreHandles[mbox]);
    }
    else
    {
        // We are the slave, just set status
        mbox_hw[mbox]->slave_status = MBOX_STATUS_NOT_READY;
    }

    // Notify the other side that we've changed status
    send_interrupt(mbox);
}

static void mbox_task(void* pvParameters)
{
    const mbox_e mbox = (mbox_e)pvParameters;
    uint32_t notificationValue;

    static uint8_t buffer[MBOX_MAX_MESSAGE_LENGTH] __attribute__((aligned(MBOX_BUFFER_ALIGNMENT)));

    // Disable buffering on stdout
    setbuf(stdout, NULL);

    while (1)
    {
        // ISRs set notification bits per ipi_trigger in case we want them - not currently using them
        xTaskNotifyWait(0, 0xFFFFFFFFU, &notificationValue, portMAX_DELAY);

        update_mbox_status(mbox);

        int64_t length;

        do
        {
            length = mbox_receive(mbox, buffer, sizeof(buffer));

            if (length < 0)
            {
                break;
            }

            if ((size_t)length < sizeof(mbox_message_id_t))
            {
                printf("Invalid message: length = %" PRId64 ", min length %" PRIu64 "\r\n", length, sizeof(mbox_message_id_t));
                break;
            }

            const mbox_message_id_t* const message_id = (void*)buffer;

            switch (*message_id)
            {
                case MBOX_MESSAGE_ID_REFLECT_TEST:
                    {
                        int64_t result = MBOX_send(mbox, buffer, (uint32_t)length);

                        if (result != 0)
                        {
                            printf("mbox_send error %" PRId64 "\r\n", result);
                        }
                    }
                    break;

                default:
                    printf("Invalid message id: %" PRIu64 "\r\n", *message_id);
                    printf("message length: %" PRIi64 ", buffer:\r\n", length);
                    for (int64_t i = 0; i < length; ++i)
                    {
                        if (i % 8 == 0 && i != 0) printf("\r\n");
                        printf("%02x ", buffer[i]);
                    }
            }
        }
        while (length > 0);
    }
}

static void update_mbox_status(mbox_e mbox)
{
    const char* const taskName = pcTaskGetName(xTaskGetCurrentTaskHandle());

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
                    printf("%s received slave ready, going master ready\r\n", taskName);
                    mbox_hw[mbox]->master_status = MBOX_STATUS_READY;
                    send_interrupt(mbox);
                }
            break;

            case MBOX_STATUS_WAITING:
                // The slave has requested we reset the mailbox interface.
                printf("%s received slave reset req\r\n", taskName);
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
                    printf("%s received master ready, going slave ready\r\n", taskName);
                    mbox_hw[mbox]->slave_status = MBOX_STATUS_READY;
                    send_interrupt(mbox);
                }
            break;

            case MBOX_STATUS_WAITING:
                if ((mbox_hw[mbox]->slave_status != MBOX_STATUS_READY) &&
                    (mbox_hw[mbox]->slave_status != MBOX_STATUS_WAITING))
                {
                    printf("%s received master waiting, going slave ready\r\n", taskName);
                    mbox_hw[mbox]->slave_status = MBOX_STATUS_READY;
                    send_interrupt(mbox);
                }
            break;

            case MBOX_STATUS_ERROR:
            break;
        }
    }
}

static bool mbox_ready(mbox_e mbox)
{
    return (mbox_hw[mbox]->master_status == MBOX_STATUS_READY) && (mbox_hw[mbox]->slave_status == MBOX_STATUS_READY);
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

// returns the number of bytes read into the buffer, or an error code
static int64_t mbox_receive(mbox_e mbox, void* const buffer_ptr, size_t buffer_size)
{
    volatile ringbuffer_t* const ringbuffer_ptr = mbox_master[mbox] ? &(mbox_hw[mbox]->rx_ring_buffer) : &(mbox_hw[mbox]->tx_ring_buffer);

    if (mbox_ready(mbox))
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
                    printf("mbox_receive: insufficient buffer, dropping message\r\n");
                    return RINGBUFFER_read(ringbuffer_ptr, NULL, header.length);
                }
            }
            else
            {
                printf("mbox_receive: invalid header\r\n");
            }
        }
    }

    return -1;
}

static void send_interrupt(mbox_e mbox)
{
    switch (mbox)
    {
        case MBOX_MASTER_MINION:
        {
            // Send IPI to Shire 32 HART 0
            volatile uint64_t* const ipi_trigger_ptr = (volatile uint64_t *)ESR_SHIRE(32, IPI_TRIGGER);
            *ipi_trigger_ptr = 1;
            break;
        }

        case MBOX_MAXION:
        {
            // TODO
            break;
        }

        case MBOX_PCIE:
        {
            pcie_interrupt_host(0);
            break;
        }
    }
}

static void mbox_master_minion_isr(void)
{
    BaseType_t xHigherPriorityTaskWoken;
    const uint32_t ipi_trigger = *(volatile uint32_t*)R_PU_TRG_MMIN_BASEADDR;
    volatile uint32_t* const ipi_clear_ptr = (volatile uint32_t*)(R_PU_TRG_MMIN_SP_BASEADDR);

    xTaskNotifyFromISR(taskHandles[MBOX_MASTER_MINION], ipi_trigger, eSetBits, &xHigherPriorityTaskWoken);

    *ipi_clear_ptr = ipi_trigger;

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void mbox_maxion_isr(void)
{
    BaseType_t xHigherPriorityTaskWoken;
    const uint32_t ipi_trigger = *(volatile uint32_t*)R_PU_TRG_MAX_BASEADDR;
    volatile uint32_t* const ipi_clear_ptr = (volatile uint32_t*)(R_PU_TRG_MAX_SP_BASEADDR);

    xTaskNotifyFromISR(taskHandles[MBOX_MAXION], ipi_trigger, eSetBits, &xHigherPriorityTaskWoken);

    *ipi_clear_ptr = ipi_trigger;

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void mbox_pcie_isr(void)
{
    BaseType_t xHigherPriorityTaskWoken;
    const uint32_t ipi_trigger = *(volatile uint32_t*)R_PU_TRG_PCIE_BASEADDR;
    volatile uint32_t* const ipi_clear_ptr = (volatile uint32_t*)(R_PU_TRG_PCIE_SP_BASEADDR);

    xTaskNotifyFromISR(taskHandles[MBOX_PCIE], ipi_trigger, eSetBits, &xHigherPriorityTaskWoken);

    *ipi_clear_ptr = ipi_trigger;

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// Wrapper for ugly cast
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"

static inline void* mbox_ptr_to_void_ptr(volatile const mbox_t* const mbox_ptr)
{
    return (void*)mbox_ptr;
}

#pragma GCC diagnostic pop
