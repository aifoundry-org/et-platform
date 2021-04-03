#ifndef __BL2_DDR_INIT_H__
#define __BL2_DDR_INIT_H__

#include <stdint.h>

#include "ms_reg_def.h"
#include "ddrc_reg_def.h"
#include "dm_event_def.h"

#define MODE_NUMBER 6

void ddr_init(uint8_t memshire_id);
int ddr_config(void);

uint8_t ms_init_ddr_phy_1067(uint8_t memshire);
uint8_t ms_init_seq_phase1(uint8_t memshire, uint8_t config_ecc, uint8_t config_real_pll);
uint8_t ms_init_seq_phase2(uint8_t memshire, uint8_t config_real_pll);
uint8_t ms_init_seq_phase3(uint8_t memshire);
uint8_t ms_init_seq_phase4(uint8_t memshire);




/*!
 * @struct struct ddr_event_control_block
 * @brief DDR driver error mgmt control block
 */
struct ddr_event_control_block
{
    uint32_t ce_count;              /**< Correctable error count. */
    uint32_t uce_count;             /**< Un-Correctable error count. */
    uint32_t ce_threshold;          /**< Correctable error threshold. */
    dm_event_isr_callback event_cb; /**< Event callback handler. */
};

/*! \fn int32_t ddr_error_control_init(dm_event_isr_callback event_cb)
    \brief This function initializes the ddr error control subsystem, including
           programming the default error thresholds, enabling the error interrupts
           and setting up globals.
    \param event_cb pointer to the error call back function
    \return Status indicating success or negative error
*/


int32_t ddr_error_control_init(dm_event_isr_callback event_cb);

/*! \fn int32_t ddr_error_control_deinit(void)
    \brief This function cleans up the ddr error control subsystem.
    \param none
    \return Status indicating success or negative error
*/

int32_t ddr_error_control_deinit(void);

/*! \fn int32_t ddr_enable_uce_interrupt(void)
    \brief This function enables ddr uncorrectable error interrupts.
    \param none
    \return Status indicating success or negative error
*/

int32_t ddr_enable_uce_interrupt(void);

/*! \fn int32_t ddr_disable_ce_interrupt(void)
    \brief This function dis-ables ddr corretable error interrupts when threshold is exceeded.
    \param none
    \return Status indicating success or negative error
*/

int32_t ddr_disable_ce_interrupt(void);

/*! \fn int32_t ddr_disable_uce_interrupt(void)
    \brief This function dis-ables ddr un-corretable error interrupts
    \param none
    \return Status indicating success or negative error
*/

int32_t ddr_disable_uce_interrupt(void);

/*! \fn int32_t ddr_set_ce_threshold(uint32_t ce_threshold)
    \brief This function programs the ddr correctable error threshold
    \param ce_threshold threshold value to set
    \return Status indicating success or negative error
*/

int32_t ddr_set_ce_threshold(uint32_t ce_threshold);

/*! \fn int32_t ddr_get_ce_count(uint32_t *ce_count)
    \brief This function provides the current correctable error count
    \param ce_count pointer to variable to hold count value
    \return Status indicating success or negative error
*/

int32_t ddr_get_ce_count(uint32_t *ce_count);

/*! \fn int32_t ddr_get_uce_count(uint32_t *uce_count)
    \brief This function provides the current un-correctable error count
    \param uce_count pointer to variable to hold count value
    \return Status indicating success or negative error
*/

int32_t ddr_get_uce_count(uint32_t *uce_count);

void ddr_error_threshold_isr(void);  //TODO: WILL BE MADE STATIC FUNCION WITH ACTUAL ISR IMPLEMENTATION

/*! \fn int ddr_get_memory_details(char *mem_vendor, char *mem_part)
    \brief This function get Memory vendor and part details
    \param mem_vendor pointer to variable to hold Memory vendor value
    \param mem_part pointer to variable to hold Memory part value
    \return Status indicating success or negative error
*/
int ddr_get_memory_details(char *mem_vendor, char *mem_part);

/*! \fn int ddr_get_memory_type(char *mem_type)
    \brief This function get Memory type details
    \param mem_type pointer to variable to hold memory type value
    \return Status indicating success or negative error
*/
int ddr_get_memory_type(char *mem_type);

#endif

