#include <bl2_watchdog.h>

/* The driver can populate this structure with the defaults that will be used during the init
 phase.*/

static struct watchdog_control_block wdog_control_block __attribute__((section(".data")));

int32_t watchdog_init(uint32_t timeout_msec)
{
    //TODO : Init Watchdog counter with the given timeout */
    //TODO: Enable interrupt handler for the SPIO_WDT_INTR*/
    //TODO: Enable PMIC interface to handle second interrupt
    //   and reset the system, spio_wdt_sys_rstn*.
    //TODO: Starts the watch dog
    //https://esperantotech.atlassian.net/browse/SW-6751

    (void)timeout_msec;

    return 0;
}

int32_t watchdog_error_init(dm_event_isr_callback event_cb)
{
    /* Save the handle to the timeout callback */
    wdog_control_block.event_cb = event_cb;

    return 0;
}

int32_t watchdog_start(void)
{
    //TODO:https://esperantotech.atlassian.net/browse/SW-6751
    return 0;
}

int32_t watchdog_stop(void)
{
    // TODO: https://esperantotech.atlassian.net/browse/SW-6751
    return 0;
}

void watchdog_kick(void)
{
    //TODO: Feed the wdog - https://esperantotech.atlassian.net/browse/SW-6751
}

int32_t get_watchdog_timeout(uint32_t *timeout_msec)
{
    *timeout_msec = wdog_control_block.timeout_msec;
    return 0;
}

int32_t get_watchdog_max_timeout(uint32_t *timeout_msec)
{
    *timeout_msec = wdog_control_block.max_timeout_msec;
    return 0;
}

void watchdog_isr(void)
{
    //TODO : https://esperantotech.atlassian.net/browse/SW-6751
    /* Take any corrective action required */

    /* Invoke the event handler callback */
    if (wdog_control_block.event_cb) {
        struct event_message_t message;
        /* add details in message header and fill payload */
        FILL_EVENT_HEADER(&message.header, WDOG_TIMEOUT,
                          sizeof(struct event_message_t) - sizeof(struct cmn_header_t));
        FILL_EVENT_PAYLOAD(&message.payload, CRITICAL, 0, 0, 0);
        wdog_control_block.event_cb(CORRECTABLE, &message);
    }
}
