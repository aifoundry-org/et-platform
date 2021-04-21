
#include "bl2_pcie.h"

/* The driver can populate this structure with the defaults that will be used during the init
    phase.*/

static struct pcie_event_control_block event_control_block __attribute__((section(".data")));

/*!
 * @struct struct pcie_event_control_block
 * @brief PCIE driver error mgmt control block
 */
struct pcie_event_control_block {
    uint32_t ce_count; /**< Correctable error count. */
    uint32_t uce_count; /**< Un-Correctable error count. */
    uint32_t ce_threshold; /**< Correctable error threshold. */
    dm_event_isr_callback event_cb; /**< Event callback handler. */
};

/*! \var static uint32_t PCIE_SPEED[5]
    \brief PCIE Speed Generation
*/
static uint32_t PCIE_SPEED[5] = { PCIE_GEN_1, PCIE_GEN_2, PCIE_GEN_3, PCIE_GEN_4, PCIE_GEN_5 };

int32_t pcie_error_control_init(dm_event_isr_callback event_cb)
{
    event_control_block.event_cb = event_cb;
    return 0;
}

int32_t pcie_error_control_deinit(void)
{
    return 0;
}

int32_t pcie_enable_uce_interrupt(void)
{
    return 0;
}

int32_t pcie_disable_ce_interrupt(void)
{
    return 0;
}

int32_t pcie_disable_uce_interrupt(void)
{
    return 0;
}

int32_t pcie_set_ce_threshold(uint32_t ce_threshold)
{
    /* set countable errors threshold */
    event_control_block.ce_threshold = ce_threshold;
    return 0;
}

int32_t pcie_get_ce_count(uint32_t *ce_count)
{
    /* get correctable errors count */
    *ce_count = event_control_block.ce_count;
    return 0;
}

int32_t pcie_get_uce_count(uint32_t *uce_count)
{
    /* get un-correctable errors count */
    *uce_count = event_control_block.uce_count;
    return 0;
}

void pcie_error_threshold_isr(void)
{
    // TODO: This is just an example implementation.
    // The final driver implementation will read these values from the
    // hardware, create a message and invoke call back with message and error type as parameters.
    uint8_t error_type = CORRECTABLE;

    if ((error_type == UNCORRECTABLE) ||
        (++event_control_block.ce_count > event_control_block.ce_threshold)) {
        struct event_message_t message;

        /* add details in message header and fill payload */
        FILL_EVENT_HEADER(&message.header, PCIE_UCE,
                          sizeof(struct event_message_t) - sizeof(struct cmn_header_t));
        FILL_EVENT_PAYLOAD(&message.payload, CRITICAL, 1024, 1, 0);

        /* call the callback function and post message */
        event_control_block.event_cb(CORRECTABLE, &message);
    }
}

int32_t setup_pcie_gen3_link_speed(void)
{
    //TODO: https://esperantotech.atlassian.net/browse/SW-6607
    return 0;
}

int32_t setup_pcie_gen4_link_speed(void)
{
    //TODO: https://esperantotech.atlassian.net/browse/SW-6607
    return 0;
}

int32_t setup_pcie_lane_width_x4(void)
{
    //TODO: https://esperantotech.atlassian.net/browse/SW-6607
    return 0;
}

int32_t setup_pcie_lane_width_x8(void)
{
    //TODO: https://esperantotech.atlassian.net/browse/SW-6607
    return 0;
}

int32_t pcie_retrain_phy(void)
{
    //TODO: https://esperantotech.atlassian.net/browse/SW-6607
    return 0;
}

int pcie_get_speed(char *pcie_speed)
{
    uint32_t pcie_gen, tmp;

    tmp = ioread32(PCIE_CUST_SS +
                   DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_LINK_DBG_2_ADDRESS);

    // Get the PCIE Gen
    pcie_gen = DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_LINK_DBG_2_RATE_GET(tmp);

    sprintf(pcie_speed, "%d", PCIE_SPEED[pcie_gen - 1]);

    return 0;
}

int PShire_Initialize(void)
{
    PCIe_release_pshire_from_reset();
    configure_pcie_pll();
    return 0;
}

int PCIE_Controller_Initialize(void)
{
    PCIe_init(true /* expect_link_up*/);
    return 0;
}

int PCIE_Phy_Initialize(void)
{
    /* interface to initialize PCIe phy */
    return 0;
}

int PShire_Voltage_Update(uint8_t voltage)
{
    return pmic_set_voltage(PCIE, voltage);
}

int Pshire_PLL_Program(uint8_t mode)
{
    return  configure_pshire_pll(mode);
}

int Pshire_NOC_update_routing_table(void)
{
    /* interface to update routing table */
    return 0;
}

int PCIe_Phy_Firmware_Update (uint64_t* image)
{
    /* interface to initialize PCIe phy */
    (void)image;
    return 0;
}