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
       main_memory(testLog& log_);
       ~main_memory();

       // Read and write
       void read (uint64_t ad, int size, void * data);
       void write(uint64_t ad, int size, const void * data);
       char read8(uint64_t ad);
       void write8(uint64_t ad, char d);

       // Creates a new region
       bool new_region(uint64_t base, uint64_t size, int flags = MEM_REGION_RW);

       // load file contents into memory
       bool load_file(std::string filename, uint64_t ad, unsigned buf_size = 256);
       bool load_elf(std::string filename);

       // dump memory contents into file
       bool dump_file(std::string filename, uint64_t ad, uint64_t size, unsigned buf_size = 256);

       // dump all memory contents into file
       bool dump_file(std::string filename);

       // sets function to retrieve current thread from emu (in case it is required to known which minion accesses the mem)
       void setGetThread(main_memory_region::func_ptr_get_thread f) { getthread = f; }

       // function to configure printf from RTL
       void setPrintfBase(const char* binary);

       // allow memory regions to be dynamically created
       void create_mem_at_runtime();

       region_pointer find_region_containing(uint64_t ad) {
           region_iterator it = find(ad);
           return (it == regions_.end()) ? region_pointer(nullptr) : *it;
       }

    private:
       typedef std::list<region_pointer>    region_list_type;
       typedef region_list_type::iterator   region_iterator;

       region_list_type regions_;

       testLog& log;
       main_memory_region::func_ptr_get_thread getthread;

       bool runtime_mem_regions = false;

    private:
       void dump_regions();

       region_iterator find(uint64_t ad) {
           region_iterator ret = regions_.begin();
           while(ret != regions_.end())
           {
               if((* (* ret)) == ad) { return ret; }
               ret++;
           }
           return ret;
       }
};

#endif // _MAIN_MEMORY_H_

