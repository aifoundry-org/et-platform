// Local defines
#include "scp_directory.h"
#include "emu_gio.h"

// Logging variables and macros
uint32_t sd_log_minion = 0x2048;

#define SD_LOG(minion, cmd) \
  { if((minion == 0xFFFFFFFF) || (sd_log_minion == 0xFFFFFFFF) || (minion == sd_log_minion)) \
    { \
      cmd; \
    } \
  }

/*! \brief Scp directory constructor
 *
 *  This function creates a new object of the type Scp directory
 */
scp_directory::scp_directory()
{
  // Marks all L1 entries as invalid
  for(uint32_t minion = 0; minion < EMU_NUM_MINIONS; minion++)
  {
    for(uint32_t entry = 0 ; entry < L1_SCP_ENTRIES; entry++)
    {
      minion_scp_info[minion].l1_scp_line_status[entry] = L1Scp_Invalid;
      minion_scp_info[minion].l1_scp_line_id[entry] = -1;
    }
  }
  // Marks all L2 entries as invalid
  for(uint32_t shire = 0; shire < EMU_NUM_SHIRES; shire++)
  {
    for(uint32_t entry = 0; entry < L2_SCP_ENTRIES; entry++)
    {
      shire_scp_info[shire].l2_scp_line_status[entry] = L2Scp_Valid;
    }
  }
}

/*! \brief Fills an L1 scp entry
 *
 *  Sets an entry of the L1 scp of a specific minion as filled and sets
 *  also the id related to the fill
 */
void scp_directory::l1_scp_fill(uint32_t current_thread, uint32_t idx, uint32_t id)
{
  // Ignore TENB entries
  if(idx >= L1_SCP_ENTRIES) return;
  uint32_t minion_id = current_thread / EMU_THREADS_PER_MINION;
  uint32_t minion    = minion_id % EMU_MINIONS_PER_SHIRE;
  uint32_t shire     = current_thread / EMU_THREADS_PER_SHIRE;
  minion_scp_info[minion_id].l1_scp_line_status[idx] = L1Scp_Fill;
  minion_scp_info[minion_id].l1_scp_line_id[idx]     = id;
  SD_LOG(minion_id, printf("scp_directory::l1_scp_fill => fill shire: %i, minion: %i, line: %i, id: %i\n", shire, minion, idx, id));
}

/*! \brief Waits for fills to L1 scp to finish
 *
 *  Waits for L1 scp fills of a given minion to finish. Only the fills associated
 *  to the request ID will be marked as valid
 */
void scp_directory::l1_scp_wait(uint32_t current_thread, uint32_t id)
{
  uint32_t minion_id = current_thread / EMU_THREADS_PER_MINION;
  uint32_t minion    = minion_id % EMU_MINIONS_PER_SHIRE;
  uint32_t shire     = current_thread / EMU_THREADS_PER_SHIRE;
  SD_LOG(minion_id, printf("scp_directory::l1_scp_wait => shire: %i, minion: %i, id: %i\n", shire, minion, id));

  // For all the entries of the minion
  for(uint32_t entry = 0 ; entry < L1_SCP_ENTRIES; entry++)
  {
    // Entry has an outstanding fill and same id
    if((minion_scp_info[minion_id].l1_scp_line_status[entry] == L1Scp_Fill) && (minion_scp_info[minion_id].l1_scp_line_id[entry] == id))
    {
      minion_scp_info[minion_id].l1_scp_line_status[entry] = L1Scp_Valid;
      minion_scp_info[minion_id].l1_scp_line_id[entry]     = -1;
      SD_LOG(minion_id, printf("scp_directory::l1_scp_wait => valid shire: %i, minion: %i, line: %i, id: %i\n", shire, minion, entry, id));
    }
  }
}

/*! \brief Read data from L1 scp
 *
 *  Data is read from L1 scp. If data is not valid an error is raised
 */
void scp_directory::l1_scp_read(uint32_t current_thread, uint32_t idx)
{
  uint32_t minion_id = current_thread / EMU_THREADS_PER_MINION;
  uint32_t minion    = minion_id % EMU_MINIONS_PER_SHIRE;
  uint32_t shire     = current_thread / EMU_THREADS_PER_SHIRE;
  SD_LOG(minion_id, printf("scp_directory::l1_scp_read => shire: %i, minion: %i, line: %i,\n", shire, minion, idx));

  if(minion_scp_info[minion_id].l1_scp_line_status[idx] != L1Scp_Valid)
  {
    LOG_ALL_MINIONS(FTL, "scp_directory::l1_scp_read => line state is not valid!! It is %i\n", minion_scp_info[minion_id].l1_scp_line_status[idx]);
  }
}

/*! \brief Fills an L2 scp entry
 *
 *  Sets an entry of the L2 scp of a specific minion/shire as filled and sets
 *  also the id related to the fill
 */
void scp_directory::l2_scp_fill(uint32_t current_thread, uint32_t idx, uint32_t id)
{
  uint32_t minion_id = current_thread / EMU_THREADS_PER_MINION;
  uint32_t minion    = minion_id % EMU_MINIONS_PER_SHIRE;
  uint32_t shire     = current_thread / EMU_THREADS_PER_SHIRE;
  SD_LOG(minion_id, printf("scp_directory::l2_scp_fill => valid shire: %i, minion: %i, line: %i, id: %i\n", shire, minion, idx, id));

  if(shire_scp_info[shire].l2_scp_line_status[idx] == L2Scp_Fill)
  {
    LOG_ALL_MINIONS(FTL, "scp_directory::l2_scp_fill => setting as fill an already fill line %i\n", idx);
  }

  // Marks the line as in fill
  shire_scp_info[shire].l2_scp_line_status[idx] = L2Scp_Fill;

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
void scp_directory::l2_scp_wait(uint32_t current_thread, uint32_t id)
{
  uint32_t minion_id = current_thread / EMU_THREADS_PER_MINION;
  uint32_t minion    = minion_id % EMU_MINIONS_PER_SHIRE;
  uint32_t shire     = current_thread / EMU_THREADS_PER_SHIRE;
  SD_LOG(minion_id, printf("scp_directory::l2_scp_wait => shire: %i, minion: %i, id: %i\n", shire, minion, id));

  // Goes over all entries of minion
  auto it = shire_scp_info[shire].l2_scp_fills[minion].begin();
  while(it != shire_scp_info[shire].l2_scp_fills[minion].end())
  {
    // Same ID, set line as valid and remove from list
    if(it->id == id)
    {
      SD_LOG(minion_id, printf("scp_directory::l2_scp_wait => valid shire: %i, line: %i\n", shire, it->line));
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
void scp_directory::l2_scp_read(uint32_t current_thread, uint64_t addr)
{
  uint32_t minion_id    = current_thread / EMU_THREADS_PER_MINION;
  uint32_t minion       = minion_id % EMU_MINIONS_PER_SHIRE;
  uint32_t shire        = current_thread / EMU_THREADS_PER_SHIRE;
  uint32_t shire_access = (addr >> 23) & 0x3FULL;
  uint32_t line_access  = (addr >> 6) & 0x1FFFF;

  SD_LOG(minion_id, printf("scp_directory::l2_scp_read => shire: %i, minion: %i, addr: %016llX, shire_addr: %i, line: %i\n", shire, minion, (long long unsigned int) addr, shire_access, line_access));

  // Shouldn't receive this type of access
  if(addr & 0x40000000ULL)
  {
    LOG_ALL_MINIONS(FTL, "scp_directory::l2_scp_read => non-linear address type %016llx\n", (long long unsigned int) addr);
  }

  if(shire_access >= EMU_NUM_SHIRES)
  {
    LOG_ALL_MINIONS(FTL, "scp_directory::l2_scp_read => accessing shire %i beyond limit %i\n", shire_access, EMU_NUM_SHIRES);
  }

  if(shire_scp_info[shire_access].l2_scp_line_status[line_access] != L2Scp_Valid)
  {
    LOG_ALL_MINIONS(FTL, "scp_directory::l2_scp_read => line state is not valid!! It is %i\n", shire_scp_info[shire_access].l2_scp_line_status[line_access]);
  }

}

