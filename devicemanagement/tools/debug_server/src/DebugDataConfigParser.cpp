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

static bool checkAddressExists(uint64_t addr, string& accessInitiator, string& accessType) {
  bool addrFound = false;
  // Loop through the mapped addresses
  for (auto item : debugMemDataCfg) {

    if ((addr >= item.StartAddress) && (addr < (item.StartAddress + item.ReadSize))) {
      accessInitiator = item.AccessInitiator;
      accessType = item.AccessType;
      addrFound = true;
      break;
    }
  }
  return addrFound;
}

// Function  to get the Validate Access Initiator
static bool validateAccessInitiator(uint64_t accessInitiator) {
  bool validAccessor = false;

  if ((accessInitiator >= cmHartID_0 && accessInitiator <= cmHartID_2047) ||
      (accessInitiator >= cmHartID_2080 && accessInitiator <= cmHartID_2111)) {
    validAccessor = true;
  } else if ((accessInitiator >= mmHartID_2048 && accessInitiator <= mmHartID_2079)) {
    validAccessor = true;
  }

  return validAccessor;
}

// Function to Validate Access Type
static bool validateAccessType(string& accessType) {
  bool validAccessType = false;

  if (accessType.compare("GLOBAL_ATOMIC") == 0) {
    validAccessType = true;
  } else if (accessType.compare("LOCAL_ATOMIC") == 0) {
    validAccessType = true;
  } else if (accessType.compare("NORMAL") == 0) {
    validAccessType = true;
  }

  return validAccessType;
}

// Function to Translate Access Type from string to integer type.
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

// Function to validate and return attributes for a memory address
bool getAttributesForAddr(uint64_t addr, uint64_t* accessInitiator, uint8_t* accessType) {
  bool validAttr = true;
  bool addrFound;
  // By default access type is normal and access initiator will be SP
  *accessType = MEM_ACCESS_TYPE_NORMAL;
  *accessInitiator = defaultAccessInitiatorSP;
  string tmpAccessType;
  string tmpAccessInitiator;

  // Check if the address request is in the mapped address and get attributes for memory address
  addrFound = checkAddressExists(addr, tmpAccessInitiator, tmpAccessType);

  if (addrFound) {
    // Accesor is either MM or WM
    if (tmpAccessInitiator.compare("SP") != 0) {

      // Access Type will be MEM_ACCESS_TYPE_GLOBAL_ATOMIC or MEM_ACCESS_TYPE_LOCAL_ATOMIC or MEM_ACCESS_TYPE_NORMAL
      if (validateAccessType(tmpAccessType)) {
        *accessType = translateAccessTypeStrtoInt(tmpAccessType);
      } else {
        // Invalid attributes
        validAttr = false;
      }

      // Access Initiator is CM or MM Hart ID
      if (validateAccessInitiator(atoi(tmpAccessInitiator.c_str()))) {
        *accessInitiator = atoi(tmpAccessInitiator.c_str());
      } else {
        // Invalid attributes
        validAttr = false;
      }
    }
  }

  return validAttr;
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