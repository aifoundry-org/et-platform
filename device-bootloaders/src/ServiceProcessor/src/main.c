#include "serial.h"
#include "interrupt.h"
#include "mailbox.h"
#include "pcie.h"
#include "dummy_isr.h"
#include "etsoc_hal/cm_esr.h"
#include "etsoc_hal/rm_esr.h"
#include "hal_device.h"

#include "sp_pll.h"
#include "sp_ddr_config.h"
#include "minion_pll_and_dll.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>

#define TASK_STACK_SIZE 4096 // overkill for now

void taskA(void *pvParameters);
void taskB(void *pvParameters);

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize);
void vApplicationIdleHook(void);
void vApplicationTickHook(void);
void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName);

static int release_memshire_from_reset(void) {
    volatile Reset_Manager_t * rm_esr = (Reset_Manager_t *)R_SP_CRU_BASEADDR;
    //volatile Clock_Manager_t * cm_esr = (Clock_Manager_t *)R_SP_CRU_BASEADDR;

    rm_esr->rm_memshire_cold.R = ((Reset_Manager_rm_memshire_cold_t){ .B = { .rstn = 0x01 } }).R;
    rm_esr->rm_memshire_warm.R = ((Reset_Manager_rm_memshire_warm_t){ .B = { .rstn = 0xFF } }).R;
    return 0;
}

static int release_minions_from_cold_reset(void) {
    volatile Reset_Manager_t * rm_esr = (Reset_Manager_t *)R_SP_CRU_BASEADDR;
    //volatile Clock_Manager_t * cm_esr = (Clock_Manager_t *)R_SP_CRU_BASEADDR;

    rm_esr->rm_minion.R = ((Reset_Manager_rm_minion_t){ .B = { .cold_rstn = 1, .warm_rstn = 1 } }).R;
 
    return 0;
}

static int release_minions_from_warm_reset(void) {
    volatile Reset_Manager_t * rm_esr = (Reset_Manager_t *)R_SP_CRU_BASEADDR;
    //volatile Clock_Manager_t * cm_esr = (Clock_Manager_t *)R_SP_CRU_BASEADDR;

    rm_esr->rm_minion_warm_a.R = ((Reset_Manager_rm_minion_warm_a_t){ .B = { .rstn = 0xFFFFFFFF } }).R;
    rm_esr->rm_minion_warm_b.R = ((Reset_Manager_rm_minion_warm_b_t){ .B = { .rstn = 0x3 } }).R;
 
    return 0;
}

int main(void)
{
    // Disable buffering on stdout
    setbuf(stdout, NULL);

    SERIAL_init(UART0);
    printf("alive\r\n");

    SERIAL_init(UART1);
    SERIAL_write(UART1, "alive\r\n", 7);

    sp_pll_stage_0_init();
    printf("SP PLLs 0 and 1 and PShire PLL configured and locked.\n");

#if 0
    INT_init();
    PCIe_init(false /*expect_link_up*/);
    MBOX_init();
#else
    printf("Skipping INT, PCIe and MBOX init...\n");
#endif

    sp_pll_stage_1_init();
    printf("SP PLLs 2 & 4 configured and locked.\n");

    if (0 != release_memshire_from_reset()) {
        printf("release_memshire_from_reset() failed!\n");
        goto FATAL_ERROR;
    }
    printf("Memshire released from reset\n");

    if (0 != ddr_config()) {
        printf("ddr_config() failed!\n");
        goto FATAL_ERROR;
    }
    printf("DRAM ready.\n");

    if (0 != release_minions_from_cold_reset()) {
        printf("release_minions_from_cold_reset() failed!\n");
        goto FATAL_ERROR;
    }
    printf("Released Minion shires from cold reset.\n");

    if (0 != release_minions_from_warm_reset()) {
        printf("release_minions_from_warm_reset() failed!\n");
        goto FATAL_ERROR;
    }
    printf("Released Minions from warm reset.\n");

    if (0 != configure_minion_plls_and_dlls()) {
        printf("configure_minion_plls_and_dlls() failed!\n");
        goto FATAL_ERROR;
    }
    printf("Minion shires PLLs and DLLs configured.\n");

    if (0 != enable_minion_neighborhoods()) {
        printf("Failed to enable minion neighborhoods!\n");
        goto FATAL_ERROR;
    }
    printf("Minion neighborhoods enabled.\n");

    if (0 != enable_minion_threads()) {
        printf("Failed to enable minion threads!\n");
        goto FATAL_ERROR;
    }
    printf("Minion threads enabled.\n");


    static TaskHandle_t taskHandleA;
    static StackType_t stackA[TASK_STACK_SIZE];
    static StaticTask_t taskBufferA;

    static TaskHandle_t taskHandleB;
    static StackType_t stackB[TASK_STACK_SIZE];
    static StaticTask_t taskBufferB;

    taskHandleA = xTaskCreateStatic(taskA,
                                    "task A",
                                    TASK_STACK_SIZE,
                                    NULL,
                                    1,
                                    stackA,
                                    &taskBufferA);

    taskHandleB = xTaskCreateStatic(taskB,
                                    "task B",
                                    TASK_STACK_SIZE,
                                    NULL,
                                    1,
                                    stackB,
                                    &taskBufferB);

    if ((taskHandleA == NULL) || (taskHandleB == NULL))
    {
        printf("taskHandle error\r\n");
    }

    vTaskStartScheduler();

FATAL_ERROR:
    printf("FATAL ERROR!!!\n");
    for(;;);
}

void taskA(void *pvParameters)
{
    (void)pvParameters;

    // Disable buffering on stdout
    setbuf(stdout, NULL);

    while (1)
    {
        //printf("A");
        vTaskDelay(2U);
    }
}

void taskB(void *pvParameters)
{
    (void)pvParameters;

    // Disable buffering on stdout
    setbuf(stdout, NULL);

    while (1)
    {
        //printf("B");
        vTaskDelay(3U);
    }
}

/* configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
    /* If the buffers to be provided to the Idle task are declared inside this
    function then they must be declared static - otherwise they will be allocated on
    the stack and so not exists after this function exits. */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
    state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationIdleHook(void)
{
    // "WFI is available in all of the supported S and M privilege modes,
    //  and optionally available to U-mode for implementations that support U-mode interrupts."
    asm("wfi");
}

void vApplicationTickHook(void)
{
    // TODO FIXME watchdog checking goes here
    //SERIAL_write(UART0, ".", 1);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
}
