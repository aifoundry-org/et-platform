#include "build_configuration.h"
#include "fcc.h"
#include "flb.h"
#include "hart.h"
#include "kernel.h"
#include "kernel_info.h"
#include "layout.h"
#include "message.h"
#include "syscall.h"

#include <stdint.h>

static void handle_message(uint64_t shire_id, uint64_t hart_id, message_t* const message_ptr);

void __attribute__((noreturn)) main(void)
{
    uint64_t temp;
    bool result;

    // Setup supervisor trap vector and sscratch
    asm volatile (
        "la    %0, trap_handler \n"
        "csrw  stvec, %0        \n"
        "csrw  sscratch, sp     \n" // sscratch points to S-mode stack scratch region
        : "=&r" (temp)
    );

    const uint64_t shire_id = get_shire_id();
    const uint64_t hart_id = get_hart_id();

    message_init_worker(shire_id, hart_id);

    // Enable supervisor software interrupts
    asm volatile (
        "csrsi sie, 0x2     \n" // Enable supervisor software interrupts
        "csrsi sstatus, 0x2 \n" // Enable interrupts
    );

    // First HART in the shire
    if (hart_id % 64U == 0U)
    {
        // Enable all thread1s so they can run BIST, master can communicate with them, etc.
        syscall(SYSCALL_ENABLE_THREAD1, 0, 0, 0);
    }

    // TODO run BIST

    message_t message = {.id = MESSAGE_ID_SHIRE_READY, .data = {0}};
    message_number_t previous_broadcast_message_number = 0xFFFFFFFFU;

    WAIT_FLB(64, 31, result);

    // Last thread to join barrier
    if (result)
    {
        message_send_worker(shire_id, hart_id, &message);
    }

    for (;;)
    {
        // Wait for a message from the master
        asm volatile ("wfi");

        if (broadcast_message_available(previous_broadcast_message_number))
        {
            previous_broadcast_message_number = broadcast_message_receive_worker(&message);
            handle_message(shire_id, hart_id, &message);
        }

        if (message_available(shire_id, hart_id))
        {
            message_receive_worker(shire_id, hart_id, &message);
            handle_message(shire_id, hart_id, &message);
        }
    }
}

static void handle_message(uint64_t shire_id, uint64_t hart_id, message_t* const message_ptr)
{
    if (message_ptr->id == MESSAGE_ID_KERNEL_LAUNCH)
    {
        const uint64_t* const kernel_entry_addr = (uint64_t*)message_ptr->data[0];
        const uint64_t* const kernel_stack_addr = (uint64_t*)(KERNEL_UMODE_STACK_BASE - (hart_id * KERNEL_UMODE_STACK_SIZE));
        const kernel_params_t* const kernel_params_ptr = (kernel_params_t*)message_ptr->data[1];
        const grid_config_t* const grid_config_ptr = (grid_config_t*)message_ptr->data[2];

        if (0 < launch_kernel(kernel_entry_addr, kernel_stack_addr, kernel_params_ptr, grid_config_ptr))
        {
            // TODO FIXME send an error message if the kernel returns an error
            message_ptr->id = MESSAGE_ID_KERNEL_LAUNCH_NACK;
            message_send_worker(shire_id, hart_id, message_ptr);
        }
    }
    else if (message_ptr->id == MESSAGE_ID_LOOPBACK)
    {
        message_send_worker(shire_id, hart_id, message_ptr);
    }
    else
    {
        // TODO FIXME HACK for now, reflect all other received message back to master
        message_send_worker(shire_id, hart_id, message_ptr);
    }
}
