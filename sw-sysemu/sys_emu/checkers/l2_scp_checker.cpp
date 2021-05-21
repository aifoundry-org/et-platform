/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

// Local defines
#include "l2_scp_checker.h"
#include "emu_gio.h"

// Logging variables and macros
uint32_t l2_scp_checker_log_shire  = 64;              // None by default
uint32_t l2_scp_checker_log_line   = 1 * 1024 * 1024; // None by default
uint32_t l2_scp_checker_log_minion = 2048;            // None by default

#define L2_SCP_CHECKER_LOG(shire, line, minion, cmd) \
  { if((shire == 0xFFFFFFFF) || (l2_scp_checker_log_shire == 0xFFFFFFFF) || (shire == l2_scp_checker_log_shire)) \
    { \
      if((line == 0xFFFFFFFF) || (l2_scp_checker_log_line == 0xFFFFFFFF) || (line == l2_scp_checker_log_line)) \
      { \
        if((minion == 0xFFFFFFFF) || (l2_scp_checker_log_minion == 0xFFFFFFFF) || (minion == l2_scp_checker_log_minion)) \
        { \
          cmd; \
        } \
      } \
    } \
  }

/*! \brief Scp directory constructor
 *
 *  This function creates a new object of the type Scp directory
 */
l2_scp_checker::l2_scp_checker(bemu::System* chip) : bemu::Agent(chip)
{
  // Marks all L2 entries as invalid
  for(uint32_t shire = 0; shire < EMU_NUM_SHIRES; shire++)
  {
    for(uint32_t entry = 0; entry < L2_SCP_ENTRIES; entry++)
    {
      shire_scp_info[shire].l2_scp_line_status[entry] = L2Scp_Valid;
      shire_scp_info[shire].l2_scp_line_addr[entry]   = 0x0ULL;
    }
  }
}

/*! \brief Fills an L2 scp entry
 *
 *  Sets an entry of the L2 scp of a specific minion/shire as filled and sets
 *  also the id related to the fill
 */
void l2_scp_checker::l2_scp_fill(uint32_t thread, uint32_t idx, uint32_t id, uint64_t src_addr)
{
  uint32_t minion_id = thread / EMU_THREADS_PER_MINION;
  uint32_t minion    = minion_id % EMU_MINIONS_PER_SHIRE;
  uint32_t shire     = thread / EMU_THREADS_PER_SHIRE;
  L2_SCP_CHECKER_LOG(shire, idx, minion_id, printf("l2_scp_checker::l2_scp_fill => valid shire: %i, minion: %i, line: %i, id: %i\n", shire, minion, idx, id));

  if((shire_scp_info[shire].l2_scp_line_status[idx] == L2Scp_Fill) && (shire_scp_info[shire].l2_scp_line_addr[idx] != src_addr))
  {
    LOG_AGENT(FTL, *this, "l2_scp_checker::l2_scp_fill => filling with a different address an already inflight fill line %i. Old addr %016llX, new addr %016llX\n",
                 idx, (long long unsigned int) shire_scp_info[shire].l2_scp_line_addr[idx], (long long unsigned int) src_addr);
  }

  // If line is being overwritten with exact same addr as before and it is already valid, the line
  // stays as valid. This is required due prefetching several times same cacheline with same contents
  // in convolution nodes. When optimized, we could disable this.
  // TODO: this is a potential hole in the checking, as it make sense for close prefetches, but for
  // long prefetches the contents of the source address might have changed
  if((shire_scp_info[shire].l2_scp_line_status[idx] == L2Scp_Valid) && (shire_scp_info[shire].l2_scp_line_addr[idx] == src_addr))
  {
    return;
  }

  // Marks the line as in fill
  shire_scp_info[shire].l2_scp_line_status[idx] = L2Scp_Fill;
  shire_scp_info[shire].l2_scp_line_addr[idx]   = src_addr;

  // Enters pending l2 scp fill for minion
  l2_scp_fill_info_t l2_scp_fill;
  l2_scp_fill.line = idx;
  l2_scp_fill.id   = id;
  shire_scp_info[shire].l2_scp_fills[minion].push_back(l2_scp_fill);
}

/*! \brief Waits for fills to L2 scp to finish
 *
 *  Waits for L2 scp fills of a given minion to finish. Only the fills associated
 *  to the request ID will be marked as valid
 */
void l2_scp_checker::l2_scp_wait(uint32_t thread, uint32_t id)
{
  uint32_t minion_id = thread / EMU_THREADS_PER_MINION;
  uint32_t minion    = minion_id % EMU_MINIONS_PER_SHIRE;
  uint32_t shire     = thread / EMU_THREADS_PER_SHIRE;
  L2_SCP_CHECKER_LOG(shire, 0xFFFFFFFF, minion_id, printf("l2_scp_checker::l2_scp_wait => shire: %i, minion: %i, id: %i\n", shire, minion, id));

  // Goes over all entries of minion
  auto it = shire_scp_info[shire].l2_scp_fills[minion].begin();
  while(it != shire_scp_info[shire].l2_scp_fills[minion].end())
  {
    // Same ID, set line as valid and remove from list
    if(it->id == id)
    {
      L2_SCP_CHECKER_LOG(shire, it->line, minion_id, printf("l2_scp_checker::l2_scp_wait => valid shire: %i, line: %i\n", shire, it->line));
      shire_scp_info[shire].l2_scp_line_status[it->line] = L2Scp_Valid;
      auto it_orig = it;
      it++;
      shire_scp_info[shire].l2_scp_fills[minion].erase(it_orig);
    }
    else
    {
      it++;
    }
  }
}

/*! \brief Read data from L2 scp
 *
 *  Data is read from L2 scp. If data is not valid an error is raised
 */
void l2_scp_checker::l2_scp_read(uint32_t thread, uint64_t addr)
{
  uint32_t minion_id    = thread / EMU_THREADS_PER_MINION;
  uint32_t minion       = minion_id % EMU_MINIONS_PER_SHIRE;
  uint32_t shire        = thread / EMU_THREADS_PER_SHIRE;

  // Non-linear address, convert it!
  if(addr & 0x40000000ULL)
  {
    uint64_t new_addr;
    new_addr =              addr & 0x3FULL;               // Within cacheline
    new_addr = new_addr | ((addr >> 5)  &   0x7FFFC0ULL); // Offset
    new_addr = new_addr | ((addr << 17) &  0xF800000ULL); // Shire Id [4:0]
    new_addr = new_addr | (addr         & 0x30000000ULL); // Shire Id [6:5]
    addr = new_addr;
  }

  uint32_t shire_access = (addr >> 23) & 0x3FULL;
  uint32_t line_access  = (addr >> 6) & 0x1FFFF;

  L2_SCP_CHECKER_LOG(shire, line_access, minion_id, printf("l2_scp_checker::l2_scp_read => shire: %i, minion: %i, addr: %016llX, shire_addr: %i, line: %i\n", shire, minion, (long long unsigned int) addr, shire_access, line_access));

  if(shire_access >= EMU_NUM_SHIRES)
  {
    LOG_AGENT(FTL, *this, "l2_scp_checker::l2_scp_read => accessing shire %i beyond limit %i\n", shire_access, EMU_NUM_SHIRES);
  }

  if(shire_scp_info[shire_access].l2_scp_line_status[line_access] != L2Scp_Valid)
  {
    LOG_AGENT(FTL, *this, "l2_scp_checker::l2_scp_read => line state is not valid!! It is %i\n", shire_scp_info[shire_access].l2_scp_line_status[line_access]);
  }
}

