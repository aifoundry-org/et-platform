/*------------------------------------------------------------------------- 
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*------------------------------------------------------------------------- 
*/

/**
* @file $Id$ 
* @version $Release$ 
* @date $Date$ 
* @author 
*
* @brief usb_phy.c   
*/ 
/** 
 *  @Component    USB PHY 
 * 
 *  @Filename     usb_phy.c 
 * 
 *  @Description  USB PHY Driver
 * 
 *//*======================================================================== */ 



/* =============================================================================
 * STANDARD INCLUDE FILES
 * =============================================================================
 */ 
#include "cpu.h"
#include "soc.h"
#include "et.h"
#include "api.h"
#include "usb_phy.h"
#include "rm_esr.h"
#include "spio_misc_esr.h"
/* =============================================================================
 * GLOBAL VARIABLES DECLARATIONS
 * =============================================================================
 */


/* =============================================================================
 * LOCAL TYPES AND DEFINITIONS
 * =============================================================================
 */
  
/* =============================================================================
 * LOCAL VARIABLES DECLARATIONS
 * =============================================================================
 */

 /* ============================================================================
 * LOCAL FUNCTIONS PROTOTYPES
 * =============================================================================
 */

/* =============================================================================
 * FUNCTIONS
 * =============================================================================
 */

et_status_t usb_phy_resets( uint8_t reset_id, uint8_t action ){

    et_status_t est = ET_PASS;
    
    /* et_cru_reset(cru_handle, SOC_RESET_e, Action);  
    cru_handle  - keyword, always the same, don't modify
    SOC_RESET_e - enum from dv/tests/ioshire/sw/inc/soc.h
    Action      - ASSERT=0/RELEASE=1 */
      
    switch(reset_id){
    
        case(1):
            est += et_cru_reset(cru_handle, RESET_N_IOS_USB2_0_ESR_APB_CLK, action);
            break;
        case(2):
            est += et_cru_reset(cru_handle, RESET_N_IOS_USB2_0_PHY_POR, action);
            est += et_cru_reset(cru_handle, RESET_N_IOS_USB2_0_PHY_PORT0, action); 
            break;       
        case(3):
            est += et_cru_reset(cru_handle, RESET_N_IOS_USB2_0_PHY_POR, action);
            break;
        case(4):
            est += et_cru_reset(cru_handle, RESET_N_IOS_USB2_0_PHY_PORT0, action);
            break;
        default:
            est += et_cru_reset(cru_handle, RESET_N_IOS_USB2_0_ESR_APB_CLK, action);
            est += et_cru_reset(cru_handle, RESET_N_IOS_USB2_0_PHY_POR, action);
            est += et_cru_reset(cru_handle, RESET_N_IOS_USB2_0_PHY_PORT0, action);  
            break;  
    
    } /* Mybe some cases are unnecessary. Need to reade Spec more....*/
    
    return est;    
}

/*==================== Function Separator =============================*/
et_status_t usb_resets( uint8_t action ){

    et_status_t est = ET_PASS;
    uint32_t rdVal;
    
    Reset_Manager* prm;
    prm = (Reset_Manager*)R_SP_CRU_BASEADDR;
    
    rdVal = get_bitField( prm -> rm_usb2_0, RESET_MANAGER_RM_USB2_0_PHY_PO_RSTN );
    
    /* por reset need to be deassserted */
    if( rdVal != 0x1 ){
        return ET_FAIL;
    }
    // else if(wait fot cloks or wait in test befor call function, but think here)    
    else{
        est += et_cru_reset(cru_handle, RESET_N_IOS_USB2_0_RELOC_APB_CLK, action);
        est += et_cru_reset(cru_handle, RESET_N_IOS_USB2_0_AHB2AXI_AHB_CLK, action);
        //est += et_cru_reset(cru_handle, RESET_N_IOS_USB2_0_CTRL_PHY_CLK, action);
        est += et_cru_reset(cru_handle, RESET_N_IOS_USB2_0_DBG_UTMI_CLK, action);
        est += et_cru_reset(cru_handle, RESET_N_IOS_USB2_0_CTRL_AHB_CLK, action);
    }
    
    return est;
    
}
/*==================== Function Separator =============================*/

et_status_t usb_phy_clk_frequency_cfg( uint32_t refclk_sel, uint32_t fsel, uint32_t  pll_btune )
{
    et_status_t est = ET_PASS;
    Usb2_phy_config_esr* usb_phy_esr;
    uint32_t rdPor;
    uint32_t rdPort;
    Reset_Manager* prm;
    
    prm = (Reset_Manager*)R_SP_CRU_BASEADDR;
    usb_phy_esr = (Usb2_phy_config_esr*)(R_SP_U0ESR_BASEADDR);
        
    rdPor = get_bitField( prm ->  rm_usb2_0, RESET_MANAGER_RM_USB2_0_PHY_PO_RSTN );
    rdPort = get_bitField( prm -> rm_usb2_0, RESET_MANAGER_RM_USB2_0_PHY_PORT0_RSTN );
    
    /* esr apb reset need to be deassserted */
    if(  get_bitField(prm -> rm_usb2_0, RESET_MANAGER_RM_USB2_0_ESR_RSTN ) != 0x1 ){
        printDbg( "USB2 abp ESR reset needs to be released!");
        return ET_FAIL; 
    }   
    else if( (rdPor != 0x0) || (rdPort != 0x0) ){ 
        printDbg( "POR resets need to be assetred during ESR cfg!" );
        return ET_FAIL; 
    }        
    else{
        /* Cfg ref clock with aloved values */
        if( (((fsel < 0x3 || fsel == 0x7) && pll_btune == 0x1) || (fsel == 0x7 && pll_btune == 0x0)) && refclk_sel == 0x2 ){ //Need defines for these hardcoded values
            usb_phy_esr -> config_refclksel = refclk_sel;
            usb_phy_esr -> config_pllbtune  = pll_btune;
            usb_phy_esr -> config_fsel = fsel;
            est += ET_PASS;
        }
        else{
            est += ET_FAIL;
        }
    }
    
    return est;

}

/*==================== Function Separator =============================*/

et_status_t set_ust_ip_pin( uint32_t select ){

    et_status_t est = ET_PASS;
    Spio_misc_esr* misc_ptr;
     
    misc_ptr = (Spio_misc_esr*)R_SP_MISC_BASEADDR;
    
    if( select > 0x1 ){
        printDbg("SW sent invalid option. It can be 0 for disable and 1 for enable\n");
        est = ET_FAIL;        
    }
    else{
        if( select == 0x1 )
           misc_ptr -> PU_USB20_UST_EN = 0x1;
        else
           misc_ptr -> PU_USB20_UST_EN = 0x0;        
    }
    
    return est;   
}

/*==================== Function Separator =============================*/


/*****     < EOF >     *****/

