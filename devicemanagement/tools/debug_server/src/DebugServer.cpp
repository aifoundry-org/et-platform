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
                                       {"node", required_argument, 0, 'n'},
                                       {"port", required_argument, 0, 'p'},
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

  // Initialize Google's logging library.
  logging::LoggerDefault loggerDefault_;

  while (1)
  {

    /* TODO: To be implemented, add support for shire mask and thread mask input args.*/ 
    c = getopt_long(argc, argv, "n:p:h", long_options, &option_index);

    switch (c)
    {
        case 'h':
        printUsage(argv[0]);
        break;

        case 'n':
        /* TODO: Implement support for starting the debug server on specified ETSoC1 device index. */
        DV_LOG(INFO) << "n option provided " << std::endl;
        break;

        case 'p':
        /* TODO: Implement support for starting the debug server on specified TCP port. */
        DV_LOG(INFO) << "p option provided" << std::endl;
        break;

        case 's':
        /* TODO: Implement support for shire id. */
        DV_LOG(INFO) << "s option provided" << std::endl;
        break;

        case 't':
        /* TODO: Implement support for thread mask id. */
        DV_LOG(INFO) << "t option provided" << std::endl;
        break;

        default:
        break;
    }

    // Send DM Start Debug Server command to device

    // Start the GDB Server that serves as proxy to GDB client
    // TODO: To be implemented.Process the input argument for shire mask and thread mask.
    MinionDebugInterface minionDebugInterface(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);
    GdbServer gdbServer(/*Minion Debug Interface =*/&minionDebugInterface, /*tcp port=*/51000);

    std::thread gdbThread(&GdbServer::serverThread, &gdbServer);

    DV_LOG(INFO) << "GDB thread, started.." << std::endl;

    gdbThread.join();

    DV_LOG(INFO) << "GDB thread, exiting .." << std::endl;
  }

  return 0;
}
