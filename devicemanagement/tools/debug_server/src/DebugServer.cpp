//******************************************************************************
// Copyright (C) 2021, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include <DebugDataConfigParser.hpp>
#include <GdbServer.hpp>
#include <chrono>
#include <dlfcn.h>
#include <getopt.h>
#include <glog/logging.h>
#include <iostream>
#include <string>
#include <thread>
#include <utils.h>

using namespace dev;
using namespace device_mgmt_api;
using namespace device_management;
using namespace std::chrono_literals;
using Clock = std::chrono::system_clock;
using Timepoint = Clock::time_point;
using TimeDuration = Clock::duration;

static struct option long_options[] = {{"help", no_argument, 0, 'h'},
                                       {"node", required_argument, 0, 'n'},
                                       {"port", required_argument, 0, 'p'},
                                       {"shire_id", required_argument, 0, 's'},
                                       {"thread_mask", required_argument, 0, 'm'},
                                       {"bp_timeout", required_argument, 0, 't'},
                                       {"filepath", required_argument, 0, 'x'},
                                       {0, 0, 0, 0}};

void printUsage(char* argv) {
  std::cout << std::endl;
  std::cout << "Debug Server Usage: " << argv << " [-p port] [-n device_index] [-h]" << std::endl;
  std::cout << "Optional input arguments:" << std::endl;
  std::cout << "-p specify the TCP port on which the debug server is to be started, default:51000" << std::endl;
  std::cout << "-n ETSoC1 device index on which the debug server is to be started, device_index:0" << std::endl;
  std::cout << "-s Shire ID" << std::endl;
  std::cout << "-m Thread mask" << std::endl;
  std::cout << "-t BP timeout" << std::endl;
  std::cout << "-x Metadata file path consisting of details of FW data to be dumped" << std::endl;
  std::cout << "-h Help, usage instructions" << std::endl;
}

int main(int argc, char** argv) {
  int c;
  int option_index = 0;
  uint8_t device_idx = defaultDeviceIdx;    // Default device index 0
  uint32_t port = defaultRSPConnPort;       // Default port 51000
  uint8_t shire_id = defaultShireID;        // Shire ID 0
  uint64_t thread_mask = defaultThreadMask; // Default thread mask 0x0000000000000001
  uint64_t bp_timeout = defaultBPTimeout;   // Default timeout for bp to be hit
  bool exit = false;
  char* filePath;

  // Initialize Google's logging library
  // logging::LoggerDefault loggerDefault_;

  while (1) {
    c = getopt(argc, argv, "h:n:p:s:m:t:x:");

    /* Detect the end of the options. */
    if (c == -1)
      break;

    switch (c) {
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

    case 'm':
      thread_mask = strtoul(optarg, NULL, 0);
      break;

    case 't':
      bp_timeout = strtoul(optarg, NULL, 0);
      break;

    case 'x': {
      if (optarg == NULL) {
        std::cout << "Invalid filepath. Specify the required file with details for FW data dump to be dumped"
                  << std::endl;
      } else {
        filePath = optarg;
      }
      break;
    }

    default:
      break;
    }
  }

  if (!exit) {
    // If file path is valid
    if (filePath != NULL) {
      // Parse the file and store it in data struct
      processConfigFile(filePath);
    }

    // TODO : Error check for shire ID and thread_mask input parameters
    // Start the GDB Server that serves as proxy to GDB client
    MinionDebugInterface minionDebugInterface(device_idx, shire_id, thread_mask, bp_timeout);
    GdbServer gdbServer(/*Minion Debug Interface =*/&minionDebugInterface, /*tcp port=*/port);

    std::thread gdbThread(&GdbServer::serverThread, &gdbServer);
    DV_LOG(INFO) << "GDB Server, Started.." << std::endl;
    gdbThread.join();
    DV_LOG(INFO) << "Debug Server, Exiting .." << std::endl;
  }

  return 0;
}
