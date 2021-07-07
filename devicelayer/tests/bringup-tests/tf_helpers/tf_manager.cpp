// For Simulator management
#include "deviceLayer/IDeviceLayer.h"
#include "Autogen.h"
#include <experimental/filesystem>

// For threading and misc
#include <iostream>
#include <string.h>
#include <pthread.h>
#include <sys/ioctl.h>

// Socket interface to text executor
#include <sys/socket.h> // For socket functions
#include <netinet/in.h> // For sockaddr_in
#include <iostream> // For cout
#include <unistd.h> // For read

namespace fs = std::experimental::filesystem;
namespace {
  constexpr uint64_t kSysEmuMaxCycles = std::numeric_limits<uint64_t>::max();
  constexpr uint64_t kSysEmuMinionShiresMask = 0x1FFFFFFFFu;
  constexpr auto kTfMgrTid = 1;
  constexpr auto kTfSimTid = 2;
}

// Globals
struct tf_mgr_data {
    int thread_id;
    unsigned short port;
};

struct tf_sim_data {
    int thread_id;
    std::shared_ptr<dev::IDeviceLayer> devicLayerInst;
};

// This thread gets spawned to managed Simulator lib instance
void *tf_sim_thread(void *thread_data)
{
    emu::SysEmuOptions sysEmuOptions;
    auto sim_data = (struct tf_sim_data*)thread_data;

    std::cout << "Sim thread spawned ..: " << std::endl;

    // Launch Sysemu through IDevice Abstraction
    sysEmuOptions.bootromTrampolineToBL2ElfPath = BOOTROM_TRAMPOLINE_TO_BL2_ELF;
    sysEmuOptions.spBL2ElfPath = BL2_ELF_TF; // point to testframework build of SPFW
    sysEmuOptions.machineMinionElfPath = MACHINE_MINION_ELF;
    sysEmuOptions.masterMinionElfPath = MASTER_MINION_ELF_TF; // point to testframework build of MMFW
    sysEmuOptions.workerMinionElfPath = WORKER_MINION_ELF;
    sysEmuOptions.executablePath = std::string(SYSEMU_INSTALL_DIR) + "sys_emu";
    sysEmuOptions.runDir = fs::current_path();
    sysEmuOptions.maxCycles = kSysEmuMaxCycles;
    sysEmuOptions.minionShiresMask = kSysEmuMinionShiresMask;
    sysEmuOptions.puUart0Path = sysEmuOptions.runDir + "/tests/run/pu_uart0_tx.log";
    sysEmuOptions.puUart1Path = sysEmuOptions.runDir + "/tests/run/pu_uart1_tx.log";
    sysEmuOptions.spUart0Path = sysEmuOptions.runDir + "/tests/run/sp_uart0_tx.log";
    // Route SP UART0 TX and RX traffic to named unix fifos used by TF
    sysEmuOptions.spUart1FifoInPath = "tests/run/sp_uart1_rx";
    sysEmuOptions.spUart1FifoOutPath = "tests/run/sp_uart1_tx";

    sysEmuOptions.startGdb = false;

    sim_data->devicLayerInst = dev::IDeviceLayer::createSysEmuDeviceLayer(sysEmuOptions);

    std::cout << "Sim thread completed ..: " << std::endl;

    pthread_exit(nullptr);
}

// A server that enables TF to manage simulator lifecycle
// and other needed testing related functions
void *tf_mgmt_server_thread(void *thread_data)
{
    bool sim_started = false;
    int sockfd;
    int connection;
    std::array<char, 100> buffer;
    pthread_t tf_sim;
    int retval;

    struct tf_sim_data sim_data;
    auto mgr_data = (const struct tf_mgr_data*)thread_data;
    std::shared_ptr<dev::IDeviceLayer> devicLayerInst;

    // Create a socket (IPv4, TCP)
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cout << "Failed to create socket. errno: " << errno << std::endl;
    }

    int iMode=0;
    ioctl(sockfd, _IONBF, &iMode);

    // Listen to port 9999 on any address
    sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = INADDR_ANY;
    sockaddr.sin_port = htons(mgr_data->port);
    if (bind(sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
        std::cout << "Failed to bind to port 9999. errno: " << errno << std::endl;
    }

    // Start listening. Hold at most 10 connections in the queue
    if (listen(sockfd, 10) < 0) {
        std::cout << "Failed to listen on socket. errno: " << errno << std::endl;
    }

    std::cout << "TF Manager: Waiting on client!............." << std::endl;

    // Grab a connection from the queue
    auto addrlen = sizeof(sockaddr);
    connection = accept(sockfd, (struct sockaddr*)&sockaddr, (socklen_t*)&addrlen);
    if (connection < 0) {
        std::cout << "Failed to grab connection. errno: " << errno << std::endl;
    }
    std::cout << "Client connected !...." << std::endl;

    while(true)
    {

        std::cout << "Read socket..... " << std::endl;

        std::fill_n(buffer.begin(), 10, 0);

        ssize_t bytesRead=0;
        while(bytesRead != 1) {
            bytesRead = read(connection, buffer.begin(), 1);
            if(bytesRead < 0) {
                std::cout << "Error:Reading socket" << bytesRead << std::endl;
                exit(-1);
            }
        }

        std::cout << "Processing user command, size: " << bytesRead << std::endl;
        std::cout << "start_sim:" << strncmp(buffer.begin(), "1", bytesRead) << "size:" << bytesRead << std::endl;
        std::cout << "stop_sim:" << strncmp(buffer.begin(), "2", bytesRead) << "size:" << bytesRead << std::endl;

        if(0 == strncmp(buffer.begin(), "1", 1))
        {
            std::cout << "Start Simulator (start_sim).." << std::endl;

            sim_data.thread_id = kTfSimTid;
            if(!sim_started) {
                retval = pthread_create(&tf_sim, nullptr, tf_sim_thread, (void*) &sim_data);
            }
            if((!sim_started) && retval) {
                std::cout << "Error:Unable to spawn TF Simulator thread,"
                    << retval << std::endl;
                exit(-1);
            }
            pthread_join(tf_sim, nullptr);

            devicLayerInst = (std::shared_ptr<dev::IDeviceLayer>) sim_data.devicLayerInst;
            sim_started = true;
        }
        else if(0 == strncmp(buffer.begin(), "2", 1))
        {
            std::cout << "Stop Simulator (stop_sim).." << std::endl;

            if(sim_started) {
                std::cout << "Stop Simulator (stop_sim).." << std::endl;
                //Call lib-sysemu API to finalize sysemu instance
                sim_started = false;
            }

            (void)sim_started;
            break;
        }
    }

    close(connection);
    close(sockfd);

    pthread_exit(nullptr);
}

int main(int argc, char** argv) {

    pthread_t tf_mgmt_server;
    int retval;
    struct tf_mgr_data tf_mgr_data;

    if(argc < 2) {
        std::cout <<
            "Error: Expected usage: <launchSim> <port range 9000-9999>" << std::endl;
    }

    tf_mgr_data.thread_id = kTfMgrTid;
    tf_mgr_data.port = (unsigned short)atoi(argv[1]);

    retval = pthread_create(&tf_mgmt_server, nullptr, tf_mgmt_server_thread, (void*) &tf_mgr_data);
    if (retval) {
        std::cout << "Error:Unable to spawn TF Server thread," << retval << std::endl;
        exit(-1);
    }
    pthread_join(tf_mgmt_server, nullptr);

    pthread_exit(nullptr);
}

