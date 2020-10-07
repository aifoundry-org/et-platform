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
#include "sys_emu.h"
#include "memory/mailbox_region.h"

// sw-sysemu
#include "sim_api_communicate.h"

using namespace std;

sim_api_communicate::SysEmuWrapper::SysEmuWrapper(sim_api_communicate* sim)
    : AbstractSimulator(sim->comm_path_),
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

bool sim_api_communicate::SysEmuWrapper::read(uint64_t ad, size_t size, void *data)
{
    LOG_NOTHREAD(DEBUG, "sim_api_communicate: read(ad = %" PRIx64 ", size = %zu)", ad, size);
    sim_->mem->read(*this, ad, size, data);
    return true;
}

bool sim_api_communicate::SysEmuWrapper::write(uint64_t ad, size_t size, const void *data)
{
    LOG_NOTHREAD(DEBUG, "sim_api_communicate: write(ad = %" PRIx64 ", size = %zu)", ad, size);
    sim_->mem->write(*this, ad, size, data);
    return true;
}

bool sim_api_communicate::SysEmuWrapper::mb_read(simulator_api::MailBoxTarget target, uint32_t offset, size_t size, void *data)
{
    LOG_NOTHREAD(DEBUG, "sim_api_communicate: mb_read(target = %d, offset = %d, size = %d)", target, offset, size);

    switch (target) {
    case simulator_api::MailBoxTarget::MB_TARGET_MM:
        sim_->mem->pu_mbox_space.pu_mbox_pc_mm.read(*this, offset, size,
            reinterpret_cast<bemu::MemoryRegion::pointer>(data));
        return true;
    case simulator_api::MailBoxTarget::MB_TARGET_SP:
        sim_->mem->pu_mbox_space.pu_mbox_pc_sp.read(*this, offset, size,
            reinterpret_cast<bemu::MemoryRegion::pointer>(data));
        return true;
    }

    return false;
}

bool sim_api_communicate::SysEmuWrapper::mb_write(simulator_api::MailBoxTarget target, uint32_t offset, size_t size, const void *data)
{
    LOG_NOTHREAD(DEBUG, "sim_api_communicate: mb_write(target = %d, offset = %d, size = %d)", target, offset, size);

    switch (target) {
    case simulator_api::MailBoxTarget::MB_TARGET_MM:
        sim_->mem->pu_mbox_space.pu_mbox_pc_mm.write(*this, offset, size,
            reinterpret_cast<bemu::MemoryRegion::const_pointer>(data));
        return true;
    case simulator_api::MailBoxTarget::MB_TARGET_SP:
        sim_->mem->pu_mbox_space.pu_mbox_pc_sp.write(*this, offset, size,
            reinterpret_cast<bemu::MemoryRegion::const_pointer>(data));
        return true;
    }

    return false;
}

bool sim_api_communicate::SysEmuWrapper::raise_device_interrupt(simulator_api::DeviceInterruptType type)
{
    LOG_NOTHREAD(DEBUG, "sim_api_communicate: raise_device_interrupt(type = %d)", (int)type);

    switch (type) {
    case simulator_api::DeviceInterruptType::PU_PLIC_PCIE_MESSAGE_INTERRUPT: {
        uint32_t trigger = 1;
        sim_->mem->pu_mbox_space.pu_trg_pcie.write(*this, bemu::MMM_INT_INC, sizeof(trigger),
            reinterpret_cast<bemu::MemoryRegion::const_pointer>(&trigger));
        break;
    }
    case simulator_api::DeviceInterruptType::SPIO_PLIC_MBOX_HOST_INTERRUPT: {
        uint32_t trigger = 1;
        sim_->mem->pu_mbox_space.pu_trg_pcie.write(*this, bemu::IPI_TRIGGER, sizeof(trigger),
            reinterpret_cast<bemu::MemoryRegion::const_pointer>(&trigger));
        break;
    }
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
sim_api_communicate::sim_api_communicate(bool sim_api_async) :
    done_(false),
    sim_api_async_(sim_api_async),
    wrapper_(this),
    // If sim_api_async is true then we enable threading in the api-server
    // where a separate thread is listening for simulator-api commands
    sim_api_(&wrapper_, sim_api_async)
{
}

sim_api_communicate::~sim_api_communicate()
{
}

bool sim_api_communicate::init()
{
    LOG_NOTHREAD(INFO, "%s", "sim_api_communicate: Init");
    return sim_api_.init();
}

bool sim_api_communicate::is_enabled()
{
    return sim_api_.is_enabled();
}

bool sim_api_communicate::is_done()
{
    return done_;
}

void sim_api_communicate::get_next_cmd(std::list<int> *enabled_threads)
{
    // pass if the sim-api call will be blocking or not
    // Return immediately if there is no
    // pending message from the host. Do not block as the runtime
    // expects that the device is always executing
    sim_api_.nextCmd(sim_api_async_);
}

void sim_api_communicate::set_comm_path(const std::string &comm_path)
{
    comm_path_ = comm_path;
    wrapper_.setCommunicationPath(comm_path_);
}

bool sim_api_communicate::raise_host_interrupt()
{
    LOG_NOTHREAD(DEBUG, "%s", "sim_api_communicate: Raise Host Interrupt");
    sim_api_.raiseHostInterrupt();
    return true;
}
