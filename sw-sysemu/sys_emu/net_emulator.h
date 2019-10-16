#ifndef _NET_EMULATOR_
#define _NET_EMULATOR_

// STD
#include <list>

// Global
#include <stdlib.h>
#include <inttypes.h>

// Forward declarations
namespace bemu {
struct MainMemory;
}

typedef struct
{
    uint64_t compute_pc;
    uint64_t helper_pc;
    uint64_t shire_mask;
    uint64_t minion_mask;
    char     name[1024];
    int      id;
    uint64_t tensor_a;
    uint64_t tensor_b;
    uint64_t tensor_c;
    uint64_t tensor_d;
    uint64_t tensor_e;
    uint32_t compute_size;
    uint32_t helper_size;
    uint64_t info_pointer;
} net_emulator_layer;

// Class that emulates the network behaviour (maxion interrupts)
class net_emulator
{
    public:
        // Constructor and destructor
        net_emulator() = default;
        net_emulator(bemu::MainMemory* mem_) : helper_done(false), mem(mem_), enabled(false) {}

        // Set
        void set_file(char * net_desc_file);

        // Access
        bool done();
        bool is_enabled();

        // Execution
        void get_new_ipi(std::list<int> * running_threads, std::list<int> * ipi_threads, uint64_t * new_pc);
    private:
        std::list<net_emulator_layer> layers;      // List of layers to do
        bool                          helper_done; // Helper IPI already sent
        bemu::MainMemory*             mem;         // Pointer to the memory
        bool                          enabled;     // net emu enabled only for nets
};

#endif // _INSTRUCTION_
