#ifndef _MAIN_MEMORY_H_
#define _MAIN_MEMORY_H_

// Global
#include <cinttypes>
#include <list>
#include <memory>

// Local
#include "elfio/elfio.hpp"
#include "main_memory_region.h"

class main_memory
{
public:
    typedef std::shared_ptr<main_memory_region>  region_pointer;

public:
    // Constructors and destructors
    main_memory();

    // Read and write
    void read(uint64_t addr, size_t n, void* result);
    void write(uint64_t addr, size_t n, const void* source);

    char read8(uint64_t addr) {
        char d;
        this->read(addr, 1, &d);
        return d;
    }

    void write8(uint64_t addr, char d) {
        this->write(addr, 1, &d);
    }

    // Creates a new region
    bool new_region(uint64_t base, size_t size);

    // load file contents into memory
    bool load_file(std::string filename, uint64_t addr, unsigned buf_size = 256);
    bool load_elf(std::string filename);

    // dump memory contents into file
    bool dump_file(std::string filename, uint64_t addr, size_t size, unsigned buf_size = 256);

    // dump all memory contents into file
    bool dump_file(std::string filename);

    // allow memory regions to be dynamically created
    void create_mem_at_runtime() {
        runtime_mem_regions = true;
    }

    region_pointer find_region_containing(uint64_t addr) {
        region_iterator it = find(addr);
        return (it == regions.end()) ? region_pointer(nullptr) : *it;
    }

private:
    typedef std::list<region_pointer>    region_list_type;
    typedef region_list_type::iterator   region_iterator;

    region_list_type regions;
    bool             runtime_mem_regions = false;

private:
    void dump_regions() const;

    region_iterator find(uint64_t addr) {
        return std::find_if(regions.begin(), regions.end(), [=](const region_pointer& p) {
            return *p == addr;
        });
    }

    /*region_iterator find(uint64_t addr) {
        region_iterator ret = regions.begin();
        while(ret != regions.end())
        {
            if((* (* ret)) == addr) { return ret; }
            ret++;
        }
        return ret;
    }*/
};

#endif // _MAIN_MEMORY_H_

