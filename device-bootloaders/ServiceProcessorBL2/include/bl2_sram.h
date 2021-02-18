#ifndef __BL2_SRAM__
#define __BL2_SRAM__

#include <stdint.h>
#include "dm_event_def.h"


/*!
 * @struct struct sram_event_control_block
 * @brief SRAM driver error mgmt control block
 */
struct sram_event_control_block
{
    uint32_t ce_count;              /**< Correctable error count. */
    uint32_t uce_count;             /**< Un-Correctable error count. */
    uint32_t ce_threshold;          /**< Correctable error threshold. */
    dm_event_isr_callback event_cb; /**< Event callback handler. */
};

/*! \fn int32_t sram_error_control_init(dm_event_isr_callback event_cb)
    \brief This function initializes the sram error control subsystem, including
           programming the default error thresholds, enabling the error interrupts
           and setting up globals.
    \param event_cb pointer to the error call back function
    \return Status indicating success or negative error
*/


int32_t sram_error_control_init(dm_event_isr_callback event_cb);

/*! \fn int32_t sram_error_control_deinit(void)
    \brief This function cleans up the sram error control subsystem.
    \param none
    \return Status indicating success or negative error
*/

int32_t sram_error_control_deinit(void);

/*! \fn int32_t sram_enable_uce_interrupt(void)
    \brief This function enables sram uncorretable error interrupts.
    \param none
    \return Status indicating success or negative error
*/

int32_t sram_enable_uce_interrupt(void);

/*! \fn int32_t sram_disable_ce_interrupt(void)
    \brief This function dis-ables sram corretable error interrupts when threshold is exceeded.
    \param none
    \return Status indicating success or negative error
*/

int32_t sram_disable_ce_interrupt(void);

/*! \fn int32_t sram_disable_uce_interrupt(void)
    \brief This function dis-ables sram un-corretable error interrupts
    \param none
    \return Status indicating success or negative error
*/

int32_t sram_disable_uce_interrupt(void);

/*! \fn int32_t sram_set_ce_threshold(uint32_t ce_threshold)
    \brief This function programs the sram correctable error threshold
    \param ce_threshold threshold value to set
    \return Status indicating success or negative error
*/

int32_t sram_set_ce_threshold(uint32_t ce_threshold);

/*! \fn int32_t sram_get_ce_count(uint32_t *ce_count)
    \brief This function provides the current correctable error count
    \param ce_count pointer to variable to hold count value
    \return Status indicating success or negative error
*/

int32_t sram_get_ce_count(uint32_t *ce_count);

/*! \fn int32_t sram_get_uce_count(uint32_t *uce_count)
    \brief This function provides the current un-correctable error count
    \param uce_count pointer to variable to hold count value
    \return Status indicating success or negative error
*/

int32_t sram_get_uce_count(uint32_t *uce_count);

void sram_error_threshold_isr(void);  //TODO: WILL BE MADE STATIC FUNCION WITH ACTUAL ISR IMPLEMENTATION

#endif
