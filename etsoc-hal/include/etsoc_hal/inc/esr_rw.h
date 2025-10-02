#include "fc_esr.h"

typedef  enum
   {
      RO,
      RW,
      RW1C,
      WO,
      RC
   } esr_rw_access_t;

typedef struct esr_reg_desc_t
   {
      fc_esr_region_t   region;
      esr_protection_t  PP;
      uint64_t          addr;
      esr_rw_access_t   rw_access;
      uint64_t          w_mask;
      uint64_t          r_mask;
      uint8_t           has_resetVal;
      uint64_t          resetVal;
      uint8_t           has_readVal;
   } ESR_REG_DESC_t;

ESR_REG_DESC_t neigh_Regs[] =
   {
      // mod                    PP            addr     rw_access    w_mask    r_mask   has_resetVal   resetVal   has_readVal
      { REGION_SHIRE_NEIGH,   PP_MACHINE,   0x003,   RW,   0xFFFFFFFFFFFF,   0xFFFFFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_NEIGH,   PP_MACHINE,   0x004,   RW,   0x7,   0x7,   1,   0x0, 0},
      { REGION_SHIRE_NEIGH,   PP_MACHINE,   0x007,   RW,   0x3,   0x3,   1,   0x0, 0},
      { REGION_SHIRE_NEIGH,   PP_USER,   0x008,   RW,   0x7FFFFFFFFFFF,   0x7FFFFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_NEIGH,   PP_MESSAGES,   0x1ff0,   RW,   0xFFFFFFFF,   0xFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_NEIGH,   PP_MESSAGES,   0x1ff1,   RO,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_NEIGH,   PP_MESSAGES,   0x1ff2,   RW,   0xFFFFFFFFFFFF,   0xFFFFFFFF0000,   1,   0x0, 1},
      { REGION_SHIRE_NEIGH,   PP_MESSAGES,   0x1ff3,   RO,   0x3FF,   0x3FF,   1,   0x0, 0},
      { REGION_SHIRE_NEIGH,   PP_MACHINE,   0x00D,   RW,   0x1,   0x1,   1,   0x0, 0},
      //{ REGION_SHIRE_NEIGH,   PP_MACHINE,   0x00E,   RW,   0xFF,   0xFF,   1,   0x0, 0}, Register is forced inside evl_neigh_interface.sv
      { REGION_SHIRE_NEIGH,   PP_MACHINE,   0x00F,   RW,   0xF,   0xF,   1,   0x0, 0},
      { REGION_SHIRE_NEIGH,   PP_MACHINE,   0x010,   RW,   0x1FFFFFFFFFFFFF,   0x1FFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_NEIGH,   PP_MACHINE,   0x011,   RO,   0x3FFFFFFFF,   0x3FFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_NEIGH,   PP_MACHINE,   0x012,   RW,   0x1,   0x1,   1,   0x0, 1},
      { REGION_SHIRE_NEIGH,   PP_USER,   0x1000,   RW,   0xFFFF,   0xFFFF,   1,   0x0, 0},
      { REGION_SHIRE_NEIGH,   PP_USER,   0x1001,   RO,   0xFFFF,   0xFFFF,   1,   0x0, 0},
      { REGION_SHIRE_NEIGH,   PP_USER,   0x1002,   RW,   0xFFFFFFFFFFFF,   0xFFFFFFFFFFFF,   1,   0x0, 0},

      { REGION_SHIRE_OTHER,   PP_MACHINE,   0xFFFFFFFFFFFF,   RW,  0,   0,   1,   0x0, 0}
   };

ESR_REG_DESC_t cache_Regs[] =
   {
      // mod                    PP            addr     rw_access    w_mask    r_mask   has_resetVal   resetVal   has_readVal 
      //{ REGION_SHIRE_CACHE,   PP_MACHINE,   0x000,   RW,   0x3FFFFFFFFFFFF,   0x3FFFFFFFFFFFF,   1,   0x0, 0},
      //{ REGION_SHIRE_CACHE,   PP_MACHINE,   0x001,   RW,   0x3FFFF,   0x3FFFF,   1,   0x0, 0},
      //{ REGION_SHIRE_CACHE,   PP_MACHINE,   0x002,   RW,   0x3FFFFFFFFFF,   0x3FFFFFFFFFF,   1,   0x0, 0},
      //{ REGION_SHIRE_CACHE,   PP_MACHINE,   0x003,   RW,   0xFFF1FFF0FFF0FFF,   0xFFF1FFF0FFF0FFF,   1,   0x0, 0},
      //{ REGION_SHIRE_CACHE,   PP_MACHINE,   0x004,   RW,   0xFFF1FFF0FFF0FFF,   0xFFF1FFF0FFF0FFF,   1,   0x0, 0},
      //{ REGION_SHIRE_CACHE,   PP_MACHINE,   0x005,   RW,   0xFFF1FFF0FFF0FFF,   0xFFF1FFF0FFF0FFF,   1,   0x0, 0},
      { REGION_SHIRE_CACHE,   PP_MACHINE,   0x006,   RW,   0xFF00EF03,   0xFF00EF03,   0,   0x0, 1},
      { REGION_SHIRE_CACHE,   PP_MACHINE,   0x007,   RW,   0x3FF00030703,   0x3FF00030703,   0,   0x0, 1},
      { REGION_SHIRE_CACHE,   PP_MACHINE,   0x008,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   0,   0x0, 1},
      { REGION_SHIRE_CACHE,   PP_MACHINE,   0x009,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   0,   0x0, 1},
      { REGION_SHIRE_CACHE,   PP_MACHINE,   0x00A,   RW,   0xFFFFFF,   0xFFFFFF,   0,   0x0, 1},
      { REGION_SHIRE_CACHE,   PP_MACHINE,   0x00B,   RW,   0x1FF,   0x1FF,   1,   0x0, 0},
      { REGION_SHIRE_CACHE,   PP_MACHINE,   0x00C,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   0,   0x0, 1},
      { REGION_SHIRE_CACHE,   PP_MACHINE,   0x00D,   RO,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   0,   0x0, 0},
      { REGION_SHIRE_CACHE,   PP_MACHINE,   0x00E,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   0,   0x0, 1},
      { REGION_SHIRE_CACHE,   PP_MACHINE,   0x00F,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   0,   0x0, 1},
      //{ REGION_SHIRE_CACHE,   PP_MACHINE,   0x010,   RO,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   0,   0x0, 0},
      //{ REGION_SHIRE_CACHE,   PP_MACHINE,   0x011,   RO,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   0,   0x0, 0},
      //{ REGION_SHIRE_CACHE,   PP_MACHINE,   0x012,   RO,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   0,   0x0, 0},
      //{ REGION_SHIRE_CACHE,   PP_MACHINE,   0x013,   RO,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   0,   0x0, 0},
      { REGION_SHIRE_CACHE,   PP_MACHINE,   0x014,   RW,   0xFF,   0xFF,   1,   0x0, 0},
      //{ REGION_SHIRE_CACHE,   PP_MACHINE,   0x017,   RW,   0x3FFFFFFF,   0x3FFFFFFF,   0,   0x0, 1},
      { REGION_SHIRE_CACHE,   PP_MACHINE,   0x018,   RW,   0xFFFFFFFFFF,   0xFFFFFFFFFF,   0,   0x0, 1},
      { REGION_SHIRE_CACHE,   PP_MACHINE,   0x019,   RW,   0xFFFFFFFFFF,   0xFFFFFFFFFF,   0,   0x0, 1},
      { REGION_SHIRE_CACHE,   PP_MACHINE,   0x01A,   RW,   0xFFFFFFFFFF,   0xFFFFFFFFFF,   0,   0x0, 1},
      { REGION_SHIRE_CACHE,   PP_MACHINE,   0x01B,   RW,   0x7FFFFFFFFFFFF,   0x7FFFFFFFFFFFF,   0,   0x0, 1},
      { REGION_SHIRE_CACHE,   PP_MACHINE,   0x01C,   RW,   0x7FFFFFFFFFFFF,   0x7FFFFFFFFFFFF,   0,   0x0, 1},
      { REGION_SHIRE_CACHE,   PP_MESSAGES,   0x3f0,   RW,   0xFFFFFFFFFF,   0xFFFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_CACHE,   PP_MESSAGES,   0x3f1,   RW,   0xFFFFFFFFFF,   0xFFFFFFFFFF,   0,   0x0, 0},
      { REGION_SHIRE_CACHE,   PP_MESSAGES,   0x3f2,   RW,   0x7FFFFFFFF,   0x7FFFFFFFF,   1,   0x0, 0},

      { REGION_SHIRE_OTHER,   PP_MACHINE,   0xFFFFFFFFFFFF,   RW,  0,   0,   1,   0x0, 0}
   };

ESR_REG_DESC_t other_Regs_1[] =
   {
      // mod                    PP            addr     rw_access    w_mask    r_mask   has_resetVal   resetVal   has_readVal 
      { REGION_SHIRE_OTHER,   PP_MACHINE,   0x000,   RW,   0x3F,   0x3F,   1,   0x0, 0},
      //{ REGION_SHIRE_OTHER,   PP_MACHINE,   0x001,   RW,   0x3FC0000,   0x3FFFFFF,   1,   0x0, 0}, shire_config check write val not to disable neigh, cache...
      //{ REGION_SHIRE_OTHER,   PP_MACHINE,   0x002,   RW,   0xFFFFFFFF,   0xFFFFFFFF,   1,   0x0, 0}, Register is forced inside evl_shire_interface.sv
      { REGION_SHIRE_OTHER,   PP_MACHINE,   0x003,   RO,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_OTHER,   PP_MACHINE,   0x004,   RO,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_OTHER,   PP_USER,   0x010,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_MACHINE,   0x011,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_OTHER,   PP_MACHINE,   0x012,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_OTHER,   PP_MACHINE,   0x013,   RW,   0xFFFFFFFF,   0xFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x018,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x019,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x01a,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x01b,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x020,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x021,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x022,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x023,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x024,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x025,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x026,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x027,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x028,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x029,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x02a,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x02b,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x02c,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x02d,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x02e,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x02f,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x030,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x031,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x032,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x033,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x034,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1}
   };

ESR_REG_DESC_t other_Regs_2[] =
   {
      // mod                    PP            addr     rw_access    w_mask    r_mask   has_resetVal   resetVal   has_readVal 
      { REGION_SHIRE_OTHER,   PP_USER,   0x035,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x036,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x037,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x038,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x039,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x03a,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x03b,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x03c,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x03d,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x03e,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_USER,   0x03f,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 1},
      //{ REGION_SHIRE_OTHER,   PP_MESSAGES,   0x3ff0,   RO,   0x7FF,   0x7FF,   1,   0x0, 0},   Debug reg, skip for now
      { REGION_SHIRE_OTHER,   PP_MACHINE,   0x043,   RW,   0xFFFFFFFF,   0xFFFFFFFF,   1,   0x0, 0},
      //{ REGION_SHIRE_OTHER,   PP_MACHINE,   0x044,   RW,   0xFFF,   0xFFF,   1,   0x0, 0},
      { REGION_SHIRE_OTHER,   PP_MACHINE,   0x045,   RW,   0xFFFFFFFF,   0xFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_OTHER,   PP_MACHINE,   0x046,   RW,   0xFFFFFFFF,   0xFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_OTHER,   PP_MACHINE,   0x047,   RO,   0xFFFFFFFF,   0xFFFFFFFF,   1,   0x0, 0},
      //{ REGION_SHIRE_OTHER,   PP_MACHINE,   0x048,   RW,   0xFFFFFFFF,   0xFFFFFFFF,   1,   0x0, 0}, Register is forced inside evl_shire_interface.sv
      { REGION_SHIRE_OTHER,   PP_MACHINE,   0x049,   RO,   0xFFFF,   0xFFFF,   1,   0x0, 0},
      //{ REGION_SHIRE_OTHER,   PP_MACHINE,   0x04a,   RW,   0x1FFF0,   0x1FFF0,   1,   0x0, 0},
      { REGION_SHIRE_OTHER,   PP_MACHINE,   0x04b,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_OTHER,   PP_MACHINE,   0x04c,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_OTHER,   PP_MACHINE,   0x04d,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_OTHER,   PP_MACHINE,   0x04e,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 0},
      //{ REGION_SHIRE_OTHER,   PP_MACHINE,   0x051,   RO,   0x3FFFF,   0x3FFFF,   1,   0x0, 0},
      { REGION_SHIRE_OTHER,   PP_SUPERVISOR,   0x052,   RW,   0x1,   0x1,   1,   0x0, 0},
      //{ REGION_SHIRE_OTHER,   PP_MACHINE,   0x053,   RW,   0xF,   0xF,   1,   0x0, 0},
      { REGION_SHIRE_OTHER,   PP_MACHINE,   0x054,   RW,   0xFFFFFFFFF,   0xFFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_OTHER,   PP_MACHINE,   0x055,   RW,   0x3FFFF,   0x3FFFF,   1,   0x0, 0},
      { REGION_SHIRE_OTHER,   PP_MACHINE,   0x056,   RW,   0xFFFFFFFFF,   0xFFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_OTHER,   PP_MACHINE,   0x057,   RW,   0x3FFFFFFFFFFFFF,   0x3FFFFFFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_OTHER,   PP_MACHINE,   0x058,   RO,   0x1FFFFF,   0x1FFFFF,   1,   0x0, 0},
      //{ REGION_SHIRE_OTHER,   PP_MACHINE,   0x059,   RW,   0x3FFF,   0x3FFF,   1,   0x0, 0},
      { REGION_SHIRE_OTHER,   PP_MACHINE,   0x05a,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   0,   0x0, 0}, 
      //{ REGION_SHIRE_OTHER,   PP_MACHINE,   0x05b,   RO,   0x3FFFF,   0x3FFFF,   1,   0x0, 0},  //need to set dll_rdy_ip in order to initialize dll_read_data
      //{ REGION_SHIRE_OTHER,   PP_MESSAGES,   0x3ff1,   RW,   0xFFFF,   0xFFFF,   1,   0x0, 0},    Debug reg, skip for now
      { REGION_SHIRE_OTHER,   PP_SUPERVISOR,   0x05d,   RW,   0x1,   0x1,   1,   0x0, 0},
      { REGION_SHIRE_OTHER,   PP_USER,   0x05f,   RW,   0xFFFFFFFFFFFFF,   0xFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_SUPERVISOR,   0x060,   RW,   0xFFFFFFFFFFFFF,   0xFFFFFFFFFFFFF,   1,   0x0, 1},
      { REGION_SHIRE_OTHER,   PP_MACHINE,   0x061,   RW,   0xFFFFFFFFFFFFF,   0xFFFFFFFFFFFFF,   1,   0x0, 1},
      //{ REGION_SHIRE_OTHER,   PP_MACHINE,   0x062,   RW,   0x3FF,   0x3FF,   1,   0x0, 0},
      //{ REGION_SHIRE_OTHER,   PP_MESSAGES,   0x3ff4,   RW,   0x1,   0x1,   1,   0x0, 0},   Debug reg, skip for now
      { REGION_SHIRE_OTHER,   PP_MACHINE,   0x068,   RW,   0xFF,   0xFF,   1,   0x0, 0},

      { REGION_SHIRE_OTHER,   PP_MACHINE,   0xFFFFFFFFFFFF,   RW,  0,   0,   1,   0x0, 0}
   };

ESR_REG_DESC_t rbox_Regs[] =
   {
      // mod                    PP            addr     rw_access    w_mask    r_mask   has_resetVal   resetVal   has_readVal 
      { REGION_SHIRE_RBOX,   PP_USER,   0x000,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_RBOX,   PP_USER,   0x001,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_RBOX,   PP_USER,   0x002,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_RBOX,   PP_USER,   0x003,   RW,   0xFFFFFFFF,   0xFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_RBOX,   PP_USER,   0x004,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_RBOX,   PP_USER,   0x005,   RO,   0xFFFFFFFF,   0xFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_RBOX,   PP_USER,   0x006,   RW,   0xFFFFFFFF,   0xFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_RBOX,   PP_USER,   0x007,   RW,   0xFFFFFFFF,   0xFFFFFFFF,   1,   0x0, 0},

      { REGION_SHIRE_OTHER,   PP_MACHINE,   0xFFFFFFFFFFFF,   RW,  0,   0,   1,   0x0, 0}
   };

ESR_REG_DESC_t mem_Regs[] =
   {
      // mod                    PP            addr     rw_access    w_mask    r_mask   has_resetVal   resetVal   has_readVal 
      { REGION_SHIRE_MEM,   PP_MESSAGES,   0x000,   RW,   0x3FFFFFFFFFFFF,   0x3FFFFFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_MEM,   PP_MESSAGES,   0x001,   RW,   0xF1,   0xF1,   1,   0x0, 1},
      { REGION_SHIRE_MEM,   PP_MESSAGES,   0x002,   RO,   0xFF00FFFFFFFF,   0xFF00FFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_MEM,   PP_MESSAGES,   0x003,   RW,   0x1,   0x1,   1,   0x0, 0},
      { REGION_SHIRE_MEM,   PP_MESSAGES,   0x004,   RO,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 0},

      { REGION_SHIRE_OTHER,   PP_MACHINE,   0xFFFFFFFFFFFF,   RW,  0,   0,   1,   0x0, 0}
   };

ESR_REG_DESC_t ddrc_Regs[] =
   {
      // mod                    PP            addr     rw_access    w_mask    r_mask   has_resetVal   resetVal   has_readVal 
      { REGION_SHIRE_DDRC,   PP_MESSAGES,   0x000,   RW,   0x10F,   0x10F,   1,   0x0, 0},
      { REGION_SHIRE_DDRC,   PP_MESSAGES,   0x001,   RW,   0x3FFFF,   0x3FFFF,   1,   0x0, 1},
      { REGION_SHIRE_DDRC,   PP_MESSAGES,   0x002,   RW,   0xF,   0xF,   1,   0x0, 0},
      { REGION_SHIRE_DDRC,   PP_MESSAGES,   0x003,   RW,   0xFFFFFFFFF,   0xFFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_DDRC,   PP_MESSAGES,   0x004,   RW,   0xFFFFFFFFF,   0xFFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_DDRC,   PP_MESSAGES,   0x005,   RO,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_DDRC,   PP_MESSAGES,   0x006,   RO,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_DDRC,   PP_MESSAGES,   0x007,   RW,   0x3,   0x3,   1,   0x0, 1},
      { REGION_SHIRE_DDRC,   PP_MESSAGES,   0x008,   RO,   0xFFFF,   0xFFFF,   1,   0x0, 0},
      { REGION_SHIRE_DDRC,   PP_MESSAGES,   0x009,   RW,   0xFFFF,   0xFFFF,   1,   0x0, 0},
      { REGION_SHIRE_DDRC,   PP_MESSAGES,   0x00A,   RW,   0xFFFF,   0xFFFF,   1,   0x0, 0},
      { REGION_SHIRE_DDRC,   PP_MESSAGES,   0x00B,   RW,   0xFFF7,   0xFFF7,   1,   0x0, 1},
      { REGION_SHIRE_DDRC,   PP_MESSAGES,   0x00C,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_DDRC,   PP_MESSAGES,   0x00D,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_DDRC,   PP_MACHINE,   0x00E,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   1,   0x0, 0},
      { REGION_SHIRE_DDRC,   PP_MESSAGES,   0x00F,   RW,   0xFFFFFF01,   0xFFFFFF01,   1,   0x0, 0},
      { REGION_SHIRE_DDRC,   PP_MACHINE,   0x011,   RW,   0xFFFFFFFFFF,   0xFFFFFFFFFF,   0,   0x0, 1},
      { REGION_SHIRE_DDRC,   PP_MACHINE,   0x012,   RW,   0xFFFFFFFFFF,   0xFFFFFFFFFF,   0,   0x0, 1},
      { REGION_SHIRE_DDRC,   PP_MACHINE,   0x013,   RW,   0xFFFFFFFFFF,   0xFFFFFFFFFF,   0,   0x0, 1},
      { REGION_SHIRE_DDRC,   PP_MACHINE,   0x010,   RW,   0x1FFFFFFFFF,   0x1FFFFFFFFF,   0,   0x0, 1},
      { REGION_SHIRE_DDRC,   PP_MACHINE,   0x014,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   0,   0x0, 1},
      { REGION_SHIRE_DDRC,   PP_MACHINE,   0x015,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   0,   0x0, 1},
      { REGION_SHIRE_DDRC,   PP_MACHINE,   0x016,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   0,   0x0, 1},
      { REGION_SHIRE_DDRC,   PP_MACHINE,   0x017,   RW,   0xFFFFFFFFFFFFFFFF,   0xFFFFFFFFFFFFFFFF,   0,   0x0, 1},

      { REGION_SHIRE_OTHER,   PP_MACHINE,   0xFFFFFFFFFFFF,   RW,  0,   0,   1,   0x0, 0}
   };

ESR_REG_DESC_t p_shire_esr_Regs[] =
   {
      // mod                    PP            addr     rw_access    w_mask    r_mask

      { REGION_P_SHIRE_ESR,   PP_MESSAGES,   0x000,   RW,   0x3,   0x3,   1,   0x0, 0},
      { REGION_P_SHIRE_ESR,   PP_MESSAGES,   0x004,   RW,   0x1,   0x1,   1,   0x0, 0},
      { REGION_P_SHIRE_ESR,   PP_MESSAGES,   0x008,   RO,   0x1FF,   0x1FF,   1,   0x0, 0},
      { REGION_P_SHIRE_ESR,   PP_MESSAGES,   0x00C,   RW,   0xFFFFFFFF,   0xFFFFFFFF,   1,   0x0, 0},
      { REGION_P_SHIRE_ESR,   PP_MESSAGES,   0x010,   RW,   0xFF,   0xFF,   1,   0x0, 0},
      { REGION_P_SHIRE_ESR,   PP_MESSAGES,   0x014,   RW,   0xFFFFFFFF,   0xFFFFFFFF,   1,   0x0, 0},
      { REGION_P_SHIRE_ESR,   PP_MESSAGES,   0x018,   RW,   0xFFFFFFFF,   0xFFFFFFFF,   1,   0x0, 0},
      { REGION_P_SHIRE_ESR,   PP_MESSAGES,   0x01C,   RW1C,   0x1FFFF,   0x1FFFF,   1,   0x0, 0},
      { REGION_P_SHIRE_ESR,   PP_MESSAGES,   0x020,   RW,   0x1FFFF,   0x1FFFF,   1,   0x0, 0},
      { REGION_P_SHIRE_ESR,   PP_MESSAGES,   0x024,   WO,   0x1FFFF,   0x1FFFF,   1,   0x0, 0},
      { REGION_P_SHIRE_ESR,   PP_MESSAGES,   0x028,   RO,   0xFF,   0xFF,   1,   0x0, 0},
      { REGION_P_SHIRE_ESR,   PP_MESSAGES,   0x02C,   RO,   0x7FF,   0x7FF,   1,   0x0, 0}, 

      { REGION_P_SHIRE_USRESR,   PP_USER,   0x000,   WO,   0xF,   0xF,   1,   0x0, 0},
      { REGION_P_SHIRE_USRESR,   PP_USER,   0x004,   WO,   0xF,   0xF,   1,   0x0, 0},
      { REGION_P_SHIRE_USRESR,   PP_USER,   0x008,   RC,   0x1F,   0x1F,   1,   0x0, 0},
      { REGION_P_SHIRE_USRESR,   PP_USER,   0x00C,   RC,   0x1F,   0x1F,   1,   0x0, 0},
      { REGION_P_SHIRE_USRESR,   PP_USER,   0x010,   RC,   0x1F,   0x1F,   1,   0x0, 0},
      { REGION_P_SHIRE_USRESR,   PP_USER,   0x014,   RC,   0x1F,   0x1F,   1,   0x0, 0},
      { REGION_P_SHIRE_USRESR,   PP_USER,   0x018,   RC,   0x1F,   0x1F,   1,   0x0, 0},
      { REGION_P_SHIRE_USRESR,   PP_USER,   0x01C,   RC,   0x1F,   0x1F,   1,   0x0, 0},
      { REGION_P_SHIRE_USRESR,   PP_USER,   0x020,   RC,   0x1F,   0x1F,   1,   0x0, 0},
      { REGION_P_SHIRE_USRESR,   PP_USER,   0x024,   RC,   0x1F,   0x1F,   1,   0x0, 0},

      { REGION_P_SHIRE_NOPCIESR,   PP_USER,   0x000,   RW,   0xFFFFFF,   0xFFFFFF,   1,   0x0, 0},
      { REGION_P_SHIRE_NOPCIESR,   PP_USER,   0x004,   RW,   0xFFFFFF,   0xFFFFFF,   1,   0x0, 0},
      { REGION_P_SHIRE_NOPCIESR,   PP_USER,   0x008,   RW,   0xFFFFFF,   0xFFFFFF,   1,   0x0, 0},
      { REGION_P_SHIRE_NOPCIESR,   PP_USER,   0x00C,   RW,   0xFFFFFF,   0xFFFFFF,   1,   0x0, 0},
      { REGION_P_SHIRE_NOPCIESR,   PP_USER,   0x010,   RW,   0xF,   0xF,   1,   0x0, 0},
      { REGION_P_SHIRE_NOPCIESR,   PP_USER,   0x014,   RW,   0x1,   0x1,   1,   0x0, 0},
      { REGION_P_SHIRE_NOPCIESR,   PP_USER,   0x018,   RW,   0xFFFFFFFF,   0xFFFFFFFF,   1,   0x0, 0},
      { REGION_P_SHIRE_NOPCIESR,   PP_USER,   0x01C,   RW,   0x3,   0x3,   1,   0x0, 0},

      { REGION_SHIRE_OTHER,   PP_MACHINE,   0xFFFFFFFFFFFF,   RW,  0,   0,   1,   0x0, 0}
   };