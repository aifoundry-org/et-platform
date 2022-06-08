/***********************************************************************
*
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
#ifndef __BL2_CACHE_CONTROL__
#define __BL2_CACHE_CONTROL__

#include <stdint.h>
#include "dm_event_def.h"
#include "bl_error_code.h"
#include "config/mgmt_build_config.h"
#include <interrupt.h>
#include "etsoc/isa/io.h"
#include "hwinc/etsoc_shire_cache_esr.h"
#include "hwinc/sp_cru_reset.h"
#include "hwinc/sp_plic.h"
#include "hwinc/spio_plic_intr_device.h"
#include "hwinc/hal_device.h"
#include "esr.h"
#include "minion_configuration.h"

/*! \def SPIO_PLIC
*/
#ifndef SPIO_PLIC
#define SPIO_PLIC R_SP_PLIC_BASEADDR
#endif


/*! \def NUMBER_OF_MINSHIRE
*/
#define NUMBER_OF_MINSHIRE     33

#define FOR_EACH_MINSHIRE(statements)                                              \
            {                                                                      \
                for(minshire = 0;minshire < NUMBER_OF_MINSHIRE;++minshire) {       \
                   if (shire_mask & 1) {                                           \
                      statements                                                   \
                   }                                                               \
                   shire_mask >>= 1;                                               \
                }                                                                  \
            }

/*! \def CACHE_LINE_SIZE
    \brief Macro representing the 64-byte cache line size
*/
#define CACHE_LINE_SIZE                   64

/*! \def L2_SHIRE_BANKS
    \brief Macro representing the number of L2 banks in a shire.
    TODO: Its a dummy value.
*/
#define L2_SHIRE_BANKS 4U

/*! \def BIT
    \brief Macto setting bit position
 */
#ifndef BIT
#define BIT(x) (0x1ul << x)
#endif

/*!
 * @enum shire_cache_error_type
 * @brief Enum defining event/error type
 */
enum shire_cache_error_type {
    SINGLE_BIT_ECC = 0,
    DOUBLE_BIT_ECC,
    ECC_ERROR_COUNTER_SATURATION,
    DECODE_AND_SLAVE_ERRORS,
    PERFORMANCE_COUNTER_SATURATION
};

/*!
 * @struct struct sram_event_control_block
 * @brief SRAM driver error mgmt control block
 */
typedef struct sram_event_control_block
{
    uint32_t ce_count;              /**< Correctable error count. */
    uint32_t uce_count;             /**< Un-Correctable error count. */
    uint32_t ce_threshold;          /**< Correctable error threshold. */
    dm_event_isr_callback event_cb; /**< Event callback handler. */
} sram_event_control_block_t;

/*!
 * @struct struct sram_and_icache_event_control_block
 * @brief SRAM and ICache driver error mgmt control block
 */
struct sram_and_icache_event_control_block
{
    sram_event_control_block_t sram;
    sram_event_control_block_t icache;
};

/*! \def SC_BANK_BROADCAST
    \brief Shire cache bank broadcast ID
*/
#define SC_BANK_BROADCAST 0xFu

/*! \def SC_BANK_SIZE
    \brief Shire cache bank size (1MB)
*/
#define SC_BANK_SIZE 0x400u

/*! \def SC_BANK_NUM
    \brief Number of banks in shire cache
*/
#define SC_BANK_NUM 4

int32_t icache_error_control_init(dm_event_isr_callback event_cb);
int32_t icache_error_control_deinit(uint64_t shire_mask);

int32_t icache_err_interrupt_enable(uint64_t shire_mask, uint32_t interrupt_mask);
int32_t icache_enable_ce_interrupt(uint64_t shire_mask);
int32_t icache_enable_uce_interrupt(uint64_t shire_mask);
int32_t icache_enable_ecc_counter_saturation_interrupt(uint64_t shire_mask);
int32_t icache_enable_all_interrupts(uint64_t shire_mask);

int32_t icache_err_interrupt_disable(uint64_t shire_mask, uint32_t interrupt_mask);
int32_t icache_disable_ce_interrupt(uint64_t shire_mask);
int32_t icache_disable_uce_interrupt(uint64_t shire_mask);
int32_t icache_disble_ecc_counter_saturation_interrupt(uint64_t shire_mask);
int32_t icache_disable_all_interrupts(uint64_t shire_mask);

void    sram_and_icache_error_isr(void);

/*! \fn int32_t sram_error_control_init(dm_event_isr_callback event_cb)
    \brief This function inits sram error control subsystem.
    \param event_cb Event callback handler.
    \return Status indicating success or negative error
*/

int32_t sram_error_control_init(dm_event_isr_callback event_cb);

/*! \fn int32_t sram_error_control_deinit(uint64_t shire_mask)
    \brief This function cleans up the sram error control subsystem.
    \param shire_mask Active shire mask
    \return Status indicating success or negative error
*/

int32_t sram_error_control_deinit(uint64_t shire_mask);

/*! \fn int32_t sc_err_interrupt_enable(uint64_t shire_mask, uint32_t interrupt_mask)
    \brief This function enables shire cache error interrupts.
    \param shire_mask Active shire mask
           interrupt_mask Mask for interrupts to be enabled
    \return Status indicating success or negative error
*/
int32_t sc_err_interrupt_enable(uint64_t shire_mask, uint32_t interrupt_mask);

/*! \fn int32_t sram_enable_ecc_counter_saturation_interrupt(uint64_t shire_mask)
    \brief This function enables sram ecc counter saturation interrupts.
    \param shire_mask Active shire mask
    \return Status indicating success or negative error
*/
int32_t sram_enable_ecc_counter_saturation_interrupt(uint64_t shire_mask);

/*! \fn int32_t sram_enable_decode_and_slave_interrupt(uint64_t shire_mask)
    \brief This function enables sram decode and slave error interrupts.
    \param shire_mask Active shire mask
    \return Status indicating success or negative error
*/
int32_t sram_enable_decode_and_slave_interrupt(uint64_t shire_mask);

/*! \fn int32_t sram_enable_perf_counter_saturation_interrupt(uint64_t shire_mask)
    \brief This function enables sram performance counter saturation interrupts.
    \param shire_mask Active shire mask
    \return Status indicating success or negative error
*/
int32_t sram_enable_perf_counter_saturation_interrupt(uint64_t shire_mask);

/*! \fn int32_t sram_enable_all_interrupts(uint64_t shire_mask)
    \brief This function enables all sram error interrupts.
    \param shire_mask Active shire mask
    \return Status indicating success or negative error
*/
int32_t sram_enable_all_interrupts(uint64_t shire_mask);

/*! \fn int32_t sc_err_interrupt_disable(uint64_t shire_mask, uint32_t interrupt_mask)
    \brief This function disables shire cache error interrupts.
    \param shire_mask Active shire mask
           interrupt_mask Mask for interrupts to be enabled
    \return Status indicating success or negative error
*/
int32_t sc_err_interrupt_disable(uint64_t shire_mask, uint32_t interrupt_mask);

/*! \fn int32_t sram_disable_all_interrupts(uint64_t shire_mask)
    \brief This function disables all sram error interrupts.
    \param shire_mask Active shire mask
    \return Status indicating success or negative error
*/
int32_t sram_disable_all_interrupts(uint64_t shire_mask);

/*! \fn int32_t sram_enable_ce_interrupt(uint64_t shire_mask)
    \brief This function enables sram correctable error interrupts.
    \param shire_mask Active shire mask
    \return Status indicating success or negative error
*/

int32_t sram_enable_ce_interrupt(uint64_t shire_mask);

/*! \fn int32_t sram_enable_uce_interrupt(uint64_t shire_mask)
    \brief This function enables sram uncorrectable error interrupts.
    \param shire_mask Active shire mask
    \return Status indicating success or negative error
*/

int32_t sram_enable_uce_interrupt(uint64_t shire_mask);

/*! \fn int32_t sram_disable_ce_interrupt(uint64_t shire_mask)
    \brief This function dis-ables sram corretable error interrupts when threshold is exceeded.
    \param shire_mask Active shire mask
    \return Status indicating success or negative error
*/

int32_t sram_disable_ce_interrupt(uint64_t shire_mask);

/*! \fn int32_t sram_disable_uce_interrupt(uint64_t shire_mask)
    \brief This function dis-ables sram un-corretable error interrupts
    \param shire_mask Active shire mask
    \return Status indicating success or negative error
*/

int32_t sram_disable_uce_interrupt(uint64_t shire_mask);

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

/*! \fn int32_t icache_error_control_init(dm_event_isr_callback event_cb)
    \brief This function inits icache error control subsystem.
    \param event_cb Event callback handler.
    \return Status indicating success or negative error
*/

int32_t sram_error_control_init(dm_event_isr_callback event_cb);

/*! \fn uint32_t Cache_Control_SCP_size(uint64_t shire_mask)
    \brief This function provides SCP size (KB)
    \param shire_mask Active shire mask
    \return SCP size in KiloBytes
*/
uint32_t Cache_Control_SCP_size(uint64_t shire_mask);

/*! \fn uint32_t Cache_Control_L2_size(uint64_t shire_mask)
    \brief This function provides L2 size (KB)
    \param shire_mask Active shire mask
    \return L2 size in KiloBytes
*/
uint32_t Cache_Control_L2_size(uint64_t shire_mask);

/*! \fn uint32_t Cache_Control_L3_size(uint64_t shire_mask)
    \brief This function provides L3 cache size (KB)
    \param shire_mask Active shire mask
    \return L3 size in KiloBytes
*/
uint32_t Cache_Control_L3_size(uint64_t shire_mask);

/*! \fn int cache_scp_l2_l3_size_config(uint16_t scp_size, uint16_t l2_size, uint16_t l3_size,
*                                    uint64_t shire_mask)
    \brief This function sets cache configuration.
*          Configuration is per cache bank (1MB), and all banks are configured the same.
*          Following constraint must be followed:
*             - (scp_size + l2_size + l3_size) <= 0x400
*             - scp_size, l2_size and l3_size must be multiple of 2
*             - if l2_size !=0 then l2_size >= 0x40
    \param scp_size scratchpad size
    \param l2_size l2 cache size
    \param l3_size l3 cache size
    \param shire_mask shire_mask
    \return Status indicating success or negative error
*/
int cache_scp_l2_l3_size_config(uint16_t scp_size, uint16_t l2_size, uint16_t l3_size,
                                    uint64_t shire_mask);

#endif
