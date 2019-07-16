#ifndef _API_COMMUNICATE_
#define _API_COMMUNICATE_

// STD
#include <list>

// Global

// Local
#include "memory/main_memory.h"

// Defines

// Scratch area to share information when MM sends an IPI to Compute Minions
#define FW_KERNEL_LAUNCH_IPI_INFO  0x8000600000ULL
// Address where the host will place the kernel launch information (synced with fw_common.h)
#define RT_HOST_KERNEL_LAUNCH_INFO 0x8200000000ULL

// Class that receives commands from the runtime API and forwards it to SoC
class api_communicate
{
    public:
        // Constructor and destructor
        api_communicate(bemu::MainMemory* mem_)
        : mem(mem_), enabled(false), done(false), communication_channel(-1)
        { }

        // Set
        void set_comm_path(char * api_comm_path);

        // Access
        bool is_done();
        bool is_enabled();

        // Execution
        void get_next_cmd(std::list<int> * enabled_threads);

    private:
        bemu::MainMemory* mem;                   // Pointer to the memory
        bool              enabled;               // The communication through API has been enabled
        bool              done;                  // Is there pending work to be done
        int               communication_channel; // Opened descriptor for socket communication

        // Socket auxiliar
        ssize_t read_bytes(int fd, void * buf, size_t count);
        ssize_t write_bytes(int fd, const void * buf, size_t count);

        // Types
        typedef struct {
            uint64_t tensor_a;     // Pointer to tensor A
            uint64_t tensor_b;     // Pointer to tensor B
            uint64_t tensor_c;     // Pointer to tensor C
            uint64_t tensor_d;     // Pointer to tensor D
            uint64_t tensor_e;     // Pointer to tensor E
            uint64_t tensor_f;     // Pointer to tensor F
            uint64_t tensor_g;     // Pointer to tensor G
            uint64_t tensor_h;     // Pointer to tensor H
            uint64_t kernel_id;    // Id for this Kernel
        } layer_dynamic_info;

        typedef enum
        {
            kIPIShutdown,
            kIPIInitMem,
            kIPIInitExecMem,
            kIPIWriteMem,
            kIPIReadMem,
            kIPIExecute,
            kIPISync,
            kIPIContinue
        } CmdTypes;

        typedef struct
        {
            long long mem_addr;
            long long mem_size;
        } MemDescMsg;

        typedef struct
        {
            long long          launch_pc;
            layer_dynamic_info params;
        } LaunchDescMsg;

        // Struct that stores kernel launch information copied by the Host when using runtime
        typedef struct {
            uint64_t unused;
            uint64_t compute_pc;
            layer_dynamic_info params;
        } __attribute__((packed)) rt_host_kernel_launch_info_t;
};

#endif // _API_COMMUNICATE_

