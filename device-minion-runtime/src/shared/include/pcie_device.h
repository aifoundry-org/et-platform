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

#include "etsoc_hal/inc/DWC_pcie_subsystem_custom.h"
#include "etsoc_hal/inc/DWC_pcie_dbi_cpcie_usp_4x8.h"
#include "etsoc_hal/inc/ns_noc_io_pcie_soc_ip.h"
#include "etsoc_hal/inc/pshire_esr.h"
#include "etsoc_hal/inc/hal_device.h"

//There are two Synopsis PCIe controllers on the SoC. We are using only one of
//them, as a PCIe gen 4.0 x8 endpoint (a.k.a. usp - upstream facing port).
#define PCIE0 R_PCIE0_DBI_SLV_BASEADDR

//The PCIe controller doesn't implement a bunch of features (e.x. MSI
//interrupts), but leaves interfaces for the client to implement them, if
//needed. Synopsis gave us a custom-for-Esperanto bundle of implementations
//for some of the unimplemented features we need.
#define PCIE_CUST_SS R_PCIE_APB_SUBSYS_BASEADDR

//Lastly, there are some features not implemented by the Synopsis PCIe
//controller or custom subsystem that Esperanto implements in ESRs.

//PShire ESRs accessible only by the SP
#define PCIE_ESR R_PCIE_ESR_BASEADDR

//PShire ESRs accessible to processors on the SoC (SP, MM, MX) but not PShire
#define PCIE_NOPCIESR R_PCIE_NOPCIESR_BASEADDR

//PShire ESRs accessible to Host/SP/MM/Maxion
#define PCIE_USRESR R_PCIE_USRESR_BASEADDR

//The MSI-X Engine will snoop writes on the AXI port of the PCIe controller,
//and fire an interrupt when you write to a special address on the port. You
//pick the address. I picked offset 0 arbitrarily ¯\_(ツ)_/¯.
#define MSIX_TRIG_REG R_PCIE0_SLV_BASEADDR

//NoC Registers for PShire NoC RegBus. Only usable by SP.
#define PCIE_NOC R_SP_PSHIRE_REGBUS_BASEADDR

#endif
