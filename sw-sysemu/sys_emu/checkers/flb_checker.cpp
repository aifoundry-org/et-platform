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
#include "flb_checker.h"
#include "emu_gio.h"

// STD
#include <algorithm>

// Logging macro
#define FLB_CHECKER_LOG(shire, cmd) \
  { if((shire == 0xFFFFFFFF) || (log_shire == 0xFFFFFFFF) || (shire == log_shire)) \
    { \
      cmd; \
    } \
  }

/*! \brief Fills an L2 scp entry
 *
 *  Checks an access to FLB entry
 */
void flb_checker::access(uint32_t oldval, uint32_t limit, uint32_t flb, uint32_t thread)
{
    uint32_t shire = thread / EMU_THREADS_PER_SHIRE;
    uint32_t shire_thread = thread % EMU_THREADS_PER_SHIRE;

    FLB_CHECKER_LOG(shire, LOG_AGENT(DEBUG, *this, "flb_checker::access => shire: %i, thread: %i, entry: %i, limit: %i, oldval: %i",
          shire, shire_thread, flb, limit, oldval));

    // Can't go beyond limit
    if(oldval > limit)
    {
        LOG_AGENT(FTL, *this, "flb_checker::access => accessing with a limit lower than current value! shire: %i, thread: %i, entry: %i, limit: %i, oldval: %i",
            shire, shire_thread, flb, oldval, limit);
    }

    // Can't change the limit except for the first access
    if((shire_flb_info[shire][flb].alive_threads.size() != 0) && (limit != shire_flb_info[shire][flb].limit))
    {
        LOG_AGENT(FTL, *this, "flb_checker::access => changing the limit of a non empty FLB entry! shire: %i, thread: %i, entry: %i, limit: %i, new limit: %i",
            shire, shire_thread, flb, shire_flb_info[shire][flb].limit, limit);
    }

    // Update the limit
    if(shire_flb_info[shire][flb].alive_threads.size() == 0)
    {
        shire_flb_info[shire][flb].limit = limit;
        FLB_CHECKER_LOG(shire, LOG_AGENT(DEBUG, *this, "flb_checker::access => updating entry limit! shire : %i, thread: %i, entry: %i, limit %i",
              shire, shire_thread, flb, limit));
    }

    // Checks if hart was already in the list
    auto it = std::find(shire_flb_info[shire][flb].alive_threads.begin(), shire_flb_info[shire][flb].alive_threads.end(), shire_thread);
    if(it != shire_flb_info[shire][flb].alive_threads.end())
    {
        LOG_AGENT(FTL, *this, "flb_checker::access => accessing with an already present thread!! shire: %i, thread: %i, entry: %i, limit: %i",
            shire, shire_thread, flb, limit);
    }

    // Adds a hart in the list
    shire_flb_info[shire][flb].alive_threads.push_back(shire_thread);
    FLB_CHECKER_LOG(shire, LOG_AGENT(DEBUG, *this, "flb_checker::access => adding thread to the entry alive list! shire : %i, thread: %i, entry: %i, limit: %i",
          shire, shire_thread, flb, limit));

    // Drains the list when done
    if(shire_flb_info[shire][flb].limit == oldval)
    {
        shire_flb_info[shire][flb].alive_threads.clear();
        FLB_CHECKER_LOG(shire, LOG_AGENT(DEBUG, *this, "flb_checker::access => draining entry alive list! shire : %i, thread: %i, entry: %i, limit: %i",
              shire, shire_thread, flb, limit));
    }
}

