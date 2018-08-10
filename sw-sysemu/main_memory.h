#ifndef _MAIN_MEMORY_H_
#define _MAIN_MEMORY_H_

// Global
#include <fstream>
#include <boost/ptr_container/ptr_vector.hpp>
#include <inttypes.h>

// Local
#include "testLog.h"
#include "main_memory_region.h"
#include "main_memory_region_rbox.h"

class main_memory
{
    public:
        // Constructors and destructors
        main_memory(std::string logname);
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

        // dump memory contents into file
        bool dump_file(std::string filename, uint64_t ad, uint64_t size, unsigned buf_size = 256);

        // sets function to retrieve current thread from emu (in case it is required to known which minion accesses the mem)
        void setGetThread(main_memory_region::func_ptr_get_thread f) { getthread = f; }
        
        // function to configure printf from RTL
        void setPrintfBase(const char* binary);

        // rbox credits interface for sysemu
        void decRboxCredit(uint16_t thread);
        void incRboxCredit(uint16_t thread);
        uint16_t getRboxCredit(uint16_t thread);

        // allow memory regions to be dynamically created
        void create_mem_at_runtime();
    private:
        void dump_regions();

        boost::ptr_vector<main_memory_region> regions_;
        typedef boost::ptr_vector<main_memory_region>::iterator rg_it_t;

        testLog log;
        main_memory_region::func_ptr_get_thread getthread;

        main_memory_region_rbox *rbox;

        bool runtime_mem_regions = false;
};

#endif // _MAIN_MEMORY_H_

