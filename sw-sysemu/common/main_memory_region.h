/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_MAIN_MEMORY_REGION
#define BEMU_MAIN_MEMORY_REGION

#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <fstream>
#include <stdexcept>

//namespace bemu {


// Memory region of the main memory
struct main_memory_region
{
    main_memory_region(uint64_t addr, size_t sz, bool alloc = true)
            : buf(alloc ? new char[sz]() : nullptr), base(addr), count(sz)
    {
        if (sz % 4)
            throw std::invalid_argument("main_memory_region: "
                                        "size must be multiple of 4");
    }

    virtual ~main_memory_region()
    { delete[] buf; }

    // read and write
    virtual void write(uint64_t addr, size_t n, const void* source)
    { std::copy_n(reinterpret_cast<const char*>(source), n, buf + (addr-base)); }

    virtual void read(uint64_t addr, size_t n, void* result)
    { std::copy_n(buf + (addr-base), n, reinterpret_cast<char*>(result)); }

    // operators to compare, used for finding the region of a memory access
    bool operator==(uint64_t addr) const
    { return (addr >= base) && ((addr - base) < count); }

    bool operator!=(uint64_t addr) const
    { return !(*this == addr); }

    // dump region data to a file
    virtual void dump_file(std::ofstream* f) {
        static char str[32];
        for (size_t offset = 0; offset < count; offset += 4) {
            snprintf(str, 32, "%010" PRIX64 " %08" PRIX32,
                    base + offset, *reinterpret_cast<uint32_t*>(buf + offset));
            f->write(str, strlen(str));
        }
    }

    // for exposition only
    char* const    buf;
    const uint64_t base;
    const size_t   count;
};


//} // namespace bemu

#endif // BEMU_MAIN_MEMORY_REGION
