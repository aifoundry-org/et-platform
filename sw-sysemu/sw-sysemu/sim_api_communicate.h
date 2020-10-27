#ifndef _SIM_API_COMMUNICATE_
#define _SIM_API_COMMUNICATE_

// STD
#include <list>
#include <sstream>

// Global
#include <grpcpp/grpcpp.h>

// Local
#include "agent.h"
#include "api_communicate.h"
#include "emu_gio.h"
#include "memory/main_memory.h"

#define ENABLE_LOGGING 0

#define SLOG(...) {                             \
    std::stringstream out_ ;                    \
    out_ << __VA_ARGS__ ;                          \
    LOG_NOTHREAD(INFO, "%s", out_.str().c_str());  \
} while(0)

/*
#define SDEBUG(...) {                             \
    std::stringstream out_ ;                    \
    out_ << __VA_ARGS__ ;                          \
    LOG_NOTHREAD(INFO, "%s", out_.str().c_str());  \
} while(0)
*/

#include "esperanto/simulator_server.h"

// Class that receives commands from the runtime API and forwards it to SoC
class sim_api_communicate final : public api_communicate
{
    public:
        // Constructor and destructor
        sim_api_communicate(bool sim_api_async);
        ~sim_api_communicate();

        bool init() override;
        bool is_enabled() override;
        bool is_done() override;
        void get_next_cmd(std::list<int> *enabled_threads) override;
        void set_comm_path(const std::string &comm_path) override;
        bool raise_host_interrupt(uint32_t bitmap) override;
    private:
        friend class SysEmuWrapper;
        class SysEmuWrapper final : public bemu::Agent, public AbstractSimulator {
        public:
            SysEmuWrapper(sim_api_communicate* sim);
            bool boot(uint32_t shire_id, uint32_t thread0_enable, uint32_t thread1_enable) override;
            bool shutdown() override;
            bool is_done() override;
            int active_threads() override;
            bool memory_read(uint64_t ad, size_t size, void *data) override;
            bool memory_write(uint64_t ad, size_t size, const void *data) override;
            bool mailbox_read(simulator_api::MailboxTarget target, uint32_t offset, size_t size, void *data)  override;
            bool mailbox_write(simulator_api::MailboxTarget target, uint32_t offset, size_t size, const void *data)  override;
            bool raise_device_interrupt(simulator_api::DeviceInterruptType type) override;

            /* For bemu::Agent */
            std::string name() const override { return std::string("SysEmuWrapper"); }
        protected:
            void shire_threads_set_pc(unsigned shire_id, uint64_t pc);
        private:
            void print_iatus();
            bool iatu_translate(uint64_t host_addr, uint64_t size, uint64_t &device_addr,
                                uint64_t &access_size);
            sim_api_communicate* sim_;
        };

        bool done_;
        std::string comm_path_;
        bool sim_api_async_;
        SysEmuWrapper wrapper_;
        SimAPIServer<SysEmuWrapper> sim_api_;
};


#endif // _SIM_API_COMMUNICATE_
