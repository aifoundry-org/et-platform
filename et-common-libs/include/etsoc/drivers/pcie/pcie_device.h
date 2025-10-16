/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------
*/

#ifndef PCIE_DEVICE_H
#define PCIE_DEVICE_H

#include "hwinc/pcie_apb_subsys.h"
#include "hwinc/pcie0_dbi_slv.h"
#include "hwinc/sp_pshire_regbus.h"
#include "hwinc/pcie_esr.h"
#include "hwinc/pcie_nopciesr.h"
#include "hwinc/hal_device.h"

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
