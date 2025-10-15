/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------
*/

/**
 *  @Component      USB PHY
 *
 *  @Filename       usb_phy.h
 *
 *  @Description    The USB PHY SW component 
 *
 *//*======================================================================== */

#ifndef __USB_PHY_H
#define __USB_PHY_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "usb2_config_esrs.h"

/* =============================================================================
 * EXPORTED DEFINITIONS
 * =============================================================================
 */ 
 
 /* =============================================================================
 * EXPORTED TYPES
 * =============================================================================
 */

/*-------------------------------------------------------------------------*//**
 * @FUNCTION      usb_phy_reset_release
 *
 * @BRIEF         release usb phy resets
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   release usb phy resets
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t usb_phy_resets( uint8_t reset_id, uint8_t action );


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      usb_reset_release
 *
 * @BRIEF         release usb resets
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   release usb resets
 *
 *//*------------------------------------------------------------------------ */
extern et_status_t usb_resets( uint8_t action );

/* ------------------------------------------------------------------------*//**
 * @FUNCTION      usb_phy_clk_config
 *
 * @BRIEF         configure usb phy cloks
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   configure usb phy cloks
 *id
 *//*------------------------------------------------------------------------ */
extern et_status_t usb_phy_clk_frequency_cfg( uint32_t refclk_sel, uint32_t fsel, uint32_t  pll_btune );


/* ------------------------------------------------------------------------*//**
 * @FUNCTION      set_ust_ip_pin
 *
 * @BRIEF         Enable/disable UltraSoc controller
 *
 * @RETURNS       et_status_t
 *
 * @DESCRIPTION   Enable/disable UltraSoc controller
 *id
 *//*------------------------------------------------------------------------ */
extern et_status_t set_ust_ip_pin( uint32_t select );

#ifdef __cplusplus
}
#endif

#endif	/* __USB_PHY_H */


/*     <EOF>     */

