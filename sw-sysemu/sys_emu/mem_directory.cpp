#include "mem_directory.h"
#include "emu_defines.h"

/*! \brief Directory update for a memory block.
 *
 *  This function updates the memory line in the directory and then predicts if there may have been incoherency
 *  return true if coherent, false in cc
 */

bool mem_directory::update(uint64_t address, op_location_t location, uint32_t shire_id, uint32_t minion_id)
{

    mem_info_t new_entry;
    new_entry.level = location;    

    bool coherent = false;
    directory_map_t::iterator it;
    it = m_directory_map.find(address);    // does the entry already exists?

    // Compute new entry and check coherency
    if(location == MINION)  // minion_id of shire_id has the most updated value
    {
        new_entry.shire_mask    = (1<<shire_id);
        new_entry.minion_mask   = (1<<minion_id);
        coherent                = (location == GLOBAL) || ((it->second.minion_mask & 1<<minion_id) && (it->second.shire_mask & 1<<shire_id));
    }
    else if(location == SHIRE) // shire_id has the most updated value in shire cache
    {
        new_entry.shire_mask    = (1<<shire_id);
        new_entry.minion_mask   = 0xFFFFFFFF;
        coherent                = (it->second.level != MINION) && (it->second.shire_mask & 1<<shire_id);
    }
    else
    {
        new_entry.shire_mask    = 0xFFFFFFFFFFFFFFFF;
        new_entry.minion_mask   = 0xFFFFFFFF;
        coherent                = (it->second.level == GLOBAL);
    }

    // Update directory
    if(it != m_directory_map.end()) // operator[] is the preferred alternative to update an entry, but I already have the pointer...
    {
        it->second = new_entry;
    }
    else    // insert into the map saves three C++ calls w.r.t operator[]
    {
        m_directory_map.insert(directory_map_t::value_type(address, new_entry));
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
    directory_map_t::iterator    it = m_directory_map.find(address);
    if(it != m_directory_map.end())
    {
        if(location == MINION)
        {
            return (it->second.minion_mask & 1<<minion_id) && (it->second.shire_mask & 1<<shire_id);
        }
        if(location == SHIRE)
        {
            return (it->second.level != MINION) && (it->second.shire_mask & 1<<shire_id);
        }
        return (it->second.level == GLOBAL);
    }

    // If there is no entry in the directory we assume: {location=GLOBAL, minion_mask=0, shire_mask=0}
    /*
    if(location == MINION)
    {
        return (it->second.minion_mask & 1<<minion_id) && (it->second.shire_mask & 1<<shire_id); => FALSE
    }
    else if(location == SHIRE)
    {
        return (it->second.level != MINION) && (it->second.shire_mask & 1<<shire_id);           => FALSE
    }
    else
    {
        return (it->second.level == GLOBAL);                                                    => GLOBAL==GLOBAL => TRUE
    }
    */        
    return (location == GLOBAL);

}

bool mem_directory::access(uint64_t addr, mem_access_type macc, uint32_t current_thread)
{

    op_location_t location = MINION;
    unsigned char operation = 0;

    uint32_t shire_id   = current_thread / EMU_THREADS_PER_SHIRE;
    uint32_t minion_id  = (current_thread & 0x0000001F) >> 1;

    switch (macc)
    {
    case Mem_Access_Load:
        //location = MINION;
        break;
    case Mem_Access_LoadL:
        location = SHIRE;
        break;
    case Mem_Access_LoadG:
        location = GLOBAL;
        break;
    case Mem_Access_TxLoad: // Normal op. Tensor load 0 reads from L2 SCP and stores into L1 SCP, tensor load 1 reads from L2 SCP into buffer in the VPU. Can we distinguish both cases in BEMU?
        location = GLOBAL;
        break;
    case Mem_Access_Prefetch: // Prefetch cache-op. Like a load from L2 to L1. The lookup op will fail if the line has not been written by the minion
        //location = MINION;  // TODO I need Minion, local, and global identifiers for Mem_Access_Prefetch. 
        break;
    case Mem_Access_Store:
        operation = 1;
        //location  = MINION;
        break;
    case Mem_Access_StoreL:
        operation = 1;
        location  = SHIRE;
        break;
    case Mem_Access_StoreG:
        operation = 1;
        location  = GLOBAL;
        break;
    case Mem_Access_TxStore: // Normal op. Tensor store to L1
        operation = 1; // TODO Differentiate between tensorStore(minion), tensorStoreFromSCP(local or global depending on the address)
        //location  = MINION;
        break;
    case Mem_Access_AtomicL:
        operation = 1;
        location  = SHIRE;
        break;
    case Mem_Access_AtomicG:
        operation = 1;
        location  = GLOBAL;
        break;        
    case Mem_Access_CacheOp: // Evict, Flush
        /*  How can we distinguish the three cases?
            -Evict: to L2, handle like a write?    TODO check PRM. They behave differently depending on the address
            -Flush: 
        */
        throw std::invalid_argument("unexpected operation");
        break;        
    case Mem_Access_Fetch:   // Load instruction from memory. This must not be included in the directory. Do nothing
        return true;
        break;
    case Mem_Access_PTW:     // Page table walker access. Must not be invoked. Fail if so.
        throw std::invalid_argument("unexpected operation");
        break;
    }

    bool coherent;
    if(operation)
    { 
        coherent = update(addr, location, shire_id, minion_id);

        if(!coherent)
        {
            LOG(DEBUG, "\t%s(Coherency Write Hazard) addr=0x%x, location=%d, shire_id=%u, minion_id=%u", addr, location, shire_id, minion_id);
            return false;
        }
        return true;            
    }            
        
    coherent = lookup(addr, location, shire_id, minion_id);

    if(!coherent)
    {
        LOG(DEBUG, "\t%s(Coherency Read Hazard) addr=0x%x, location=%d, shire_id=%u, minion_id=%u", addr, location, shire_id, minion_id);
        return false;
    }
    return true;            

}











