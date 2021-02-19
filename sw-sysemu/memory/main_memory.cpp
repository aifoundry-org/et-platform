/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include "emu_defines.h"
#include "memory/mailbox_region.h"
#include "memory/maxion_region.h"
#include "memory/peripheral_region.h"
#include "memory/scratch_region.h"
#include "memory/sparse_region.h"
#include "memory/svcproc_region.h"
#include "memory/sysreg_region.h"
#ifdef SYS_EMU
#include "memory/pcie_region.h"
#endif

namespace bemu {


void MainMemory::reset()
{
    size_t pos = 0;
    regions[pos++].reset(new MaxionRegion<pu_maxion_base, 256_MiB>());
    regions[pos++].reset(new PeripheralRegion<pu_io_base, 256_MiB>());
    regions[pos++].reset(new MailboxRegion<pu_mbox_base, 512_MiB>());
    regions[pos++].reset(new SvcProcRegion<spio_base, 1_GiB>());
    regions[pos++].reset(new ScratchRegion<scp_base, 4_MiB, EMU_NUM_SHIRES>());
    regions[pos++].reset(new SysregRegion<sysreg_base, 4_GiB>());
#ifdef SYS_EMU
    regions[pos++].reset(new PcieRegion<pcie_base, 256_GiB>());
#endif
    regions[pos++].reset(new SparseRegion<dram_base, EMU_DRAM_SIZE, 16_MiB>());
}


void MainMemory::pu_plic_interrupt_pending_set(const Agent& agent, uint32_t source)
{
    auto ptr = dynamic_cast<PeripheralRegion<pu_io_base, 256_MiB>*>(regions[1].get());
    ptr->pu_plic.interrupt_pending_set(agent, source);
}


void MainMemory::pu_plic_interrupt_pending_clear(const Agent& agent, uint32_t source)
{
    auto ptr = dynamic_cast<PeripheralRegion<pu_io_base, 256_MiB>*>(regions[1].get());
    ptr->pu_plic.interrupt_pending_clear(agent, source);
}


void MainMemory::sp_plic_interrupt_pending_set(const Agent& agent, uint32_t source)
{
    auto ptr = dynamic_cast<SvcProcRegion<spio_base, 1_GiB>*>(regions[3].get());
    ptr->sp_plic.interrupt_pending_set(agent, source);
}


void MainMemory::sp_plic_interrupt_pending_clear(const Agent& agent, uint32_t source)
{
    auto ptr = dynamic_cast<SvcProcRegion<spio_base, 1_GiB>*>(regions[3].get());
    ptr->sp_plic.interrupt_pending_clear(agent, source);
}


void MainMemory::pu_uart0_set_rx_fd(int fd)
{
    auto ptr = dynamic_cast<PeripheralRegion<pu_io_base, 256_MiB>*>(regions[1].get());
    ptr->pu_uart0.rx_fd = fd;
}


void MainMemory::pu_uart1_set_rx_fd(int fd)
{
    auto ptr = dynamic_cast<PeripheralRegion<pu_io_base, 256_MiB>*>(regions[1].get());
    ptr->pu_uart1.rx_fd = fd;
}


int MainMemory::pu_uart0_get_rx_fd() const
{
    auto ptr = dynamic_cast<PeripheralRegion<pu_io_base, 256_MiB>*>(regions[1].get());
    return ptr->pu_uart0.rx_fd;
}


int MainMemory::pu_uart1_get_rx_fd() const
{
    auto ptr = dynamic_cast<PeripheralRegion<pu_io_base, 256_MiB>*>(regions[1].get());
    return ptr->pu_uart1.rx_fd;
}


void MainMemory::spio_uart0_set_rx_fd(int fd)
{
    auto ptr = dynamic_cast<SvcProcRegion<spio_base, 1_GiB>*>(regions[3].get());
    ptr->spio_uart0.rx_fd = fd;
}


void MainMemory::spio_uart1_set_rx_fd(int fd)
{
    auto ptr = dynamic_cast<SvcProcRegion<spio_base, 1_GiB>*>(regions[3].get());
    ptr->spio_uart1.rx_fd = fd;
}


int MainMemory::spio_uart0_get_rx_fd() const
{
    auto ptr = dynamic_cast<SvcProcRegion<spio_base, 1_GiB>*>(regions[3].get());
    return ptr->spio_uart0.rx_fd;
}


int MainMemory::spio_uart1_get_rx_fd() const
{
    auto ptr = dynamic_cast<SvcProcRegion<spio_base, 1_GiB>*>(regions[3].get());
    return ptr->spio_uart1.rx_fd;
}

void MainMemory::pu_uart0_set_tx_fd(int fd)
{
    auto ptr = dynamic_cast<PeripheralRegion<pu_io_base, 256_MiB>*>(regions[1].get());
    ptr->pu_uart0.tx_fd = fd;
}


void MainMemory::pu_uart1_set_tx_fd(int fd)
{
    auto ptr = dynamic_cast<PeripheralRegion<pu_io_base, 256_MiB>*>(regions[1].get());
    ptr->pu_uart1.tx_fd = fd;
}


int MainMemory::pu_uart0_get_tx_fd() const
{
    auto ptr = dynamic_cast<PeripheralRegion<pu_io_base, 256_MiB>*>(regions[1].get());
    return ptr->pu_uart0.tx_fd;
}


int MainMemory::pu_uart1_get_tx_fd() const
{
    auto ptr = dynamic_cast<PeripheralRegion<pu_io_base, 256_MiB>*>(regions[1].get());
    return ptr->pu_uart1.tx_fd;
}


void MainMemory::spio_uart0_set_tx_fd(int fd)
{
    auto ptr = dynamic_cast<SvcProcRegion<spio_base, 1_GiB>*>(regions[3].get());
    ptr->spio_uart0.tx_fd = fd;
}


void MainMemory::spio_uart1_set_tx_fd(int fd)
{
    auto ptr = dynamic_cast<SvcProcRegion<spio_base, 1_GiB>*>(regions[3].get());
    ptr->spio_uart1.tx_fd = fd;
}


int MainMemory::spio_uart0_get_tx_fd() const
{
    auto ptr = dynamic_cast<SvcProcRegion<spio_base, 1_GiB>*>(regions[3].get());
    return ptr->spio_uart0.tx_fd;
}


int MainMemory::spio_uart1_get_tx_fd() const
{
    auto ptr = dynamic_cast<SvcProcRegion<spio_base, 1_GiB>*>(regions[3].get());
    return ptr->spio_uart1.tx_fd;
}


bool MainMemory::pu_rvtimer_is_active() const
{
    auto ptr = dynamic_cast<SysregRegion<sysreg_base, 4_GiB>*>(regions[5].get());
    return ptr->ioshire_pu_rvtimer.is_active();
}


uint64_t MainMemory::pu_rvtimer_read_mtime() const
{
    auto ptr = dynamic_cast<SysregRegion<sysreg_base, 4_GiB>*>(regions[5].get());
    return ptr->ioshire_pu_rvtimer.read_mtime();
}


uint64_t MainMemory::pu_rvtimer_read_mtimecmp() const
{
    auto ptr = dynamic_cast<SysregRegion<sysreg_base, 4_GiB>*>(regions[5].get());
    return ptr->ioshire_pu_rvtimer.read_mtimecmp();
}


void MainMemory::pu_rvtimer_update(const Agent& agent, uint64_t cycle)
{
    auto ptr = dynamic_cast<SysregRegion<sysreg_base, 4_GiB>*>(regions[5].get());
    ptr->ioshire_pu_rvtimer.update(agent, cycle);
}


void MainMemory::pu_rvtimer_write_mtime(const Agent& agent, uint64_t cycle)
{
    auto ptr = dynamic_cast<SysregRegion<sysreg_base, 4_GiB>*>(regions[5].get());
    ptr->ioshire_pu_rvtimer.write_mtime(agent, cycle);
}


void MainMemory::pu_rvtimer_write_mtimecmp(const Agent& agent, uint64_t cycle)
{
    auto ptr = dynamic_cast<SysregRegion<sysreg_base, 4_GiB>*>(regions[5].get());
    ptr->ioshire_pu_rvtimer.write_mtimecmp(agent, cycle);
}


bool MainMemory::spio_rvtimer_is_active() const
{
#ifdef SYS_EMU
    auto ptr = dynamic_cast<SvcProcRegion<spio_base, 1_GiB>*>(regions[3].get());
    return ptr->sp_rvtim.rvtimer.is_active();
#else
    return false;
#endif
}


void MainMemory::spio_rvtimer_update(const Agent& agent, uint64_t cycle)
{
#ifdef SYS_EMU
    auto ptr = dynamic_cast<SvcProcRegion<spio_base, 1_GiB>*>(regions[3].get());
    ptr->sp_rvtim.rvtimer.update(agent, cycle);
#else
    (void) agent;
    (void) cycle;
#endif
}


void MainMemory::pcie0_dbi_slv_trigger_done_int(const Agent& agent, bool wrch, int channel)
{
#ifdef SYS_EMU
    auto ptr = dynamic_cast<PcieRegion<pcie_base, 256_GiB>*>(regions[6].get());
    ptr->pcie0_dbi_slv.trigger_done_int(agent, wrch, channel);
#else
    (void) agent;
    (void) wrch;
    (void) channel;
#endif
}


}
