#ifndef _MEM_DIRECTORY_H_
#define _MEM_DIRECTORY_H_

#include <map>

#include "emu_defines.h"

typedef enum {MINION, SHIRE, GLOBAL} op_location_t;

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

  bool update(uint64_t address, op_location_t location, uint32_t shire_id, uint32_t minion_id);
  bool lookup(uint64_t address, op_location_t location, uint32_t shire_id, uint32_t minion_id);

public:

  bool access(uint64_t addr, mem_access_type macc, uint32_t current_thread);
};

#endif
