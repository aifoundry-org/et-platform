#include "mem_directory.h"
#include "emu_defines.h"
#include "emu_gio.h"

/*! \brief Directory update for a memory block.
 *
 *  This function updates the memory line in the directory and then predicts if there may have been incoherency
 *  return true if coherent, false in cc
 */

bool mem_directory::update(uint64_t address, op_location_t location, uint32_t shire_id, uint32_t minion_id, cacheop_type cop)
{

    mem_info_t new_entry;
    new_entry.level = location;    

    bool coherent = false;
    directory_map_t::iterator it;
    it = m_directory_map.find(address);    // does the entry already exists?

    printf("mem_directory::update => addr %016llx, location %i, shire id %i, minion_id %i, ptr %i\n", (long long unsigned int) address, location, shire_id, minion_id, it == m_directory_map.end());

    // Writing to minion L1
    if(location == COH_MINION)  // minion_id of shire_id has the most updated value
    {
        new_entry.shire_mask    = 1ULL<<shire_id;
        new_entry.minion_mask   = 1<<minion_id;
        coherent                = (it == m_directory_map.end()) || ((it->second.minion_mask & (1<<minion_id)) && (it->second.shire_mask & (1ULL<<shire_id)));
    }
    // Writing to shire L2
    else if(location == COH_SHIRE) // shire_id has the most updated value in shire cache
    {
        new_entry.shire_mask    = 1ULL<<shire_id;
        new_entry.minion_mask   = 0xFFFFFFFF;

        // Global, ok to update
        if(it == m_directory_map.end())
        {
            coherent = true;
        }
        // For cacheops local check they have the contents
        else if(cop == CacheOp_EvictL2)
        {
            // Data was in L1, minion needs to have latest contents
            if(it->second.level == COH_MINION)
            {
                coherent = (it->second.minion_mask & (1<<minion_id)) && (it->second.shire_mask & (1ULL<<shire_id));
            }
            // Data was not in L1, is like a nop
            else
            {
                coherent = true;
            }
        }
        // Regular ops
        else
        {
            coherent = (it->second.level == COH_SHIRE) && (it->second.shire_mask & (1ULL<<shire_id));
        }
    }
    // Writing to coallescing buffer
    else if(location == COH_CB) // shire_id has the most updated value in shire cache
    {
        new_entry.shire_mask    = 1ULL<<shire_id;
        new_entry.minion_mask   = 0x0;

        // Global, ok to update
        if(it == m_directory_map.end())
        {
            coherent = true;
        }
        // Otherwise must be in same coallescing buffer
        else
        {
            printf("Checking coherent for COH_CB %016llx\n", (long long unsigned int) it->second.shire_mask);
            coherent = (it->second.level == COH_CB) && (it->second.shire_mask & (1ULL<<shire_id));
        }
    }
    else
    {
        // Already global
        if(it == m_directory_map.end())
        {
            coherent = true;
        }
        // For cacheops to global check they have the contents
        else if((cop == CacheOp_EvictL3) || (cop == CacheOp_EvictDDR))
        {
            // Data was in L1, minion needs to have latest contents
            if(it->second.level == COH_MINION)
            {
                coherent = (it->second.minion_mask & (1<<minion_id)) && (it->second.shire_mask & (1ULL<<shire_id));
            }
            // Data was in L2, shire needs to have latest contents
            else
            {
                coherent = (it->second.level == COH_SHIRE) && (it->second.shire_mask & (1ULL<<shire_id));
            }
        }
    }

    printf("Coherent is %i\n", coherent);

    // Update directory
    if(it != m_directory_map.end()) // operator[] is the preferred alternative to update an entry, but I already have the pointer...
    {
        // If going to global, remove it from list to make it faster
        if(location == COH_GLOBAL)
        {
            m_directory_map.erase(it);
            printf("mem_directory::update delete => location %i, shire %016llX, minion %08X\n", new_entry.level, (long long unsigned int) new_entry.shire_mask, new_entry.minion_mask);
        }
        else
        {
            it->second = new_entry;
            printf("mem_directory::update update => location %i, shire %016llX, minion %08X\n", new_entry.level, (long long unsigned int) new_entry.shire_mask, new_entry.minion_mask);
        }
    }
    else if(location != COH_GLOBAL) // insert into the map saves three C++ calls w.r.t operator[]
    {
        m_directory_map.insert(directory_map_t::value_type(address, new_entry));
        printf("mem_directory::update insert => location %i, shire %016llX, minion %08X\n", new_entry.level, (long long unsigned int) new_entry.shire_mask, new_entry.minion_mask);
    }

    return coherent; 
}

 
/*! \brief Directory lookup for a memory block.
 *
 *  This function lookups the memory line in the directory and then predicts if there may have been incoherency
 *  return true if coherent, false in cc
 */
bool mem_directory::lookup(uint64_t address, op_location_t location, uint32_t shire_id, uint32_t minion_id)
{
    // does the entry already exists?
    directory_map_t::iterator it = m_directory_map.find(address);
    
    printf("mem_directory::lookup => addr %016llx, location %i, shire id %i, minion_id %i, ptr %i\n", (long long unsigned int) address, location, shire_id, minion_id, it == m_directory_map.end());

    if(it != m_directory_map.end())
    {
        printf("mem_directory::lookup found => location %i, shire %016llX, minion %08X\n", it->second.level, (long long unsigned int) it->second.shire_mask, it->second.minion_mask);
        if(location == COH_MINION)
        {
            return (it->second.minion_mask & (1<<minion_id)) && (it->second.shire_mask & (1ULL<<shire_id));
        }
        else if(location == COH_SHIRE)
        {
            return (it->second.level != COH_MINION) && (it->second.shire_mask & (1ULL<<shire_id));
        }
        // In coallescing buffer, can't read from it
        return false;
    }

    // Not present
    return true;
}

bool mem_directory::access(uint64_t addr, mem_access_type macc, cacheop_type cop, uint32_t current_thread)
{

    op_location_t location = COH_MINION;
    unsigned char operation = 0;

    uint32_t shire_id   = current_thread / EMU_THREADS_PER_SHIRE;
    uint32_t minion_id  = (current_thread >> 1) & 0x0000001F;

    switch (macc)
    {
    case Mem_Access_Load:
        //location = COH_MINION;
        break;
    case Mem_Access_LoadL:
        location = COH_SHIRE;
        break;
    case Mem_Access_LoadG:
        location = COH_GLOBAL;
        break;
    case Mem_Access_TxLoad: // Normal op. Tensor load 0 reads from L2 SCP and stores into L1 SCP, tensor load 1 reads from L2 SCP into buffer in the VPU. Can we distinguish both cases in BEMU?
        location = COH_GLOBAL;
        break;
    case Mem_Access_Prefetch: // Prefetch cache-op. Like a load from L2 to L1. The lookup op will fail if the line has not been written by the minion
        //location = COH_MINION;  // TODO I need Minion, local, and global identifiers for Mem_Access_Prefetch. 
        break;
    case Mem_Access_Store:
        operation = 1;
        //location  = COH_MINION;
        break;
    case Mem_Access_StoreL:
        operation = 1;
        location  = COH_SHIRE;
        break;
    case Mem_Access_StoreG:
        operation = 1;
        location  = COH_GLOBAL;
        break;
    case Mem_Access_TxStore: // Normal op. Tensor store to L1
        operation = 1;
        location  = COH_CB;
        break;
    case Mem_Access_AtomicL:
        operation = 1;
        location  = COH_SHIRE;
        break;
    case Mem_Access_AtomicG:
        operation = 1;
        location  = COH_GLOBAL;
        break;        
    case Mem_Access_CacheOp:
        if((cop == CacheOp_EvictL2) || (cop == CacheOp_EvictL3) || (cop == CacheOp_EvictDDR))
            operation = 1;
        if((cop == CacheOp_None) || (cop == CacheOp_EvictL3) || (cop == CacheOp_EvictDDR))
            location = COH_GLOBAL;
        else if((cop == CacheOp_EvictL2))
            location = COH_SHIRE;
        break;        
    case Mem_Access_Fetch:   // Load instruction from memory. This must not be included in the directory. Do nothing
        return true;
        break;
    case Mem_Access_PTW:     // Page table walker access. Must not be invoked. Fail if so.
        throw std::invalid_argument("unexpected operation PTW");
        break;
    }

    bool coherent;
    if(operation)
    { 
        coherent = update(addr & ~0x3FULL, location, shire_id, minion_id, cop);

        if(!coherent)
        {
            //LOG(FTL, "\t(Coherency Write Hazard) addr=%llx, location=%d, shire_id=%u, minion_id=%u", (long long unsigned int) addr & ~0x3FULL, location, shire_id, minion_id);
            LOG(WARN, "\t(Coherency Write Hazard) addr=%llx, location=%d, shire_id=%u, minion_id=%u", (long long unsigned int) addr & ~0x3FULL, location, shire_id, minion_id);
            return false;
        }
    }
    else
    {   
        coherent = lookup(addr & ~0x3FULL, location, shire_id, minion_id);

        if(!coherent)
        {
            LOG(WARN, "\t(Coherency Read Hazard) addr=%llx, location=%d, shire_id=%u, minion_id=%u", (long long unsigned int) addr & ~0x3FULL, location, shire_id, minion_id);
            return false;
        }
    }
    return true;            
}


void mem_directory::cb_drain(uint32_t shire_id, uint32_t cache_bank)
{
    printf("mem_directory::cb_drain => shire_id %i, bank %ir\n", shire_id, cache_bank);
}


void mem_directory::l2_flush(uint32_t shire_id, uint32_t cache_bank)
{
    printf("mem_directory::l2_flush => shire_id %i, bank %ir\n", shire_id, cache_bank);

    // L2 flush drains CB as well
    cb_drain(shire_id, cache_bank);
}

void mem_directory::l2_evict(uint32_t shire_id, uint32_t cache_bank)
{
    printf("mem_directory::l2_evict => shire_id %i, bank %ir\n", shire_id, cache_bank);
    directory_map_t::iterator it = m_directory_map.begin();

    while(it != m_directory_map.end())
    {
        //printf("Found %016llx, %i, %016llx, %08x\n", (long long unsigned int) it->first, it->second.level, (long long unsigned int) it->second.shire_mask, it->second.minion_mask);
        bool evict = (it->second.level == COH_SHIRE) || (it->second.level == COH_CB);
        bool shire = it->second.shire_mask & (1ULL<<shire_id);
        bool bank  = (((it->first & 0xC) >> 6) == cache_bank);
        if(evict && shire && bank)
        {
            
            printf("Found %016llx, removing\n", (long long unsigned int) it->first);
            directory_map_t::iterator it_orig = it;
            it++;
            m_directory_map.erase(it_orig);
        }
        else
        {
            it++;
        }
    }

    // L2 evict drains CB as well
    cb_drain(shire_id, cache_bank);
}

