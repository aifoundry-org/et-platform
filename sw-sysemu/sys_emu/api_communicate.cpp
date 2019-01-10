// Global
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <assert.h>

// Local
#include "api_communicate.h"

// Constructor
api_communicate::api_communicate(main_memory * mem_)
{
    log                   = NULL;
    mem                   = mem_;
    enabled               = false;
    done                  = false;
    communication_channel = -1;
}

// Destructor
api_communicate::~api_communicate()
{
}

// Set
void api_communicate::set_comm_path(char * api_comm_path)
{
    enabled = true;
    * log << LOG_INFO << "api_communicate: Init" << endm;

    communication_channel = socket( AF_UNIX, SOCK_STREAM, 0);

    sockaddr_un addr_un;
    memset( &addr_un, 0, sizeof(addr_un));

    addr_un.sun_family = AF_UNIX;
    strcpy( addr_un.sun_path, api_comm_path);

    if ( connect( communication_channel, (sockaddr*)&addr_un, sizeof(addr_un)) != 0 )
    {
        * log << LOG_FTL << "Failed to open IPI communication pipet" << endm;
    }
}

// Set
void api_communicate::set_log(testLog * log_)
{
    log = log_;
}

// Done
bool api_communicate::is_done(){
    return done;
}

// Enabed
bool api_communicate::is_enabled(){
    return enabled;
}

// Execution
void api_communicate::get_next_cmd(std::list<int> * enabled_threads, std::list<int> * ipi_threads_t0, uint64_t * new_pc_t0, std::list<int> * ipi_threads_t1, uint64_t * new_pc_t1)
{
    // Waits until all minions are idle
    if(!enabled || (enabled_threads->size() != 0)) return;

    * log << LOG_INFO << "api_communicate: command" << endm;

    char cmd;
    if ( read_bytes( communication_channel, &cmd, 1) != 1 )
    {
        * log << LOG_FTL << "api_communicate: communucation command retrieve failed." << endm;
        return;
    }

    switch ( cmd )
    {
        case kIPIInitMem:
            {
                MemDescMsg mem_def;

                read_bytes( communication_channel, &mem_def, sizeof(MemDescMsg));
                * log << LOG_INFO << "api_communicate: Init mem 0x" << std::hex << mem_def.mem_addr << ", 0x" << mem_def.mem_size << std::dec << endm;
                mem->new_region( mem_def.mem_addr, mem_def.mem_size);
            }
            break;
        case kIPIInitExecMem:
            {
                MemDescMsg mem_def;

                read_bytes( communication_channel, &mem_def, sizeof(MemDescMsg));
                * log << LOG_INFO << "api_communicate: Init exec mem 0x" << std::hex << mem_def.mem_addr << ", 0x" << mem_def.mem_size << std::dec << endm;
                mem->new_region( mem_def.mem_addr, mem_def.mem_size);
            }
            break;
        case kIPIReadMem:
            {
                MemDescMsg mem_def;

                read_bytes( communication_channel, &mem_def, sizeof(MemDescMsg));
                char * tmp_mem = new char[mem_def.mem_size];
                * log << LOG_INFO << "api_communicate: Read mem 0x" << std::hex << mem_def.mem_addr << ", 0x" << mem_def.mem_size << std::dec << endm;
                mem->read(mem_def.mem_addr, mem_def.mem_size, tmp_mem);
                write_bytes( communication_channel, tmp_mem, mem_def.mem_size);
                delete[] tmp_mem;
            }
            break;
        case kIPIWriteMem:
            {
                MemDescMsg mem_def;

                read_bytes( communication_channel, &mem_def, sizeof(MemDescMsg));
                char* tmp_mem = new char[mem_def.mem_size];
                * log << LOG_INFO << "api_communicate: Write mem 0x" << std::hex << mem_def.mem_addr << ", 0x" << mem_def.mem_size << std::dec << endm;
                read_bytes( communication_channel, tmp_mem, mem_def.mem_size);
                mem->write(mem_def.mem_addr, mem_def.mem_size, tmp_mem);
                delete[] tmp_mem;
            }
            break;
        case kIPIExecute:
            {
                LaunchDescMsg launch_def;

                assert(32 < (EMU_NUM_MINIONS / EMU_MINIONS_PER_SHIRE));

                read_bytes( communication_channel, &launch_def, sizeof(LaunchDescMsg));
                * log << LOG_INFO << "api_communicate: Execute 0x" << std::hex << launch_def.thread0_pc << ", 0x" << launch_def.thread1_pc << std::dec << endm;

                // Gets the PCs
                * new_pc_t0 = launch_def.thread0_pc;
                * new_pc_t1 = launch_def.thread1_pc;

                // Generate list of IPI based on masks for compute minions
                for(int s = 0; s < 32; s++)
                {
                    // If shire enabled
                    if((launch_def.shire_mask >> s) & 1)
                    {
                        for(int m = 0; m < EMU_MINIONS_PER_SHIRE; m++)
                        {
                            // If minion enabled
                            if((launch_def.minion_mask >> m) & 1)
                            {
                                int minion_id = s * EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION + m * EMU_THREADS_PER_MINION;
                                ipi_threads_t0->push_back(minion_id);
                                ipi_threads_t1->push_back(minion_id+1);
                            }
                        }
                    }
                }
            }
            break;
        case kIPISync:
            {
                char ret = 0;
                * log << LOG_INFO << "api_communicate: sync" << endm;
                write( communication_channel, &ret, 1 );
            }
            break;
        case kIPIContinue:
            * log << LOG_INFO << "api_communicate: continue" << endm;
            
            assert(32 < (EMU_NUM_MINIONS / EMU_MINIONS_PER_SHIRE));

            // PC 0 means continue
            * new_pc_t0 = 0;
            * new_pc_t1 = 0;
            // Resume operation on all compute minions
            for(int s = 0; s < 32; s++)
            {
                for(int m = 0; m < EMU_MINIONS_PER_SHIRE; m++)
                {
                    int minion_id = s * EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION + m * EMU_THREADS_PER_MINION;
                    ipi_threads_t0->push_back(minion_id);
                    ipi_threads_t1->push_back(minion_id+1);
                }
            }
            break;
        case kIPIShutdown:
            * log << LOG_INFO << "api_communicate: shutdown" << endm;
            done = true;
            break;
        default:
            * log << LOG_FTL << "api_communicate: Unknown command received" << endm;
    }
}

// Auxiliar function to write bytes to socket
ssize_t api_communicate::read_bytes(int fd, void * buf, size_t count)
{
    size_t n_bytes_read = 0;
    do
    {
        ssize_t r = TEMP_FAILURE_RETRY(read(fd, (char *)buf + n_bytes_read, count - n_bytes_read));
        if ( r <= 0 )
        {
            assert(!"Not all bytes read");
            return -1;
        }
        n_bytes_read += r;
    } while ( n_bytes_read < count );

    assert( n_bytes_read == count );
    return n_bytes_read;
}

// Auxiliar function to write bytes to socket
ssize_t api_communicate::write_bytes(int fd, const void * buf, size_t count)
{
    size_t n_bytes_written = 0;
    do
    {
        ssize_t w = TEMP_FAILURE_RETRY(write(fd, (char *)buf + n_bytes_written, count - n_bytes_written));
        if ( w <= 0 )
        {
            assert(!"Not all bytes written");
            return -1;
        }
        n_bytes_written += w;
    } while ( n_bytes_written < count );

    assert( n_bytes_written == count );
    return n_bytes_written;
}

