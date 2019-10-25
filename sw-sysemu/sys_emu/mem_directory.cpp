#include "mem_directory.h"
#include "emu_defines.h"
#include "emu_gio.h"

uint64_t md_log_addr   = 0x1;
uint32_t md_log_minion = 2048;
uint64_t global_time_stamp = 0;

#define MD_LOG(addr, minion, cmd) \
    { if((addr == 0x0) || (md_log_addr == 0x0) || (addr == md_log_addr)) \
    { \
        if((minion == 0xFFFFFFFF) || (md_log_minion == 0xFFFFFFFF) || (minion == md_log_minion)) \
        { \
            cmd; \
        } \
    } }

// Returns an int with the mask of a boolean array
uint64_t bool_array_to_int(bool * array, uint64_t size)
{
    uint64_t ret = 0;

    for(uint64_t i = 0; i < size; i++)
    {
        if(array[i]) ret += 1 << i;
    }
    return ret;
}

/*! \brief Directory update for a memory block write
 *
 *  This function updates the memory line in the directory and then predicts if there may have been incoherency
 *  return true if coherent, false in cc
 */

bool mem_directory::write(uint64_t address, op_location_t location, uint32_t shire_id, uint32_t minion_id, uint32_t thread_id, size_t size, uint32_t cb_quarter)
{
    uint32_t minion = shire_id * EMU_MINIONS_PER_SHIRE + minion_id;

    // Increments the global time stamp for every write
    global_time_stamp++;

    global_directory_map_t::iterator it_global;
    shire_directory_map_t::iterator  it_shire;
    minion_directory_map_t::iterator it_minion;

    // Looks for the entry at different levels
    it_global = global_directory_map.find(address);
    it_shire  = shire_directory_map[shire_id].find(address);
    it_minion = minion_directory_map[minion].find(address);

    // Assertions
    bool minion_found = it_minion != minion_directory_map[minion].end();
    bool shire_found  = it_shire  != shire_directory_map[shire_id].end();
    bool global_found = it_global != global_directory_map.end();

    if(minion_found && !shire_found)
    {
       LOG_ALL_MINIONS(FTL, "mem_directory::write minion entry found and shire entry not found for addr %llX when doing write\n", (long long unsigned int) address);
    }
    if(shire_found && !global_found)
    {
       LOG_ALL_MINIONS(FTL, "mem_directory::write shire entry found and global entry not found for addr %llX when doing write\n", (long long unsigned int) address);
    }

    // Info
    MD_LOG(address, minion, printf("mem_directory::write => addr %016llX, location %i, shire id %i, minion_id %i, thread_id %i, size %i, cb quarter %i, found (%i, %i, %i)\n",
        (long long unsigned int) address, location, shire_id, minion_id, thread_id, (int) size, cb_quarter,
        global_found, shire_found, minion_found));

    uint32_t l1_set = dcache_index(address, l1_minion_control[minion], (minion << 1) | thread_id, EMU_THREADS_PER_MINION);
    // Marks the L1 accessed sets
    if(location == COH_MINION)
    {
        for(uint32_t way = 0; way < L1D_NUM_WAYS; way++)
        {
            MD_LOG(address, minion, printf("mem_directory::write => setting l1 set %i and way %i\n", l1_set, way));
            l1_minion_valid[minion][l1_set][way] = true;
        }
    }

    // Checks if data is in any minion besides the current one
    bool data_in_other_minion = false;
    if(shire_found)
    {
        for(uint32_t i = 0; i < EMU_MINIONS_PER_SHIRE; i++)
            data_in_other_minion |= it_shire->second.minion_mask[i] && (i != minion_id);
    }

    // Checks if data is in any shire besides current one
    bool data_in_other_shire = false;
    if(global_found)
    {
        for(uint32_t shire = 0; shire < EMU_NUM_SHIRES; shire++)
            data_in_other_shire |= it_global->second.shire_mask[shire] && (shire != shire_id);
    }

    // Checks if access is coherent
    bool coherent = true;

    // Minion access must be coherent
    coherent &= !minion_found                                                                         // Not accessed yet by minion
             || ((l1_minion_control[minion] == 0) && (location == COH_MINION))                        // Cache is shared and access is minion
             || (!it_minion->second.thread_mask_write[thread_id^1] && (location == COH_MINION))       // Access is minion and other thread is not dirty
             || (!it_minion->second.thread_mask_write[0] && !it_minion->second.thread_mask_write[1]); // Data is not dirty and accessing beyond minion

    // Shire access must be coherent
    coherent &= !shire_found                                                                                                                            // Not in shire
             || ((it_shire->second.l2_dirty_minion_id == 255)       && !it_shire->second.cb_dirty && !data_in_other_minion)                             // Hasn't been written by minion or in CB and is not stale in another minion
             || ((it_shire->second.l2_dirty_minion_id == minion_id) &&  it_shire->second.l2_dirty && !data_in_other_minion && (location == COH_MINION)) // If dirty, was the same minion and not stale in another minion
             || (!it_shire->second.cb_dirty_quarter[cb_quarter]     && !it_shire->second.l2_dirty                          && (location == COH_CB));    // Not rewriting same CB quarter

    // Global access must be coherent
    coherent &= !global_found                                                                                                                        // Not in global
             || ((it_global->second.l2_dirty_shire_id == 255)      && !data_in_other_shire && !it_global->second.cb_dirty)                           // Still clean
             || ((it_global->second.l2_dirty_shire_id == shire_id) && !data_in_other_shire && ((location == COH_MINION) || (location == COH_SHIRE))) // Rewriting in same shire
             || (!it_global->second.cb_dirty_quarter[cb_quarter]   && (it_global->second.l2_dirty_shire_id == 255) && (location == COH_CB));         // CB quarter was still not written

    if(!coherent) dump_state(it_global, it_shire, it_minion, shire_id, minion);

    bool update_minion = (location == COH_MINION);
    bool update_shire  = (location == COH_MINION) || (location == COH_SHIRE) || (location == COH_CB);
    bool update_global = (location != COH_GLOBAL);

    // Updates the minion directory
    if(update_minion)
    {
        uint32_t adjusted_thread_id = (l1_minion_control[minion] == 0) ? 0 : thread_id;

        // Not present, insert
        if(it_minion == minion_directory_map[minion].end())
        {
            minion_mem_info_t new_entry;
            for(uint32_t thread = 0; thread < EMU_THREADS_PER_MINION; thread++)
            {
                new_entry.thread_mask_read [thread] = false;
                new_entry.thread_mask_write[thread] = false;
                new_entry.thread_set       [thread] = 255;
            }
            new_entry.thread_mask_read [adjusted_thread_id] = true;
            new_entry.thread_mask_write[adjusted_thread_id] = true;
            new_entry.thread_set       [adjusted_thread_id] = l1_set;
            new_entry.time_stamp                            = global_time_stamp;

            minion_directory_map[minion].insert(minion_directory_map_t::value_type(address, new_entry));
            MD_LOG(address, minion, printf("mem_directory::write insert minion directory => addr %016llX, shire_id %i, minion_id %i, thread_id: %i, mask_write: 0x%X, mask_read: 0x%X, set: 0x%X, time_stamp: %llu\n",
                  (long long unsigned int) address, shire_id, minion_id, thread_id, (int32_t) bool_array_to_int(new_entry.thread_mask_write, EMU_THREADS_PER_MINION),
                  (int32_t) bool_array_to_int(new_entry.thread_mask_read, EMU_THREADS_PER_MINION), new_entry.thread_set[adjusted_thread_id], (long long unsigned int) new_entry.time_stamp));
        }
        // Update
        else
        {
            it_minion->second.thread_mask_read [adjusted_thread_id] = true;
            it_minion->second.thread_mask_write[adjusted_thread_id] = true;
            it_minion->second.thread_set       [adjusted_thread_id] = l1_set;
            it_minion->second.time_stamp                            = global_time_stamp;
            MD_LOG(address, minion, printf("mem_directory::write update minion directory => addr %016llX, shire_id %i, minion_id %i, thread_id: %i, mask_write: 0x%X, mask_read: 0x%X, set: 0x%X, time_stamp: %llu\n",
                  (long long unsigned int) address, shire_id, minion_id, thread_id, (int32_t) bool_array_to_int(it_minion->second.thread_mask_write, EMU_THREADS_PER_MINION),
                  (int32_t) bool_array_to_int(it_minion->second.thread_mask_read, EMU_THREADS_PER_MINION), it_minion->second.thread_set[adjusted_thread_id], (long long unsigned int) it_minion->second.time_stamp));
        }
    }

    // Update shire contents
    if(update_shire)
    {
        shire_mem_info_t new_value;

        // L2 dirty
        new_value.l2                 = (location == COH_MINION) || (location == COH_SHIRE);
        new_value.l2_dirty           = (location == COH_MINION) || (location == COH_SHIRE);
        new_value.l2_dirty_minion_id = (location == COH_MINION) ? minion_id : 255;
        new_value.time_stamp         = global_time_stamp;

        // CB dirty
        new_value.cb_dirty = (location == COH_CB);
        for(int quarter = 0; quarter < 4; quarter++)
          new_value.cb_dirty_quarter[quarter] = false;
        if(new_value.cb_dirty)
            new_value.cb_dirty_quarter[cb_quarter] = true;

        // Minion mask
        for(uint32_t i = 0; i < EMU_MINIONS_PER_SHIRE; i++)
            new_value.minion_mask[i] = false;
        new_value.minion_mask[minion_id] = (location == COH_MINION);

        // Not present, insert
        if(it_shire == shire_directory_map[shire_id].end())
        {
            shire_directory_map[shire_id].insert(shire_directory_map_t::value_type(address, new_value));

            MD_LOG(address, minion, printf("mem_directory::write insert shire directory => addr %016llX, shire_id %i, l2: %i, l2_dirty: %i, l2_dirty_minion_id: %i, cb_dirty: %i, cb_quarter: 0x%X, minion_mask: 0x%X\n",
                  (long long unsigned int) address, shire_id, new_value.l2, new_value.l2_dirty, new_value.l2_dirty_minion_id,
                  new_value.cb_dirty, (int32_t) bool_array_to_int(new_value.cb_dirty_quarter, 4), (int32_t) bool_array_to_int(new_value.minion_mask, 32)));
        }
        // Update
        else
        {
            it_shire->second.l2                     = new_value.l2;
            it_shire->second.l2_dirty               = new_value.l2_dirty;
            it_shire->second.l2_dirty_minion_id     = new_value.l2_dirty_minion_id;
            it_shire->second.time_stamp             = new_value.time_stamp;
            it_shire->second.cb_dirty               = new_value.cb_dirty;
            it_shire->second.minion_mask[minion_id] = new_value.minion_mask[minion_id];
            if(new_value.cb_dirty)
                it_shire->second.cb_dirty_quarter[cb_quarter] = true;

            MD_LOG(address, minion, printf("mem_directory::write update shire directory => addr %016llX, shire_id %i, l2: %i, l2_dirty: %i, l2_dirty_minion_id: %i, cb_dirty: %i, cb_quarter: 0x%X, minion_mask: 0x%X\n",
                  (long long unsigned int) address, shire_id, it_shire->second.l2, it_shire->second.l2_dirty, it_shire->second.l2_dirty_minion_id,
                  it_shire->second.cb_dirty, (int32_t) bool_array_to_int(it_shire->second.cb_dirty_quarter, 4), (int32_t) bool_array_to_int(new_value.minion_mask, 32)));
        }
    }

    // Update global contents
    if(update_global)
    {
        global_mem_info_t new_value;

        // Set CB bits
        new_value.cb_dirty = (location == COH_CB);
        for(int quarter = 0; quarter < 4; quarter++)
          new_value.cb_dirty_quarter[quarter] = false;
        new_value.cb_dirty_quarter[cb_quarter] = new_value.cb_dirty;

        // Sets dirty bits
        new_value.l2_dirty_shire_id = new_value.cb_dirty ? 255 : shire_id;
        new_value.time_stamp        = global_time_stamp;

        // Shire mask
        for(uint32_t shire = 0; shire < EMU_NUM_SHIRES; shire++)
            new_value.shire_mask[shire] = false;
        new_value.shire_mask[shire_id] = true;

        // Not present, insert
        if(it_global == global_directory_map.end())
        {
            global_directory_map.insert(global_directory_map_t::value_type(address, new_value));
            MD_LOG(address, minion, printf("mem_directory::write insert global directory => addr %016llX\n", (long long unsigned int) address));
        }
        // Update
        else
        {
            it_global->second.time_stamp                   = new_value.time_stamp;
            it_global->second.l2_dirty_shire_id            = new_value.l2_dirty_shire_id;
            it_global->second.shire_mask[shire_id]         = new_value.shire_mask[shire_id];
            it_global->second.cb_dirty                     = new_value.cb_dirty;
            it_global->second.cb_dirty_quarter[cb_quarter] = new_value.cb_dirty;
            MD_LOG(address, minion, printf("mem_directory::write update global directory => addr %016llX\n", (long long unsigned int) address));
        }
    }

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

    // Assertions
    bool minion_found = it_minion != minion_directory_map[minion].end();
    bool shire_found  = it_shire  != shire_directory_map[shire_id].end();
    bool global_found = it_global != global_directory_map.end();

    if(minion_found && !shire_found)
    {
       LOG_ALL_MINIONS(FTL, "mem_directory::read minion entry found and shire entry not found for addr %llX when doing write\n", (long long unsigned int) address);
    }
    if(shire_found && !global_found)
    {
       LOG_ALL_MINIONS(FTL, "mem_directory::read shire entry found and global entry not found for addr %llX when doing write\n", (long long unsigned int) address);
    }

    // Info
    MD_LOG(address, minion, printf("mem_directory::read => addr %016llX, location %i, shire id %i, minion_id %i, thread_id %i, found (%i, %i, %i)\n",
        (long long unsigned int) address, location, shire_id, minion_id, thread_id,
        global_found, shire_found, minion_found));

    uint32_t l1_set = dcache_index(address, l1_minion_control[minion], (minion << 1) | thread_id, EMU_THREADS_PER_MINION);
    // Marks the L1 accessed sets
    if(location == COH_MINION)
    {
        for(uint32_t way = 0; way < L1D_NUM_WAYS; way++)
        {
            MD_LOG(address, minion, printf("mem_directory::read => setting l1 set %i and way %i\n", l1_set, way));
            l1_minion_valid[minion][l1_set][way] = true;
        }
    }

    // Get timestamps
    uint64_t it_minion_time_stamp = -1;
    uint64_t it_shire_time_stamp  = -1;
    uint64_t it_global_time_stamp = -1;
    if(minion_found) it_minion_time_stamp = it_minion->second.time_stamp;
    if(shire_found)  it_shire_time_stamp  = it_shire->second.time_stamp;
    if(global_found) it_global_time_stamp = it_global->second.time_stamp;

    // Checks if access is coherent
    bool coherent = true;

    // Minion access must be coherent
    coherent &= (it_minion == minion_directory_map[minion].end())                                     // Data not in minion
             || ((l1_minion_control[minion] == 0) && (location == COH_MINION))                        // Cache is shared and access is minion
             || (!it_minion->second.thread_mask_write[thread_id^1] && (location == COH_MINION))       // Access is minion and other thread is not dirty
             || (!it_minion->second.thread_mask_write[0] && !it_minion->second.thread_mask_write[1]); // Data is not dirty and accessing beyond minion

    // Shire access must be coherent
    coherent &= (it_shire == shire_directory_map[shire_id].end())                                                               // Data not in shire
             || ((it_shire->second.l2_dirty_minion_id == 255)       && !it_shire->second.cb_dirty)                              // Hasn't been written by minion (clean in shire or dirty in L2) or in CB
             || ((it_shire->second.l2_dirty_minion_id == minion_id) &&  it_shire->second.l2_dirty && (location == COH_MINION)); // If dirty, was the same minion

    // Global access must be coherent
    coherent &= (it_global == global_directory_map.end())                                                                     // Not in global
             || ((it_global->second.l2_dirty_shire_id == 255) && !it_global->second.cb_dirty)                                 // Still clean
             || ((it_global->second.l2_dirty_shire_id == shire_id) && ((location == COH_MINION) || (location == COH_SHIRE))); // Reading dirty inside shire

    // Time stamp is coherent (minion has new version than shire, shire newer than global)
    coherent &= (it_minion_time_stamp >= it_shire_time_stamp) && (it_shire_time_stamp >= it_global_time_stamp);

    if(!coherent) dump_state(it_global, it_shire, it_minion, shire_id, minion);

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
            new_entry.thread_set      [adjusted_thread_id] = l1_set;
            new_entry.time_stamp                           = shire_found  ? it_shire_time_stamp
                                                           : global_found ? it_global_time_stamp
                                                           :                global_time_stamp;

            minion_directory_map[minion].insert(minion_directory_map_t::value_type(address, new_entry));
            MD_LOG(address, minion, printf("mem_directory::read insert minion directory => addr %016llX, shire_id %i, minion_id %i, thread_id: %i, mask_write: 0x%X, mask_read: 0x%X, set: 0x%X, time_stamp: %llu\n",
                  (long long unsigned int) address, shire_id, minion_id, thread_id, (int32_t) bool_array_to_int(new_entry.thread_mask_write, EMU_THREADS_PER_MINION),
                  (int32_t) bool_array_to_int(new_entry.thread_mask_read, EMU_THREADS_PER_MINION), new_entry.thread_set[adjusted_thread_id], (long long unsigned int) new_entry.time_stamp));
        }
        // Update
        else
        {
            it_minion->second.thread_mask_read[adjusted_thread_id] = true;
            it_minion->second.thread_set      [adjusted_thread_id] = l1_set;
            MD_LOG(address, minion, printf("mem_directory::read update minion directory => addr %016llX, shire_id %i, minion_id %i, thread_id: %i, mask_write: 0x%X, mask_read: 0x%X, set: 0x%X, time_stamp: %llu\n",
                  (long long unsigned int) address, shire_id, minion_id, thread_id, (int32_t) bool_array_to_int(it_minion->second.thread_mask_write, EMU_THREADS_PER_MINION),
                  (int32_t) bool_array_to_int(it_minion->second.thread_mask_read, EMU_THREADS_PER_MINION), it_minion->second.thread_set[adjusted_thread_id], (long long unsigned int) it_minion->second.time_stamp));
        }
    }

    // Update shire contents
    if(update_shire)
    {
        shire_mem_info_t new_value;

        // L2 dirty
        new_value.l2                 = true;
        new_value.l2_dirty           = false;
        new_value.l2_dirty_minion_id = 255;
        new_value.time_stamp         = global_found ? it_global_time_stamp
                                     :                global_time_stamp;

        // CB dirty
        new_value.cb_dirty = false;
        for(uint32_t i = 0; i < 4; i++)
            new_value.cb_dirty_quarter[i] = false;

        // Minion mask
        for(uint32_t i = 0; i < EMU_MINIONS_PER_SHIRE; i++)
            new_value.minion_mask[i] = false;
        new_value.minion_mask[minion_id] = (location == COH_MINION);

        // Not present, insert
        if(it_shire == shire_directory_map[shire_id].end())
        {
            shire_directory_map[shire_id].insert(shire_directory_map_t::value_type(address, new_value));
            MD_LOG(address, minion, printf("mem_directory::read insert shire directory => addr %016llX, shire_id %i, l2: %i, l2_dirty: %i, l2_dirty_minion_id: %i, cb_dirty: %i, cb_quarter: 0x%X, minion_mask: 0x%X\n",
                  (long long unsigned int) address, shire_id, new_value.l2, new_value.l2_dirty, new_value.l2_dirty_minion_id,
                  new_value.cb_dirty, (int32_t) bool_array_to_int(new_value.cb_dirty_quarter, 4), (int32_t) bool_array_to_int(new_value.minion_mask, 32)));
        }
        // Update
        else
        {
            it_shire->second.l2                     = new_value.l2;
            it_shire->second.minion_mask[minion_id] = new_value.minion_mask[minion_id];
            MD_LOG(address, minion, printf("mem_directory::read update shire directory => addr %016llX, shire_id %i, l2: %i, l2_dirty: %i, l2_dirty_minion_id: %i, cb_dirty: %i, cb_quarter: 0x%X, minion_mask: 0x%X\n",
                  (long long unsigned int) address, shire_id, it_shire->second.l2, it_shire->second.l2_dirty, it_shire->second.l2_dirty_minion_id,
                  it_shire->second.cb_dirty, (int32_t) bool_array_to_int(it_shire->second.cb_dirty_quarter, 4), (int32_t) bool_array_to_int(new_value.minion_mask, 32)));
        }
    }

    // Update global contents
    if(update_global)
    {
        global_mem_info_t new_value;

        new_value.time_stamp        = global_time_stamp;
        new_value.cb_dirty          = false;
        new_value.l2_dirty_shire_id = 255;
        for(int quarter = 0; quarter < 4; quarter++)
          new_value.cb_dirty_quarter[quarter] = false;
        for(uint32_t shire = 0; shire < EMU_NUM_SHIRES; shire++)
            new_value.shire_mask[shire] = false;
        new_value.shire_mask[shire_id] = true;

        // Not present, insert
        if(it_global == global_directory_map.end())
        {
            global_directory_map.insert(global_directory_map_t::value_type(address, new_value));
            MD_LOG(address, minion, printf("mem_directory::read insert global directory => addr %016llX\n", (long long unsigned int) address));
        }
        // Update
        else
        {
            it_global->second.shire_mask[shire_id] = new_value.shire_mask[shire_id];
            MD_LOG(address, minion, printf("mem_directory::read update global directory => addr %016llX\n", (long long unsigned int) address));
        }
    }

    return coherent;
}

/*! \brief Directory update for a memory block evict
 *
 *  This function updates the memory line in the directory and then predicts if there may have been incoherency
 *  return true if coherent, false in cc
 */

bool mem_directory::evict_va(uint64_t address, op_location_t location, uint32_t shire_id, uint32_t minion_id, uint32_t thread_id, bool * dirty_evict)
{
    uint32_t minion = shire_id * EMU_MINIONS_PER_SHIRE + minion_id;

    global_directory_map_t::iterator it_global;
    shire_directory_map_t::iterator  it_shire;
    minion_directory_map_t::iterator it_minion;

    // Looks for the entry at different levels
    it_global = global_directory_map.find(address);
    it_shire  = shire_directory_map[shire_id].find(address);
    it_minion = minion_directory_map[minion].find(address);

    // Assertions
    bool minion_found = it_minion != minion_directory_map[minion].end();
    bool shire_found  = it_shire  != shire_directory_map[shire_id].end();
    bool global_found = it_global != global_directory_map.end();
    if(minion_found && !shire_found)
    {
       LOG_ALL_MINIONS(FTL, "mem_directory::evict_va minion entry found and shire entry not found for addr %llX when doing write\n", (long long unsigned int) address);
    }
    if(shire_found && !global_found)
    {
       LOG_ALL_MINIONS(FTL, "mem_directory::evict_va shire entry found and global entry not found for addr %llX when doing write\n", (long long unsigned int) address);
    }

    MD_LOG(address, minion, printf("mem_directory::evict_va => addr %016llX, location %i, shire id %i, minion_id %i, thread_id %i, found (%i, %i, %i)\n",
        (long long unsigned int) address, location, shire_id, minion_id, thread_id,
        global_found, shire_found, minion_found));

    // Checks if access is coherent
    bool coherent = true;

    // Minion access must be coherent
    coherent &= (it_minion == minion_directory_map[minion].end())  // Not present
             || (l1_minion_control[minion] == 0)                   // Shared mode
             || !it_minion->second.thread_mask_write[thread_id^1]; // Other thread is not dirty

    // Shire access must be coherent (not in shire, or not dirty or already dirty in same minion
    coherent &= (it_shire  == shire_directory_map[shire_id].end())                                 // Data not in shire
             || ((it_shire->second.l2_dirty_minion_id == 255) && !it_shire->second.cb_dirty)       // Data is not dirty and not in CB
             || ((it_shire->second.l2_dirty_minion_id == minion_id) && it_shire->second.l2_dirty); // Minion doing evict is the one with dirty data

    // Global access must be coherent (not found or dirty in same shire)
    coherent &= (it_global == global_directory_map.end())                                     // Data not in global
             || ((it_global->second.l2_dirty_shire_id == 255) && !it_global->second.cb_dirty) // Data is clean
             || (it_global->second.l2_dirty_shire_id == shire_id);                            // Evicting from the shire with dirty data

    if(!coherent) dump_state(it_global, it_shire, it_minion, shire_id, minion);

    bool update_minion = ((location == COH_SHIRE) || (location == COH_GLOBAL)) && (it_minion != minion_directory_map[minion].end());
    bool update_shire  = (location == COH_GLOBAL) && (it_shire  != shire_directory_map[shire_id].end());

    // First sets the evict VA as not dirty
    * dirty_evict = false;

    // Updates the minion directory
    if(update_minion)
    {
        uint32_t adjusted_thread_id = (l1_minion_control[minion] == 0) ? 0 : thread_id;
        MD_LOG(address, minion, printf("mem_directory::evict_va update minion directory => addr %016llX, shire_id %i, minion_id %i\n", (long long unsigned int) address, shire_id, minion_id));

        // Gets if dirty data from L1 is dirty
        * dirty_evict |= it_minion->second.thread_mask_write[adjusted_thread_id];

        // Clears status bits
        it_minion->second.thread_mask_read [adjusted_thread_id] = false;
        it_minion->second.thread_mask_write[adjusted_thread_id] = false;

        MD_LOG(address, minion, printf("mem_directory::evict_va update minion directory => addr %016llX, shire_id %i, minion_id %i, thread_id: %i, mask_write: 0x%X, mask_read: 0x%X, set: 0x%X, time_stamp: %llu\n",
              (long long unsigned int) address, shire_id, minion_id, thread_id, (int32_t) bool_array_to_int(it_minion->second.thread_mask_write, EMU_THREADS_PER_MINION),
              (int32_t) bool_array_to_int(it_minion->second.thread_mask_read, EMU_THREADS_PER_MINION), it_minion->second.thread_set[adjusted_thread_id], (long long unsigned int) it_minion->second.time_stamp));

        // Checks if all sets are clear
        bool all_clear = true;
        for(uint32_t thread = 0; thread < EMU_THREADS_PER_MINION; thread++)
        {
            all_clear &= (!it_minion->second.thread_mask_read[thread] && !it_minion->second.thread_mask_write[thread]);
        }

        // Line is no longer in minion, update shire state and remove in minion
        if(all_clear)
        {
            MD_LOG(address, minion, printf("mem_directory::evict_va => line no longer in minion\n"));

            // Clears the minion dirty flag in shire, must have an entry
            if(it_shire == shire_directory_map[shire_id].end())
            {
                LOG_ALL_MINIONS(FTL, "Should have found address %llX in shire when doing evict_va\n", (long long unsigned int) address);
            }

            // Clears the minion dirty flag in shire, must have an entry
            if((it_shire->second.l2_dirty_minion_id != 255) && (it_shire->second.l2_dirty_minion_id != minion_id))
            {
                LOG_ALL_MINIONS(FTL, "\t(Coherency EvictVA Hazard wrong minion) addr=%llX, location=%d, shire_id=%u, minion_id=%u, thread_id=%u, l2_dirty_minion_id=%u\n", (long long unsigned int) address, location, shire_id, minion_id, thread_id, it_shire->second.l2_dirty_minion_id);
            }
            it_shire->second.l2_dirty_minion_id     = 255;
            it_shire->second.minion_mask[minion_id] = false;
            if(it_shire->second.time_stamp < it_minion->second.time_stamp)
            {
                it_shire->second.time_stamp = it_minion->second.time_stamp;
            }

            MD_LOG(address, minion, printf("mem_directory::evict_va update shire directory => addr %016llX, shire_id %i, l2: %i, l2_dirty: %i, l2_dirty_minion_id: %i, cb_dirty: %i, cb_quarter: 0x%X, minion_mask: 0x%X\n",
                  (long long unsigned int) address, shire_id, it_shire->second.l2, it_shire->second.l2_dirty, it_shire->second.l2_dirty_minion_id,
                  it_shire->second.cb_dirty, (int32_t) bool_array_to_int(it_shire->second.cb_dirty_quarter, 4), (int32_t) bool_array_to_int(it_shire->second.minion_mask, 32)));

            // Remove minion entry
            MD_LOG(address, minion, printf("mem_directory::evict_va remove minion directory => addr %016llX, shire_id %i, minion_id %i\n", (long long unsigned int) address, shire_id, minion_id));
            minion_directory_map[minion].erase(it_minion);
        }
    }

    // Update shire contents
    if(update_shire)
    {
        // CBs should be drained with ESRs, not evict_va
        if(it_shire->second.cb_dirty)
        {
            LOG_ALL_MINIONS(FTL, "\t(Coherency EvictVA Hazard CB evict) addr=%llX, location=%d, shire_id=%u, minion_id=%u, thread_id=%u", (long long unsigned int) address, location, shire_id, minion_id, thread_id);
        }

        // Gets if dirty data from L1 is dirty
        * dirty_evict |= it_shire->second.l2_dirty;

        // Checks if all sets are clear
        bool all_clear = !it_shire->second.cb_dirty;
        for(uint32_t i = 0; i < EMU_MINIONS_PER_SHIRE; i++)
            all_clear &= (!it_shire->second.minion_mask[i]);
        it_shire->second.l2 = false;
        it_shire->second.l2_dirty = false;

        MD_LOG(address, minion, printf("mem_directory::evict_va update shire directory => addr %016llX, shire_id %i, l2: %i, l2_dirty: %i, l2_dirty_minion_id: %i, cb_dirty: %i, cb_quarter: 0x%X, minion_mask: 0x%X\n",
              (long long unsigned int) address, shire_id, it_shire->second.l2, it_shire->second.l2_dirty, it_shire->second.l2_dirty_minion_id,
              it_shire->second.cb_dirty, (int32_t) bool_array_to_int(it_shire->second.cb_dirty_quarter, 4), (int32_t) bool_array_to_int(it_shire->second.minion_mask, 32)));

        // Line is no longer in shire, update global state and remove in shire
        if(all_clear)
        {
            // Remove the dirty in global entry
            if(it_global == global_directory_map.end())
            {
                throw std::invalid_argument("Should have found address in global");
            }

            // Clears the minion dirty flag in global, must have an entry
            if((it_global->second.l2_dirty_shire_id != 255) && (it_global->second.l2_dirty_shire_id != shire_id))
            {
                LOG_ALL_MINIONS(FTL, "\t(Coherency EvictVA Hazard wrong shire) addr=%llX, location=%d, shire_id=%u, minion_id=%u, thread_id=%u, l2_dirty_shire_id=%u", (long long unsigned int) address, location, shire_id, minion_id, thread_id, it_global->second.l2_dirty_shire_id);
            }
            it_global->second.l2_dirty_shire_id    = 255;
            it_global->second.shire_mask[shire_id] = false;
            if(it_global->second.time_stamp < it_shire->second.time_stamp)
            {
                it_global->second.time_stamp = it_shire->second.time_stamp;
            }
        }
    }

    // Check if need to remove shire entry
    if(it_shire != shire_directory_map[shire_id].end() && (location == COH_GLOBAL))
    {
        bool all_clear = !it_shire->second.l2 && !it_shire->second.l2_dirty && !it_shire->second.cb_dirty;
        for(uint32_t i = 0; i < EMU_MINIONS_PER_SHIRE; i++)
            all_clear &= (it_shire->second.minion_mask[i] == false);

        if(all_clear)
        {
            MD_LOG(address, minion, printf("mem_directory::evict_va remove shire directory => addr %016llX, shire_id %i\n", (long long unsigned int) address, shire_id));
            shire_directory_map[shire_id].erase(it_shire);
        }
    }

    // Check if need to remove global entry
    if(it_global != global_directory_map.end() && (location == COH_GLOBAL))
    {
        bool all_clear = true;
        for(uint32_t shire = 0; shire < EMU_NUM_SHIRES; shire++)
            all_clear &= (it_global->second.shire_mask[shire] == false);

        if(all_clear)
        {
            MD_LOG(address, minion, printf("mem_directory::evict_va remove global directory => addr %016llX\n", (long long unsigned int) address));
            global_directory_map.erase(it_global);
        }
    }

    return coherent;
}

// Private function that clears a set of a minion l1
void mem_directory::l1_clear_set(uint32_t shire_id, uint32_t minion_id, uint32_t set)
{
    uint32_t minion = shire_id * EMU_MINIONS_PER_SHIRE + minion_id;
    MD_LOG(0, minion, printf("mem_directory::l1_clear_set => clearing set %i of minion %i with size %i\n", set, minion, (int) minion_directory_map[minion].size()));

    // Runs through all elements that match in same set
    minion_directory_map_t::iterator it_minion = minion_directory_map[minion].begin();

    while(it_minion != minion_directory_map[minion].end())
    {
        uint64_t addr = it_minion->first;
        MD_LOG(addr, minion, printf("mem_directory::l1_clear_set => addr %016llX belongs to minion\n", (long long unsigned int) addr));

        // Clears the valids if set match
        for(uint32_t thread = 0; thread < EMU_THREADS_PER_MINION; thread++)
        {
            if(it_minion->second.thread_set[thread] == set)
            {
                it_minion->second.thread_mask_write[thread] = false;
                it_minion->second.thread_mask_read [thread] = false;
            }
        }

        MD_LOG(addr, minion, printf("mem_directory::l1_clear_set update minion directory => addr %016llX, shire_id %i, minion_id %i, mask_write: 0x%X, mask_read: 0x%X, time_stamp: %llu\n",
              (long long unsigned int) addr, shire_id, minion_id, (int32_t) bool_array_to_int(it_minion->second.thread_mask_write, EMU_THREADS_PER_MINION),
              (int32_t) bool_array_to_int(it_minion->second.thread_mask_read, EMU_THREADS_PER_MINION), (long long unsigned int) it_minion->second.time_stamp));

        // Checks if all sets are clear
        bool all_clear = true;
        for(uint32_t thread = 0; thread < EMU_THREADS_PER_MINION; thread++)
        {
            all_clear &= (!it_minion->second.thread_mask_read[thread] && !it_minion->second.thread_mask_write[thread]);
            MD_LOG(addr, minion, printf("mem_directory::l1_clear_set => addr %016llX belongs to minion, thread %i, read %i, write %i, set %i\n", (long long unsigned int) addr, thread, it_minion->second.thread_mask_read[thread], it_minion->second.thread_mask_write[thread], it_minion->second.thread_set[thread]));
        }

        // Line is no longer in minion, update shire state and remove in minion
        if(all_clear)
        {
            // Clears the minion dirty flag in shire
            shire_directory_map_t::iterator it_shire;
            it_shire = shire_directory_map[shire_id].find(addr);

            // Must have an entry
            if(it_shire == shire_directory_map[shire_id].end())
            {
                LOG_ALL_MINIONS(FTL, "Should have found address %llX in shire when doing l1_clear_set\n", (long long unsigned int) addr);
            }

            it_shire->second.l2_dirty_minion_id     = 255;
            it_shire->second.minion_mask[minion_id] = false;
            if(it_shire->second.time_stamp < it_minion->second.time_stamp)
            {
                it_shire->second.time_stamp = it_minion->second.time_stamp;
            }

            MD_LOG(addr, minion, printf("mem_directory::l1_clear_set update shire directory => addr %016llX, shire_id %i, l2: %i, l2_dirty: %i, l2_dirty_minion_id: %i, cb_dirty: %i, cb_quarter: 0x%X, minion_mask: 0x%X\n",
                  (long long unsigned int) addr, shire_id, it_shire->second.l2, it_shire->second.l2_dirty, it_shire->second.l2_dirty_minion_id,
                  it_shire->second.cb_dirty, (int32_t) bool_array_to_int(it_shire->second.cb_dirty_quarter, 4), (int32_t) bool_array_to_int(it_shire->second.minion_mask, 32)));

            // Remove minion entry
            MD_LOG(addr, minion, printf("mem_directory::l1_clear_set remove minion directory => addr %016llX, shire_id %i, minion_id %i\n", (long long unsigned int) addr, shire_id, minion_id));
            minion_directory_map_t::iterator it_orig = it_minion;
            it_minion++;
            minion_directory_map[minion].erase(it_orig);

            // Checks if need to remove shire entry
            all_clear = !it_shire->second.l2 && !it_shire->second.l2_dirty && !it_shire->second.cb_dirty;
            for(uint32_t i = 0; i < EMU_MINIONS_PER_SHIRE; i++)
                all_clear &= !it_shire->second.minion_mask[i];

            if(all_clear)
            {
                // Removes entry from shire
                MD_LOG(addr, 0xFFFFFFFF, printf("mem_directory::l1_clear_set remove shire directory => addr %016llX, shire_id %i\n", (long long unsigned int) addr, shire_id));
                shire_directory_map[shire_id].erase(it_shire);

                // Clears global
                global_directory_map_t::iterator it_global;
                it_global = global_directory_map.find(addr);

                // Must have an entry
                if(it_global == global_directory_map.end())
                {
                    LOG_ALL_MINIONS(FTL, "Should have found address %llX in global when doing l1_clear_set\n", (long long unsigned int) addr);
                }

                it_global->second.shire_mask[shire_id] = false;

                // Checks if no shires have the cacheline
                all_clear = true;
                for(uint32_t shire = 0; shire < EMU_NUM_SHIRES; shire++)
                    all_clear &= (it_global->second.shire_mask[shire] == false);

                // Remove from global
                if(all_clear)
                {
                    MD_LOG(addr, 0xFFFFFFFF, printf("mem_directory::l1_clear_set remove global directory => addr %016llX\n", (long long unsigned int) addr));
                    global_directory_map.erase(it_global);
                }
            }
        }
        else
        {
            it_minion++;
        }
    }
}

// Private function that dumps coherent state
void mem_directory::dump_state(global_directory_map_t::iterator it_global, shire_directory_map_t::iterator it_shire, minion_directory_map_t::iterator it_minion, uint32_t shire_id, uint32_t minion)
{
    if(it_global != global_directory_map.end())
    {
        MD_LOG(0, minion, printf("Dumping global state\n"));
        MD_LOG(0, minion, printf("  time_stamp: %llu\n", (long long unsigned int) it_global->second.time_stamp));
        MD_LOG(0, minion, printf("  l2_dirty_shire_id: %i\n", it_global->second.l2_dirty_shire_id));
        MD_LOG(0, minion, printf("  cb_dirty: %i\n", it_global->second.cb_dirty));
        MD_LOG(0, minion, printf("  cb_dirty_quarter: %i%i%i%i\n", it_global->second.cb_dirty_quarter[3], it_global->second.cb_dirty_quarter[2], it_global->second.cb_dirty_quarter[1], it_global->second.cb_dirty_quarter[0]));
        for(uint32_t shire = 0; shire < EMU_NUM_SHIRES; shire++)
        {
            MD_LOG(0, minion, printf("  shire_mask[%i] = %i\n", shire, it_global->second.shire_mask[shire]));
        }
    }
    if(it_shire != shire_directory_map[shire_id].end())
    {
        MD_LOG(0, minion, printf("Dumping shire state\n"));
        MD_LOG(0, minion, printf("  time_stamp: %llu\n", (long long unsigned int) it_shire->second.time_stamp));
        MD_LOG(0, minion, printf("  l2: %i\n", it_shire->second.l2));
        MD_LOG(0, minion, printf("  l2_dirty: %i\n", it_shire->second.l2_dirty));
        MD_LOG(0, minion, printf("  l2_dirty_minion_id: %i\n", it_shire->second.l2_dirty_minion_id));
        MD_LOG(0, minion, printf("  cb_dirty: %i\n", it_shire->second.cb_dirty));
        MD_LOG(0, minion, printf("  cb_dirty_quarter: %i%i%i%i\n", it_shire->second.cb_dirty_quarter[3], it_shire->second.cb_dirty_quarter[2], it_shire->second.cb_dirty_quarter[1], it_shire->second.cb_dirty_quarter[0]));
        for(uint32_t i = 0; i < EMU_MINIONS_PER_SHIRE; i++)
        {
            MD_LOG(0, minion, printf("  minion_mask[%i] = %i\n", i, it_shire->second.minion_mask[i]));
        }
    }
    if(it_minion != minion_directory_map[minion].end())
    {
        MD_LOG(0, minion, printf("Dumping minion state\n"));
        MD_LOG(0, minion, printf("  time_stamp: %llu\n", (long long unsigned int) it_minion->second.time_stamp));
        for(uint32_t thread = 0; thread < EMU_THREADS_PER_MINION; thread++)
        {
              MD_LOG(0, minion, printf("  thread_mask_write[%i] = %i\n", thread, it_minion->second.thread_mask_write[thread]));
              MD_LOG(0, minion, printf("  thread_mask_read[%i] = %i\n", thread, it_minion->second.thread_mask_read[thread]));
              MD_LOG(0, minion, printf("  thread_set[%i] = %i\n", thread, it_minion->second.thread_set[thread]));
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
bool mem_directory::access(uint64_t addr, mem_access_type macc, cacheop_type cop, uint32_t current_thread, size_t size, mreg_t mask)
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
    case Mem_Access_TxLoad:
        location = COH_SHIRE;
        break;
    case Mem_Access_TxLoadL2Scp:
        location = COH_GLOBAL;
        break;
    case Mem_Access_Prefetch:
        if     (cop == CacheOp_PrefetchL1)
            location = COH_MINION;
        else if(cop == CacheOp_PrefetchL2)
            location = COH_SHIRE;
        else if(cop == CacheOp_PrefetchL3)
            location = COH_GLOBAL;
        else
            LOG_ALL_MINIONS(FTL, "Invalid CacheOp type %i for prefetch!!\n", (int) cop);
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
    case Mem_Access_TxStore:
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
        else
            LOG_ALL_MINIONS(FTL, "CacheOp %i not supported yet!!\n", (int) cop);
        if((cop == CacheOp_EvictL3) || (cop == CacheOp_EvictDDR))
            location = COH_GLOBAL;
        else if((cop == CacheOp_EvictL2))
            location = COH_SHIRE;
        break;
    case Mem_Access_Fetch: // Load instruction from memory. This must not be included in the directory. Do nothing
        return true;
        break;
    case Mem_Access_PTW:     // Page table walker access. Must not be invoked. Fail if so.
        throw std::invalid_argument("unexpected operation PTW");
        break;
    }

    // Adjusts the size based on mask
    int start = 0;
    if     (size == 32) start = 7;
    else if(size == 16) start = 3;
    else if(size == 8)  start = 1;
    else                start = 0;
    for(int i = start; i >= 0; i--)
    {
        if(mask[i] == 0)
            size -= 4;
        else
            break;
    }

    bool coherent;
    if(operation & 1)
    {
        coherent = read(addr & ~0x3FULL, location, shire_id, minion_id, thread_id);

        if(!coherent)
        {
            LOG_ALL_MINIONS(FTL, "\t(Coherency Read Hazard) addr=%llX, location=%d, shire_id=%u, minion_id=%u, thread_id=%u", (long long unsigned int) addr & ~0x3FULL, location, shire_id, minion_id, thread_id);
            return false;
        }

        if(((addr & 0x3FULL) + size) > 64)
        {
            coherent = read((addr & ~0x3FULL) + 64, location, shire_id, minion_id, thread_id);

            if(!coherent)
            {
                LOG_ALL_MINIONS(FTL, "\t(Coherency Read Hazard Unaligned Access) addr=%llX, location=%d, shire_id=%u, minion_id=%u, thread_id=%u", (long long unsigned int) (addr & ~0x3FULL) + 64, location, shire_id, minion_id, thread_id);
                return false;
            }
        }
    }
    // EvictVA must be done before write, as we don't allow writes to higher levels in
    // the hierarchy if lower levels are still dirty
    // Doing first EvictVA cleans the dirty of the hierarchy and then the write sets
    // correct final dirty status
    bool dirty_evict_va = true;
    if(operation & 4)
    {
        coherent = evict_va(addr & ~0x3FULL, location, shire_id, minion_id, thread_id, &dirty_evict_va);

        if(!coherent)
        {
            LOG_ALL_MINIONS(FTL, "\t(Coherency EvictVA Hazard) addr=%llX, location=%d, shire_id=%u, minion_id=%u, thread_id=%u", (long long unsigned int) addr & ~0x3FULL, location, shire_id, minion_id, thread_id);
            return false;
        }
    }
    // Write operation
    // For evict VA only do writes if it is a dirty evict
    if((operation & 2) && dirty_evict_va)
    {
        coherent = write(addr & ~0x3FULL, location, shire_id, minion_id, thread_id, size, (addr & 0x30ULL) >> 4);
        
        if(!coherent)
        {
            LOG_ALL_MINIONS(FTL, "\t(Coherency Write Hazard) addr=%llX, location=%d, shire_id=%u, minion_id=%u, thread_id=%u", (long long unsigned int) addr & ~0x3FULL, location, shire_id, minion_id, thread_id);
            return false;
        }

        if(((addr & 0x3FULL) + size) > 64)
        {
            coherent = write((addr & ~0x3FULL) + 64, location, shire_id, minion_id, thread_id, size, (addr & 0x30ULL) >> 4);

            if(!coherent)
            {
                LOG_ALL_MINIONS(FTL, "\t(Coherency Write Hazard Unaligned Access) addr=%llX, location=%d, shire_id=%u, minion_id=%u, thread_id=%u", (long long unsigned int) (addr & ~0x3FULL) + 64, location, shire_id, minion_id, thread_id);
                return false;
            }
        }
    }
    return true;
}

// Public function called when there's an ESR write that drains an L2 coallescing buffer
void mem_directory::cb_drain(uint32_t shire_id, uint32_t cache_bank)
{
    MD_LOG(0, 0, printf("mem_directory::cb_drain => shire_id %i, bank %i\n", shire_id, cache_bank));

    shire_directory_map_t::iterator it_shire;

    // Goes through all entries and evicts the one in same cache bank
    it_shire  = shire_directory_map[shire_id].begin();
    while(it_shire != shire_directory_map[shire_id].end())
    {
        // Same set and CB is dirty
        uint64_t addr = it_shire->first;
        bool bank = (((addr & 0xC0) >> 6) == cache_bank);
        if(bank && it_shire->second.cb_dirty)
        {
            // Looks the global entry
            global_directory_map_t::iterator it_global;
            it_global = global_directory_map.find(addr);

            // Remove the dirty in global entry
            if(it_global == global_directory_map.end())
            {
                throw std::invalid_argument("Should have found address in global");
            }

            // Clears the CB dirty bits in global and shire
            for(int quarter = 0; quarter < 4; quarter++)
            {
                if(it_shire->second.cb_dirty_quarter[quarter])
                {
                    it_global->second.cb_dirty_quarter[quarter] = false;
                    it_shire->second.cb_dirty_quarter[quarter] = false;
                }
            }
            // Update time stamp if newer
            if(it_global->second.time_stamp < it_shire->second.time_stamp)
            {
                it_global->second.time_stamp = it_shire->second.time_stamp;
            }

            // Clears the CB dirty bit in global (only if all quarters) and shire (always)
            it_shire->second.cb_dirty = false;
            MD_LOG(addr, 0xFFFFFFFF, printf("mem_directory::cb_drain update shire directory => addr %016llX, shire_id %i, l2: %i, l2_dirty: %i, l2_dirty_minion_id: %i, cb_dirty: %i, cb_quarter: 0x%X, minion_mask: 0x%X\n",
                  (long long unsigned int) addr, shire_id, it_shire->second.l2, it_shire->second.l2_dirty, it_shire->second.l2_dirty_minion_id,
                  it_shire->second.cb_dirty, (int32_t) bool_array_to_int(it_shire->second.cb_dirty_quarter, 4), (int32_t) bool_array_to_int(it_shire->second.minion_mask, 32)));

            bool all_clear = true;
            for(int quarter = 0; quarter < 4; quarter++)
                all_clear &= !it_global->second.cb_dirty_quarter[quarter];

            if(all_clear)
                it_global->second.cb_dirty = false;

            // Checks if need to remove shire entry
            all_clear = !it_shire->second.l2 && !it_shire->second.l2_dirty && !it_shire->second.cb_dirty;
            for(uint32_t i = 0; i < EMU_MINIONS_PER_SHIRE; i++)
                all_clear &= !it_shire->second.minion_mask[i];

            if(all_clear)
            {
                // Removes entry from shire
                MD_LOG(addr, 0xFFFFFFFF, printf("mem_directory::cb_drain remove shire directory => addr %016llX, shire_id %i\n", (long long unsigned int) addr, shire_id));
                it_global->second.shire_mask[shire_id] = false;

                // Checks if no shires have the cacheline
                all_clear = true;
                for(uint32_t shire = 0; shire < EMU_NUM_SHIRES; shire++)
                    all_clear &= (it_global->second.shire_mask[shire] == false);

                // Shire removal done after global to guarantee data dependency in iterator
                shire_directory_map_t::iterator it_orig = it_shire;
                it_shire++;
                shire_directory_map[shire_id].erase(it_orig);

                // Remove from global
                if(all_clear)
                {
                    MD_LOG(addr, 0xFFFFFFFF, printf("mem_directory::cb_drain remove global directory => addr %016llX\n", (long long unsigned int) addr));
                    global_directory_map.erase(it_global);
                }
            }
            else
            {
                it_shire++;
            }
        }
        else
        {
            it_shire++;
        }
    }
}

// Public function called when there's an ESR write that flushes an L2 bank
void mem_directory::l2_flush(uint32_t shire_id, uint32_t cache_bank)
{
    MD_LOG(0, 0, printf("mem_directory::l2_flush => shire_id %i, bank %ir\n", shire_id, cache_bank));
    LOG_ALL_MINIONS(FTL, "L2 flush not implemented yet!!%s\n", "");

    // L2 flush drains CB as well
    cb_drain(shire_id, cache_bank);
}

// Public function called when there's an ESR write that evicts an L2 bank
void mem_directory::l2_evict(uint32_t shire_id, uint32_t cache_bank)
{
    MD_LOG(0, md_log_minion + 1, printf("mem_directory::l2_evict => shire_id %i, bank %i\n", shire_id, cache_bank));
    shire_directory_map_t::iterator it_shire = shire_directory_map[shire_id].begin();

    while(it_shire != shire_directory_map[shire_id].end())
    {
        uint64_t addr = it_shire->first;
        bool bank = (((addr & 0xC0) >> 6) == cache_bank);
        if(bank && !it_shire->second.cb_dirty)
        {
            MD_LOG(addr, md_log_minion + 1, printf("mem_directory::l2_evict => evicting addr %016llX, shire_id %i\n", (long long unsigned int) addr, shire_id));

            global_directory_map_t::iterator it_global;
            it_global = global_directory_map.find(addr);

            // Remove the dirty in global entry
            if(it_global == global_directory_map.end())
            {
                throw std::invalid_argument("Should have found address in global");
            }

            // If dirty and not in any minion, set to not dirty in L2
            if(it_shire->second.l2_dirty && (it_shire->second.l2_dirty_minion_id == 255))
            {
                it_global->second.l2_dirty_shire_id = 255;
                it_shire->second.l2_dirty           = false;
                it_global->second.time_stamp        = it_shire->second.time_stamp;
            }

            // Line is no present in l2
            it_shire->second.l2 = false;

            MD_LOG(addr, md_log_minion + 1, printf("mem_directory::l2_update update shire directory => addr %016llX, shire_id %i, l2: %i, l2_dirty: %i, l2_dirty_minion_id: %i, cb_dirty: %i, cb_quarter: 0x%X, minion_mask: 0x%X\n",
                  (long long unsigned int) addr, shire_id, it_shire->second.l2, it_shire->second.l2_dirty, it_shire->second.l2_dirty_minion_id,
                  it_shire->second.cb_dirty, (int32_t) bool_array_to_int(it_shire->second.cb_dirty_quarter, 4), (int32_t) bool_array_to_int(it_shire->second.minion_mask, 32)));

            // Checks if no minions have the line anymore
            bool all_clear = !it_shire->second.l2 && !it_shire->second.l2_dirty && !it_shire->second.cb_dirty;
            for(uint32_t i = 0; i < EMU_MINIONS_PER_SHIRE; i++)
                all_clear &= (it_shire->second.minion_mask[i] == false);

            // If no minion has the cacheline, remove from shire
            if(all_clear)
            {
                it_global->second.shire_mask[shire_id] = false;

                // Checks if no shires have the cacheline
                all_clear = true;
                for(uint32_t shire = 0; shire < EMU_NUM_SHIRES; shire++)
                    all_clear &= (it_global->second.shire_mask[shire] == false);

                // Remove from shire
                MD_LOG(addr, md_log_minion + 1, printf("mem_directory::l2_evict remove shire directory => addr %016llX, shire_id %i\n", (long long unsigned int) addr, shire_id));
                shire_directory_map_t::iterator it_orig = it_shire;
                it_shire++;
                shire_directory_map[shire_id].erase(it_orig);

                // Remove from global
                if(all_clear)
                {
                    MD_LOG(addr, md_log_minion + 1, printf("mem_directory::l2_evict remove global directory => addr %016llX\n", (long long unsigned int) addr));
                    global_directory_map.erase(it_global);
                }
            }
            else
            {
                it_shire++;
            }
        }
        else
        {
            it_shire++;
        }
    }

    // L2 evict drains CB as well
    cb_drain(shire_id, cache_bank);
}

// Public function called when code executed an evict SW
void mem_directory::l1_evict_sw(uint32_t shire_id, uint32_t minion_id, uint32_t set, uint32_t way)
{
    // Clears set and way
    uint32_t minion = shire_id * EMU_MINIONS_PER_SHIRE + minion_id;
    l1_minion_valid[minion][set][way] = false;

    MD_LOG(0, minion, printf("mem_directory::l1_evict_sw => shire_id %i, minion_id %i, set %i, way %i\n", shire_id, minion_id, set, way));

    // Checks if all ways of a set are clear
    bool all_clear = true;
    for(uint32_t it_way = 0; it_way < L1D_NUM_WAYS; it_way++)
    {
        all_clear &= (l1_minion_valid[minion][set][it_way] == false);
    }

    if(all_clear)
        l1_clear_set(shire_id, minion_id, set);
}

// Public function called when code executed a flush SW
void mem_directory::l1_flush_sw(uint32_t shire_id, uint32_t minion_id, uint32_t set, uint32_t way)
{
    // Clears set and way
    uint32_t minion = shire_id * EMU_MINIONS_PER_SHIRE + minion_id;
    l1_minion_valid[minion][set][way] = false;
    MD_LOG(0, minion, printf("mem_directory::l1_flush_sw => shire_id %i, minion_id %i, set %i, way %i\n", shire_id, minion_id, set, way));
    LOG_ALL_MINIONS(FTL, "L1 flush not implemented yet!!%s\n", "");

    // Checks if all ways of a set are clear
    bool all_clear = true;
    for(uint32_t it_way = 0; it_way < L1D_NUM_WAYS; it_way++)
    {
        all_clear &= (l1_minion_valid[minion][set][it_way] == false);
    }

    if(all_clear)
        l1_clear_set(shire_id, minion_id, set);
}

// Public function to notify changes in the cache control state
void mem_directory::mcache_control_up(uint32_t shire_id, uint32_t minion_id, uint32_t val)
{
    // Checks that all sets and ways are clear
    uint32_t minion = shire_id * EMU_MINIONS_PER_SHIRE + minion_id;
    MD_LOG(0, minion, printf("mem_directory::mcache_control_up => shire_id %i, minion_id %i, val %i\n", shire_id, minion_id, val));

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
        LOG_ALL_MINIONS(FTL, "\t(Coherency mcache control Hazard) shire_id=%u, minion_id=%u changed mcache control and cache not clear", shire_id, minion_id);
    }
    if(all_clear && minion_directory_map[minion].size())
    {
        throw std::invalid_argument("minion_directory_map should be empty!");
    }

    l1_minion_control[minion] = val;
}

