#include <et_ioctl.h>
#include <iostream>

int main() {
    std::cout << "ESPERANTO_PCIE_IOCTL_MAGIC: " << std::hex << "0x" << ESPERANTO_PCIE_IOCTL_MAGIC << std::endl;
    std::cout << "sizeof(fw_update_desc): " << sizeof(fw_update_desc) << std::endl;
    std::cout << "sizeof(cmd_desc):       " << sizeof(cmd_desc) << std::endl;
    std::cout << "sizeof(rsp_desc):       " << sizeof(rsp_desc) << std::endl;
    std::cout << "sizeof(sq_threshold):   " << sizeof(sq_threshold) << std::endl;
    std::cout << "sizeof(dram_info):      " << sizeof(dram_info) << std::endl;
    std::cout << "SUCCESS" << std::endl;
}