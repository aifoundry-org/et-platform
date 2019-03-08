#ifndef ETTEE_REGISTRY_H
#define ETTEE_REGISTRY_H

#include <stddef.h>
#include <memory>

// Registry internal interface.

struct EtKernelInfo {
    const void *elf_p = nullptr;    // Esperanto registered ELF, if present.
    size_t elf_size = 0;            // Size of registered ELF, if it present.
    size_t offset = 0;              // Kernel entry point offset inside the ELF, if it present.
    const std::string &name;        // Kernel device name, exists for any valid hostFun.

    EtKernelInfo(const std::string &name) : name(name) {}
};

EtKernelInfo etrtGetKernelInfoByHostFun(const void *hostFun);


// Utility functions.

void parse_elf(const void *elf_p,
               size_t *elf_size_p,
               std::unordered_map<std::string, size_t> *kernel_offset_p,
               std::unordered_map<std::string, size_t> *raw_kernel_offset_p);


#endif  // ETTEE_REGISTRY_H

