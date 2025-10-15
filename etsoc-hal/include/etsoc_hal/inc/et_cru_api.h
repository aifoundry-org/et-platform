/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------
*/

/**
 *  @Component      CRU
 *
 *  @Filename       et_cru_api.h
 *
 *  @Description    The CRU component contains API interface to SPIO CRU
 *
 *//*======================================================================== */

#ifndef __ET_CRU_API_H
#define __ET_CRU_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "api.h"
#include "cm_esr.h"
#include "rm_esr.h"

#include "soc.h"

/* =============================================================================
 * EXPORTED DEFINITIONS
 * =============================================================================
 */


#define ET_CRU_AXI_GOLD_DIVIDER       1
#define ET_CRU_AXI_SILVER_DIVIDER     2
#define ET_CRU_AXI_BRONZE_DIVIDER     4
#define ET_CRU_AXI_AHB_DIVIDER        4


/*-------------------------------------------------------------------------*//**
 * @TYPE         ET_CRU_RESET_ACTION_e
 *
 * @BRIEF        CRU API input clock enum type.
 *
 * @DESCRIPTION  Enum of reset action used in et_cru_reset()
 *
 *//*------------------------------------------------------------------------ */
typedef enum resetAction
{
    ASSERT = 0x0,
    RELEASE

}ET_CRU_RESET_ACTION_e;



/*-------------------------------------------------------------------------*//**
 * @TYPE         CRU_USB_ID_e
 *
 * @BRIEF        CRU USB type ID
 *
 * @DESCRIPTION  Enum type for USB ID (0 or 1). Used for USB reset functions.
 *
 *//*------------------------------------------------------------------------ */
typedef enum cruUsbId
{
    ET_CRU_USB2_0 = 0x0,
    ET_CRU_USB2_1

} CRU_USB_ID_e;


 /* =============================================================================
 * EXPORTED TYPES
 * =============================================================================
 */


/*-------------------------------------------------------------------------*//**
 * @TYPE         ET_CRU_CFG_t
 *
 * @BRIEF        CRU configuration structure
 *
 * @DESCRIPTION  struct which defines CRU configuration parameters
 *
 */
typedef struct et_cru_cfg
{
  int empty;
} ET_CRU_CFG_t;


/*-------------------------------------------------------------------------*//**
 * @TYPE         ET_CRU_MODES_t
 *
 * @BRIEF        CRU API structure
 *
 * @DESCRIPTION  Struct which contains info about available programming modes for PLLs
 *
 */
typedef struct et_cru_modes_cfg {
    uint64_t mode;
    uint64_t inputFreq;
    uint64_t outputFreq;
} ET_CRU_MODES_t;


/*-------------------------------------------------------------------------*//**
 * @TYPE         ET_CRU_API_t[]
 *
 * @BRIEF        CRU API structure
 *
 * @DESCRIPTION  Struct which defines CRU API interface
 *
 */
static const ET_CRU_MODES_t gs_et_cru_modes[] = {
  {
      .mode = 1,
      .inputFreq =  100000000,
      .outputFreq = 2000000000
  },
  {
      .mode = 2,
      .inputFreq =  100000000,
      .outputFreq = 1500000000
  },
  {
      .mode = 3,
      .inputFreq =  100000000,
      .outputFreq = 1000000000
  },
  {
      .mode = 4,
      .inputFreq =  100000000,
      .outputFreq = 750000000
  },
  {
      .mode = 5,
      .inputFreq =  100000000,
      .outputFreq = 500000000
  },
  {
      .mode = 6,
      .inputFreq =  100000000,
      .outputFreq = 1010000000
  },
  {
      .mode = 7,
      .inputFreq =  24000000,
      .outputFreq = 2000000000
  },
  {
      .mode = 8,
      .inputFreq =  24000000,
      .outputFreq = 1500000000
  },
  {
      .mode = 9,
      .inputFreq =  24000000,
      .outputFreq = 1000000000
  },
  {
      .mode = 10,
      .inputFreq =  24000000,
      .outputFreq = 750000000
  },
  {
      .mode = 11,
      .inputFreq =  24000000,
      .outputFreq = 500000000
  },
  {
      .mode = 12,
      .inputFreq =  24000000,
      .outputFreq = 1010000000
  },
  {
      .mode = 13,
      .inputFreq =  40000000,
      .outputFreq = 2000000000
  },
  {
      .mode = 14,
      .inputFreq =  40000000,
      .outputFreq = 1500000000
  },
  {
      .mode = 15,
      .inputFreq =  40000000,
      .outputFreq = 1000000000
  },
  {
      .mode = 16,
      .inputFreq =  40000000,
      .outputFreq = 750000000
  },
  {
      .mode = 17,
      .inputFreq =  40000000,
      .outputFreq = 500000000
  },
  {
      .mode = 18,
      .inputFreq =  40000000,
      .outputFreq = 1010000000
  }

};


/*-------------------------------------------------------------------------*//**
 * @TYPE         ET_CRU_API_t
 *
 * @BRIEF        CRU API structure
 *
 * @DESCRIPTION  Struct which defines CRU API interface
 *
 */
typedef struct et_cru_api
{
     Clock_Manager*     cm_esr; /* Pointer to clock register set */
     Reset_Manager*     rm_esr; /* Pointer to reset register set */

     PLL_API_t*         spio_pll0_handle; /* Pointer to PLL0 handle */
     PLL_API_t*         spio_pll1_handle; /* Pointer to PLL1 handle */
     PLL_API_t*         spio_pll2_handle; /* Pointer to PLL2 handle */
     PLL_API_t*         spio_pll4_handle; /* Pointer to PLL4 handle */

     ET_CRU_CFG_t       cfg;    // IP Configuration structure */

} ET_CRU_API_t;

/* =============================================================================
 * EXPORTED VARIABLES
 * =============================================================================
 */



 /* =============================================================================
 * EXPORTED FUNCTIONS
 * =============================================================================
 */

 /* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_init
 *
 * @BRIEF         Initialize CRU API
 *
 * @RETURNS       void
 *
 * @DESCRIPTION   Initialize CRU API
 *
 *//*------------------------------------------------------------------------ */
extern void et_cru_init( void );



/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_open
 *
 * @BRIEF         Open CRU API
 *
 * @param[in]     API_IP_PARAMS_t *pIpParams -> Pointer to API_IP_PARAMS_t
 *                                              structure holding IP instantiation
 *                                              parameters
 *
 * @RETURNS       et_handle_t
 *
 * @DESCRIPTION   Open CRU API and initialize its structures
 *
 *//*------------------------------------------------------------------------ */
extern et_handle_t et_cru_open( API_IP_PARAMS_t *pIpParams );


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_close
 *
 * @BRIEF         Close CRU API
 *
 * @param[in]     et_handle_t handle -> Handle to opened CRU
 *
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Close CRU API and initialize its structures
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t et_cru_close( et_handle_t handle );


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_setClockFreq
 *
 * @BRIEF         Set CRU frequency
 *
 * @param[in]     et_handle_t      handle        -> Handle to opened CRU
 * @param[in]     uint64_t         id            -> Enum type for output clock which we want to set
 * @param[in]     uint64_t         target_freq   -> Target frequency to which we want to set output clock
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   (Re)configure CRU and/or PLL(s) in SPIO to generate desired target frequency
 *                of a specific output clock from SPIO CRU module.
 *                This function will return ET_OK if frequency change is successful
 *                This function will return ET_FAIL if frequency change is not possible
 *                to achieve or if reconfiguration is not successful.
 *
 *                NOTE: Due to hardcoded values of dividers in SPIO CRU module,
 *                      if changing output CRU frequency requires a change of
 *                      some PLL's frequency, this function WILL ALSO CHANGE
 *                      OTHER OUTPUTS CLOCKS that are derived from that PLL.
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t et_cru_setClockFreq( et_handle_t handle, uint64_t id, uint64_t target_freq );


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_getClockFreq
 *
 * @BRIEF         Get CRU frequency
 *
 * @param[in]     et_handle_t      handle        -> Handle to opened CRU
 * @param[in]     uint64_t id                    -> Enum type for output clock which we want to get
 *
 * @RETURNS       uint64_t
 *
 * @DESCRIPTION   Get current value of SoC specific output clock
 *                You can find available SoC Clocks for which you
 *                can get current frequency in:
 *
 *                dv/tests/ioshire/sw/inc/soc.h -> SOC_CLOCK_e
 *
 *
 *//*------------------------------------------------------------------------ */
extern uint64_t et_cru_getClockFreq( uint64_t id );



 /* =============================================================================
 * CLOCK CONTROL RELATED API FUNCTIONS
 * =============================================================================
 */



/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_enablePLLoutput
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 * @param[in]     uint32_t pllId                -> PLL's ID [0:4]
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Allow PLL<pllId> output to be used to generate clocks in CRU
 *                By default PLL is NOT USED (even if PLL output exists).
 *                Must be set explicitly with this function.
 *
 *//*------------------------------------------------------------------------ */
 extern et_status_t et_cru_enablePLLoutput(et_handle_t handle, uint32_t pllId);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_disablePLLoutput
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 * @param[in]     uint32_t pllId                -> PLL's ID [0:4]
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Do not allow PLL<pllId> output to be used to generate clocks
 *                DOES NOT affect PLL output, just its usage in CRU logic.
 *
 *//*------------------------------------------------------------------------ */
 extern et_status_t et_cru_disablePLLoutput(et_handle_t handle, uint32_t pllId);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_enablePLL0Mission
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Use the PLL0's output clock as the mission clock
 *                This function must be called AFTER enabling
 *                PLL0 output with et_cru_enablePLLoutput()
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t et_cru_enablePLL0Mission(et_handle_t handle);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_disablePLL0Mission
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Use the external clock as the mission clock
 *                This function must be called AFTER enabling
 *                PLL0 output with et_cru_enablePLLoutput()
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t et_cru_disablePLL0Mission(et_handle_t handle);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_clk_noc_dvfs
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 * @param[in]     uint64_t id                   -> mux(APB,PLL2)
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Glichless MUX which is used to give clk_gl output clock
 *                using mux(APB,PLL2)
 *
 *//*------------------------------------------------------------------------ */
/* RTLMIN-5448
extern et_status_t et_cru_clk_noc_dvfs(et_handle_t handle, uint64_t id);
*/



 /* =============================================================================
 * INTERRUPT RELATED API FUNCTIONS
 * =============================================================================
 */


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_enableAllInterrupts
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Enable both LOSS->LOCK and LOCK->LOSS interrupts
 *                for all the PLL's
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t et_cru_enableAllInterrupts(et_handle_t handle);

/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_enableAllLockInterrupts
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Enable both LOSS->LOCK
 *                for all the PLL's
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t et_cru_enableAllLockInterrupts(et_handle_t handle);

/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_enableAllLossInterrupts
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Enable both LOCK->LOSS interrupts
 *                for all the PLL's
 *
 *//*------------------------------------------------------------------------ */

extern et_status_t et_cru_enableAllLossInterrupts(et_handle_t handle);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_disableAllInterrupts
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Disable both LOSS->LOCK and LOCK->LOSS interrupts
 *                for all the PLL's
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t et_cru_disableAllInterrupts(et_handle_t handle);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_enablePllLockInterrupt
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 * @param[in]     uint32_t pllId                -> PLL ID
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Enables LOSS->LOCK interrupt
 *                for a PLL with pllId. This function will also
 *                disable interrupts for other SPIO PLLs.
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t et_cru_enablePllLockInterrupt(et_handle_t handle, uint32_t pllId);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_enablePllLossInterrupt
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 * @param[in]     uint32_t pllId                -> PLL ID
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Enables LOCK->LOSS interrupt
 *                for a PLL with pllId. This function will also
 *                disable interrupts for other SPIO PLLs.
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t et_cru_enablePllLossInterrupt(et_handle_t handle, uint32_t pllId);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_disablePllInterrupt
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 * @param[in]     uint32_t pllId                -> PLL ID
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Disable both LOSS->LOCK and LOCK->LOSS interrupt
 *                for a PLL with pllId.
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t et_cru_disablePllInterrupt(et_handle_t handle, uint32_t pllId);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_disablePllLockInterrupt
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 * @param[in]     uint32_t pllId                -> PLL ID
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Disable LOSS->LOCK interrupt
 *                for a PLL with pllId.
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t et_cru_disablePllLockInterrupt(et_handle_t handle, uint32_t pllId);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_disablePllLossInterrupt
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 * @param[in]     uint32_t pllId                -> PLL ID
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Disable LOCK->LOSS interrupt
 *                for a PLL with pllId.
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t et_cru_disablePllLossInterrupt(et_handle_t handle, uint32_t pllId);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_clearAllPllInterrupts
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 * @param[in]     uint32_t pllId                -> PLL ID
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Clear LOSS->LOCK and LOCK->LOSS interrupts
 *                for every SPIO PLL.
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t et_cru_clearAllPllInterrupts(et_handle_t handle);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_clearAllPllLossInterrupts
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 * @param[in]     uint32_t pllId                -> PLL ID
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Clear LOCK->LOSS interrupts
 *                for every SPIO PLL.
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t et_cru_clearAllPllLossInterrupts(et_handle_t handle);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_clearAllPllLockInterrupts
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 * @param[in]     uint32_t pllId                -> PLL ID
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Clear LOSS->LOCK interrupts
 *                for every SPIO PLL.
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t et_cru_clearAllPllLockInterrupts(et_handle_t handle);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_clearPllLockInterrupt
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 * @param[in]     uint32_t pllId                -> PLL ID
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Clear LOSS->LOCK interrupt
 *                for a PLL with pllId.
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t et_cru_clearPllLockInterrupt(et_handle_t handle, uint32_t pllId);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_clearPllLossInterrupt
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 * @param[in]     uint32_t pllId                -> PLL ID
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Clear LOCK->LOSS interrupt
 *                for a PLL with pllId.
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t et_cru_clearPllLossInterrupt(et_handle_t handle, uint32_t pllId);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_getLockInterruptStatus
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 * @param[in]     uint32_t pllId                -> PLL ID
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   Returns LOCK interrupt status
 *                for a PLL with pllId.
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t et_cru_getLockInterruptStatus(et_handle_t handle, uint32_t pllId);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_getLossInterruptStatus
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 * @param[in]     uint32_t pllId                -> PLL ID
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   Returns LOSS interrupt status
 *                for a PLL with pllId.
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t et_cru_getLossInterruptStatus(et_handle_t handle, uint32_t pllId);


 /* =============================================================================
 * RM GET STATUS API FUNCTIONS
 * =============================================================================
 */

/* Prefix legend */

/* et_cru_rm_get_status     - Get Reset Manager Status */
/* et_cru_rm_get_status_ots - Get Reset Manager Status - One Time Sample */

/* -------- rm_status register -------- */
extern uint32_t et_cru_rm_get_status_rm1(et_handle_t handle);
extern uint32_t et_cru_rm_get_status_rm2(et_handle_t handle);
extern uint32_t et_cru_rm_get_status_boot_finish(et_handle_t handle);
extern uint32_t et_cru_rm_get_status_boot_error(et_handle_t handle);
extern uint32_t et_cru_rm_get_status_boot_sp_hold_reset(et_handle_t handle);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_rm_get_status_vault_abort_ack
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   1: indicate the vault acknowledges the abort_req, then it is safe
 *                   to apply vault soft reset.
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t et_cru_rm_get_status_vault_abort_ack(et_handle_t handle);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_rm_get_status_lockbox_bringup
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   Reflect efuse one-time-sample value: 0: Production part, 1: Bringup part.
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t et_cru_rm_get_status_lockbox_bringup(et_handle_t handle);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_rm_get_status_ots_lockbox_jtag_rot_enable
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   Reflect efuse one-time-sample value: jtag_rot_enable
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t et_cru_rm_get_status_ots_lockbox_jtag_rot_enable(et_handle_t handle);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_rm_get_status_ots_lockbox_jtag_dft_enable
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   Reflect efust one-time-sample value: jtag_dft_enable
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t et_cru_rm_get_status_ots_lockbox_jtag_dft_enable(et_handle_t handle);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_rm_get_status_ots_lockbox_debug_enable
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   Reflect efust one-time-sample value: debug enable
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t et_cru_rm_get_status_ots_lockbox_debug_enable(et_handle_t handle);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_rm_get_status_ots_lockbox_maxion_nonsecure_access_enable
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   Reflect efuse one-time-sample value: maxion_nonsecure_access_enable
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t et_cru_rm_get_status_ots_lockbox_maxion_nonsecure_access_enable(et_handle_t handle);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_rm_get_status_ots_lockbox_minion_nonsecure_access_enable
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   Reflect efuse one-time-sample value: minion_nonsecure_access_enable
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t et_cru_rm_get_status_ots_lockbox_minion_nonsecure_access_enable(et_handle_t handle);


/* -------- rm_status2 register -------- */


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_rm_get_status_debug_enable
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   reflect the value on the boot pin: debug_enable.
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t et_cru_rm_get_status_debug_enable(et_handle_t handle);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_rm_get_status_emergency_dbg
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   reflect the value on the boot pin: emergency_dbg.
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t et_cru_rm_get_status_emergency_dbg(et_handle_t handle);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_rm_get_status_error_sms_bihr
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   Boot status: Error on sms BIHR.
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t et_cru_rm_get_status_error_sms_bihr(et_handle_t handle);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_rm_get_status_error_sms_bist
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   Boot status: Error on sms BIST.
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t et_cru_rm_get_status_error_sms_bist(et_handle_t handle);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_rm_get_status_error_sms_udr
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   Boot status: Error on sms UDR.
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t et_cru_rm_get_status_error_sms_udr(et_handle_t handle);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_rm_get_status_error_vaultsms_bihr
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   Boot status: Error on vaultsms BIHR.
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t et_cru_rm_get_status_error_vaultsms_bihr(et_handle_t handle);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_rm_get_status_error_vaultsms_bist
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   Boot status: Error on vaultsms BIST.
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t et_cru_rm_get_status_error_vaultsms_bist(et_handle_t handle);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_rm_get_status_error_vaultsms_udr
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   Boot status: Error on vaultsms UDR.
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t et_cru_rm_get_status_error_vaultsms_udr(et_handle_t handle);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_rm_get_status_skip_sms_bihr
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   reflect the value on the boot pin: soc_boot_skip_sms_bihr.
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t et_cru_rm_get_status_skip_sms_bihr(et_handle_t handle);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_rm_get_status_skip_sms_bist
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   reflect the value on the boot pin: soc_boot_skip_sms_bist.
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t et_cru_rm_get_status_skip_sms_bist(et_handle_t handle);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_rm_get_status_skip_sms_udr
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   reflect the value on the boot pin: soc_boot_skip_sms_udr.
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t et_cru_rm_get_status_skip_sms_udr(et_handle_t handle);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_rm_get_status_skip_vault
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   reflect the value on the boot pin: soc_boot_skip_vault.
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t et_cru_rm_get_status_skip_vault(et_handle_t handle);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_rm_get_status_strap_in
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   reflect the value on the strap_in[3:0].
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t et_cru_rm_get_status_strap_in(et_handle_t handle);


 /* =============================================================================
 * END OF RM GET STATUS API FUNCTIONS
 * =============================================================================
 */

 /* =============================================================================
 * MISC API FUNCTIONS
 * =============================================================================
 */


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_getLock2lossStatus
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 * @param[in]     uint32_t pllId                -> PLL ID
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   Get stable bit status for fixed output CRU clock
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t et_cru_getLock2lossStatus(et_handle_t handle, uint32_t pllId);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_getLoss2lockStatus
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 * @param[in]     uint32_t pllId                -> PLL ID
 *
 * @RETURNS       uint32_t
 *
 * @DESCRIPTION   Get stable bit status for fixed output CRU clock
 *
 *//*------------------------------------------------------------------------ */
extern uint32_t et_cru_getLoss2lockStatus(et_handle_t handle, uint32_t pllId);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_clearLossPending
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 * @param[in]     uint32_t pllId                -> PLL ID
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Clear loss pending bit of PLL status register
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t et_cru_clearLossPending(et_handle_t handle, uint32_t pllId);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_clearLockPending
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 * @param[in]     uint32_t pllId                -> PLL ID
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Clear lock pending bit of PLL status register
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t et_cru_clearLockPending(et_handle_t handle, uint32_t pllId);


/* =============================================================================
 * OTHER RESET API FUNCTIONS
 * =============================================================================
 */


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_reset
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 * @param[in]     uint64_t soc_reset_e          -> Enum type of SoC reset
 * @param[in]     uint64_t action               -> Assert or deassert
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t et_cru_reset( et_handle_t handle, uint64_t soc_reset_e, uint64_t action);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_releaseCruSysReset
 *
 * @BRIEF         Get CRU frequency API
 *
 * @param[in]     et_handle_t handle            -> Handle to opened CRU
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Generate a system reset cru_sys_reset.
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t et_cru_releaseCruSysReset( et_handle_t handle);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      et_cru_maxionPll
 *
 * @BRIEF         Enable the Maxion PLL
 *
 * @param[in]     et_handle_t handle            -> Handle to opened PLL
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION  Enables Maxion PLL output, switch from bypass clock
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t et_cru_maxionPll( et_handle_t handle);


#ifdef __cplusplus
}
#endif

#endif  /* __ET_CRU_API_H */


/*     <EOF>     */

