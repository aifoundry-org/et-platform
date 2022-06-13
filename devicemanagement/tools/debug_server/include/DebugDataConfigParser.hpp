#ifndef DEBUG_DATA_PARSER__H
#define DEBUG_DATA_PARSER__H

#include <cstdint>
#include <string>

using namespace std;

constexpr uint16_t defaultAccessInitiatorSP = 0xFFFF; /* TODO: Define Valid Identifier */
constexpr uint16_t cmHartID_0 = 0;
constexpr uint16_t cmHartID_2047 = 2047;
constexpr uint16_t mmHartID_2048 = 2048;
constexpr uint16_t mmHartID_2079 = 2079;
constexpr uint16_t cmHartID_2080 = 2080;
constexpr uint16_t cmHartID_2111 = 2111;

struct DebugMemDataConfig {
public:
  DebugMemDataConfig(uint64_t startAddress, uint64_t size, string accessInitiator, string accessType) {
    StartAddress = startAddress;
    ReadSize = size;
    AccessInitiator = accessInitiator;
    AccessType = accessType;
  }

  uint64_t StartAddress;
  uint64_t ReadSize;
  string AccessInitiator;
  string AccessType;
};

void processConfigFile(string filepath);
bool getAttributesForAddr(uint64_t addr, uint64_t* accessInitiator, uint8_t* accessType);

#endif