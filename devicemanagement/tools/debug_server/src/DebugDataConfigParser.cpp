//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------
#include <DebugDataConfigParser.hpp>
#include <deviceManagement/DeviceManagement.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using namespace device_mgmt_api;

vector<DebugMemDataConfig> debugMemDataCfg;

// Function to get the attributes for memory address from the mapped data struct
static int getAttributes(uint64_t addr, string& accessInitiator, string& accessType) {
  int validAttributes = -1;
  // Loop through the addresses mapped
  for (auto item : debugMemDataCfg) {

    if ((addr >= item.StartAddress) && (addr < (item.StartAddress + item.ReadSize))) {
      accessInitiator = item.AccessInitiator;
      accessType = item.AccessType;
      validAttributes = 0;
      break;
    }
  }
  return validAttributes;
}

// Function  to get the attributes for memory address from the mapped data struct
static uint8_t translateAccessTypeStrtoInt(string& accessType) {
  uint8_t accType;

  if (accessType.compare("GLOBAL_ATOMIC") == 0) {
    accType = MEM_ACCESS_TYPE_GLOBAL_ATOMIC;
  } else if (accessType.compare("LOCAL_ATOMIC") == 0) {
    accType = MEM_ACCESS_TYPE_LOCAL_ATOMIC;
  } else {
    accType = MEM_ACCESS_TYPE_NORMAL;
  }

  return accType;
}

// Function to return attributes for a memory address
void getAttributesForAddr(uint64_t addr, uint64_t* accessInitiator, uint8_t* accessType) {
  int validAttributes = -1;
  // By default access type is normal and access initiator will be SP
  *accessType = MEM_ACCESS_TYPE_NORMAL;
  *accessInitiator = defaultAccessInitiatorSP;
  string tmpAccessType;
  string tmpAccessInitiator;

  // Get the accessor/access type attribute for memory address
  validAttributes = getAttributes(addr, tmpAccessInitiator, tmpAccessType);

  // Accesor is either MM or WM
  if ((validAttributes == 0) && (tmpAccessInitiator.compare("SP") != 0)) {
    // Access Type will be MEM_ACCESS_TYPE_GLOBAL_ATOMIC or MEM_ACCESS_TYPE_LOCAL_ATOMIC or MEM_ACCESS_TYPE_NORMAL
    *accessType = translateAccessTypeStrtoInt(tmpAccessType);

    // Access Initiator is CM or MM Hart ID
    *accessInitiator = atoi(tmpAccessInitiator.c_str());
  }
}

// Parse the file from the path provided and store it in
// global data struct.
void processConfigFile(string filepath) {

  ifstream inputFile;
  inputFile.open(filepath);
  string line = "";
  uint64_t startAddress;
  uint64_t size;
  string accessInitiator;
  string accessType;

  // Skip the header line in the input file
  getline(inputFile, line);
  line = "";

  // Parse the attributes specified
  while (getline(inputFile, line)) {

    stringstream inputString(line);
    string tempString = "";

    getline(inputString, tempString, ',');
    startAddress = strtoull(tempString.c_str(), 0, 16);

    tempString = "";
    getline(inputString, tempString, ',');
    size = strtoull(tempString.c_str(), 0, 16);

    getline(inputString, accessInitiator, ',');
    getline(inputString, accessType, ',');

    DebugMemDataConfig config(startAddress, size, accessInitiator, accessType);

    debugMemDataCfg.push_back(config);
    line = "";
  }
}