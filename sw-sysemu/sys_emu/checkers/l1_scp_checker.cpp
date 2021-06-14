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
#include "l1_scp_checker.h"
#include "emu_gio.h"

// Logging macros
#define L1_SCP_CHECKER_LOG(minion, cmd) \
  { if((minion == 0xFFFFFFFF) || (log_minion == 0xFFFFFFFF) || (minion == log_minion)) \
    { \
      cmd; \
    } \
  }

/*! \brief Scp directory constructor
 *
 *  This function creates a new object of the type L1 Scp checker
 */
l1_scp_checker::l1_scp_checker(bemu::System* chip) : bemu::Agent(chip)
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
}

/*! \brief Fills an L1 scp entry
 *
 *  Sets an entry of the L1 scp of a specific minion as filled and sets
 *  also the id related to the fill
 */
void l1_scp_checker::l1_scp_fill(uint32_t thread, uint32_t idx, uint32_t id)
{
  // Ignore TENB entries
  if(idx >= L1_SCP_ENTRIES) return;
  uint32_t minion_id = thread / EMU_THREADS_PER_MINION;
  uint32_t minion    = minion_id % EMU_MINIONS_PER_SHIRE;
  uint32_t shire     = thread / EMU_THREADS_PER_SHIRE;
  minion_scp_info[minion_id].l1_scp_line_status[idx] = L1Scp_Fill;
  minion_scp_info[minion_id].l1_scp_line_id[idx]     = id;
  L1_SCP_CHECKER_LOG(minion_id, printf("l1_scp_checker::l1_scp_fill => fill shire: %i, minion: %i, line: %i, id: %i\n", shire, minion, idx, id));
}

/*! \brief Waits for fills to L1 scp to finish
 *
 *  Waits for L1 scp fills of a given minion to finish. Only the fills associated
 *  to the request ID will be marked as valid
 */
void l1_scp_checker::l1_scp_wait(uint32_t thread, uint32_t id)
{
  uint32_t minion_id = thread / EMU_THREADS_PER_MINION;
  uint32_t minion    = minion_id % EMU_MINIONS_PER_SHIRE;
  uint32_t shire     = thread / EMU_THREADS_PER_SHIRE;
  L1_SCP_CHECKER_LOG(minion_id, printf("l1_scp_checker::l1_scp_wait => shire: %i, minion: %i, id: %i\n", shire, minion, id));

  // For all the entries of the minion
  for(uint32_t entry = 0 ; entry < L1_SCP_ENTRIES; entry++)
  {
    // Entry has an outstanding fill and same id
    if((minion_scp_info[minion_id].l1_scp_line_status[entry] == L1Scp_Fill) && (minion_scp_info[minion_id].l1_scp_line_id[entry] == id))
    {
      minion_scp_info[minion_id].l1_scp_line_status[entry] = L1Scp_Valid;
      minion_scp_info[minion_id].l1_scp_line_id[entry]     = -1;
      L1_SCP_CHECKER_LOG(minion_id, printf("l1_scp_checker::l1_scp_wait => valid shire: %i, minion: %i, line: %i, id: %i\n", shire, minion, entry, id));
    }
  }
}

/*! \brief Read data from L1 scp
 *
 *  Data is read from L1 scp. If data is not valid an error is raised
 */
void l1_scp_checker::l1_scp_read(uint32_t thread, uint32_t idx)
{
  uint32_t minion_id = thread / EMU_THREADS_PER_MINION;
  uint32_t minion    = minion_id % EMU_MINIONS_PER_SHIRE;
  uint32_t shire     = thread / EMU_THREADS_PER_SHIRE;
  L1_SCP_CHECKER_LOG(minion_id, printf("l1_scp_checker::l1_scp_read => shire: %i, minion: %i, line: %i,\n", shire, minion, idx));

  if(minion_scp_info[minion_id].l1_scp_line_status[idx] != L1Scp_Valid)
  {
    LOG_AGENT(FTL, *this, "l1_scp_checker::l1_scp_read => line state is not valid!! It is %i\n", minion_scp_info[minion_id].l1_scp_line_status[idx]);
  }
}

