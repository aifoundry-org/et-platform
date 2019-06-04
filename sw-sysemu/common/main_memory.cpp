// Global
#include <cassert>
#include <fstream>

// Local
#include "esrs.h"
#include "emu_gio.h"
#include "main_memory.h"
#include "main_memory_region_esr.h"
#include "main_memory_region_io.h"
#include "main_memory_region_reserved.h"
#include "main_memory_region_scp.h"
#include "main_memory_region_scp_linear.h"

#define CACHE_LINE_MASK 0xFFFFFFFFC0ULL
#define CACHE_LINE_SIZE (64)

// Constructor
main_memory::main_memory()
{
    main_memory_region* p;

    // ESRs
    p = new main_memory_region_esr(this, ESR_REGION_BASE, ESR_REGION_SIZE);
    regions.push_back(region_pointer(p));

    // Reserved memory regions
    p = new main_memory_region_reserved(0x200000000, 248*1024*1024*1024ull);
    regions.push_back(region_pointer(p));

    // For all the shires and the local shire mask
    for (int i = 0; i <= EMU_NUM_SHIRES; i++)
    {
        // Need to add the ESR space for the Local Shire.
        int shire = (i == EMU_NUM_SHIRES) ? 255 : ((i == EMU_IO_SHIRE_SP) ? IO_SHIRE_ID : i);

        // NB: Here we assume that all banks have the same ESR value!
        p = new main_memory_region_scp(this, L2_SCP_BASE + (shire & 0x7F) * L2_SCP_OFFSET, L2_SCP_SIZE,
                                       (shire != 255) ? &shire_cache_esrs[shire] : nullptr, (shire != 255));
        regions.push_front(region_pointer(p));
    }

    p = new main_memory_region_scp_linear(this, L2_SCP_LINEAR_BASE, L2_SCP_LINEAR_SIZE);
    regions.push_front(region_pointer(p));

    // IO region
    main_memory_region* io = new main_memory_region_io(IO_R_PU_TIMER_BASE, IO_R_PU_TIMER_SIZE);
    regions.push_back(region_pointer(io));
}


// Read a bunch of bytes
void main_memory::read(uint64_t addr, size_t n, void* result)
{
    region_iterator r = find(addr);

    if (r == regions.end()) {
        if (runtime_mem_regions == true) {
            LOG_NOTHREAD(DEBUG, "read(0x%" PRIx64 ", %zu): addr not in region, creating on the fly", addr, n);
            new_region(addr, n);
            r = find(addr);
            if (r == regions.end()) {
                dump_regions();
                LOG_NOTHREAD(ERR, "read(0x%" PRIx64 ", %zu): addr not in region, something went wrong when trying to create region", addr, n);
            }
        } else {
            dump_regions();
            LOG_NOTHREAD(ERR, "read(0x%" PRIx64 ", %zu): addr not in region", addr, n);
        }
    }

    if (r != regions.end()) {
        if ((*(* r)) != (addr + n - 1)) {
            region_iterator next_region = find(addr+n-1);
            if ((next_region == regions.end()) && (runtime_mem_regions == true)) {
                LOG_NOTHREAD(DEBUG, "read(0x%" PRIx64 ", %zu): crosses section boundaries, creating next section on the fly", addr, n);
                new_region(addr+n-1, 1);
                next_region = find(addr+n-1);
                if (next_region == regions.end()) {
                    dump_regions();
                    LOG_NOTHREAD(ERR, "read(0x%" PRIx64 ", %zu): something went wrong when trying to create region next region", addr, n);
                }
            }
            if (next_region != regions.end()) {
                // Read part from next region and concatenate
                uint64_t next_cl = (addr + CACHE_LINE_SIZE) & CACHE_LINE_MASK;
                uint64_t next_size = addr + n - next_cl;
                (* r)->read(addr, n-next_size, result);
                (* next_region)->read(next_cl, next_size, (void*)(uint64_t(result)+n-next_size));
            } else {
                dump_regions();
                LOG_NOTHREAD(ERR, "read(0x%" PRIx64 ", %zu): crosses section boundaries and next region doesn't exist", addr, n);
            }
        } else {
            (* r)->read(addr, n, result);
        }
    }
}


// Writes a bunch of bytes
void main_memory::write(uint64_t addr, size_t n, const void* source)
{
    region_iterator r = find(addr);

    if (r == regions.end()) {
        if (runtime_mem_regions == true) {
            LOG_NOTHREAD(DEBUG, "write(0x%" PRIx64 ", %zu): addr not in region, creating on the fly", addr, n);
            new_region(addr, n);
            r = find(addr);
            if (r == regions.end()) {
                dump_regions();
                LOG_NOTHREAD(ERR, "write(0x%" PRIx64 ", %zu): addr not in region, something went wrong when trying to create region", addr, n);
            }
        } else {
            dump_regions();
            LOG_NOTHREAD(ERR, "write(0x%" PRIx64 ", %zu): addr not in region", addr, n);
        }
    }

    if (r != regions.end()) {
        if ((* (* r)) != (addr + n - 1)) {
            region_iterator next_region = find(addr+n-1);
            if ((next_region == regions.end()) && (runtime_mem_regions == true)) {
                LOG_NOTHREAD(DEBUG, "write(0x%" PRIx64 ", %zu): crosses section boundaries, creating on the fly", addr, n);
                new_region(addr+n-1, 1);
                next_region = find(addr+n-1);
                if (next_region == regions.end()) {
                    dump_regions();
                    LOG_NOTHREAD(ERR, "write(0x%" PRIx64 ", %zu): something went wrong when trying to create region next region", addr, n);
                }
            }
            if (next_region != regions.end()) {
                // Read part from next region and concatenate
                uint64_t next_cl = (addr + CACHE_LINE_SIZE) & CACHE_LINE_MASK;
                uint64_t next_size = addr + n - next_cl;
                (* r)->write(addr, n-next_size, source);
                (* next_region)->write(next_cl, next_size, (void*)(uint64_t(source)+n-next_size));
            } else {
                dump_regions();
                LOG_NOTHREAD(ERR, "write(0x%" PRIx64 ", %zu): crosses section boundaries and next region doesn't exist", addr, n);
            }
        } else {
            (* r)->write(addr, n, source);
        }
    }
}


// Creates a new region
bool main_memory::new_region(uint64_t base, uint64_t size)
{
    uint64_t top;
    // Regions are always multiple of cache lines
    top  = ((base + size + CACHE_LINE_SIZE - 1) & CACHE_LINE_MASK) - 1;
    base = base & CACHE_LINE_MASK;

    while (find(base) != regions.end()) base += CACHE_LINE_SIZE;
    while (find(top)  != regions.end()) top  -= CACHE_LINE_SIZE;

    if (top <= base) return false;
    size = top - base + 1;

    unsigned overlap = (find(base) != regions.end()) + (find(top)  != regions.end());

    if (overlap > 0) {
        LOG_NOTHREAD(ERR, "new_region(base=0x%" PRIx64 ", size=%zu, top=0x%" PRIx64 "): overlaps with existing region and won't be created", base, size, top);
        return false;
    }

    LOG_NOTHREAD(DEBUG, "new_region(base=0x%" PRIx64 ", size=%zu, top=0x%" PRIx64 ")", base, size, top);
    main_memory_region* r = new main_memory_region(base, size);
    assert(r);
    regions.push_front(region_pointer(r));
    //dump_regions();
    return true;
}


bool main_memory::load_file(std::string filename, uint64_t addr, unsigned buf_size)
{
    std::ifstream f(filename.c_str(), std::ios_base::binary);

    if (!f.is_open()) {
        LOG_NOTHREAD(ERR, "cannot open %s", filename.c_str());
        return false;
    }

    char * buf = new char[buf_size];
    unsigned size;
    do {
        f.read(buf, buf_size);
        size = f.gcount();
        if (size > 0)
            write(addr, size, buf);
        addr += size;
    } while (size > 0);

    delete[] buf;
    return  true;
}


bool main_memory::load_elf(std::string filename)
{
    ELFIO::elfio reader;

    // Load ELF data
    if (!reader.load(filename)) {
        LOG_NOTHREAD(FTL, "cannot open ELF file %s", filename.c_str());
        return false;
    }

    ELFIO::Elf_Half seg_num = reader.segments.size();
    for (int i = 0; i < seg_num; ++i) {
        const ELFIO::segment* pseg = reader.segments[i];
        bool load = pseg->get_type() & PT_LOAD;
        LOG_NOTHREAD(INFO, "Segment[%d] VA: 0x%" PRIx64 "\tType: 0x%" PRIx32 "%s",
                     i, pseg->get_virtual_address(), pseg->get_type(),
                     (load ? " (LOAD)" : ""));

        for (int j = 0; j < pseg->get_sections_num(); ++j) {
            const ELFIO::section* psec = reader.sections[pseg->get_section_index_at(j)];
            bool alloc = psec->get_flags() & SHF_ALLOC;

            LOG_NOTHREAD(INFO, "Section[%d] %s\tVA: 0x%" PRIx64 "\tSize: 0x%" PRIx64
                         "\tType: 0x%" PRIx32 "\tFlags: 0x%" PRIx64 "%s",
                         j, psec->get_name().c_str(), psec->get_address(), psec->get_size(),
                         psec->get_type(), psec->get_flags(), (alloc ? " (ALLOC)" : ""));

            if (psec->get_size() != 0) {
                if (alloc == true) {
                    new_region(psec->get_address(), psec->get_size());
                }
                if ((load == true) && (psec->get_type() != SHT_NOBITS)) {
                    write(psec->get_address(), psec->get_size(), (void*)psec->get_data());
                }
            }
        }
    }
    return  true;
}


bool main_memory::dump_file(std::string filename, uint64_t addr, uint64_t size, unsigned buf_size)
{
    std::ofstream f(filename.c_str(), std::ios_base::binary);

    if (!f.is_open()) {
        LOG_NOTHREAD(ERR, "cannot open %s", filename.c_str());
        return false;
    }

    char * buf = new char[buf_size];
    while (size > 0 && f.good()) {
        uint size_ = size > buf_size ? buf_size : size;
        read(addr, size_, buf);
        f.write(buf, size_);
        addr += size_;
        size -= size_;
    }
    delete [] buf;

    if (f.good()) {
        return true;
    } else {
        LOG_NOTHREAD(ERR, "error when writing into %s", filename.c_str());
        return false;
    }
}


bool main_memory::dump_file(std::string filename)
{
    std::ofstream f(filename.c_str(), std::ios_base::binary);

    if (!f.is_open()) {
        LOG_NOTHREAD(ERR, "cannot open %s", filename.c_str());
        return false;
    }

    for(auto &r:regions)
        r->dump_file(&f);

    if (f.good()) {
        return true;
    } else {
        LOG_NOTHREAD(ERR, "error when writing into %s", filename.c_str());
        return false;
    }
}


void main_memory::dump_regions() const
{
    LOG_NOTHREAD(DEBUG, "%s", "dumping regions:");
    for (auto const& r: regions)
        LOG_NOTHREAD(DEBUG, "\tBase: 0x%" PRIx64 ", Size: 0x%zx", r->base, r->count);
}
