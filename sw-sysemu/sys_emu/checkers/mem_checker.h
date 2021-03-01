/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef _MEM_CHECKER_H_
#define _MEM_CHECKER_H_

#include <map>

#include "emu_defines.h"
#include "cache.h"

typedef enum {COH_MINION, COH_SHIRE, COH_CB, COH_GLOBAL} op_location_t;

struct global_mem_info_t
{
    uint8_t  l2_dirty_shire_id;          // Which shire has dirty data
    bool     shire_mask[EMU_NUM_SHIRES]; // Which shires have the line in l1/l2
    bool     cb_dirty;                   // Data dirty in Coallescing Buffer
    bool     cb_dirty_quarter[4];        // Chunks of 128b that are dirty
    uint64_t time_stamp;                 // Time stamp of the value in the L3
    uint64_t latest_time_stamp;          // Time stamp of the latest written value
};

struct shire_mem_info_t
{
    bool     l2;                                 // Data in L2
    bool     l2_dirty;                           // Data dirty in L2
    uint8_t  l2_dirty_minion_id;                 // Which minion has the line dirty (255 is none)
    bool     cb_dirty;                           // Data dirty in Coallescing Buffer
    bool     cb_dirty_quarter[4];                // Chunks of 128b that are dirty
    bool     minion_mask[EMU_MINIONS_PER_SHIRE]; // Which minions have the line in l1
    uint64_t time_stamp;                         // Time stamp of the value
};

struct minion_mem_info_t
{
    bool     thread_mask_write[EMU_THREADS_PER_MINION]; // Which thread has written the line
    bool     thread_mask_read[EMU_THREADS_PER_MINION];  // Which thread has read the line
    uint8_t  thread_set[EMU_THREADS_PER_MINION];        // Set where each thread stored the line
    uint64_t time_stamp;                                // Time stamp of the value
};

typedef std::map<uint64_t, global_mem_info_t> global_directory_map_t;
typedef std::map<uint64_t, shire_mem_info_t>  shire_directory_map_t;
typedef std::map<uint64_t, minion_mem_info_t> minion_directory_map_t;

class mem_checker
{

private:

    // Directories global, per shire and per minion
    global_directory_map_t global_directory_map;
    shire_directory_map_t  shire_directory_map[EMU_NUM_SHIRES];
    minion_directory_map_t minion_directory_map[EMU_NUM_MINIONS];

    // Minion L1 status per set/way
    bool l1_minion_valid[EMU_NUM_MINIONS][L1D_NUM_SETS][L1D_NUM_WAYS];
    uint32_t l1_minion_control[EMU_NUM_MINIONS];

    // Write and read functions
    bool write   (uint64_t pc, uint64_t address, op_location_t location, uint32_t shire_id, uint32_t minion_id, uint32_t thread_id, size_t size, uint32_t cb_quarter);
    bool read    (uint64_t pc, uint64_t address, op_location_t location, uint32_t shire_id, uint32_t minion_id, uint32_t thread_id);
    bool evict_va(uint64_t pc, uint64_t address, op_location_t location, uint32_t shire_id, uint32_t minion_id, uint32_t thread_id, bool * dirty_evict);

    void l1_clear_set(uint32_t shire_id, uint32_t minion_id, uint32_t set, bool evict);

    bool is_minion_clean(minion_directory_map_t::iterator it_minion);
    bool is_minion_dirty(minion_directory_map_t::iterator it_minion);
    bool is_shire_clean (shire_directory_map_t::iterator  it_shire);
    bool is_shire_dirty (shire_directory_map_t::iterator  it_shire);
    bool is_global_clean(global_directory_map_t::iterator it_global);

    void dump_minion(minion_mem_info_t * minion_info, std::string func, std::string op, uint64_t addr, uint32_t shire_id, uint32_t minion_id, uint32_t thread_id);
    void dump_shire (shire_mem_info_t  * shire_info,  std::string func, std::string op, uint64_t addr, uint32_t shire_id, uint32_t minion);
    void dump_global(global_mem_info_t * global_info, std::string func, std::string op, uint64_t addr, uint32_t minion);

    void dump_state(global_directory_map_t::iterator it_global, shire_directory_map_t::iterator it_shire, minion_directory_map_t::iterator it_minion, uint32_t shire_id, uint32_t minion);

public:

    mem_checker();

    bool access(uint64_t pc, uint64_t addr, bemu::mem_access_type macc, bemu::cacheop_type cop, uint32_t thread, size_t size, bemu::mreg_t mask);
    void cb_drain(uint32_t shire_id, uint32_t cache_bank);
    void l2_flush(uint32_t shire_id, uint32_t cache_bank);
    void l2_evict(uint32_t shire_id, uint32_t cache_bank);
    void l1_evict_sw(uint32_t shire_id, uint32_t minion_id, uint32_t set, uint32_t way);
    void l1_flush_sw(uint32_t shire_id, uint32_t minion_id, uint32_t set, uint32_t way);
    void mcache_control_up(uint32_t shire_id, uint32_t minion_id, uint32_t val);
};

#endif

