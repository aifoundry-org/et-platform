#ifndef __BL2_PCIE__
#define __BL2_PICE__

#include <stdint.h>
#include "dm_event_def.h"

/*!
 * @struct struct pcie_event_control_block
 * @brief PCIE driver error mgmt control block
 */
struct pcie_event_control_block
{
    uint32_t ce_count;              /**< Correctable error count. */
    uint32_t uce_count;             /**< Un-Correctable error count. */
    uint32_t ce_threshold;          /**< Correctable error threshold. */
    dm_event_isr_callback event_cb; /**< Event callback handler. */
};


/*! \fn int32_t pcie_error_control_init(dm_event_isr_callback event_cb)
    \brief This function initializes the PCIe error control subsystem, including
           programming the default error thresholds, enabling the error interrupts
           and setting up globals.
    \param event_cb pointer to the error call back function
    \return Status indicating success or negative error
*/


int32_t pcie_error_control_init(dm_event_isr_callback event_cb);

/*! \fn int32_t pcie_error_control_deinit(void)
    \brief This function cleans up the PCIe error control subsystem.
    \param none
    \return Status indicating success or negative error
*/

int32_t pcie_error_control_deinit(void);

/*! \fn int32_t pcie_enable_uce_interrupt(void)
    \brief This function enables pcie uncorretable error interrupts.
    \param none
    \return Status indicating success or negative error
*/

int32_t pcie_enable_uce_interrupt(void);

/*! \fn int32_t pcie_disable_ce_interrupt(void)
    \brief This function dis-ables pcie corretable error interrupts when threshold is exceeded.
    \param none
    \return Status indicating success or negative error
*/

int32_t pcie_disable_ce_interrupt(void);

/*! \fn int32_t pcie_disable_uce_interrupt(void)
    \brief This function dis-ables pcie un-corretable error interrupts
    \param none
    \return Status indicating success or negative error
*/

int32_t pcie_disable_uce_interrupt(void);

/*! \fn int32_t pcie_set_ce_threshold(uint32_t ce_threshold)
    \brief This function programs the PCIe correctable error threshold
    \param ce_threshold threshold value to set
    \return Status indicating success or negative error
*/

int32_t pcie_set_ce_threshold(uint32_t ce_threshold);

/*! \fn int32_t pcie_get_ce_count(uint32_t *ce_count)
    \brief This function provides the current correctable error count
    \param ce_count pointer to variable to hold count value
    \return Status indicating success or negative error
*/

int32_t pcie_get_ce_count(uint32_t *ce_count);

/*! \fn int32_t pcie_get_uce_count(uint32_t *uce_count)
    \brief This function provides the current un-correctable error count
    \param uce_count pointer to variable to hold count value
    \return Status indicating success or negative error
*/

int32_t pcie_get_uce_count(uint32_t *uce_count);

void pcie_error_threshold_isr(void);  //TODO: WILL BE MADE STATIC FUNCION WITH ACTUAL ISR IMPLEMENTATION

int32_t setup_pcie_gen3_link_speed(void);
int32_t setup_pcie_gen4_link_speed(void);
int32_t setup_pcie_lane_width_x4(void);
int32_t setup_pcie_lane_width_x8(void);
int32_t pcie_retrain_phy(void);

#endif
