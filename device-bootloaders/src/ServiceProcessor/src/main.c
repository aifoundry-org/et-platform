#include "serial.h"
#include "interrupt.h"
#include "pcie.h"
#include "dummy_isr.h"

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

int main(void)
{
    // Disable buffering on stdout
    setbuf(stdout, NULL);

    SERIAL_init(UART0);
    printf("alive\r\n");

    SERIAL_init(UART1);
    SERIAL_write(UART1, "alive\r\n", 7);

    INT_init();
    PCIe_init();

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
}

void taskA(void *pvParameters)
{
    (void)pvParameters;

    // Disable buffering on stdout
    setbuf(stdout, NULL);

    while (1)
    {
        printf("A");
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
        printf("B");
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
    SERIAL_write(UART0, ".", 1);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
}
