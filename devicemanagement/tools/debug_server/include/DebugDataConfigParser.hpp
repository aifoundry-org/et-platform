#ifndef DEBUG_DATA_PARSER__H
#define DEBUG_DATA_PARSER__H

#include <cstdint>
#include <string>

using namespace std;

constexpr uint16_t defaultAccessInitiatorSP = 0xFFFF; /* TODO: Define Valid Identifier */

struct DebugMemDataConfig {
public:
  DebugMemDataConfig(uint64_t startAddress, uint64_t endAddress, string accessInitiator, string accessType) {
    StartAddress = startAddress;
    ReadSize = endAddress;
    AccessInitiator = accessInitiator;
    AccessType = accessType;
  }

  uint64_t StartAddress;
  uint64_t ReadSize;
  string AccessInitiator;
  string AccessType;
};

void processConfigFile(string filepath);
void getAttributesForAddr(uint64_t start_addr, uint64_t* accessInitiator, uint8_t* accessType);

#endif