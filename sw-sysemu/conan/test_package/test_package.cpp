#include "sw-sysemu/ISysEmu.h"

#include <iostream>

int main() {
    emu::SysEmuOptions options;
    std::cout << "logFile: " << options.logFile << std::endl;
    std::cout << "SUCCESS" << std::endl;
}