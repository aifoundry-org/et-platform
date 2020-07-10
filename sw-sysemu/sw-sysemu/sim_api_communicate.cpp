// Global
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string>
#include <assert.h>
#include <unistd.h>
#include <endian.h>

// Local
#include "sim_api_communicate.h"
#include "emu_gio.h"
#include "sys_emu.h"

using namespace std;

sim_api_communicate::SysEmuWrapper::SysEmuWrapper(sim_api_communicate* sim)
    : AbstractSimulator(sim->comm_path_),
      sim_(sim)
{
}

bool sim_api_communicate::SysEmuWrapper::boot(uint64_t pc)
{
    LOG_NOTHREAD(INFO, "sim_api_communicate: boot(pc = 0x%016" PRIx64 ")", pc);

    // Boot all compute shire threads
    for (int s = 0; s < EMU_NUM_COMPUTE_SHIRES; s++) {
        shire_threads_set_pc(s, pc);
        sys_emu::shire_enable_threads(s, 0, 0);
    }

    // Boot all master shire threads
    shire_threads_set_pc(EMU_MASTER_SHIRE, pc);
    sys_emu::shire_enable_threads(EMU_MASTER_SHIRE, 0, 0);

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

bool sim_api_communicate::SysEmuWrapper::mb_read(struct mbox_t* mbox)
{
    LOG_NOTHREAD(DEBUG, "%s", "sim_api_communicate: mb_read");
    sim_->mem->pu_mbox_space.pu_mbox_pc_mm.read(*this, 0, sizeof(*mbox),
        reinterpret_cast<bemu::MemoryRegion::pointer>(mbox));
    return true;
}

bool sim_api_communicate::SysEmuWrapper::mb_write(const struct mbox_t& mbox)
{
    LOG_NOTHREAD(DEBUG, "%s", "sim_api_communicate: mb_write");
    sim_->mem->pu_mbox_space.pu_mbox_pc_mm.write(*this, 0, sizeof(mbox),
        reinterpret_cast<bemu::MemoryRegion::const_pointer>(&mbox));
    return true;
}

bool sim_api_communicate::SysEmuWrapper::raise_device_interrupt(simulator_api::DeviceInterruptType type)
{
    LOG_NOTHREAD(DEBUG, "sim_api_communicate: raise_device_interrupt(type = %d)", (int)type);

    switch (type) {
    case simulator_api::DeviceInterruptType::PU_PLIC_PCIE_MESSAGE_INTERRUPT:
        sim_->mem->pu_mbox_space.pu_trg_mmin.interrupt_inc();
        break;
    case simulator_api::DeviceInterruptType::MASTER_SHIRE_IPI_INTERRUPT:
        // Send an IPI to the thread 0 of Master Shire
        sys_emu::raise_software_interrupt(EMU_MASTER_SHIRE, 1);
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
