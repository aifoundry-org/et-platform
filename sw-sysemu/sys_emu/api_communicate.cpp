// Global
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <assert.h>
#include <unistd.h>
#include <endian.h>

// Local
#include "api_communicate.h"
#include "emu_gio.h"
#include "sys_emu.h"

// Set
void api_communicate::set_comm_path(char * api_comm_path)
{
    enabled = true;
    LOG_NOTHREAD(INFO, "%s", "api_communicate: Init");

    communication_channel = socket( AF_UNIX, SOCK_STREAM, 0);

    sockaddr_un addr_un;
    memset( &addr_un, 0, sizeof(addr_un));

    addr_un.sun_family = AF_UNIX;
    strcpy( addr_un.sun_path, api_comm_path);

    if ( connect( communication_channel, (sockaddr*)&addr_un, sizeof(addr_un)) != 0 )
    {
        LOG_NOTHREAD(FTL, "%s", "Failed to open IPI communication pipe");
    }
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
void api_communicate::get_next_cmd(std::list<int> * enabled_threads)
{
    // Waits until all minions are idle
    if(!enabled || (enabled_threads->size() != 0)) return;

    LOG_NOTHREAD(INFO, "%s", "api_communicate: command");

    char cmd;
    if ( read_bytes( communication_channel, &cmd, 1) != 1 )
    {
        LOG_NOTHREAD(FTL, "%s", "api_communicate: communucation command retrieve failed.");
        return;
    }

    switch ( cmd )
    {
        case kIPIInitMem:
            {
                MemDescMsg mem_def;

                read_bytes( communication_channel, &mem_def, sizeof(MemDescMsg));
                LOG_NOTHREAD(WARN, "api_communicate: Init mem 0x%llx, 0x%llx command ignored",
                             mem_def.mem_addr, mem_def.mem_size);
            }
            break;
        case kIPIInitExecMem:
            {
                MemDescMsg mem_def;

                read_bytes( communication_channel, &mem_def, sizeof(MemDescMsg));
                LOG_NOTHREAD(WARN, "api_communicate: Init exec mem 0x%llx, 0x%llx command ignored",
                             mem_def.mem_addr, mem_def.mem_size);
            }
            break;
        case kIPIReadMem:
            {
                MemDescMsg mem_def;

                read_bytes( communication_channel, &mem_def, sizeof(MemDescMsg));
                char * tmp_mem = new char[mem_def.mem_size];
                LOG_NOTHREAD(INFO, "api_communicate: Read mem 0x%llx, 0x%llx", mem_def.mem_addr, mem_def.mem_size);
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
                LOG_NOTHREAD(INFO, "api_communicate: Write mem 0x%llx, 0x%llx", mem_def.mem_addr, mem_def.mem_size);
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
                LOG_NOTHREAD(INFO, "api_communicate: Execute: launch_pc: 0x%llx", launch_def.launch_pc);

                rt_host_kernel_launch_info_t launch_info;
                launch_info.compute_pc = htole64(launch_def.launch_pc);
                // TODO: Endianness
                launch_info.params = launch_def.params;

                // Write the kernel launch parameters to memory (RT_HOST_KERNEL_LAUNCH_INFO in fw_common.h)
                mem->write(RT_HOST_KERNEL_LAUNCH_INFO, sizeof(launch_info), &launch_info);

                LOG_NOTHREAD(DEBUG, "api_communicate: Execute: %s", "Sending IPI to Master Shire thread 0");

                // Send an IPI to the thread 0 of Master Shire
                sys_emu::raise_software_interrupt(32, 1);
            }
            break;
        case kIPISync:
            {
                int ret;
                char val = 0;
                LOG_NOTHREAD(INFO, "%s", "api_communicate: sync");
                if ((ret = write( communication_channel, &val, 1 )) < 0) {
                    LOG_NOTHREAD(ERR, "Error, write() returned: %d", ret);
                }
            }
            break;
        case kIPIContinue:
            {
                LOG_NOTHREAD(INFO, "%s", "api_communicate: continue");
                assert(32 < (EMU_NUM_MINIONS / EMU_MINIONS_PER_SHIRE));

                // Write 0s tell we are in runtime mode (see fw_master_scode.cc)
                rt_host_kernel_launch_info_t launch_info;
                memset(&launch_info, 0, sizeof(launch_info));

                // Write the kernel launch parameters to memory (RT_HOST_KERNEL_LAUNCH_INFO in fw_common.h)
                mem->write(RT_HOST_KERNEL_LAUNCH_INFO, sizeof(launch_info), &launch_info);

                // Boot all compute shire minions
                for (int s = 0; s < EMU_NUM_COMPUTE_SHIRES; s++) {
                    for (int m = 0; m < EMU_MINIONS_PER_SHIRE; m++) {
                            int minion_id = s * EMU_THREADS_PER_SHIRE +
                                            m * EMU_THREADS_PER_MINION;
                            enabled_threads->push_back(minion_id);
                            enabled_threads->push_back(minion_id + 1);
                    }
                }

                // Boot all master shire minions
                for (int m = 0; m < EMU_MINIONS_PER_SHIRE; m++) {
                        int minion_id = EMU_MASTER_SHIRE * EMU_THREADS_PER_SHIRE +
                                        m * EMU_THREADS_PER_MINION;
                        enabled_threads->push_back(minion_id);
                        enabled_threads->push_back(minion_id + 1);
                }
            }
            break;
        case kIPIShutdown:
            LOG_NOTHREAD(INFO, "%s", "api_communicate: shutdown");
            done = true;
            break;
        default:
            LOG_NOTHREAD(FTL, "%s", "api_communicate: Unknown command received");
            break;
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

