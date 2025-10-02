#ifndef __REG_PLIC_REGS_H__
#define __REG_PLIC_REGS_H__

#include "plic_regs_macro.h"

#ifndef INTH_NUMBER_OF_INT
  #define INTH_NUMBER_OF_INT 200
#endif

#ifndef INTH_TARGETS
  #define INTH_TARGETS   12
#endif

#define INTH_REGS ( (INTH_NUMBER_OF_INT >> 5) + ((INTH_NUMBER_OF_INT % 32) ? 1 : 0) )

  struct priority_regs {
    volatile uint32_t priority_source[INTH_NUMBER_OF_INT];
  };

  struct pending_regs {
    volatile uint32_t pending[INTH_REGS];
  };
	
  struct hartRegs {
    volatile uint32_t threshold_target;
    volatile uint32_t maxid_target;
    uint8_t           res[0x1000 - 2 * sizeof(uint32_t)];
  };
  
  struct hart_regs {  
    struct hartRegs reg[INTH_TARGETS];
  };
	

#endif /* __REG_PLIC_REGS_H__*/
