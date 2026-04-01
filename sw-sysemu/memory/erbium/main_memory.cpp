/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#include "memory/erbium/main_memory.h"
#include "emu_defines.h"
#include "devices/mram_bridge_er.h"
#include "devices/plic_er.h"
#include "devices/shakti_uart.h"
#include "devices/sysregs_er.h"
#include "system.h"
#include "memory/sysreg_region.h"
#include "memory/dense_region.h"

namespace bemu {

void MainMemory::reset()
{
    regions[erbreg_idx].reset(new SysregsEr<region_bases[erbreg_idx]>());
    regions[mram_bridge_idx].reset(new MramBridgeEr<region_bases[mram_bridge_idx], region_sizes[mram_bridge_idx]>());
    regions[uart_idx].reset(new ShaktiUart<region_bases[uart_idx], region_sizes[uart_idx]>());
    regions[bootrom_idx].reset(new DenseRegion<region_bases[bootrom_idx], region_sizes[bootrom_idx], false>());
    regions[sram_idx].reset(new DenseRegion<region_bases[sram_idx], region_sizes[sram_idx]>());
    regions[dram_idx].reset(new DenseRegion<region_bases[dram_idx], region_sizes[dram_idx]>());
    regions[otp_idx].reset(new DenseRegion<region_bases[otp_idx], region_sizes[otp_idx], false>());
    regions[sysreg_idx].reset(new SysregRegion<region_bases[sysreg_idx], region_sizes[sysreg_idx]>());
    regions[plic_idx].reset(new ER_PLIC<region_bases[plic_idx], region_sizes[plic_idx]>());
}

void MainMemory::wdt_clock_tick(const Agent& agent, uint64_t cycle)
{
    auto ptr = dynamic_cast<SysregsEr<region_bases[erbreg_idx]>*>(regions[erbreg_idx].get());
    ptr->wdt_clock_tick(agent, cycle);
}

inline auto& MainMemory::rvtimer() const {
    auto ptr = dynamic_cast<SysregRegion<region_bases[sysreg_idx], region_sizes[sysreg_idx]>*>(regions[sysreg_idx].get());
    return ptr->rvtimer;
}

bool MainMemory::rvtimer_is_active() const {
    return rvtimer().is_active();
}

uint64_t MainMemory::rvtimer_read_mtime() const {
    return rvtimer().read_mtime();
}

uint64_t MainMemory::rvtimer_read_mtimecmp() const {
    return rvtimer().read_mtimecmp();
}

uint64_t MainMemory::rvtimer_read_time_config() const {
    return rvtimer().read_time_config();
}

void MainMemory::rvtimer_clock_tick(const Agent& agent, uint64_t cycle) {
    // cycle at 200MHz
    if ((cycle % 5) == 0) {
        rvtimer().prescaler_tick(agent);
    }
}

void MainMemory::rvtimer_write_mtime(const Agent& agent, uint64_t value) {
    rvtimer().write_mtime(agent, value);
}

void MainMemory::rvtimer_write_mtimecmp(const Agent& agent, uint64_t value) {
    rvtimer().write_mtimecmp(agent, value);
}

void MainMemory::rvtimer_write_time_config(const Agent& agent, uint64_t value) {
    rvtimer().write_time_config(agent, value);
}

void MainMemory::rvtimer_reset() {
    rvtimer().reset();
}

void MainMemory::uart_set_tx_fd(int fd) {
    auto ptr = dynamic_cast<ShaktiUart<region_bases[uart_idx], region_sizes[uart_idx]>*>(regions[uart_idx].get());
    ptr->tx_fd = fd;
}

void MainMemory::uart_set_rx_fd(int fd) {
    auto ptr = dynamic_cast<ShaktiUart<region_bases[uart_idx], region_sizes[uart_idx]>*>(regions[uart_idx].get());
    ptr->rx_fd = fd;
}

int MainMemory::uart_get_tx_fd() const {
    auto ptr = dynamic_cast<ShaktiUart<region_bases[uart_idx], region_sizes[uart_idx]>*>(regions[uart_idx].get());
    return ptr->tx_fd;
}

int MainMemory::uart_get_rx_fd() const {
    auto ptr = dynamic_cast<ShaktiUart<region_bases[uart_idx], region_sizes[uart_idx]>*>(regions[uart_idx].get());
    return ptr->rx_fd;
}

bool MainMemory::is_mram_dsleep() const {
    auto ptr = dynamic_cast<SysregsEr<region_bases[erbreg_idx]>*>(regions[erbreg_idx].get());
    return ptr->is_mram_dsleep();
}

void MainMemory::mram_bridge_set_slverr(uint32_t bits) {
    auto ptr = dynamic_cast<MramBridgeEr<region_bases[mram_bridge_idx], region_sizes[mram_bridge_idx]>*>(regions[mram_bridge_idx].get());
    ptr->set_slverr(bits);
}

bool MainMemory::is_uart_enabled() const {
    auto ptr = dynamic_cast<SysregsEr<region_bases[erbreg_idx]>*>(regions[erbreg_idx].get());
    return ptr->is_uart_enabled();
}

void MainMemory::plic_interrupt_pending_set(const Agent& agent, uint32_t source)
{
    auto ptr = dynamic_cast<ER_PLIC<region_bases[plic_idx], region_sizes[plic_idx]>*>(regions[plic_idx].get());
    ptr->interrupt_pending_set(agent, source);
}

void MainMemory::plic_interrupt_pending_clear(const Agent& agent, uint32_t source)
{
    auto ptr = dynamic_cast<ER_PLIC<region_bases[plic_idx], region_sizes[plic_idx]>*>(regions[plic_idx].get());
    ptr->interrupt_pending_clear(agent, source);
}

} // namespace bemu
