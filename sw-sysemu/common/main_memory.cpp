// Global
#include <cassert>
#include <fstream>

// Local
#include "esrs.h"
#include "emu_gio.h"
#include "main_memory.h"
#include "main_memory_region_esr.h"
#include "main_memory_region_reserved.h"
#include "main_memory_region_scratchpad.h"

#define CACHE_LINE_MASK 0xFFFFFFFFC0ULL
#define CACHE_LINE_SIZE (64)

// Constructor
main_memory::main_memory()
{
    main_memory_region* p;

    // L2 Scratchpad
    p = new main_memory_region_scratchpad(SCP_REGION_BASE, SCP_REGION_SIZE);
    regions.push_front(region_pointer(p));

    // ESRs
    p = new main_memory_region_esr(this, ESR_REGION_BASE, ESR_REGION_SIZE);
    regions.push_back(region_pointer(p));

    // Reserved memory regions
    p = new main_memory_region_reserved(0x200000000, 248*1024*1024*1024ull);
    regions.push_back(region_pointer(p));
    // Limit Cacheable memory region to 32 GB (physical memory available)
    p = new main_memory_region_reserved(0x8800000000ull, 0x3800000000ull);
    regions.push_back(region_pointer(p));
    // Limit Non-cacheable memory region to 32 GB (physical memory available)
    p = new main_memory_region_reserved(0xc800000000ull, 0x3800000000ull);
    regions.push_back(region_pointer(p));
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

    bool drop = false;
    bool resize = false;

    uint64_t new_base = base;
    uint64_t new_top  = top;

    region_list_type deleted_regions;

    for (auto it = regions.begin(); it != regions.end(); )
    {
        /* LOG_NOTHREAD(DEBUG, "checking old_region(base=0x%" PRIx64 ", size=%zu, top=0x%" PRIx64 ") for overlaps", */
        /*              (*it)->base, (*it)->count, (*it)->base + (*it)->count - 1); */

        if (   ((new_base >= (*it)->base) && (new_base < ((*it)->base + (*it)->count)))
            && ((new_top  >= (*it)->base) && (new_top  < ((*it)->base + (*it)->count))))
        {
            drop = true;
            LOG_NOTHREAD(DEBUG, "dropping new_region(base=0x%" PRIx64 ", size=%zu, top=0x%" PRIx64 "):"
                              "fully included in old_region(base=0x%" PRIx64 ", size=%zu, top=0x%" PRIx64 ")",
                         base, top - base + 1, top, (*it)->base, (*it)->count, (*it)->base + (*it)->count - 1);
            break;
        }

        if (   (((*it)->base >= new_base) && ((*it)->base < top))
            && ((((*it)->base + (*it)->count) > new_base) && (((*it)->base + (*it)->count) < top)))
        {
            if (new_base == (*it)->base)
            {
                resize = true;
                new_base = (*it)->base + (*it)->count;
                LOG_NOTHREAD(DEBUG, "clipping new_region(base=0x%" PRIx64 ", size=%zu, top=0x%" PRIx64 "): by old region to new_base=0x%" PRIx64,
                             base, top - base + 1, top, new_base);
                it++;
            }
            else if (new_top == ((*it)->base + (*it)->count - 1))
            {
                resize = true;
                new_top = (*it)->base - 1;
                LOG_NOTHREAD(DEBUG, "clipping new_region(base=0x%" PRIx64 ", size=%zu, top=0x%" PRIx64 "): by old region to new_top=0x%" PRIx64,
                             base, top - base + 1, top, new_top);
                it++;
            }
            else
            {
                // Needs to copy data from old region to new region which doesn't exist yet!!!
                LOG_NOTHREAD(DEBUG, "removing old_region(base=0x%" PRIx64 ", size=%zu, top=0x%" PRIx64 "): fully included in new region",
                             (*it)->base, (*it)->count, (*it)->base + (*it)->count - 1);
                deleted_regions.push_back(*it);
                it = regions.erase(it);
            }
        }
        else
        {
            if ((new_base >= (*it)->base) && (new_base < ((*it)->base + (*it)->count)))
            {
                resize = true;
                new_base = (*it)->base + (*it)->count;
                LOG_NOTHREAD(DEBUG, "clipping new_region(base=0x%" PRIx64 ", size=%zu, top=0x%" PRIx64 "): by old region to new_base=0x%" PRIx64,
                             base, top - base + 1, top, new_base);
            }

            if ((new_top >= (*it)->base) && (new_top < ((*it)->base + (*it)->count)))
            {
                resize = true;
                new_top = (*it)->base - 1;
                LOG_NOTHREAD(DEBUG, "clipping new_region(base=0x%" PRIx64 ", size=%zu, top=0x%" PRIx64 "): by old region to new_top=0x%" PRIx64,
                             base, top - base + 1, top, new_top);
            }

            if (new_base >= new_top)
            {
                drop = true;
                LOG_NOTHREAD(DEBUG, "dropping new_region(base=0x%" PRIx64 ", size=%zu, top=0x%" PRIx64 "): fully included by other regions",
                    base, top - base + 1, top);
                break;
            }

            it++;
        }
    }

    if (drop && !deleted_regions.empty())
        LOG_NOTHREAD(FTL, "Shouldn't happen : new_region and old_regions (%zu) were dropped", deleted_regions.size());

    if (drop)
        return true;

    if (resize)
    {
        base = new_base;
        top  = new_top;
    }

    size = top - base + 1;

    LOG_NOTHREAD(DEBUG, "new_region(base=0x%" PRIx64 ", size=%zu, top=0x%" PRIx64 ")", base, size, top);
    main_memory_region* r = new main_memory_region(base, size);
    assert(r);
    regions.push_front(region_pointer(r));

    if (!deleted_regions.empty())
    {
        for (auto const &r: deleted_regions)
        {
            LOG_NOTHREAD(DEBUG, "copying data from old_region(base=0x%" PRIx64 ", size=%zu, top=0x%" PRIx64 ") to new_region",
                r->base, r->count, r->base + r->count - 1);
            write(r->base, r->count, r->buf);
        }

        deleted_regions.clear();
    }

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
