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

// Logging variables and macros
uint32_t flb_checker_log_shire = 64; // None by default

#define FLB_CHECKER_LOG(shire, cmd) \
  { if((shire == 0xFFFFFFFF) || (flb_checker_log_shire == 0xFFFFFFFF) || (shire == flb_checker_log_shire)) \
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

  FLB_CHECKER_LOG(shire, printf("flb_checker::access => shire: %i, thread: %i, entry: %i, oldval: %i, limit: %i\n",
    shire, shire_thread, flb, oldval, limit));

  if(oldval > limit)
  {
    LOG_AGENT(FTL, *this, "flb_checker::access => accessing with a limit lower than current value! shire: %i, thread: %i, entry: %i, oldval: %i, limit: %i\n",
                 shire, shire_thread, flb, oldval, limit);
  }
}

