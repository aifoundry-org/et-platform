/*------------------------------------------------------------------------- 
* Copyright (C) 2019, Esperanto Technologies Inc. 
* The copyright to the computer program(s) herein is the 
* property of Esperanto Technologies.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies or 
* in accordance with the terms and conditions stipulated in the 
* agreement/contract under which the program(s) have been supplied. 
*------------------------------------------------------------------------- 
*/

#ifndef PCIE_DEVICE_H
#define PCIE_DEVICE_H

#include "DWC_pcie_subsystem_custom.h"
#include "DWC_pcie_dbi_cpcie_usp_4x8.h"
#include "pshire_esr.h"
#include "hal_device.h"

//There are two Synopsis PCIe controllers on the SoC. We are using only one of
//them, as a PCIe gen 4.0 x8 endpoint (a.k.a. usp - upstream facing port).
#define PCIE0 ((volatile PE0_DWC_pcie_ctl_DBI_Slave_t* const)R_PCIE0_DBI_SLV_BASEADDR) 

//The PCIe controller doesn't implement a bunch of features (e.x. MSI
//interrupts), but leaves interfaces for the client to implement them, if
//needed. Synopsis gave us a custom-for-Esperanto bundle of implementations 
//for some of the unimplemented features we need.
#define PCIE_CUST_SS ((volatile DWC_pcie_subsystem_custom_APB_Slave_subsystem_t* const)R_PCIE_APB_SUBSYS_BASEADDR)

//Lastly, there are some features not implemented by the Synopsis PCIe
//controller or custom subsystem that Esperanto implements in ESRs.

//PShire ESRs accessible only by the SP
#define PCIE_ESR ((volatile Pshire_t* const)R_PCIE_ESR_BASEADDR)

//TODO: Get update hal_device that includes this.
#define R_PCIE_NOPCIESR_BASEADDR 0x7F80001000

//PShire ESRs accessible to processors on the SoC (SP, MM, MX) but not PShire
#define PCIE_NOPCIESR ((volatile Pshire_usr1_t* const)R_PCIE_NOPCIESR_BASEADDR)

#endif
