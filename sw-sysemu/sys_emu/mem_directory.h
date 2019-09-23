#ifndef _MEM_DIRECTORY_H_
#define _MEM_DIRECTORY_H_

#include <map>

#include "emu_defines.h"

typedef enum {COH_MINION, COH_SHIRE, COH_CB, COH_GLOBAL} op_location_t;

struct mem_info_t
{
    uint64_t        shire_mask;
    uint32_t        minion_mask;
    op_location_t   level;        // 0: minion, 1: shire, 2: global
};

typedef std::map<uint64_t, mem_info_t> directory_map_t;

class mem_directory
{

private:


  // map with address as key
  directory_map_t m_directory_map;

  bool update(uint64_t address, op_location_t location, uint32_t shire_id, uint32_t minion_id, cacheop_type cop);
  bool lookup(uint64_t address, op_location_t location, uint32_t shire_id, uint32_t minion_id);

public:

  bool access(uint64_t addr, mem_access_type macc, cacheop_type cop, uint32_t current_thread);
  void cb_drain(uint32_t shire_id, uint32_t cache_bank);
  void l2_flush(uint32_t shire_id, uint32_t cache_bank);
  void l2_evict(uint32_t shire_id, uint32_t cache_bank);
};

#endif
