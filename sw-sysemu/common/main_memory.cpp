#include "main_memory.h"
#include "main_memory_region_atomic.h"
#include "main_memory_region_tbox.h"
#include "main_memory_region_rbox.h"
#include "main_memory_region_printf.h"

using namespace std;

// Constructor
main_memory::main_memory(std::string logname)
    : log(logname, LOG_DEBUG)
{
    getthread = NULL;
    // Adds the tbox
    main_memory_region_tbox * tbox = new main_memory_region_tbox(0xFFF80000ULL, 512, log, getthread);
    regions_.push_back((main_memory_region *) tbox);
    // RBOX
    rbox = new main_memory_region_rbox(0xFFF40000ULL, 8, log, getthread);
    regions_.push_back((main_memory_region *) rbox);
    // UC writes to notify completion of kernels to master processor
    //main_memory_region * uc_writes = new main_memory_region(0x0108000000ULL, ESR_REGION_OFFSET, log, getthread, MEM_REGION_WO);
    main_memory_region * uc_writes = new main_memory_region(0x0108000800ULL, ESR_REGION_OFFSET, log, getthread, MEM_REGION_WO);
    regions_.push_back((main_memory_region *) uc_writes);
    // UC writes to the fast local barrier and mtime/mtimecmp ESRs
    for (int i = 0; i < (EMU_NUM_MINIONS/EMU_MINIONS_PER_SHIRE); i++)
    {
        for (int n = 0; n < 4; n++)
        {
            main_memory_region * neigh_esrs = new main_memory_region(0x100100000ULL + i*ESR_REGION_OFFSET + n*ESR_NEIGH_OFFSET, 65536, log, getthread);
            regions_.push_back((main_memory_region *) neigh_esrs);
        }
        main_memory_region * shire_esrs = new main_memory_region(0x100340000ULL + i*ESR_REGION_OFFSET, 131072, log, getthread);
        regions_.push_back((main_memory_region *) shire_esrs);
        // M prot for shire
        shire_esrs = new main_memory_region(0x100340000ULL + (3ULL << 30ULL) + i*ESR_REGION_OFFSET, 131072, log, getthread);
        regions_.push_back((main_memory_region *) shire_esrs);
        main_memory_region * mtm = new main_memory_region(0x1c03001d8ULL + i*ESR_REGION_OFFSET, 64, log, getthread);
        regions_.push_back((main_memory_region *) mtm);
    }
}

void main_memory::setPrintfBase(const char* binary)
{
  uint64_t symbolAddress;
  std::ostringstream command;
  command<<"nm "<<binary<<" 2>/dev/null |grep rtlPrintf_buf|cut -d' ' -f 1";
  FILE *p = popen(command.str().c_str(), "r");
  int c = fscanf(p, "%" PRIx64, &symbolAddress);
  pclose(p);
  
  if (c==1) {
    // Adds the printf region
    log<<LOG_DEBUG<<"adding printf region (@="<<hex<<symbolAddress<<") from "<<binary<<dec<<endm;
    main_memory_region_printf * printf = new main_memory_region_printf(symbolAddress, getthread);
    regions_.push_back((main_memory_region *) printf);
  }
  else {
    log<<LOG_DEBUG<<"no printf region from "<<binary<<endm;
  }
}

// Destructor
main_memory::~main_memory()
{
}

// Read a bunch of bytes
void main_memory::read(uint64_t ad, int size, void * data)
{
    log << LOG_DEBUG << "read(" << std::hex << ad << ", " << std::dec << size << ")" << endm;
    rg_it_t r = find(regions_.begin(), regions_.end(), ad);

    if(r == regions_.end())
    {
        if (runtime_mem_regions == true)
        {
           log << LOG_INFO << "Creating Mem Region at addr " << std::hex << ad << endm;
            new_region(ad & 0xFFFFFFFFC0ULL, 64);
            if (((ad + size) & 0xFFFFFFFFC0ULL) != (ad & 0xFFFFFFFFC0ULL))
            {
               new_region((ad + size) & 0xFFFFFFFFC0ULL, 64);
            }
            r = find(regions_.begin(), regions_.end(), ad);
        }
        else
        {
           log << LOG_ERR << "read(" << std::hex << ad << ", " << std::dec << size << "): ad not in region" << endm;
           dump_regions();
        }
    }

    if (r != regions_.end())
    {
        if(* r != (ad + size - 1))
        {
            log << LOG_ERR << "read(" << std::hex << ad << ", " << std::dec << size << "): crosses section boundaries" << endm;
            dump_regions();
        }
        else
            r->read(ad, size, data);
    }
}

// Writes a bunch of bytes
void main_memory::write(uint64_t ad, int size, const void * data)
{
    log << LOG_DEBUG << "write(" << std::hex << ad << ", " << std::dec << size << ")" << endm;
    rg_it_t r = find(regions_.begin(), regions_.end(), ad);

    if(r == regions_.end())
    {
        if (runtime_mem_regions == true)
        {
           log << LOG_INFO << "Creating Mem Region at addr " << std::hex << ad << endm;
            new_region(ad & 0xFFFFFFFFC0ULL, 64);
            if (((ad + size) & 0xFFFFFFFFC0ULL) != (ad & 0xFFFFFFFFC0ULL))
            {
               new_region((ad + size) & 0xFFFFFFFFC0ULL, 64);
            }
            r = find(regions_.begin(), regions_.end(), ad);
        }
        else
        {
            log << LOG_ERR << "write(" << std::hex << ad << ", " << std::dec << size << "): ad not in region" << endm;
            dump_regions();
        }
    }

    if (r != regions_.end())
    {
        if(* r != (ad + size - 1))
        {
            log << LOG_ERR << "write(" << std::hex << ad << ", " << std::dec << size << "): crosses section boundaries" << endm;
            dump_regions();
        }
        else
            r->write(ad, size, data);
    }
}

// Reads a byte
char main_memory::read8(uint64_t ad)
{
    char d;
    read(ad, 1, &d);
    return d;
}

// Writes a byte
void main_memory::write8(uint64_t ad, char d)
{
    write(ad, 1, &d);
}

// Creates a new region
bool main_memory::new_region(uint64_t base, uint64_t size, int flags)
{
    unsigned overlap = std::count(regions_.begin(), regions_.end(), base)
                     + std::count(regions_.begin(), regions_.end(), base + size - 1);

    if(overlap > 0)
    {
        log << LOG_ERR << "newRegion(" << std::hex << base << ", " << std::dec << size << "): overlaps with existing region and won't be created" << endm;
        return false;
    }
    else
    {
      regions_.push_back(new main_memory_region(base, size, log, getthread, flags));
        return true;
    }
}

// Loads a file to memory
bool main_memory::load_file(std::string filename, uint64_t ad, unsigned buf_size)
{
    std::ifstream f(filename.c_str(), std::ios_base::binary);

    if(!f.is_open())
    {
        log << LOG_ERR << "cannot open " << filename << endm;
        return false;
    }

    char * buf = new char[buf_size];
    unsigned size;
    do
    {
        f.read(buf, buf_size);
        size = f.gcount();
        if(size > 0)
            write(ad, size, buf);
        ad += size;
    } while(size > 0);

    delete [] buf;
    return  true;
}

// Dumps memory contents into a file
bool main_memory::dump_file(std::string filename, uint64_t ad, uint64_t size, unsigned buf_size)
{
    std::ofstream f(filename.c_str(), std::ios_base::binary);

    if(!f.is_open())
    {
        log << LOG_ERR << "cannot open " << filename << endm;
        return false;
    }

    char * buf = new char[buf_size];
    while(size > 0 && f.good())
    {
        uint size_ = size > buf_size ? buf_size : size;
        read(ad, size_, buf);
        f.write(buf, size_);
        ad += size_;
        size -= size_;
    }
    delete [] buf;

    if(f.good())
        return true;
    else
    {
        log << LOG_ERR << "error when writing into " << filename << endm;
        return false;
    }
}

// Dumps all the regions
void main_memory::dump_regions()
{
    log << LOG_DEBUG << "dumping regions:" << endm;
    for(auto &r:regions_)
        r.dump();
}


void main_memory::create_mem_at_runtime()
{
   runtime_mem_regions = true;
}


 
 void main_memory::decRboxCredit(uint16_t thread) {
   rbox->decCredit(thread);
 } 
 void main_memory::incRboxCredit(uint16_t thread) {
   rbox->incCredit(thread);
 }

 uint16_t main_memory::getRboxCredit(uint16_t thread) { 
   return rbox->getCredit(thread);
 }
