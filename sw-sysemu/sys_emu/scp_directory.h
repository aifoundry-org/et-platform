#ifndef _SCP_DIRECTORY_H_
#define _SCP_DIRECTORY_H_

// Global
#include <list>

// Local
#include "emu_defines.h"
#include "cache.h"

#define L2_SCP_ENTRIES 65536

// Info of an L2 Scp fill for a minion
struct l2_scp_fill_info_t
{
    uint32_t line;
    uint32_t id;
};

// L2 Scp status
enum l2_scp_status {
   L2Scp_Fill,
   L2Scp_Valid
};

struct shire_scp_info_t
{
    l2_scp_status                 l2_scp_line_status[L2_SCP_ENTRIES];  // Status of all the L2 scp entries
    uint64_t                      l2_scp_line_addr[L2_SCP_ENTRIES];    // Address that was prefetched to each L2 scp entries
    std::list<l2_scp_fill_info_t> l2_scp_fills[EMU_MINIONS_PER_SHIRE]; // For each minion in the shire list of outstanding l2 scp fills
};

// L1 Scp status
enum l1_scp_status {
   L1Scp_Invalid,
   L1Scp_Fill,
   L1Scp_Valid
};

struct minion_scp_info_t
{
    l1_scp_status l1_scp_line_status[L1_SCP_ENTRIES];
    uint32_t      l1_scp_line_id[L1_SCP_ENTRIES];
};

class scp_directory
{
  public:
    // Creator
    scp_directory();

    // Accessors
    void l1_scp_fill(uint32_t current_thread, uint32_t idx, uint32_t id);
    void l1_scp_wait(uint32_t current_thread, uint32_t id);
    void l1_scp_read(uint32_t current_thread, uint32_t idx);
    void l2_scp_fill(uint32_t current_thread, uint32_t idx, uint32_t id, uint64_t src_addr);
    void l2_scp_wait(uint32_t current_thread, uint32_t id);
    void l2_scp_read(uint32_t current_thread, uint64_t addr);

  private:
    // Directories per shire and per minion
    shire_scp_info_t  shire_scp_info[EMU_NUM_SHIRES];
    minion_scp_info_t minion_scp_info[EMU_NUM_MINIONS];
};

#endif

