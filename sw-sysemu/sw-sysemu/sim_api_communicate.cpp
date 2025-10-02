// Global
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string>
#include <assert.h>
#include <unistd.h>
#include <endian.h>

// SysEMU
#include "emu_gio.h"
#include "system.h"
#include "sys_emu.h"

// sw-sysemu
#include "sim_api_communicate.h"

using namespace std;

sim_api_communicate::SysEmuWrapper::SysEmuWrapper(sim_api_communicate* sim)
    : AbstractSimulator(sim->socket_path_),
      sim_(sim)
{
}

bool sim_api_communicate::SysEmuWrapper::boot(uint32_t shire_id, uint32_t thread0_enable, uint32_t thread1_enable)
{
    LOG_NOTHREAD(INFO, "sim_api_communicate: boot(shire = %" PRId32 ", t0 = 0x%" PRIx32 ", t1 = 0x%" PRIx32 ")",
        shire_id, thread0_enable, thread1_enable);

    sys_emu::shire_enable_threads(shire_id, ~thread0_enable, ~thread1_enable);

    return true;
}

bool sim_api_communicate::SysEmuWrapper::shutdown()
{
    LOG_NOTHREAD(INFO, "%s", "sim_api_communicate: shutdown");
    sim_->done_ = true;
    sim_->chip_->set_emu_done(true);
    return true;
}

bool sim_api_communicate::SysEmuWrapper::is_done()
{
    return sim_->done_;
}

int sim_api_communicate::SysEmuWrapper::active_threads()
{
    return sys_emu::running_threads_count();
}

void sim_api_communicate::SysEmuWrapper::print_iatus()
{
    const auto& iatus = sim_->chip_->memory.pcie0_get_iatus();

    for (int i = 0; i < iatus.size(); i++) {
        uint64_t iatu_base_addr = (uint64_t)iatus[i].upper_base_addr << 32 |
                                  (uint64_t)iatus[i].lwr_base_addr;
        uint64_t iatu_limit_addr = (uint64_t)iatus[i].uppr_limit_addr << 32 |
                                   (uint64_t)iatus[i].limit_addr;
        uint64_t iatu_target_addr = (uint64_t)iatus[i].upper_target_addr << 32 |
                                    (uint64_t)iatus[i].lwr_target_addr;

        LOG_NOTHREAD(INFO, "iATU[%d].ctrl_1: 0x%x", i, iatus[i].ctrl_1);
        LOG_NOTHREAD(INFO, "iATU[%d].ctrl_2: 0x%x", i, iatus[i].ctrl_2);
        LOG_NOTHREAD(INFO, "iATU[%d].base_addr: 0x%" PRIx64, i, iatu_base_addr);
        LOG_NOTHREAD(INFO, "iATU[%d].limit_addr : 0x%" PRIx64, i, iatu_limit_addr);
        LOG_NOTHREAD(INFO, "iATU[%d].target_addr: 0x%" PRIx64, i, iatu_target_addr);
    }
}

bool sim_api_communicate::SysEmuWrapper::iatu_translate(uint64_t pci_addr, uint64_t size,
                                                        uint64_t &device_addr,
                                                        uint64_t &access_size)
{
    const auto& iatus = sim_->chip_->memory.pcie0_get_iatus();

    for (int i = 0; i < iatus.size(); i++) {
        // Check REGION_EN (bit[31])
        if (((iatus[i].ctrl_2 >> 31) & 1) == 0)
            continue;

        // Check MATCH_MODE (bit[30]) to be Address Match Mode (0)
        if (((iatus[i].ctrl_2 >> 30) & 1) != 0) {
            LOG_NOTHREAD(FTL, "iATU[%d]: Unsupported MATCH_MODE", i);
        }

        uint64_t iatu_base_addr = (uint64_t)iatus[i].upper_base_addr << 32 |
                                  (uint64_t)iatus[i].lwr_base_addr;
        uint64_t iatu_limit_addr = (uint64_t)iatus[i].uppr_limit_addr << 32 |
                                   (uint64_t)iatus[i].limit_addr;
        uint64_t iatu_target_addr = (uint64_t)iatus[i].upper_target_addr << 32 |
                                    (uint64_t)iatus[i].lwr_target_addr;
        uint64_t iatu_size = iatu_limit_addr - iatu_base_addr + 1;

        // Address within iATU
        if (pci_addr >= iatu_base_addr && pci_addr <= iatu_limit_addr) {
            uint64_t host_access_end = pci_addr + size - 1;
            uint64_t access_end = std::min(host_access_end, iatu_limit_addr) + 1;
            uint64_t offset = pci_addr - iatu_base_addr;

            access_size = access_end - pci_addr;
            device_addr = iatu_target_addr + offset;
            return true;
        }
    }

    return false;
}

bool sim_api_communicate::SysEmuWrapper::memory_read(uint64_t device_addr, size_t size, void *data)
{
    LOG_NOTHREAD(DEBUG, "sim_api_communicate: memory_read(device_addr = %" PRIx64 ", size = %zu)", device_addr, size);

    sim_->chip_->memory.read(*this, device_addr, size, data);
    return true;
}

bool sim_api_communicate::SysEmuWrapper::memory_write(uint64_t device_addr, size_t size, const void *data)
{
    LOG_NOTHREAD(DEBUG, "sim_api_communicate: memory_write(device_addr = %" PRIx64 ", size = %zu)", device_addr, size);

    sim_->chip_->memory.write(*this, device_addr, size, data);
    return true;
}

bool sim_api_communicate::SysEmuWrapper::pci_memory_read(uint64_t pci_addr, uint64_t size, void *data)
{
    uint64_t host_access_offset = 0;

    LOG_NOTHREAD(DEBUG, "sim_api_communicate: pci_memory_read(pci_addr = %" PRIx64 ", size = %zu)", pci_addr, size);

    while (size > 0) {
        uint64_t device_addr, access_size;

        if (!iatu_translate(pci_addr, size, device_addr, access_size)) {
            LOG_NOTHREAD(WARN, "iATU: Could not find translation for host address: 0x%" PRIx64
                               ", size: 0x%" PRIx64,
                         pci_addr, size);
            print_iatus();
            break;
        }

        sim_->chip_->memory.read(*this, device_addr, access_size, (char *)data + host_access_offset);

        pci_addr += access_size;
        host_access_offset += access_size;
        size -= access_size;
    }

    // If there's pending data: access not fully covered by iATUs / translation failure
    if (size > 0) {
        return false;
    }

    return true;
}

bool sim_api_communicate::SysEmuWrapper::pci_memory_write(uint64_t pci_addr, uint64_t size, const void *data)
{
    uint64_t host_access_offset = 0;

    LOG_NOTHREAD(DEBUG, "sim_api_communicate: memory_write(pci_addr = %" PRIx64 ", size = %zu)", pci_addr, size);

    while (size > 0) {
        uint64_t device_addr, access_size;

        if (!iatu_translate(pci_addr, size, device_addr, access_size)) {
            LOG_NOTHREAD(WARN, "iATU: Could not find translation for host address: 0x%" PRIx64
                               ", size: 0x%" PRIx64,
                         pci_addr, size);
            print_iatus();
            break;
        }

        sim_->chip_->memory.write(*this, device_addr, access_size, (char *)data + host_access_offset);

        pci_addr += access_size;
        host_access_offset += access_size;
        size -= access_size;
    }

    // If there's pending data: access not fully covered by iATUs / translation failure
    if (size > 0) {
        return false;
    }

    return true;
}

bool sim_api_communicate::SysEmuWrapper::mailbox_read(simulator_api::MailboxTarget target, uint32_t offset, size_t size, void *data)
{
    LOG_NOTHREAD(DEBUG, "sim_api_communicate: mailbox_read(target = %d, offset = %d, size = %d)", target, offset, size);

    switch (target) {
    case simulator_api::MailboxTarget::MAILBOX_TARGET_MM:
        sim_->chip_->memory.pc_mm_mailbox_read(*this, offset, size, reinterpret_cast<bemu::MemoryRegion::pointer>(data));
        return true;
    case simulator_api::MailboxTarget::MAILBOX_TARGET_SP:
        sim_->chip_->memory.pc_sp_mailbox_read(*this, offset, size, reinterpret_cast<bemu::MemoryRegion::pointer>(data));
        return true;
    }

    return false;
}

bool sim_api_communicate::SysEmuWrapper::mailbox_write(simulator_api::MailboxTarget target, uint32_t offset, size_t size, const void *data)
{
    LOG_NOTHREAD(DEBUG, "sim_api_communicate: mailbox_write(target = %d, offset = %d, size = %d)", target, offset, size);

    switch (target) {
    case simulator_api::MailboxTarget::MAILBOX_TARGET_MM:
        sim_->chip_->memory.pc_mm_mailbox_write(*this, offset, size, reinterpret_cast<bemu::MemoryRegion::const_pointer>(data));
        return true;
    case simulator_api::MailboxTarget::MAILBOX_TARGET_SP:
        sim_->chip_->memory.pc_sp_mailbox_write(*this, offset, size, reinterpret_cast<bemu::MemoryRegion::const_pointer>(data));
        return true;
    }

    return false;
}

bool sim_api_communicate::SysEmuWrapper::raise_device_interrupt(simulator_api::DeviceInterruptType type)
{
    LOG_NOTHREAD(INFO, "sim_api_communicate: raise_device_interrupt(type = %d)", (int)type);

    switch (type) {
    case simulator_api::DeviceInterruptType::PU_PLIC_PCIE_MESSAGE_INTERRUPT:
        sim_->chip_->memory.pu_trg_pcie_mmm_int_inc(*this);
        break;
    case simulator_api::DeviceInterruptType::SPIO_PLIC_MBOX_HOST_INTERRUPT:
        sim_->chip_->memory.pu_trg_pcie_ipi_trigger(*this);
        break;
    default:
        return false;
    }

    return true;
}

void sim_api_communicate::SysEmuWrapper::shire_threads_set_pc(unsigned shire_id, uint64_t pc)
{
    if (shire_id == IO_SHIRE_ID)
        shire_id = EMU_IO_SHIRE_SP;

    unsigned thread0 = EMU_THREADS_PER_SHIRE * shire_id;
    unsigned shire_thread_count = (shire_id == EMU_IO_SHIRE_SP ? 1 : EMU_THREADS_PER_SHIRE);

    for (unsigned t = 0; t < shire_thread_count; ++t)
        sys_emu::thread_set_pc(thread0 + t, pc);
}

// Constructor
sim_api_communicate::sim_api_communicate(const std::string &socket_path) :
    done_(false),
    socket_path_(socket_path),
    wrapper_(this),
    sim_api_(&wrapper_)
{
    LOG_NOTHREAD(INFO, "%s", "sim_api_communicate: Init");
    sim_api_.init();
}

sim_api_communicate::~sim_api_communicate()
{
}

void sim_api_communicate::set_system(bemu::System* system)
{
    chip_ = system;
    wrapper_.Agent::chip = system;
}

void sim_api_communicate::process(void)
{
    // pass if the sim-api call will be blocking or not
    // Return immediately if there is no
    // pending message from the host. Do not block as the runtime
    // expects that the device is always executing
    sim_api_.nextCmd(true);
}

bool sim_api_communicate::raise_host_interrupt(uint32_t bitmap)
{
    LOG_NOTHREAD(INFO, "sim_api_communicate: Raise Host Interrupt (0x%" PRIx32 ")", bitmap);
    return sim_api_.raiseHostInterrupt(bitmap);
}

bool sim_api_communicate::host_memory_read(uint64_t host_addr, uint64_t size, void *data)
{
    LOG_NOTHREAD(INFO, "sim_api_communicate::host_memory_read(host_addr = 0x%" PRIx64 ", size = 0x%" PRIx64 ")",
                 host_addr, size);
    return sim_api_.readHostMemory(host_addr, size, data);
}

bool sim_api_communicate::host_memory_write(uint64_t host_addr, uint64_t size, const void *data)
{
    LOG_NOTHREAD(INFO, "sim_api_communicate::host_memory_write(host_addr = 0x%" PRIx64 ", size = 0x%" PRIx64 ")",
                 host_addr, size);
    return sim_api_.writeHostMemory(host_addr, size, data);
}

void sim_api_communicate::notify_iatu_ctrl_2_reg_write(int, uint32_t, uint32_t)
{
    /* Do nothing, we don't care about this notification */
}
