#include "esrs.h"

#define ESR_NEIGH_MINION_BOOT_RESET_VAL   0x8000001000
#define ESR_ICACHE_ERR_LOG_CTL_RESET_VAL  0x6
#define ESR_TEXTURE_CONTROL_RESET_VAL     0x5

#define ESR_SC_L3_SHIRE_SWIZZLE_CTL_RESET_VAL   0x0000987765543210ULL
#define ESR_SC_REQQ_CTL_RESET_VAL               0x00038A80
#define ESR_SC_PIPE_CTL_RESET_VAL               0x0000005CFFFFFFFFULL
#define ESR_SC_L2_CACHE_CTL_RESET_VAL           0x02800080007F007FULL
#define ESR_SC_L3_CACHE_CTL_RESET_VAL           0x0300010000FF00FFULL
#define ESR_SC_SCP_CACHE_CTL_RESET_VAL          0x0000028003FF01FFULL
#define ESR_SC_ERR_LOG_CTL_RESET_VAL            0x1FE

#define ESR_FILTER_IPI_RESET_VAL                0xFFFFFFFFFFFFFFFFULL
#define ESR_SHIRE_CONFIG_CONST_RESET_VAL        0x392A000
#define ESR_SHIRE_CACHE_RAM_CFG1_RESET_VAL      0xE800340
#define ESR_SHIRE_CACHE_RAM_CFG2_RESET_VAL      0x03A0
#define ESR_SHIRE_CACHE_RAM_CFG3_RESET_VAL      0x0D0C0343
#define ESR_SHIRE_CACHE_RAM_CFG4_RESET_VAL      0x34000D0C03A0


neigh_esrs_t        neigh_esrs[EMU_NUM_NEIGHS];
shire_cache_esrs_t  shire_cache_esrs[EMU_NUM_SHIRES];
shire_other_esrs_t  shire_other_esrs[EMU_NUM_SHIRES];
broadcast_esrs_t    broadcast_esrs[EMU_NUM_SHIRES];


void neigh_esrs_t::reset()
{
    ipi_redirect_pc = 0;
    minion_boot = ESR_NEIGH_MINION_BOOT_RESET_VAL;
    texture_image_table_ptr = 0;
    texture_control = ESR_TEXTURE_CONTROL_RESET_VAL;
    texture_status = 0;
    icache_err_log_ctl = ESR_ICACHE_ERR_LOG_CTL_RESET_VAL;
    mprot = 0;
    neigh_chicken = 0;
    vmspagesize = 0;
    pmu_ctrl = 0;
}


void shire_cache_esrs_t::reset()
{
    for (int i = 0; i < 4; ++i) {
        bank[i].sc_l2_cache_ctl = ESR_SC_L2_CACHE_CTL_RESET_VAL;
        bank[i].sc_l3_cache_ctl = ESR_SC_L3_CACHE_CTL_RESET_VAL;
        bank[i].sc_l3_shire_swizzle_ctl = ESR_SC_L3_SHIRE_SWIZZLE_CTL_RESET_VAL;
        bank[i].sc_pipe_ctl = ESR_SC_PIPE_CTL_RESET_VAL;
        bank[i].sc_scp_cache_ctl = ESR_SC_SCP_CACHE_CTL_RESET_VAL;
        bank[i].sc_reqq_ctl = ESR_SC_REQQ_CTL_RESET_VAL;
        bank[i].sc_err_log_ctl = ESR_SC_ERR_LOG_CTL_RESET_VAL;
    }
}


void shire_other_esrs_t::reset(unsigned shire)
{
    for (int i = 0; i < 32; ++i) {
        fast_local_barrier[i] = 0;
    }
    mtime_local_target = 0;
    ipi_redirect_filter = ESR_FILTER_IPI_RESET_VAL;
    ipi_trigger = 0;
    shire_cache_ram_cfg1 = ESR_SHIRE_CACHE_RAM_CFG1_RESET_VAL;
    shire_cache_ram_cfg3 = ESR_SHIRE_CACHE_RAM_CFG3_RESET_VAL;
    shire_cache_ram_cfg4 = ESR_SHIRE_CACHE_RAM_CFG4_RESET_VAL;
    shire_cache_ram_cfg2 = ESR_SHIRE_CACHE_RAM_CFG2_RESET_VAL;
    shire_config = ESR_SHIRE_CONFIG_CONST_RESET_VAL | shire;
    minion_feature = 0;
}
