#ifndef _MEM_DIRECTORY_H_
#define _MEM_DIRECTORY_H_

#include <map>

#include "emu_defines.h"
#include "cache.h"

typedef enum {COH_MINION, COH_SHIRE, COH_CB, COH_GLOBAL} op_location_t;

struct global_mem_info_t
{
    uint8_t shire_id_dirty;             // Which shire has dirty data
    bool    shire_mask[EMU_NUM_SHIRES]; // Which shires have the line in l1
};

struct shire_mem_info_t
{
    bool    l2_dirty;                           // Data dirty in L2
    bool    cb_dirty;                           // Data dirty in Coallescing Buffer
    uint8_t minion_id_dirty;                    // Which minion has the line dirty (255 is none)
    bool    minion_mask[EMU_MINIONS_PER_SHIRE]; // Which minions have the line in l1
};

struct minion_mem_info_t
{
    bool    thread_mask_write[EMU_THREADS_PER_MINION];
    bool    thread_mask_read[EMU_THREADS_PER_MINION];
    uint8_t thread_set[EMU_THREADS_PER_MINION];
};

typedef std::map<uint64_t, global_mem_info_t> global_directory_map_t;
typedef std::map<uint64_t, shire_mem_info_t>  shire_directory_map_t;
typedef std::map<uint64_t, minion_mem_info_t> minion_directory_map_t;

class mem_directory
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
  bool write   (uint64_t address, op_location_t location, uint32_t shire_id, uint32_t minion_id, uint32_t thread_id);
  bool read    (uint64_t address, op_location_t location, uint32_t shire_id, uint32_t minion_id, uint32_t thread_id);
  bool evict_va(uint64_t address, op_location_t location, uint32_t shire_id, uint32_t minion_id, uint32_t thread_id);

  void l1_clear_set(uint32_t minion, uint32_t set);

  void dump_state(global_directory_map_t::iterator it_global, shire_directory_map_t::iterator it_shire, minion_directory_map_t::iterator it_minion, uint32_t shire_id, uint32_t minion);

public:

  mem_directory();

  bool access(uint64_t addr, mem_access_type macc, cacheop_type cop, uint32_t current_thread);
  void cb_drain(uint32_t shire_id, uint32_t cache_bank);
  void l2_flush(uint32_t shire_id, uint32_t cache_bank);
  void l2_evict(uint32_t shire_id, uint32_t cache_bank);
  void l1_evict_sw(uint32_t shire_id, uint32_t minion_id, uint32_t set, uint32_t way);
  void l1_flush_sw(uint32_t shire_id, uint32_t minion_id, uint32_t set, uint32_t way);
  void mcache_control_up(uint32_t shire_id, uint32_t minion_id, uint32_t val);
};

#endif
