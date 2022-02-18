//******************************************************************************
// Copyright (C) 2021, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include <iostream>
#include <utils.h>
#include <dlfcn.h>
#include <getopt.h>
#include <chrono>
#include <thread>
#include <glog/logging.h>

#include <GdbServer.hpp>

using namespace dev;
using namespace device_mgmt_api;
using namespace device_management;
using namespace std::chrono_literals;
using Clock = std::chrono::system_clock;
using Timepoint = Clock::time_point;
using TimeDuration = Clock::duration;

static struct option long_options[] = {
                                    {"help", no_argument, 0, 'h'},
                                    {"node", required_argument, 0, 'n'},
                                    {"port", required_argument, 0, 'p'},
                                    {"shire_id", required_argument, 0, 's'},
                                    {"thread_mask", required_argument, 0, 't'},
                                    {0, 0, 0, 0}
                                    };


void printUsage(char* argv)
{
    std::cout << std::endl;
    std::cout << "Debug Server Usage: " << argv << " [-p port] [-n device_index] [-h]" << std::endl;
    std::cout << "Optional input arguments:" << std::endl;
    std::cout << "-p specify the TCP port on which the debug server is to be started, default:51000" << std::endl;
    std::cout << "-n ETSoC1 device index on which the debug server is to be started, device_index:0" << std::endl;
    std::cout << "-s Shire ID" << std::endl;
    std::cout << "-t Thread mask" << std::endl;
    std::cout << "-h Help, usage instructions" << std::endl;
}

int main(int argc, char** argv)
{
    int c;
    int option_index = 0;
    uint8_t device_idx = 0; // Default device index 0
    uint32_t port = 51000; // Default port 51000
    uint8_t shire_id = 0; // Shire ID 0
    uint64_t thread_mask = 0x0000000000000001; // Default thread mask 0x0000000000000001
    bool exit = false;

    //Initialize Google's logging library
    logging::LoggerDefault loggerDefault_;

    while (1)
    {
        c = getopt_long (argc, argv, "h:p:n:s:t:", long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c)
        {
            case 'h':
            std::cout << "Help/Usage" << std::endl;
            printUsage(argv[0]);
            exit = true;
            break;

            case 'n':
            device_idx = atoi(optarg);
            break;

            case 'p':
            port = atoi(optarg);
            break;

            case 's':
            shire_id = atoi(optarg);
            break;

            case 't':
            thread_mask = strtoul(optarg, NULL, 0);
            break;

            default:
            break;
        }

    }

    if(!exit)
    {
        // Start the GDB Server that serves as proxy to GDB client
        MinionDebugInterface minionDebugInterface(shire_id, thread_mask);
        GdbServer gdbServer(/*Minion Debug Interface =*/&minionDebugInterface, /*tcp port=*/port);

        std::thread gdbThread(&GdbServer::serverThread, &gdbServer);
        DV_LOG(INFO) << "GDB Server, Started.." << std::endl;
        gdbThread.join();
        DV_LOG(INFO) << "Debug Server, Exiting .." << std::endl;
    }

    return 0;
}
