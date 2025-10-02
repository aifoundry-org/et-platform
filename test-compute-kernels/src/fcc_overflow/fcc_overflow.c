
#include "fcc.h"
#include "common.h"

#include <stdint.h>
#include <stddef.h>

int64_t entry_point(void);

// This test checks to see if tensor error is set when each of the four fcc registers 
// in the minion overflow

int64_t entry_point(void)
{
      // Note that this is a directed test, it is sufficient to run each test once
      uint64_t i = 0;
      for(i = 0; i < 1050; i++) {
      SEND_FCC(THIS_SHIRE, THREAD_0, FCC_0, 0x1);
      //SEND_FCC(THIS_SHIRE, THREAD_0, FCC_1, 0x1);
      //SEND_FCC(THIS_SHIRE, THREAD_1, FCC_0, 0x1);
      //SEND_FCC(THIS_SHIRE, THREAD_1, FCC_1, 0x1);
      }
      return 0; 

}
