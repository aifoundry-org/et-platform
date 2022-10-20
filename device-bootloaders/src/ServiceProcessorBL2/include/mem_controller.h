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
/*! \file mem_controller.h
    \brief A C header that defines the DDR memory subsystem's
    public interfaces. These interfaces provide services using which
    DDR can be initialized and report error events.
*/
/***********************************************************************/

#ifndef __MEM_CONTROLLER_H__
#define __MEM_CONTROLLER_H__

#include <stdint.h>

#include "hwinc/ms_reg_def.h"
#include "hwinc/ddrc_reg_def.h"
#include "hwinc/spio_plic_intr_device.h"
#include "hwinc/sp_plic.h"
#include "hwinc/sp_cru_reset.h"
#include "hwinc/hal_device.h"
#include "dm_event_def.h"
#include "bl2_pmic_controller.h"
#include "bl2_reset.h"
#include "bl_error_code.h"
#include "etsoc/isa/io.h"
#include "config/mgmt_build_config.h"
#include <interrupt.h>

/*! \def DDR_BANDWIDTH
    \brief Macro representing the DDR bandwidth in MB/sec.
    TODO: Its a dummy value.
*/
#define DDR_BANDWIDTH 128000U

/*! \def NUMBER_OF_MEMSHIRE
*/
#define HW_NUMBER_OF_MEMSHIRE 8

#define MEMSHIRE_BASE 0

#define NUMBER_OF_MEMSHIRE 8

#define FOR_EACH_MEMSHIRE(statements)                                                             \
    {                                                                                             \
        for (memshire = MEMSHIRE_BASE; memshire < MEMSHIRE_BASE + NUMBER_OF_MEMSHIRE; ++memshire) \
        {                                                                                         \
            statements                                                                            \
        }                                                                                         \
    }
#define FOR_EACH_MEMSHIRE_EVEN_FIRST(statements)                                          \
    {                                                                                     \
        for (memshire = MEMSHIRE_BASE; memshire < MEMSHIRE_BASE + NUMBER_OF_MEMSHIRE;     \
             memshire += 2)                                                               \
        {                                                                                 \
            statements                                                                    \
        }                                                                                 \
        for (memshire = MEMSHIRE_BASE + 1; memshire < MEMSHIRE_BASE + NUMBER_OF_MEMSHIRE; \
             memshire += 2)                                                               \
        {                                                                                 \
            statements                                                                    \
        }                                                                                 \
    }

/*
 * DDR Controller macros - temporary fix
 */
#define DDRC_INT_STATUS_ESR_MC0_ECC_CORRECTED_ERR_INTR_GET(x)   (((x)&0x0080u) >> 7)
#define DDRC_INT_STATUS_ESR_MC0_ECC_UNCORRECTED_ERR_INTR_GET(x) ((x)&0x0001u)

/*
** Diagnostic macros
*/
#define DDR_DIAG                       (DDR_DIAG_MEMSHIRE_ID)
#define DDR_DIAG_DEBUG_INFO            (0x01 << 0)
#define DDR_DIAG_MEMSHIRE_ID           (0x01 << 1)
#define DDR_DIAG_VERIFY_REGISTER_WRITE (0x01 << 2)
#define DDR_DIAG_VERIFY_SRAM_WRITE     (0x01 << 3)

#if (DDR_DIAG & DDR_DIAG_MEMSHIRE_ID)
#define CHECK_MEMSHIRE_ID(memshire) check_memshire_revision_id(memshire)
#else
#define CHECK_MEMSHIRE_ID(memshire)
#endif

/* DDR size values */
#define SIZE_4GB  0x0000000100000000ULL
#define SIZE_8GB  0x0000000200000000ULL
#define SIZE_16GB 0x0000000400000000ULL
#define SIZE_32GB 0x0000000800000000ULL

/**
 * @enum ms_status_t
 * @brief Enum defines for memory status
 */
typedef enum
{
    WORKING,
    FAILED,
    DISABLED,
    UNINITIALIZED,
} ms_status_t;

/**
 * @struct ms_dram_status_t
 * @brief Structure contains dram status
 */
typedef struct
{
    uint32_t system_status; // each bit represent an unexpected failure
    ms_status_t physical_memshire_status[HW_NUMBER_OF_MEMSHIRE];
} ms_dram_status_t;

/**
 * @enum ddr_frequency_t
 * @brief Frequency enum defines for DDR memory
 */
typedef enum
{
    DDR_FREQUENCY_800MHZ,
    DDR_FREQUENCY_933MHZ,
    DDR_FREQUENCY_1066MHZ
} ddr_frequency_t;

/**
 * @enum ddr_capacity_t
 * @brief Capacity enum defines for DDR memory
 */
typedef enum
{
    DDR_CAPACITY_4GB,
    DDR_CAPACITY_8GB = 0x2,
    DDR_CAPACITY_16GB = 0x4,
    DDR_CAPACITY_32GB = 0x6,
} ddr_capacity_t;

/**
 * @enum ddr_vendor_t
 * @brief Vendor enum defines for DDR memory
 */
typedef enum
{
    DDR_VENDOR_MICRON,
    DDR_VENDOR_SKHYNIX,
    DDR_VENDOR_SAMSUNG,
    DDR_VENDOR_UNDEF,
} ddr_vendor_t;

/*! \struct dma_mem_region
    \brief Structure for a memory region, used to verify DMA bounds check.
*/
struct ddr_mem_info_t
{
    uint64_t ddr_mem_size;
    uint32_t ddr_vendor_id;
};

/**
 * @struct DDR_MODE
 * @brief Structure for passing into DDR driver
 */
typedef struct
{
    ddr_frequency_t frequency;
    ddr_capacity_t capacity;
    bool ecc;
    bool training;
    bool sim_only;
} DDR_MODE;

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

/*! \fn uint64_t ms_read_chip_reg(uint32_t memshire, uint32_t mr_num)
    \brief This function reads from the memory chip, and returns the register value
    \param memshire memshire id
    \param mr_num   register number
    \return data obtained from the register
*/
uint64_t ms_read_chip_reg(uint32_t memshire, uint32_t mr_num);

/*! \fn void ms_get_dram_status(void)
    \brief This function returns a const pointer to a dram status structure
    \param none
    \return const ms_dram_status_t*
*/
const ms_dram_status_t *ms_get_dram_status(void);

/*! \fn void check_memshire_revision_id(uint32_t memshire)
    \brief This function prints out ms_memory_revision_id to debug print
    \param memshire id of a specific memshire, 0-based
    \return none
*/
void check_memshire_revision_id(uint32_t memshire);

/*! \fn int configure_memshire_plls(DDR_MODE *ddr_mode)
    \brief This function initializes every memshire present in system. It internally
           calls ddr_init with memshire id
    \param ddr_mode point to a DDR_MODE structure, which contains the parameters for DDR init
    \return Zero indicating success or non-zero for error
*/
int configure_memshire_plls(const DDR_MODE *ddr_mode);

/*! \fn uint64_t ms_read_chip_reg(uint32_t memshire, uint32_t mr_num)
    \brief This function reads value from DDR module registers
    \param memshire is memshire number
    \param mr_num is register address
    \return Zero indicating success or non-zero for error
*/
uint64_t ms_read_chip_reg(uint32_t memshire, uint32_t mr_num);

/*! \fn int ddr_config(DDR_MODE *ddr_mode)
    \brief This function initializes every memshire present in system. It internally
           calls ddr_init with memshire id
    \param ddr_mode point to a DDR_MODE structure, which contains the parameters for DDR init
    \return Zero indicating success or non-zero for error
*/
int ddr_config(const DDR_MODE *ddr_mode);

/*! \fn void ms_printout_status(uint32_t memshire, bool ddrc_enabled)
    \brief This function prints the status of the DDR Controller
    \param memshire memshire id, ddrc_enabled option to disabled DDR controller
    \return N/A
*/
void ms_printout_status(uint32_t memshire, bool ddrc_enabled);

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

/*! \fn int32_t ddr_enable_ce_interrupt(void)
    \brief This function enables ddr correctable error interrupts.
    \param none
    \return Status indicating success or negative error
*/
int32_t ddr_enable_ce_interrupt(void);

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

/*! \fn int ddr_get_memory_vendor_ID(char *vendor_ID)
    \brief This function get Memory vendor ID
    \param vendor_ID pointer to variable to hold Memory vendor ID
    \return Status indicating success or negative error
*/
int ddr_get_memory_vendor_ID(char *vendor_ID);

/*! \fn int ddr_get_memory_type(char *mem_type)
    \brief This function get Memory type details
    \param mem_type pointer to variable to hold memory type value
    \return Status indicating success or negative error
*/
int ddr_get_memory_type(char *mem_type);

/*! \fn int ddr_get_memory_size(char *mem_size)
    \brief This function get Memory size
    \param mem_type pointer to variable to hold memory size value
    \return Status indicating success or negative error
*/
int ddr_get_memory_size(char *mem_size);

/*! \fn int32_t configure_memshire(void)
    \brief This function configures the MemShire and DDR Controllers
    \param NA
    \return Status indicating success or negative error
*/
int32_t configure_memshire(void);

/*! \fn uint32_t get_memshire_frequency(void)
    \brief This function returns Memshire frequency
    \param NA
    \return memshire frequency
*/
uint32_t get_memshire_frequency(void);

/*! \fn uint32_t get_ddr_frequency(void)
    \brief This function returns DDR frequency
    \param NA
    \return DDR frequency
*/
uint32_t get_ddr_frequency(void);

/*! \fn ms_write_phy_ram (..)
    \brief Helper to write to DDR Phy RAM
    \param NA
    \return NA
*/
void ms_write_phy_ram(uint32_t memshire, const uint64_t addr, const uint32_t value);

/*! \fn mem_disable_unused_clocks (void)
    \brief Disables unused clocks in memshire
    \param NA
    \return NA
*/
void mem_disable_unused_clocks(void);

/*! \fn void memshire_clear_lock_monitor(uint8_t ms_num)
    \brief This function clears lock monitor of memshire PLL
    \param ms_num Number of memshire
    \return none
*/
void memshire_pll_clear_lock_monitor(uint8_t ms_num);

/*! \fn uint32_t memshire_get_lock_monitor(uint8_t ms_num)
    \brief This function reads lock monitor of memshire PLL
    \param ms_num Number of memshire
    \return PLL lock monitor value
*/
uint32_t memshire_pll_get_lock_monitor(uint8_t ms_num);

/*! \fn void print_memshire_pll_lock_monitors(void)
    \brief This function prints lock monitors of memshire PLLs
    \param none
    \return none
*/
void print_memshire_pll_lock_monitors(void);

/*! \fn void clear_memshire_pll_lock_monitors(void)
    \brief This function clears lock monitors of memshire PLLs
    \param none
    \return none
*/
void clear_memshire_pll_lock_monitors(void);

/*! \fn uint32_t ms_verify_ddr_density (uint32_t memshire)
    \brief This function returns memory capacity 
    \param none
    \return none
*/
uint32_t ms_verify_ddr_density(uint32_t memshire);

/*! \fn void clear_memshire_pll_lock_monitors(void)
    \brief This function return DDR vendor ID
    \param none
    \return none
*/
uint32_t ms_verify_ddr_vendor(uint32_t memshire);

/*! \fn struct ddr_mem_info_t *mem_controller_get_ddr_info(void)
    \brief This function return DDR information including density and vendor ID
    \param None return pointer to hold mem info
    \return return pointer to mem info
*/
struct ddr_mem_info_t *mem_controller_get_ddr_info(void);

#endif
