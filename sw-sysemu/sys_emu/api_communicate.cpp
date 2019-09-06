// Global
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <assert.h>
#include <unistd.h>
#include <endian.h>

// Local
#include "sys_emu.h"
#include "emu_gio.h"
#include "api_communicate.h"
#include "memory/main_memory.h"

// Set
void api_communicate::set_comm_path(const char * api_comm_path)
{
    communication_path = api_comm_path;
}

bool api_communicate::init()
{
    enabled = true;
    LOG_NOTHREAD(INFO, "%s", "api_communicate: Init");

    communication_channel = socket( AF_UNIX, SOCK_STREAM, 0);

    sockaddr_un addr_un;
    memset( &addr_un, 0, sizeof(addr_un));

    addr_un.sun_family = AF_UNIX;
    strcpy( addr_un.sun_path, communication_path.c_str());

    if ( connect( communication_channel, (sockaddr*)&addr_un, sizeof(addr_un)) != 0 )
    {
        LOG_NOTHREAD(FTL, "%s", "Failed to open IPI communication pipe");
    }
    return true;
}

// Done
bool api_communicate::is_done(){
    return done;
}

// Enabed
bool api_communicate::is_enabled(){
    return enabled;
}

bool api_communicate::execute(const rt_host_kernel_launch_info_t& launch_info)
{
    fprintf(stderr,
            "api_communicate:: Going to execute kernel {0x%lx}\n"
            "  tensor_a = 0x%" PRIx64 "\n"
            "  tensor_b = 0x%" PRIx64 "\n"
            "  tensor_c = 0x%" PRIx64 "\n"
            "  tensor_d = 0x%" PRIx64 "\n"
            "  tensor_e = 0x%" PRIx64 "\n"
            "  tensor_f = 0x%" PRIx64 "\n"
            "  tensor_g = 0x%" PRIx64 "\n"
            "  tensor_h = 0x%" PRIx64 "\n"
            "  pc/id    = 0x%" PRIx64 "\n",
            launch_info.compute_pc,
            launch_info.params.tensor_a, launch_info.params.tensor_b, launch_info.params.tensor_c,
            launch_info.params.tensor_d, launch_info.params.tensor_e, launch_info.params.tensor_f,
            launch_info.params.tensor_g, launch_info.params.tensor_h, launch_info.params.kernel_id);

    // Write the kernel launch parameters to memory (FW_SCODE_KERNEL_INFO in fw_common.h)
    mem->write(RT_HOST_KERNEL_LAUNCH_INFO, sizeof(launch_info), &launch_info);

    LOG_NOTHREAD(DEBUG, "api_communicate: Execute: %s", "Sending IPI to Master Shire thread 0");

    // Send an IPI to the thread 0 of Master Shire
    sys_emu::raise_software_interrupt(32, 1);

    return true;
}

bool api_communicate::continue_exec(std::list<int> * enabled_threads)
{
    LOG_NOTHREAD(INFO, "%s", "sim_api_communicate: continue");
    assert(32 < (EMU_NUM_MINIONS / EMU_MINIONS_PER_SHIRE));

    // Write 0s tell we are in runtime mode (see fw_master_scode.cc)
    rt_host_kernel_launch_info_t launch_info;
    memset(&launch_info, 0, sizeof(launch_info));

    // Write the kernel launch parameters to memory (FW_SCODE_KERNEL_INFO in fw_common.h)
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
    return true;
}

// Execution
void api_communicate::get_next_cmd(std::list<int> * enabled_threads)
{
    // if we have received a shutdown do not check the socket for any
    // new commands.
    if (done)
    {
        return;
    }
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
                this->execute(launch_info);
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
                continue_exec(enabled_threads);
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
