#include "mem_directory.h"
#include "emu_defines.h"
#include "emu_gio.h"

/*! \brief Directory update for a memory block write
 *
 *  This function updates the memory line in the directory and then predicts if there may have been incoherency
 *  return true if coherent, false in cc
 */

bool mem_directory::write(uint64_t address, op_location_t location, uint32_t shire_id, uint32_t minion_id, uint32_t thread_id)
{
    uint32_t minion = shire_id * EMU_MINIONS_PER_SHIRE + minion_id;

    global_directory_map_t::iterator it_global;
    shire_directory_map_t::iterator  it_shire;
    minion_directory_map_t::iterator it_minion;

    // Looks for the entry at different levels
    it_global = global_directory_map.find(address);
    it_shire  = shire_directory_map[shire_id].find(address);
    it_minion = minion_directory_map[minion].find(address);

    printf("mem_directory::write => addr %016llx, location %i, shire id %i, minion_id %i, thread_id %i, found (%i, %i, %i)\n",
        (long long unsigned int) address, location, shire_id, minion_id, thread_id,
        (it_global == global_directory_map.end()), (it_shire == shire_directory_map[shire_id].end()), (it_minion == minion_directory_map[minion].end()));

    // Marks the L1 accessed sets
    if(location == COH_MINION)
    {
        uint32_t l1_set = dcache_index(address, l1_minion_control[minion], (minion << 1) | thread_id, EMU_THREADS_PER_MINION);
        for(uint32_t way = 0; way < L1D_NUM_WAYS; way++)
        {
            printf("mem_directory::write => setting l1 set %i and way %i\n", l1_set, way);
            l1_minion_valid[minion][l1_set][way] = true;
        }
    }

    // Checks if access is coherent
    bool coherent = true;

    // Minion access must be coherent (not present yet, mcache in shared mode (0) or other thread is not dirty
    coherent &= (it_minion == minion_directory_map[minion].end())
             || (l1_minion_control[minion] == 0)
             || (it_minion->second.thread_mask_write[thread_id^1] == false);
    printf("Step1: %i\n", coherent);

    // Shire access must be coherent (not in shire, or not dirty or already dirty in same minion
    coherent &= (it_shire  == shire_directory_map[shire_id].end()) 
             || (it_shire->second.minion_id_dirty == 255)
             || (it_shire->second.minion_id_dirty == minion_id);
    printf("Step2: %i\n", coherent);

    // Global access must be coherent (not found or dirty in same shire)
    coherent &= (it_global == global_directory_map.end())
             || (it_global->second.shire_id_dirty == 255)
             || (it_global->second.shire_id_dirty == shire_id);
    printf("Step3: %i\n", coherent);

    bool update_minion = (location == COH_MINION);
    bool update_shire  = (location == COH_MINION) || (location == COH_SHIRE) || (location == COH_CB);
    bool update_global = (location != COH_GLOBAL);

    // Updates the minion directory
    if(update_minion)
    {
        minion_mem_info_t new_entry;
        uint32_t adjusted_thread_id = (l1_minion_control[minion] == 0) ? 0 : thread_id;

        // Not present, insert
        if(it_minion == minion_directory_map[minion].end())
        {
            for(uint32_t thread = 0; thread < EMU_THREADS_PER_MINION; thread++)
            {
                new_entry.thread_mask_read [thread] = false;
                new_entry.thread_mask_write[thread] = false;
                new_entry.thread_set       [thread] = 255;
            }
            new_entry.thread_mask_read [adjusted_thread_id] = true;
            new_entry.thread_mask_write[adjusted_thread_id] = true;
            new_entry.thread_set       [adjusted_thread_id] = dcache_index(address, l1_minion_control[minion], (minion << 1) | thread_id, EMU_THREADS_PER_MINION);

            minion_directory_map[minion].insert(minion_directory_map_t::value_type(address, new_entry));
            printf("mem_directory::write insert minion directory => addr %016llX, shire_id %i, minion_id %i\n", (long long unsigned int) address, shire_id, minion_id);
        }
        // Update
        else
        {
            it_minion->second.thread_mask_read [adjusted_thread_id] = true;
            it_minion->second.thread_mask_write[adjusted_thread_id] = true;
            it_minion->second.thread_set       [adjusted_thread_id] = dcache_index(address, l1_minion_control[minion], (minion << 1) | thread_id, EMU_THREADS_PER_MINION);
            printf("mem_directory::write update minion directory => addr %016llX, shire_id %i, minion_id %i\n", (long long unsigned int) address, shire_id, minion_id);
        }
    }

    // Update shire contents
    if(update_shire)
    {
        shire_mem_info_t new_value;

        new_value.l2_dirty        = (location == COH_MINION) || (location == COH_SHIRE);
        new_value.cb_dirty        = (location == COH_CB);
        new_value.minion_id_dirty = (location == COH_MINION) ? minion_id : 255;
        for(uint32_t i = 0; i < EMU_MINIONS_PER_SHIRE; i++)
        {
            new_value.minion_mask[i] = false;
        }
        new_value.minion_mask[minion_id] = true;

        // Not present, insert
        if(it_shire == shire_directory_map[shire_id].end())
        {
            shire_directory_map[shire_id].insert(shire_directory_map_t::value_type(address, new_value));
            printf("mem_directory::write insert shire directory => addr %016llX, shire_id %i\n", (long long unsigned int) address, shire_id);
        }
        // Update
        else
        {
            it_shire->second.l2_dirty = new_value.l2_dirty;
            it_shire->second.cb_dirty = new_value.cb_dirty;
            it_shire->second.minion_id_dirty = new_value.minion_id_dirty;
            it_shire->second.minion_mask[minion_id] = true;
            printf("mem_directory::write update shire directory => addr %016llX, shire_id %i\n", (long long unsigned int) address, shire_id);
        }
    }

    // Update global contents
    if(update_global)
    {
        global_mem_info_t new_value;

        new_value.shire_id_dirty = shire_id;
        for(uint32_t shire = 0; shire < EMU_NUM_SHIRES; shire++)
        {
            new_value.shire_mask[shire] = false;
        }
        new_value.shire_mask[shire_id] = true;

        // Not present, insert
        if(it_global == global_directory_map.end())
        {
            global_directory_map.insert(global_directory_map_t::value_type(address, new_value));
            printf("mem_directory::write insert global directory => addr %016llX\n", (long long unsigned int) address);
        }
        // Update
        else
        {
            it_global->second.shire_id_dirty = new_value.shire_id_dirty;
            it_global->second.shire_mask[shire_id] = true;
            printf("mem_directory::write update global directory => addr %016llX\n", (long long unsigned int) address);
        }
    }

    printf("Coherent is %i\n", coherent);
    if(!coherent) dump_state(it_global, it_shire, it_minion, shire_id, minion);

    return coherent; 
}

/*! \brief Directory update for a memory block read.
 *
 *  This function lookups the memory line in the directory and then predicts if there may have been incoherency
 *  return true if coherent, false in cc
 */
bool mem_directory::read(uint64_t address, op_location_t location, uint32_t shire_id, uint32_t minion_id, uint32_t thread_id)
{
    uint32_t minion = shire_id * EMU_MINIONS_PER_SHIRE + minion_id;

    global_directory_map_t::iterator it_global;
    shire_directory_map_t::iterator  it_shire;
    minion_directory_map_t::iterator it_minion;

    // Looks for the entry at different levels
    it_global = global_directory_map.find(address);
    it_shire  = shire_directory_map[shire_id].find(address);
    it_minion = minion_directory_map[minion].find(address);

    printf("mem_directory::read => addr %016llx, location %i, shire id %i, minion_id %i, thread_id %i, found (%i, %i, %i)\n",
        (long long unsigned int) address, location, shire_id, minion_id, thread_id,
        (it_global == global_directory_map.end()), (it_shire == shire_directory_map[shire_id].end()), (it_minion == minion_directory_map[minion].end()));

    // Marks the L1 accessed sets
    if(location == COH_MINION)
    {
        uint32_t l1_set = dcache_index(address, l1_minion_control[minion], (minion << 1) | thread_id, EMU_THREADS_PER_MINION);
        for(uint32_t way = 0; way < L1D_NUM_WAYS; way++)
        {
            printf("mem_directory::write => setting l1 set %i and way %i\n", l1_set, way);
            l1_minion_valid[minion][l1_set][way] = true;
        }
    }

    // Checks if access is coherent
    bool coherent = true;

    // Minion access must be coherent (not present yet, mcache in shared mode (0) or other thread is not dirty
    coherent &= (it_minion == minion_directory_map[minion].end())
             || (l1_minion_control[minion] == 0)
             || (it_minion->second.thread_mask_write[thread_id^1] == false);

    // Shire access must be coherent (not in shire, or not dirty or already dirty in same minion
    coherent &= (it_shire  == shire_directory_map[shire_id].end())
             || (it_shire->second.minion_id_dirty == 255)
             || (it_shire->second.minion_id_dirty == minion_id);

    // Global access must be coherent (not found or dirty in same shire)
    coherent &= (it_global == global_directory_map.end())
             || (it_global->second.shire_id_dirty == 255)
             || (it_global->second.shire_id_dirty == shire_id);

    bool update_minion = (location == COH_MINION);
    bool update_shire  = (location == COH_MINION) || (location == COH_SHIRE) || (location == COH_CB);
    bool update_global = (location != COH_GLOBAL);

    // Updates the minion directory
    if(update_minion)
    {
        minion_mem_info_t new_entry;
        uint32_t adjusted_thread_id = (l1_minion_control[minion] == 0) ? 0 : thread_id;

        // Not present, insert
        if(it_minion == minion_directory_map[minion].end())
        {
            for(uint32_t thread = 0; thread < EMU_THREADS_PER_MINION; thread++)
            {
                new_entry.thread_mask_read [thread] = false;
                new_entry.thread_mask_write[thread] = false;
                new_entry.thread_set       [thread] = 255;
            }
            new_entry.thread_mask_read[adjusted_thread_id] = true;
            new_entry.thread_set      [adjusted_thread_id] = dcache_index(address, l1_minion_control[minion], (minion << 1) | thread_id, EMU_THREADS_PER_MINION);

            minion_directory_map[minion].insert(minion_directory_map_t::value_type(address, new_entry));
            printf("mem_directory::read insert minion directory => addr %016llX, shire_id %i, minion_id %i\n", (long long unsigned int) address, shire_id, minion_id);
        }
        // Update
        else
        {
            it_minion->second.thread_mask_read[adjusted_thread_id] = true;
            it_minion->second.thread_set      [adjusted_thread_id] = dcache_index(address, l1_minion_control[minion], (minion << 1) | thread_id, EMU_THREADS_PER_MINION);
            printf("mem_directory::read update minion directory => addr %016llX, shire_id %i, minion_id %i\n", (long long unsigned int) address, shire_id, minion_id);
        }
    }

    // Update shire contents
    if(update_shire)
    {
        shire_mem_info_t new_value;

        new_value.l2_dirty        = false;
        new_value.cb_dirty        = false;
        new_value.minion_id_dirty = 255;
        for(uint32_t i = 0; i < EMU_MINIONS_PER_SHIRE; i++)
        {
            new_value.minion_mask[i] = false;
        }
        new_value.minion_mask[minion_id] = true;

        // Not present, insert
        if(it_shire == shire_directory_map[shire_id].end())
        {
            shire_directory_map[shire_id].insert(shire_directory_map_t::value_type(address, new_value));
            printf("mem_directory::read insert shire directory => addr %016llX, shire_id %i\n", (long long unsigned int) address, shire_id);
        }
        // Update
        else
        {
            it_shire->second.minion_mask[minion_id] = true;
            printf("mem_directory::read update shire directory => addr %016llX, shire_id %i\n", (long long unsigned int) address, shire_id);
        }
    }

    // Update global contents
    if(update_global)
    {
        global_mem_info_t new_value;

        new_value.shire_id_dirty = 255;
        for(uint32_t shire = 0; shire < EMU_NUM_SHIRES; shire++)
        {
            new_value.shire_mask[shire] = false;
        }
        new_value.shire_mask[shire_id] = true;

        // Not present, insert
        if(it_global == global_directory_map.end())
        {
            global_directory_map.insert(global_directory_map_t::value_type(address, new_value));
            printf("mem_directory::read insert global directory => addr %016llX\n", (long long unsigned int) address);
        }
        // Update
        else
        {
            it_global->second.shire_mask[shire_id] = true;
            printf("mem_directory::read update global directory => addr %016llX\n", (long long unsigned int) address);
        }
    }

    printf("Coherent is %i\n", coherent);
    if(!coherent) dump_state(it_global, it_shire, it_minion, shire_id, minion);

    return coherent; 
}

/*! \brief Directory update for a memory block evict
 *
 *  This function updates the memory line in the directory and then predicts if there may have been incoherency
 *  return true if coherent, false in cc
 */

bool mem_directory::evict_va(uint64_t address, op_location_t location, uint32_t shire_id, uint32_t minion_id, uint32_t thread_id)
{
    uint32_t minion = shire_id * EMU_MINIONS_PER_SHIRE + minion_id;

    global_directory_map_t::iterator it_global;
    shire_directory_map_t::iterator  it_shire;
    minion_directory_map_t::iterator it_minion;

    // Looks for the entry at different levels
    it_global = global_directory_map.find(address);
    it_shire  = shire_directory_map[shire_id].find(address);
    it_minion = minion_directory_map[minion].find(address);

    printf("mem_directory::evict_va => addr %016llx, location %i, shire id %i, minion_id %i, thread_id %i, found (%i, %i, %i)\n",
        (long long unsigned int) address, location, shire_id, minion_id, thread_id,
        (it_global == global_directory_map.end()), (it_shire == shire_directory_map[shire_id].end()), (it_minion == minion_directory_map[minion].end()));

    // Checks if access is coherent
    bool coherent = true;

    // Minion access must be coherent (not present yet, mcache in shared mode (0) or other thread is not dirty
    coherent &= (it_minion == minion_directory_map[minion].end())
             || (l1_minion_control[minion] == 0)
             || (it_minion->second.thread_mask_write[thread_id^1] == false);

    // Shire access must be coherent (not in shire, or not dirty or already dirty in same minion
    coherent &= (it_shire  == shire_directory_map[shire_id].end())
             || (it_shire->second.minion_id_dirty == 255)
             || (it_shire->second.minion_id_dirty == minion_id);

    // Global access must be coherent (not found or dirty in same shire)
    coherent &= (it_global == global_directory_map.end())
             || (it_global->second.shire_id_dirty == 255)
             || (it_global->second.shire_id_dirty == shire_id);

    bool update_minion = ((location == COH_SHIRE) || (location == COH_GLOBAL)) && (it_minion != minion_directory_map[minion].end());
    bool update_shire  = (location == COH_GLOBAL) && (it_shire  != shire_directory_map[shire_id].end());

    // Updates the minion directory
    if(update_minion)
    {
        uint32_t adjusted_thread_id = (l1_minion_control[minion] == 0) ? 0 : thread_id;
        printf("mem_directory::evict_va update minion directory => addr %016llX, shire_id %i, minion_id %i\n", (long long unsigned int) address, shire_id, minion_id);
        it_minion->second.thread_mask_read [adjusted_thread_id] = false;
        it_minion->second.thread_mask_write[adjusted_thread_id] = false;

        // Checks if all sets are clear
        bool all_clear = true;
        for(uint32_t thread = 0; thread < EMU_THREADS_PER_MINION; thread++)
        {
            all_clear &= (!it_minion->second.thread_mask_read[thread] && !it_minion->second.thread_mask_write[thread]);
        }

        // Line is no longer in minion, update shire state and remove in minion
        if(all_clear)
        {
            printf("mem_directory::evict_va => line no longer in minion\n");

            // Clears the minion dirty flag in shire, must have an entry
            if(it_shire == shire_directory_map[shire_id].end())
            {
                throw std::invalid_argument("Should have found address in shire");
            }

            // Clears the minion dirty flag in shire, must have an entry
            if((it_shire->second.minion_id_dirty != 255) && (it_shire->second.minion_id_dirty != minion_id))
            {
                LOG_ALL_MINIONS(WARN, "\t(Coherency EvictVA Hazard wrong minion) addr=%llx, location=%d, shire_id=%u, minion_id=%u, thread_id=%u, minion_id_dirty=%u\n", (long long unsigned int) address, location, shire_id, minion_id, thread_id, it_shire->second.minion_id_dirty);
            }
            it_shire->second.minion_id_dirty = 255;
            it_shire->second.minion_mask[minion_id] = false;

            // Remove minion entry
            printf("mem_directory::evict_va remove minion directory => addr %016llX, shire_id %i, minion_id %i\n", (long long unsigned int) address, shire_id, minion_id);
            minion_directory_map[minion].erase(it_minion);
        }
    }

    // Update shire contents
    if(update_shire)
    {
        // CBs should be drained with ESRs, not evict_va
        if(it_shire->second.cb_dirty)
        {
            LOG_ALL_MINIONS(WARN, "\t(Coherency EvictVA Hazard CB evict) addr=%llx, location=%d, shire_id=%u, minion_id=%u, thread_id=%u", (long long unsigned int) address, location, shire_id, minion_id, thread_id);
        }

        // Checks if all sets are clear
        bool all_clear = true;
        for(uint32_t i = 0; i < EMU_MINIONS_PER_SHIRE; i++)
        {
            all_clear &= (!it_shire->second.minion_mask[i]);
        }

        // Line is no longer in minion, update shire state and remove in minion
        if(all_clear)
        {
            // Remove the dirty in global entry
            if(it_global == global_directory_map.end())
            {
                throw std::invalid_argument("Should have found address in global");
            }

            // Clears the minion dirty flag in global, must have an entry
            if((it_global->second.shire_id_dirty != 255) && (it_global->second.shire_id_dirty != shire_id))
            {
                LOG_ALL_MINIONS(WARN, "\t(Coherency EvictVA Hazard wrong shire) addr=%llx, location=%d, shire_id=%u, minion_id=%u, thread_id=%u, shire_id_dirty=%u", (long long unsigned int) address, location, shire_id, minion_id, thread_id, it_global->second.shire_id_dirty);
            }
            it_global->second.shire_id_dirty = 255;
            it_global->second.shire_mask[shire_id] = false;
        }
    }

    // Check if need to remove shire entry
    if(it_shire != shire_directory_map[shire_id].end())
    {
        bool all_clear = true;
        for(uint32_t i = 0; i < EMU_MINIONS_PER_SHIRE; i++)
        {
            all_clear &= (it_shire->second.minion_mask[i] == false);
        }
        if(all_clear)
        {
            printf("mem_directory::evict_va remove shire directory => addr %016llX, shire_id %i\n", (long long unsigned int) address, shire_id);
            shire_directory_map[shire_id].erase(it_shire);
        }
    }

    // Check if need to remove global entry
    if(it_global != global_directory_map.end())
    {
        bool all_clear = true;
        for(uint32_t shire = 0; shire < EMU_NUM_SHIRES; shire++)
        {
            all_clear &= (it_global->second.shire_mask[shire] == false);
        }
        if(all_clear)
        {
            printf("mem_directory::evict_va remove global directory => addr %016llX\n", (long long unsigned int) address);
            global_directory_map.erase(it_global);
        }
    }
    printf("Coherent is %i\n", coherent);
    if(!coherent) dump_state(it_global, it_shire, it_minion, shire_id, minion);

    return coherent; 
}

// Private function that clears a set of a minion l1
void mem_directory::l1_clear_set(uint32_t minion, uint32_t set)
{
    printf("mem_directory::l1_clear_set => clearing set %i of minion %i\n", set, minion);

    // Runs through all elements that match in same set
    minion_directory_map_t::iterator it = minion_directory_map[minion].begin();
    uint32_t shire_id  = minion / EMU_MINIONS_PER_SHIRE;

    while(it != minion_directory_map[minion].end())
    {
        uint64_t addr = it->first;
        printf("mem_directory::l1_clear_set => addr %016llX belongs to minion\n", (long long unsigned int) addr);

        // Clears the valids if set match
        for(uint32_t thread = 0; thread < EMU_THREADS_PER_MINION; thread++)
        {
            if(it->second.thread_set[thread] == set)
            {
                it->second.thread_mask_write[thread] = false;
                it->second.thread_mask_read [thread] = false;
            }
        }

        // Checks if all sets are clear
        bool all_clear = true;
        for(uint32_t thread = 0; thread < EMU_THREADS_PER_MINION; thread++)
        {
            all_clear &= (!it->second.thread_mask_read[thread] && !it->second.thread_mask_write[thread]);
           printf("mem_directory::l1_clear_set => addr %016llX belongs to minion, thread %i, read %i, write %i\n", (long long unsigned int) addr, thread, it->second.thread_mask_read[thread], it->second.thread_mask_write[thread]);
        }

        // Line is no longer in minion, update shire state and remove in minion
        if(all_clear)
        {
            printf("mem_directory::l1_clear_set => same set and minion clean, clearing addr %016llX\n", (long long unsigned int) addr);

            // Clears the minion dirty flag
            shire_directory_map_t::iterator it_shire;
            it_shire = shire_directory_map[shire_id].find(addr);

            // Must have an entry
            if(it_shire == shire_directory_map[shire_id].end())
            {
                throw std::invalid_argument("Should have found address in shire");
            }

            it_shire->second.minion_id_dirty = 255;

            // Remove minion entry
            minion_directory_map_t::iterator it_orig = it;
            minion_directory_map[minion].erase(it_orig);
            it++;            
        }
        else
        {
            it++;
        }
    }
}

// Private function that dumps coherent state
void mem_directory::dump_state(global_directory_map_t::iterator it_global, shire_directory_map_t::iterator it_shire, minion_directory_map_t::iterator it_minion, uint32_t shire_id, uint32_t minion)
{
    if(it_global != global_directory_map.end())
    {
        printf("Dumping global state\n");
        printf("  shire_id_dirty: %i\n", it_global->second.shire_id_dirty);
        for(uint32_t shire = 0; shire < EMU_NUM_SHIRES; shire++)
        {
              printf("  shire_mask[%i] = %i\n", shire, it_global->second.shire_mask[shire]);
        }
    }
    if(it_shire != shire_directory_map[shire_id].end())
    {
        printf("Dumping shire state\n");
        printf("  l2_dirty: %i\n", it_shire->second.l2_dirty);
        printf("  cb_dirty: %i\n", it_shire->second.cb_dirty);
        printf("  minion_id_dirty: %i\n", it_shire->second.minion_id_dirty);
        for(uint32_t i = 0; i < EMU_MINIONS_PER_SHIRE; i++)
        {
              printf("  minion_mask[%i] = %i\n", i, it_shire->second.minion_mask[i]);
        }
    }
    if(it_minion != minion_directory_map[minion].end())
    {
        printf("Dumping minion state\n");
        for(uint32_t thread = 0; thread < EMU_THREADS_PER_MINION; thread++)
        {
              printf("  thread_mask_write[%i] = %i\n", thread, it_minion->second.thread_mask_write[thread]);
              printf("  thread_mask_read[%i] = %i\n", thread, it_minion->second.thread_mask_read[thread]);
              printf("  thread_set[%i] = %i\n", thread, it_minion->second.thread_set[thread]);
        }
    }
}

// Constructor
mem_directory::mem_directory()
{
    // All set and ways are clear
    for(uint32_t minion = 0; minion < EMU_NUM_MINIONS; minion++)
    {
        for(uint32_t set = 0; set < L1D_NUM_SETS; set++)
        {
            for(uint32_t way = 0; way < L1D_NUM_WAYS; way++)
            {
                l1_minion_valid[minion][set][way] = false;
            }
        }
        l1_minion_control[minion] = 0;
    }
}

// Public function to access a memory position
bool mem_directory::access(uint64_t addr, mem_access_type macc, cacheop_type cop, uint32_t current_thread)
{

    op_location_t location = COH_MINION;
    unsigned char operation = 1;

    uint32_t shire_id  = current_thread / EMU_THREADS_PER_SHIRE;
    uint32_t minion_id = (current_thread >> 1) & 0x0000001F;
    uint32_t thread_id = current_thread & 1;

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
        operation = 2;
        //location  = COH_MINION;
        break;
    case Mem_Access_StoreL:
        operation = 2;
        location  = COH_SHIRE;
        break;
    case Mem_Access_StoreG:
        operation = 2;
        location  = COH_GLOBAL;
        break;
    case Mem_Access_TxStore: // Normal op. Tensor store to L1
        operation = 2;
        location  = COH_CB;
        break;
    case Mem_Access_AtomicL:
        operation = 2;
        location  = COH_SHIRE;
        break;
    case Mem_Access_AtomicG:
        operation = 2;
        location  = COH_GLOBAL;
        break;        
    case Mem_Access_CacheOp:
        if((cop == CacheOp_EvictL2) || (cop == CacheOp_EvictL3) || (cop == CacheOp_EvictDDR))
            operation = 6;
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
    if(operation & 1)
    {   
        coherent = read(addr & ~0x3FULL, location, shire_id, minion_id, thread_id);

        if(!coherent)
        {
            LOG_ALL_MINIONS(WARN, "\t(Coherency Read Hazard) addr=%llx, location=%d, shire_id=%u, minion_id=%u, thread_id=%u", (long long unsigned int) addr & ~0x3FULL, location, shire_id, minion_id, thread_id);
            return false;
        }
    }
    if(operation & 2)
    { 
        coherent = write(addr & ~0x3FULL, location, shire_id, minion_id, thread_id);

        if(!coherent)
        {
            LOG_ALL_MINIONS(WARN, "\t(Coherency Write Hazard) addr=%llx, location=%d, shire_id=%u, minion_id=%u, thread_id=%u", (long long unsigned int) addr & ~0x3FULL, location, shire_id, minion_id, thread_id);
            return false;
        }
    }
    if(operation & 4)
    {
        coherent = evict_va(addr & ~0x3FULL, location, shire_id, minion_id, thread_id);

        if(!coherent)
        {
            LOG_ALL_MINIONS(WARN, "\t(Coherency EvictVA Hazard) addr=%llx, location=%d, shire_id=%u, minion_id=%u, thread_id=%u", (long long unsigned int) addr & ~0x3FULL, location, shire_id, minion_id, thread_id);
            return false;
        }
    }
    return true;            
}

// Public function called when there's an ESR write that drains an L2 coallescing buffer
void mem_directory::cb_drain(uint32_t shire_id, uint32_t cache_bank)
{
    printf("mem_directory::cb_drain => shire_id %i, bank %ir\n", shire_id, cache_bank);
}

// Public function called when there's an ESR write that flushes an L2 bank
void mem_directory::l2_flush(uint32_t shire_id, uint32_t cache_bank)
{
    printf("mem_directory::l2_flush => shire_id %i, bank %ir\n", shire_id, cache_bank);

    // L2 flush drains CB as well
    cb_drain(shire_id, cache_bank);
}

// Public function called when there's an ESR write that evicts an L2 bank
void mem_directory::l2_evict(uint32_t shire_id, uint32_t cache_bank)
{
    printf("mem_directory::l2_evict => shire_id %i, bank %ir\n", shire_id, cache_bank);
    shire_directory_map_t::iterator it = shire_directory_map[shire_id].begin();

    while(it != shire_directory_map[shire_id].end())
    {
        //printf("Found %016llx, %i, %016llx, %08x\n", (long long unsigned int) it->first, it->second.level, (long long unsigned int) it->second.shire_mask, it->second.minion_mask);
        bool bank = (((it->first & 0xC0) >> 6) == cache_bank);
        if(bank)
        {
            printf("Found %016llx, removing\n", (long long unsigned int) it->first);
            shire_directory_map_t::iterator it_orig = it;
            it++;
            shire_directory_map[shire_id].erase(it_orig);
        }
        else
        {
            it++;
        }
    }

    // L2 evict drains CB as well
    cb_drain(shire_id, cache_bank);
}

// Public function called when code executed an evict SW
void mem_directory::l1_evict_sw(uint32_t shire_id, uint32_t minion_id, uint32_t set, uint32_t way)
{
    printf("mem_directory::l1_evict_sw => shire_id %i, minion_id %i, set %i, way %i\n", shire_id, minion_id, set, way);
    // Clears set and way
    uint32_t minion = shire_id * EMU_MINIONS_PER_SHIRE + minion_id;
    l1_minion_valid[minion][set][way] = false;

    // Checks if all ways of a set are clear
    bool all_clear = true;
    for(uint32_t it_way = 0; it_way < L1D_NUM_WAYS; it_way++)
    {
        all_clear &= (l1_minion_valid[minion][set][it_way] == false);
    }

    if(all_clear)
        l1_clear_set(minion, set);
}

// Public function called when code executed a flush SW
void mem_directory::l1_flush_sw(uint32_t shire_id, uint32_t minion_id, uint32_t set, uint32_t way)
{
    printf("mem_directory::l1_flush_sw => shire_id %i, minion_id %i, set %i, way %i\n", shire_id, minion_id, set, way);
    // Clears set and way
    uint32_t minion = shire_id * EMU_MINIONS_PER_SHIRE + minion_id;
    l1_minion_valid[minion][set][way] = false;

    // Checks if all ways of a set are clear
    bool all_clear = true;
    for(uint32_t it_way = 0; it_way < L1D_NUM_WAYS; it_way++)
    {
        all_clear &= (l1_minion_valid[minion][set][it_way] == false);
    }

    if(all_clear)
        l1_clear_set(minion, set);
}

// Public function to notify changes in the cache control state
void mem_directory::mcache_control_up(uint32_t shire_id, uint32_t minion_id, uint32_t val)
{
    printf("mem_directory::mcache_control_up => shire_id %i, minion_id %i, val %i\n", shire_id, minion_id, val);

    // Checks that all sets and ways are clear
    uint32_t minion = shire_id * EMU_MINIONS_PER_SHIRE + minion_id;
    bool all_clear = true;
    for(uint32_t set = 0; set < L1D_NUM_SETS; set++)
    {
        for(uint32_t way = 0; way < L1D_NUM_WAYS; way++)
        {
            all_clear &= (l1_minion_valid[minion][set][way] == false);
            l1_minion_valid[minion][set][way] = false;
        }
    }

    if(!all_clear)
    {
        LOG_ALL_MINIONS(WARN, "\t(Coherency mcache control Hazard) shire_id=%u, minion_id=%u changed mcache control and cache not clear", shire_id, minion_id);
    }

    l1_minion_control[minion] = val;
}


