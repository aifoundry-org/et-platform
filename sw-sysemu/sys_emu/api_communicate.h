#ifndef _API_COMMUNICATE_
#define _API_COMMUNICATE_

// STD
#include <list>
#include <string>

// Forward declarations
namespace bemu {
struct MainMemory;
}

// Class that receives commands from the runtime API and forwards it to SoC
class api_communicate
{
    public:
        api_communicate(bemu::MainMemory *mem) : mem(mem) { }

        virtual bool init() = 0;
        virtual bool is_enabled() = 0;
        virtual bool is_done() = 0;
        virtual void get_next_cmd(std::list<int> *enabled_threads) = 0;
        virtual void set_comm_path(const char *api_comm_path) = 0;
        virtual bool raise_host_interrupt() = 0;

    protected:
        bemu::MainMemory* mem;  // Pointer to the memory
};

#endif // _API_COMMUNICATE_
