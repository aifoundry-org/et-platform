
#include "cpu.h"
#include "et.h"
#include "api.h"
#include "print.h"

#include "DW_common.h"
#include "DW_ahb_dmac_public.h"
#include "DW_ahb_dmac_private.h"

DW_DEFINE_THIS_FILE;

int dw_dmac_init(struct dw_device *dev)
{
    int retVal = 0;

    DMAC_COMMON_REQUIREMENTS(dev);
    dw_dmac_setTestMode(dev, Dw_clear);
    dw_dmac_resetInstance(dev);
    
    // Disable DMAC
    retVal = dw_dmac_disable(dev);
    if(retVal != 0) return retVal;
    
    retVal = dw_dmac_disableChannel(dev, Dmac_all_channels);
    if(retVal != 0) return retVal;

    retVal = dw_dmac_disableChannelIrq(dev, Dmac_all_channels);
    if(retVal != 0) return retVal;

    retVal = dw_dmac_maskIrq(dev, Dmac_all_channels, Dmac_irq_all);
    if(retVal != 0) return retVal;

    retVal = dw_dmac_clearIrq(dev, Dmac_all_channels, Dmac_irq_all);
    return retVal;
}

void dw_dmac_enable(struct dw_device *dev)
{
    struct dw_dmac_portmap *portmap;
    uint32_t reg;

    DMAC_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_dmac_portmap *) dev->base_address;
    reg = 0;
    DW_BIT_SET(reg, DMAC_DMACFGREG_L_DMA_EN, 1);
    DMAC_OUTP(reg, portmap->dma_cfg_reg_l);
}

int dw_dmac_disable(struct dw_device *dev)
{
    struct dw_dmac_portmap *portmap;
    uint32_t reg;
    int retVal = 0;

    DMAC_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_dmac_portmap *) dev->base_address;

    // Check if DMA is disabled
    reg = DMAC_INP(portmap->dma_cfg_reg_l);
    reg = DW_BIT_GET(reg, DMAC_DMACFGREG_L_DMA_EN);

    if(reg != 0) {
        reg = 0;
        DW_BIT_SET(reg, DMAC_DMACFGREG_L_DMA_EN, 0);
        DMAC_OUTP(reg, portmap->dma_cfg_reg_l);

        if(DMAC_INP(portmap->dma_cfg_reg_l)) {
            retVal = -DW_EBUSY;
        }
    }
    return retVal;
}

int dw_dmac_enableChannel(struct dw_device *dev, enum dw_dmac_channel_number ch_num)
{
    struct dw_dmac_param *param;
    struct dw_dmac_portmap *portmap;
    uint32_t reg;
    int retVal = 0;

    DMAC_COMMON_REQUIREMENTS(dev);

    param = (struct dw_dmac_param  *) dev->comp_param;
    portmap = (struct dw_dmac_portmap *) dev->base_address;

    if(ch_num == Dmac_all_channels) {
        ch_num &= DMAC_CH_ALL_MASK;
    }

    // Check that requested channel is not busy
    retVal = dw_dmac_checkChannelBusy(dev, ch_num);
    if(retVal != 0) return retVal;
    
    reg = ch_num;
    DMAC_OUTP(reg, portmap->ch_en_reg_l);
    
    return retVal;
}

int dw_dmac_disableChannel(struct dw_device *dev, enum dw_dmac_channel_number ch_num)
{
    struct dw_dmac_param *param;
    struct dw_dmac_portmap *portmap;
    uint32_t reg;
    int retVal = 0;

    DMAC_COMMON_REQUIREMENTS(dev);

    param = (struct dw_dmac_param  *) dev->comp_param;
    portmap = (struct dw_dmac_portmap *) dev->base_address;

    if(ch_num == Dmac_all_channels) {
        ch_num &= DMAC_CH_ALL_MASK;
    }

    retVal = dw_dmac_checkChannelRange(dev, ch_num);
    if(retVal != 0) return retVal;
    
    reg = ch_num & (DMAC_MAX_CH_MASK << DMAC_MAX_CHANNELS);
    DMAC_OUTP(reg, portmap->ch_en_reg_l);

    if(DMAC_INP(portmap->ch_en_reg_l) & ch_num) {
        retVal = -DW_EBUSY;
    }
        
    return retVal;
}

int dw_dmac_enableChannelIrq(struct dw_device *dev, enum dw_dmac_channel_number ch_num)
{
    struct dw_dmac_param *param;
    struct dw_dmac_portmap *portmap;
    uint32_t reg;
    int retVal = 0;

    DMAC_COMMON_REQUIREMENTS(dev);

    param = (struct dw_dmac_param  *) dev->comp_param;
    portmap = (struct dw_dmac_portmap *) dev->base_address;

    if(ch_num == Dmac_all_channels) {
        ch_num &= DMAC_CH_ALL_MASK;
    }

    // Check that requested channel is not busy
    retVal = dw_dmac_checkChannelBusy(dev, ch_num);
    if(retVal != 0) return retVal;

    for(int i=0;i<param->num_channels;i++) {
        if(ch_num & (0x1u<<i)) {
            reg = DMAC_INP(portmap->ch[i].ctl_l);
                
            if(DW_BIT_GET(reg, DMAC_CTL_L_INT_EN) != 1) {
                DW_BIT_SET(reg, DMAC_CTL_L_INT_EN, 1);
                DMAC_OUTP(reg, portmap->ch[i].ctl_l);
            }
        }
    }
    return retVal;
}

int dw_dmac_disableChannelIrq(struct dw_device *dev, enum   dw_dmac_channel_number ch_num)
{
    struct dw_dmac_param *param;
    struct dw_dmac_portmap *portmap;
    uint32_t reg;
    int retVal = 0;

    DMAC_COMMON_REQUIREMENTS(dev);

    param = (struct dw_dmac_param  *) dev->comp_param;
    portmap = (struct dw_dmac_portmap *) dev->base_address;

    if(ch_num == Dmac_all_channels) {
        ch_num &= DMAC_CH_ALL_MASK;
    }

    retVal = dw_dmac_checkChannelBusy(dev, ch_num);
    if(retVal != 0) return retVal;

    for(int i=0;i<param->num_channels;i++) {
            if(ch_num & (0x1u<<i)) {
                reg = DMAC_INP(portmap->ch[i].ctl_l);
                if(DW_BIT_GET(reg, DMAC_CTL_L_INT_EN) != 0) {
                    DW_BIT_SET(reg, DMAC_CTL_L_INT_EN, 0);
                    DMAC_OUTP(reg, portmap->ch[i].ctl_l);
                }
            }
    }
        
    return retVal;
}

int dw_dmac_clearIrq(struct dw_device *dev, enum dw_dmac_channel_number ch_num, enum dw_dmac_irq ch_irq)
{
    struct dw_dmac_param *param;
    struct dw_dmac_portmap *portmap;
    uint32_t reg;
    int retVal = 0;

    DMAC_COMMON_REQUIREMENTS(dev);

    param = (struct dw_dmac_param  *) dev->comp_param;
    portmap = (struct dw_dmac_portmap *) dev->base_address;

    if(ch_num == Dmac_all_channels) {
        ch_num &= DMAC_CH_ALL_MASK;
    }

    retVal = dw_dmac_checkChannelRange(dev, ch_num);
    if(retVal != 0) return retVal;

    reg = ch_num & DMAC_MAX_CH_MASK;
    for(int i=0;i<DMAC_MAX_INTERRUPTS;i++) {
            if(ch_irq & (0x1u<<i))
                switch(i) {
                    case 0 : DMAC_OUTP(reg, portmap->clear_tfr_l); break;
                    case 1 : DMAC_OUTP(reg, portmap->clear_block_l); break;
                    case 2 : DMAC_OUTP(reg, portmap->clear_srctran_l); break;
                    case 3 : DMAC_OUTP(reg, portmap->clear_dsttran_l); break;
                    case 4 : DMAC_OUTP(reg, portmap->clear_err_l); break;
                }
    }

    return retVal;
}

int dw_dmac_maskIrq(struct dw_device *dev, enum dw_dmac_channel_number ch_num, enum dw_dmac_irq ch_irq)
{
    struct dw_dmac_param *param;
    struct dw_dmac_portmap *portmap;
    uint32_t reg;
    int retVal = 0;

    DMAC_COMMON_REQUIREMENTS(dev);

    param = (struct dw_dmac_param  *) dev->comp_param;
    portmap = (struct dw_dmac_portmap *) dev->base_address;

    if(ch_num == Dmac_all_channels) {
        ch_num &= DMAC_CH_ALL_MASK;
    }

    retVal = dw_dmac_checkChannelRange(dev, ch_num);
    if(retVal != 0) return retVal;
    
    reg = ch_num & (DMAC_MAX_CH_MASK << DMAC_MAX_CHANNELS);
    for(int i=0;i<DMAC_MAX_INTERRUPTS;i++) {
            if(ch_irq & (0x1u<<i))
                switch(i) {
                    case 0 : DMAC_OUTP(reg, portmap->mask_tfr_l); break;
                    case 1 : DMAC_OUTP(reg, portmap->mask_block_l); break;
                    case 2 : DMAC_OUTP(reg, portmap->mask_srctran_l); break;
                    case 3 : DMAC_OUTP(reg, portmap->mask_dsttran_l); break;
                    case 4 : DMAC_OUTP(reg, portmap->mask_err_l); break;
                }
    }

    return retVal;
}

int dw_dmac_unmaskIrq(struct dw_device *dev, enum dw_dmac_channel_number ch_num, enum dw_dmac_irq ch_irq)
{
    struct dw_dmac_param *param;
    struct dw_dmac_portmap *portmap;
    uint32_t reg;
    int retVal = 0;

    DMAC_COMMON_REQUIREMENTS(dev);

    param = (struct dw_dmac_param  *) dev->comp_param;
    portmap = (struct dw_dmac_portmap *) dev->base_address;

    if(ch_num == Dmac_all_channels) {
        ch_num &= DMAC_CH_ALL_MASK;
    }
    
    retVal = dw_dmac_checkChannelRange(dev, ch_num);
    if(retVal != 0) return retVal;

    reg = ch_num;
    for(int i=0;i<DMAC_MAX_INTERRUPTS;i++) {
            if(ch_irq & (0x1u<<i))
                switch(i) {
                    case 0 : DMAC_OUTP(reg, portmap->mask_tfr_l); break;
                    case 1 : DMAC_OUTP(reg, portmap->mask_block_l); break;
                    case 2 : DMAC_OUTP(reg, portmap->mask_srctran_l); break;
                    case 3 : DMAC_OUTP(reg, portmap->mask_dsttran_l); break;
                    case 4 : DMAC_OUTP(reg, portmap->mask_err_l); break;
                }
    }

    return retVal;
}

int dw_dmac_setChannelConfig(struct dw_device *dev, enum   dw_dmac_channel_number ch_num, struct dw_dmac_channel_config *ch)
{
    struct dw_dmac_param *param;
    struct dw_dmac_portmap *portmap;
    uint8_t chanIndex;
    uint32_t reg;
    int retVal = 0;

    DMAC_COMMON_REQUIREMENTS(dev);

    param = (struct dw_dmac_param  *) dev->comp_param;
    portmap = (struct dw_dmac_portmap *) dev->base_address;
    chanIndex = dw_dmac_getChannelIndex(ch_num);

    // Check that selected channel is not busy
    retVal = dw_dmac_checkChannelBusy(dev, ch_num);
    if(retVal != 0) return retVal;

    // check for out of range values
    if(ch->ctl_sms > (unsigned)(param->num_master_int-1) || ch->ctl_dms > (unsigned)(param->num_master_int-1) || ch->llp_lms > (unsigned)(param->num_master_int-1)) retVal = -DW_EINVAL;
    
    if(ch->cfg_ch_prior > (unsigned)(param->num_channels-1)) retVal = -DW_EINVAL;
    
    if((1 << (ch->ctl_src_msize+1)) > param->ch_max_mult_size[chanIndex] || (1 << (ch->ctl_dst_msize+1)) > param->ch_max_mult_size[chanIndex]) retVal = -DW_EINVAL;
    
    if(param->ch_fc[chanIndex] == DMAC_SRC_FC_ONLY && ch->ctl_tt_fc != Dmac_prf2mem_prf && ch->ctl_tt_fc != Dmac_prf2prf_srcprf) retVal = -DW_EINVAL;
    
    if(param->ch_fc[chanIndex] == DMAC_DST_FC_ONLY && ch->ctl_tt_fc != Dmac_mem2prf_prf && ch->ctl_tt_fc != Dmac_prf2prf_dstprf) retVal = -DW_EINVAL;
    
    if(param->ch_fc[chanIndex] == DMAC_DMA_FC_ONLY &&
       (ch->ctl_tt_fc == Dmac_prf2mem_prf    ||
            ch->ctl_tt_fc == Dmac_prf2prf_srcprf ||
            ch->ctl_tt_fc == Dmac_mem2prf_prf    ||
            ch->ctl_tt_fc == Dmac_prf2prf_dstprf))
            retVal = -DW_EINVAL;
    
    if(ch->cfg_dst_per > param->num_hs_int || ch->cfg_src_per > param->num_hs_int) retVal = -DW_EINVAL;
    
    if(retVal != 0) return retVal;
    
    reg = DMAC_INP(portmap->ch[chanIndex].ctl_l);
    reg =
    DW_BIT_BUILD(DMAC_CTL_L_INT_EN, ch->ctl_int_en) |
    DW_BIT_BUILD(DMAC_CTL_L_DST_TR_WIDTH, ch->ctl_dst_tr_width) |
    DW_BIT_BUILD(DMAC_CTL_L_SRC_TR_WIDTH, ch->ctl_src_tr_width) |
    DW_BIT_BUILD(DMAC_CTL_L_DINC, ch->ctl_dinc) |
    DW_BIT_BUILD(DMAC_CTL_L_SINC, ch->ctl_sinc) |
    DW_BIT_BUILD(DMAC_CTL_L_DEST_MSIZE, ch->ctl_dst_msize) |
    DW_BIT_BUILD(DMAC_CTL_L_SRC_MSIZE, ch->ctl_src_msize) |
    DW_BIT_BUILD(DMAC_CTL_L_SRC_GATHER_EN, ch->ctl_src_gather_en) |
    DW_BIT_BUILD(DMAC_CTL_L_DST_SCATTER_EN, ch->ctl_dst_scatter_en)|
    DW_BIT_BUILD(DMAC_CTL_L_TT_FC, ch->ctl_tt_fc) |
    DW_BIT_BUILD(DMAC_CTL_L_DMS, ch->ctl_dms) |
    DW_BIT_BUILD(DMAC_CTL_L_SMS, ch->ctl_sms) |
    DW_BIT_BUILD(DMAC_CTL_L_LLP_DST_EN, ch->ctl_llp_dst_en) |
    DW_BIT_BUILD(DMAC_CTL_L_LLP_SRC_EN, ch->ctl_llp_src_en);

    DMAC_OUTP(reg, portmap->ch[chanIndex].ctl_l);

    reg = DMAC_INP(portmap->ch[chanIndex].ctl_h);
    reg =
    DW_BIT_BUILD(DMAC_CTL_H_BLOCK_TS, ch->ctl_block_ts) |
    DW_BIT_BUILD(DMAC_CTL_H_DONE, ch->ctl_done);

    DMAC_OUTP(reg, portmap->ch[chanIndex].ctl_h);

    reg = DMAC_INP(portmap->ch[chanIndex].cfg_l);
    reg =
    DW_BIT_BUILD(DMAC_CFG_L_CH_PRIOR, ch->cfg_ch_prior) |
    DW_BIT_BUILD(DMAC_CFG_L_HS_SEL_DST, ch->cfg_hs_sel_dst) |
    DW_BIT_BUILD(DMAC_CFG_L_HS_SEL_SRC, ch->cfg_hs_sel_src) |
    DW_BIT_BUILD(DMAC_CFG_L_LOCK_CH_L, ch->cfg_lock_ch_l) |
    DW_BIT_BUILD(DMAC_CFG_L_LOCK_B_L, ch->cfg_lock_b_l) |
    DW_BIT_BUILD(DMAC_CFG_L_LOCK_CH, ch->cfg_lock_ch) |
    DW_BIT_BUILD(DMAC_CFG_L_LOCK_B, ch->cfg_lock_b) |
    DW_BIT_BUILD(DMAC_CFG_L_DST_HS_POL, ch->cfg_dst_hs_pol) |
    DW_BIT_BUILD(DMAC_CFG_L_SRC_HS_POL, ch->cfg_src_hs_pol) |
    DW_BIT_BUILD(DMAC_CFG_L_MAX_ABRST, ch->cfg_max_abrst) |
    DW_BIT_BUILD(DMAC_CFG_L_RELOAD_SRC, ch->cfg_reload_src) |
    DW_BIT_BUILD(DMAC_CFG_L_RELOAD_DST, ch->cfg_reload_dst);

    DMAC_OUTP(reg, portmap->ch[chanIndex].cfg_l);

    reg = DMAC_INP(portmap->ch[chanIndex].cfg_h);
    reg =
    DW_BIT_BUILD(DMAC_CFG_H_FCMODE, ch->cfg_fcmode) |
    DW_BIT_BUILD(DMAC_CFG_H_FIFO_MODE, ch->cfg_fifo_mode) |
    DW_BIT_BUILD(DMAC_CFG_H_PROTCTL, ch->cfg_protctl) |
    DW_BIT_BUILD(DMAC_CFG_H_DS_UPD_EN, ch->cfg_ds_upd_en) |
    DW_BIT_BUILD(DMAC_CFG_H_SS_UPD_EN, ch->cfg_ss_upd_en) |
    DW_BIT_BUILD(DMAC_CFG_H_SRC_PER, ch->cfg_src_per) |
    DW_BIT_BUILD(DMAC_CFG_H_DEST_PER, ch->cfg_dst_per);

    DMAC_OUTP(reg, portmap->ch[chanIndex].cfg_h);

    DMAC_OUTP(ch->sar, portmap->ch[chanIndex].sar_l);
    DMAC_OUTP(ch->dar, portmap->ch[chanIndex].dar_l);

    if(param->ch_hc_llp[chanIndex] == 0) {
            reg = 0;
            reg =
            DW_BIT_BUILD(DMAC_LLP_L_LMS, ch->llp_lms) |
            DW_BIT_BUILD(DMAC_LLP_L_LOC, ch->llp_loc);

            DMAC_OUTP(reg, portmap->ch[chanIndex].llp_l);
    }

    if(param->ch_src_gat_en[chanIndex] == 1) {
            reg = 0;
            reg =
            DW_BIT_BUILD(DMAC_SGR_L_SGI, ch->sgr_sgi) |
            DW_BIT_BUILD(DMAC_SGR_L_SGC, ch->sgr_sgc);

            DMAC_OUTP(reg, portmap->ch[chanIndex].sgr_l);
    }

    if(param->ch_dst_sca_en[chanIndex] == 1) {
            reg = 0;
            reg =
            DW_BIT_BUILD(DMAC_DSR_L_DSI, ch->dsr_dsi) |
            DW_BIT_BUILD(DMAC_DSR_L_DSC, ch->dsr_dsc);

            DMAC_OUTP(reg, portmap->ch[chanIndex].dsr_l);
    }

    if(param->ch_stat_src[chanIndex] == 1) {
            DMAC_OUTP(ch->sstat, portmap->ch[chanIndex].sstat_l);
            DMAC_OUTP(ch->sstatar, portmap->ch[chanIndex].sstatar_l);
    }
    if(param->ch_stat_dst[chanIndex] == 1) {
            DMAC_OUTP(ch->dstat, portmap->ch[chanIndex].dstat_l);
            DMAC_OUTP(ch->dstatar, portmap->ch[chanIndex].dstatar_l);
    }

    return retVal;
}

int dw_dmac_getChannelConfig(struct dw_device *dev, enum   dw_dmac_channel_number ch_num, struct dw_dmac_channel_config *ch)
{
    struct dw_dmac_param *param;
    struct dw_dmac_portmap *portmap;
    uint8_t chanIndex;
    uint32_t reg;
    int retVal = 0;

    DMAC_COMMON_REQUIREMENTS(dev);

    param = (struct dw_dmac_param  *) dev->comp_param;
    portmap = (struct dw_dmac_portmap *) dev->base_address;
    chanIndex = dw_dmac_getChannelIndex(ch_num);

    if(chanIndex == DMAC_MAX_CHANNELS || DMAC_CH_MASK & ch_num || ch_num == Dmac_no_channel) retVal = -DW_ECHRNG;

    if(retVal == 0) {

        reg = DMAC_INP(portmap->ch[chanIndex].ctl_l);

        ch->ctl_int_en = (enum dw_state) DW_BIT_GET(reg, DMAC_CTL_L_INT_EN);
        ch->ctl_dst_tr_width = (enum dw_dmac_transfer_width) DW_BIT_GET(reg, DMAC_CTL_L_DST_TR_WIDTH);
        ch->ctl_src_tr_width = (enum dw_dmac_transfer_width) DW_BIT_GET(reg, DMAC_CTL_L_SRC_TR_WIDTH);
        ch->ctl_dinc = (enum dw_dmac_address_increment) DW_BIT_GET(reg, DMAC_CTL_L_DINC);
        ch->ctl_sinc = (enum dw_dmac_address_increment) DW_BIT_GET(reg, DMAC_CTL_L_SINC);
        ch->ctl_dst_msize = (enum dw_dmac_burst_trans_length) DW_BIT_GET(reg, DMAC_CTL_L_DEST_MSIZE);
        ch->ctl_src_msize = (enum dw_dmac_burst_trans_length) DW_BIT_GET(reg, DMAC_CTL_L_SRC_MSIZE);
        ch->ctl_src_gather_en = (enum dw_state) DW_BIT_GET(reg, DMAC_CTL_L_SRC_GATHER_EN);
        ch->ctl_dst_scatter_en = (enum dw_state) DW_BIT_GET(reg, DMAC_CTL_L_DST_SCATTER_EN);
        ch->ctl_tt_fc = (enum dw_dmac_transfer_flow) DW_BIT_GET(reg, DMAC_CTL_L_TT_FC);
        ch->ctl_dms = (enum dw_dmac_master_number) DW_BIT_GET(reg, DMAC_CTL_L_DMS);
        ch->ctl_sms = (enum dw_dmac_master_number) DW_BIT_GET(reg, DMAC_CTL_L_SMS);
        ch->ctl_llp_dst_en = (enum dw_state) DW_BIT_GET(reg, DMAC_CTL_L_LLP_DST_EN);
        ch->ctl_llp_src_en = (enum dw_state) DW_BIT_GET(reg, DMAC_CTL_L_LLP_SRC_EN);

        reg = DMAC_INP(portmap->ch[chanIndex].ctl_h);

        ch->ctl_block_ts = DW_BIT_GET(reg, DMAC_CTL_H_BLOCK_TS);
        ch->ctl_done = (enum dw_state) DW_BIT_GET(reg, DMAC_CTL_H_DONE);

        reg = DMAC_INP(portmap->ch[chanIndex].cfg_l);

        ch->cfg_ch_prior = (enum dw_dmac_channel_priority) DW_BIT_GET(reg, DMAC_CFG_L_CH_PRIOR);
        ch->cfg_hs_sel_dst = (enum dw_dmac_sw_hw_hs_select) DW_BIT_GET(reg, DMAC_CFG_L_HS_SEL_DST);
        ch->cfg_hs_sel_src = (enum dw_dmac_sw_hw_hs_select) DW_BIT_GET(reg, DMAC_CFG_L_HS_SEL_SRC);
        ch->cfg_lock_ch_l = (enum dw_dmac_lock_level) DW_BIT_GET(reg, DMAC_CFG_L_LOCK_CH_L);
        ch->cfg_lock_b_l = (enum dw_dmac_lock_level) DW_BIT_GET(reg, DMAC_CFG_L_LOCK_B_L);
        ch->cfg_lock_ch = (enum dw_state) DW_BIT_GET(reg, DMAC_CFG_L_LOCK_CH);
        ch->cfg_lock_b = (enum dw_state) DW_BIT_GET(reg, DMAC_CFG_L_LOCK_B);
        ch->cfg_dst_hs_pol = (enum dw_dmac_polarity_level) DW_BIT_GET(reg, DMAC_CFG_L_DST_HS_POL);
        ch->cfg_src_hs_pol = (enum dw_dmac_polarity_level) DW_BIT_GET(reg, DMAC_CFG_L_SRC_HS_POL);
        ch->cfg_max_abrst = DW_BIT_GET(reg, DMAC_CFG_L_MAX_ABRST);
        ch->cfg_reload_src = (enum dw_state) DW_BIT_GET(reg, DMAC_CFG_L_RELOAD_SRC);
        ch->cfg_reload_dst = (enum dw_state) DW_BIT_GET(reg, DMAC_CFG_L_RELOAD_DST);

        reg = DMAC_INP(portmap->ch[chanIndex].cfg_h);

        ch->cfg_fcmode = (enum dw_dmac_flow_ctl_mode) DW_BIT_GET(reg, DMAC_CFG_H_FCMODE);
        ch->cfg_fifo_mode = (enum dw_dmac_fifo_mode) DW_BIT_GET(reg, DMAC_CFG_H_FIFO_MODE);
        ch->cfg_protctl = (enum dw_dmac_prot_level) DW_BIT_GET(reg, DMAC_CFG_H_PROTCTL);
        ch->cfg_ds_upd_en = (enum dw_state) DW_BIT_GET(reg, DMAC_CFG_H_DS_UPD_EN);
        ch->cfg_ss_upd_en = (enum dw_state) DW_BIT_GET(reg, DMAC_CFG_H_SS_UPD_EN);
        ch->cfg_src_per = (enum dw_dmac_hs_interface) DW_BIT_GET(reg, DMAC_CFG_H_SRC_PER);
        ch->cfg_dst_per = (enum dw_dmac_hs_interface) DW_BIT_GET(reg, DMAC_CFG_H_DEST_PER);

        ch->sar = DMAC_INP(portmap->ch[chanIndex].sar_l);
        ch->dar = DMAC_INP(portmap->ch[chanIndex].dar_l);

        if(param->ch_hc_llp[chanIndex] == 0) {
            reg = DMAC_INP(portmap->ch[chanIndex].llp_l);
            ch->llp_lms = (enum dw_dmac_master_number) DW_BIT_GET(reg, DMAC_LLP_L_LMS);
            ch->llp_loc = DW_BIT_GET(reg, DMAC_LLP_L_LOC);
        } else {
            ch->llp_lms = (enum dw_dmac_master_number) 0;
            ch->llp_loc = 0;
        }

        if(param->ch_stat_src[chanIndex] == 1) {
            ch->sstat = DMAC_INP(portmap->ch[chanIndex].sstat_l);
            ch->sstatar = DMAC_INP(portmap->ch[chanIndex].sstatar_l);
        } else {
            ch->sstat = 0;
            ch->sstatar = 0;
        }

        if(param->ch_stat_dst[chanIndex] == 1) {
            ch->dstat = DMAC_INP(portmap->ch[chanIndex].dstat_l);
            ch->dstatar = DMAC_INP(portmap->ch[chanIndex].dstatar_l);
        } else {
            ch->dstat = 0;
            ch->dstatar = 0;
        }

        if(param->ch_src_gat_en[chanIndex] == 0x1) {
            reg = DMAC_INP(portmap->ch[chanIndex].sgr_l);
            ch->sgr_sgc = DW_BIT_GET(reg, DMAC_SGR_L_SGC);
            ch->sgr_sgi = DW_BIT_GET(reg, DMAC_SGR_L_SGI);
        } else {
            ch->sgr_sgc = 0x0;
            ch->sgr_sgi = 0x0;
        }
        
        if(param->ch_dst_sca_en[chanIndex] == 0x1) {
            reg = DMAC_INP(portmap->ch[chanIndex].dsr_l);
            ch->dsr_dsc = DW_BIT_GET(reg, DMAC_DSR_L_DSC);
            ch->dsr_dsi = DW_BIT_GET(reg, DMAC_DSR_L_DSI);
        } else {
            ch->dsr_dsc = 0x0;
            ch->dsr_dsi = 0x0;
        }

    }
    return retVal;
}

int dw_dmac_setTransferType(struct dw_device *dev, enum dw_dmac_channel_number ch_num, enum dw_dmac_transfer_type transfer)
{
    struct dw_dmac_param *param;
    int retVal = 0;

    DMAC_COMMON_REQUIREMENTS(dev);

    param = (struct dw_dmac_param  *) dev->comp_param;

    if(ch_num == Dmac_all_channels) {
        ch_num &= DMAC_CH_ALL_MASK;
    }

    retVal = dw_dmac_checkChannelBusy(dev, ch_num);
    if (retVal != 0) return retVal;
    
    switch(transfer) {
            case Dmac_transfer_row1 :
                retVal = dw_dmac_setListPointerAddress(dev, ch_num, 0);
                if(retVal == 0) retVal = dw_dmac_setLlpEnable(dev, ch_num, Dmac_src_dst, Dw_clear);
                if(retVal == 0) retVal = dw_dmac_setReload(dev, ch_num, Dmac_src_dst, Dw_clear);
                break;
            case  Dmac_transfer_row2 :
                retVal = dw_dmac_setListPointerAddress(dev, ch_num, 0);
                if(retVal == 0) retVal = dw_dmac_setLlpEnable(dev, ch_num, Dmac_src_dst, Dw_clear);
            if(retVal == 0) retVal = dw_dmac_setReload(dev, ch_num, Dmac_src, Dw_clear);
                if(retVal == 0) retVal = dw_dmac_setReload(dev, ch_num, Dmac_dst, Dw_set);
                break;
            case  Dmac_transfer_row3 :
                retVal = dw_dmac_setListPointerAddress(dev, ch_num, 0);
                if(retVal == 0) retVal = dw_dmac_setLlpEnable(dev, ch_num, Dmac_src_dst, Dw_clear);
                if(retVal == 0) retVal = dw_dmac_setReload(dev, ch_num, Dmac_src, Dw_set);
                if(retVal == 0) retVal = dw_dmac_setReload(dev, ch_num, Dmac_dst, Dw_clear);
                break;
            case  Dmac_transfer_row4 :
                retVal = dw_dmac_setListPointerAddress(dev, ch_num, 0);
                if(retVal == 0) retVal = dw_dmac_setLlpEnable(dev, ch_num, Dmac_src_dst, Dw_clear);
                if(retVal == 0) retVal = dw_dmac_setReload(dev, ch_num, Dmac_src_dst, Dw_set);
                break;
            case  Dmac_transfer_row5 :
                retVal = dw_dmac_setLlpEnable(dev, ch_num, Dmac_src_dst, Dw_clear);
                if(retVal == 0) retVal = dw_dmac_setReload(dev, ch_num, Dmac_src_dst,Dw_clear);
                break;
            case  Dmac_transfer_row6 :
                retVal = dw_dmac_setLlpEnable(dev, ch_num, Dmac_src, Dw_clear);
                if(retVal == 0) retVal = dw_dmac_setLlpEnable(dev, ch_num, Dmac_dst, Dw_set);
                if(retVal == 0) retVal = dw_dmac_setReload(dev, ch_num, Dmac_src_dst, Dw_clear);
                break;
            case  Dmac_transfer_row7 :
                retVal = dw_dmac_setLlpEnable(dev, ch_num, Dmac_src, Dw_clear);
                if(retVal == 0) retVal = dw_dmac_setLlpEnable(dev, ch_num, Dmac_dst, Dw_set);
                if(retVal == 0) retVal = dw_dmac_setReload(dev, ch_num, Dmac_src, Dw_set);
                if(retVal == 0) retVal = dw_dmac_setReload(dev, ch_num, Dmac_dst, Dw_clear);
                break;
            case  Dmac_transfer_row8 :
                retVal = dw_dmac_setLlpEnable(dev, ch_num, Dmac_src, Dw_set);
                if(retVal == 0) retVal = dw_dmac_setLlpEnable(dev, ch_num, Dmac_dst, Dw_clear);
                if(retVal == 0) retVal = dw_dmac_setReload(dev, ch_num, Dmac_src_dst, Dw_clear);
                break;
            case  Dmac_transfer_row9 :
                retVal = dw_dmac_setLlpEnable(dev, ch_num, Dmac_src, Dw_set);
                if(retVal == 0) retVal = dw_dmac_setLlpEnable(dev, ch_num, Dmac_dst, Dw_clear);
                if(retVal == 0) retVal = dw_dmac_setReload(dev, ch_num, Dmac_src, Dw_clear);
                if(retVal == 0) retVal = dw_dmac_setReload(dev, ch_num, Dmac_dst, Dw_set);
                break;
            case  Dmac_transfer_row10 :
                retVal = dw_dmac_setLlpEnable(dev, ch_num, Dmac_src_dst, Dw_set);
                if(retVal == 0) retVal = dw_dmac_setReload(dev, ch_num, Dmac_src_dst, Dw_clear);
                break;
    }

    return retVal;
}

enum dw_dmac_transfer_type dw_dmac_getTransferType(struct dw_device *dev, enum dw_dmac_channel_number ch_num)
{
    struct dw_dmac_param *param;
    struct dw_dmac_portmap *portmap;
    uint8_t chanIndex, row;
    uint32_t llp_reg, ctl_reg, cfg_reg;
    enum dw_dmac_transfer_type retVal;
    //int retValFunc = 0;

    DMAC_COMMON_REQUIREMENTS(dev);

    param = (struct dw_dmac_param  *) dev->comp_param;
    portmap = (struct dw_dmac_portmap *) dev->base_address;
    
    chanIndex = dw_dmac_getChannelIndex(ch_num);

    DW_REQUIRE(chanIndex != DMAC_MAX_CHANNELS);
    DW_REQUIRE(!(DMAC_CH_MASK & ch_num));
    
    llp_reg = DMAC_INP(portmap->ch[chanIndex].llp_l);
    ctl_reg = DMAC_INP(portmap->ch[chanIndex].ctl_l);
    cfg_reg = DMAC_INP(portmap->ch[chanIndex].cfg_l);

    row     = 0x0;
    if(DW_BIT_GET(cfg_reg, DMAC_CFG_L_RELOAD_DST)) row |= 0x01;
    if(DW_BIT_GET(ctl_reg, DMAC_CTL_L_LLP_DST_EN)) row |= 0x02;
    if(DW_BIT_GET(cfg_reg, DMAC_CFG_L_RELOAD_SRC)) row |= 0x04;
    if(DW_BIT_GET(ctl_reg, DMAC_CTL_L_LLP_SRC_EN)) row |= 0x08;
    if(DW_BIT_GET(llp_reg, DMAC_LLP_L_LOC))        row |= 0x10;

    switch(row) {
        case 0x00 : retVal = Dmac_transfer_row1;   break;
        case 0x01 : retVal = Dmac_transfer_row2;   break;
        case 0x04 : retVal = Dmac_transfer_row3;   break;
        case 0x05 : retVal = Dmac_transfer_row4;   break;
        case 0x10 : retVal = Dmac_transfer_row5;   break;
        case 0x12 : retVal = Dmac_transfer_row6;   break;
        case 0x16 : retVal = Dmac_transfer_row7;   break;
        case 0x18 : retVal = Dmac_transfer_row8;   break;
        case 0x19 : retVal = Dmac_transfer_row9;   break;
        case 0x1a : retVal = Dmac_transfer_row10;  break;
    }
    
    return retVal;
}

void dw_dmac_setTestMode(struct dw_device *dev, enum dw_state state_val)
{
    struct dw_dmac_portmap *portmap;
    uint32_t reg = 0;

    DMAC_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_dmac_portmap *) dev->base_address;
    DW_BIT_SET(reg, DMAC_DMATESTREG_L_TEST_SLV_IF, state_val);
    DMAC_OUTP(reg, portmap->dma_test_reg_l);
}

enum dw_state dw_dmac_getTestMode(struct dw_device *dev)
{
    struct dw_dmac_portmap *portmap;
    uint32_t reg;
    enum dw_state retVal;

    DMAC_COMMON_REQUIREMENTS(dev);

    portmap = (struct dw_dmac_portmap *) dev->base_address;
    reg = DMAC_INP(portmap->dma_test_reg_l);
    retVal = (enum dw_state) DW_BIT_GET(reg, DMAC_DMATESTREG_L_TEST_SLV_IF);
    return retVal;
}

enum dw_dmac_transfer_flow dw_dmac_getMemPeriphFlowCtl(struct dw_device *dev, enum dw_dmac_channel_number ch_num)
{
    struct dw_dmac_param *param;
    struct dw_dmac_portmap *portmap;
    uint8_t chanIndex;
    uint32_t reg;
    enum dw_dmac_transfer_flow retVal;

    DMAC_COMMON_REQUIREMENTS(dev);

    param = (struct dw_dmac_param  *) dev->comp_param;
    portmap = (struct dw_dmac_portmap *) dev->base_address;
    
    DW_REQUIRE(!(DMAC_CH_MASK & ch_num));
    
    chanIndex = dw_dmac_getChannelIndex(ch_num);
    
    DW_REQUIRE(chanIndex != DMAC_MAX_CHANNELS);

    reg = DMAC_INP(portmap->ch[chanIndex].ctl_l);
    
    retVal = (enum dw_dmac_transfer_flow) DW_BIT_GET(reg, DMAC_CTL_L_TT_FC);
    
    return retVal;
}

enum dw_dmac_burst_trans_length dw_dmac_getBurstTransLength(struct dw_device *dev, enum dw_dmac_channel_number ch_num, enum dw_dmac_src_dst_select sd_sel)
{
    struct dw_dmac_param *param;
    struct dw_dmac_portmap *portmap;
    uint8_t chanIndex;
    uint32_t reg;
    enum dw_dmac_burst_trans_length retVal;

    DMAC_COMMON_REQUIREMENTS(dev);

    param = (struct dw_dmac_param  *) dev->comp_param;
    portmap = (struct dw_dmac_portmap *) dev->base_address;
    
    DW_REQUIRE(!(DMAC_CH_MASK & ch_num));
    DW_REQUIRE(sd_sel == Dmac_src || sd_sel == Dmac_dst);
    
    chanIndex = dw_dmac_getChannelIndex(ch_num);

    DW_REQUIRE(chanIndex != DMAC_MAX_CHANNELS);

    reg = DMAC_INP(portmap->ch[chanIndex].ctl_l);
    if(sd_sel == Dmac_src) retVal = (enum dw_dmac_burst_trans_length) DW_BIT_GET(reg, DMAC_CTL_L_SRC_MSIZE);
    else retVal = (enum dw_dmac_burst_trans_length) DW_BIT_GET(reg, DMAC_CTL_L_DEST_MSIZE);

    return retVal;
}

enum dw_dmac_transfer_width dw_dmac_getTransWidth(struct dw_device *dev, enum dw_dmac_channel_number ch_num, enum dw_dmac_src_dst_select sd_sel)
{
    struct dw_dmac_param *param;
    struct dw_dmac_portmap *portmap;
    uint8_t chanIndex;
    uint32_t reg;
    enum dw_dmac_transfer_width retVal;

    DMAC_COMMON_REQUIREMENTS(dev);

    param = (struct dw_dmac_param  *) dev->comp_param;
    portmap = (struct dw_dmac_portmap *) dev->base_address;
    
    DW_REQUIRE(!(DMAC_CH_MASK & ch_num));
    DW_REQUIRE(sd_sel == Dmac_src || sd_sel == Dmac_dst);
    
    chanIndex = dw_dmac_getChannelIndex(ch_num);

    DW_REQUIRE(chanIndex != DMAC_MAX_CHANNELS);

    reg = DMAC_INP(portmap->ch[chanIndex].ctl_l);
    if(sd_sel == Dmac_src) retVal = (enum dw_dmac_transfer_width) DW_BIT_GET(reg, DMAC_CTL_L_SRC_TR_WIDTH);
    else retVal = (enum dw_dmac_transfer_width) DW_BIT_GET(reg, DMAC_CTL_L_DST_TR_WIDTH);

    return retVal;
}

enum dw_dmac_sw_hw_hs_select dw_dmac_getHandshakingMode(struct dw_device *dev, enum dw_dmac_channel_number ch_num, enum dw_dmac_src_dst_select sd_sel)
{
    struct dw_dmac_param *param;
    struct dw_dmac_portmap *portmap;
    uint8_t chanIndex;
    uint32_t reg;
    enum dw_dmac_sw_hw_hs_select retVal;

    DMAC_COMMON_REQUIREMENTS(dev);

    param = (struct dw_dmac_param  *) dev->comp_param;
    portmap = (struct dw_dmac_portmap *) dev->base_address;
    
    DW_REQUIRE(!(DMAC_CH_MASK & ch_num));
    DW_REQUIRE(sd_sel == Dmac_src || sd_sel == Dmac_dst);
    
    chanIndex = dw_dmac_getChannelIndex(ch_num);
    
    DW_REQUIRE(chanIndex != DMAC_MAX_CHANNELS);

    reg = DMAC_INP(portmap->ch[chanIndex].cfg_l);
    if(sd_sel == Dmac_src) retVal = (enum dw_dmac_sw_hw_hs_select) DW_BIT_GET(reg, DMAC_CFG_L_HS_SEL_SRC);
    else retVal = (enum dw_dmac_sw_hw_hs_select) DW_BIT_GET(reg, DMAC_CFG_L_HS_SEL_DST);

    return retVal;
}

int dw_dmac_setListPointerAddress(struct   dw_device *dev, enum dw_dmac_channel_number ch_num, uint32_t address)
{
    struct dw_dmac_param *param;
    struct dw_dmac_portmap *portmap;
    uint32_t reg;
    int retVal = 0;

    DMAC_COMMON_REQUIREMENTS(dev);

    param = (struct dw_dmac_param  *) dev->comp_param;
    portmap = (struct dw_dmac_portmap *) dev->base_address;

    if(ch_num == Dmac_all_channels) {
        ch_num &= DMAC_CH_ALL_MASK;
    }

    retVal = dw_dmac_checkChannelBusy(dev, ch_num);
    if(retVal != 0) return retVal;

    for(int i=0; i<param->num_channels; i++)
            if((ch_num & (0x1u<<i)) && (param->ch_hc_llp[i] == 1))
                    retVal = -DW_ENOSYS;
    
    if(retVal != 0) return retVal;

    for(int i=0; i<param->num_channels; i++)
            if(ch_num & (0x1u<<i)) {
                reg = DMAC_INP(portmap->ch[i].llp_l);
                if(DW_BIT_GET(reg, DMAC_LLP_L_LOC) != address) {
                    DW_BIT_SET(reg, DMAC_LLP_L_LOC, address);
                    DMAC_OUTP(reg, portmap->ch[i].llp_l);
                }
            }

    return retVal;
}

int dw_dmac_setLlpEnable(struct dw_device *dev, enum dw_dmac_channel_number ch_num, enum dw_dmac_src_dst_select sd_sel, enum dw_state value)
{
    struct dw_dmac_param *param;
    struct dw_dmac_portmap *portmap;
    uint32_t reg;
    int retVal = 0, writeReg;
    
    DMAC_COMMON_REQUIREMENTS(dev);

    param = (struct dw_dmac_param  *) dev->comp_param;
    portmap = (struct dw_dmac_portmap *) dev->base_address;

    if(ch_num == Dmac_all_channels) {
        ch_num &= DMAC_CH_ALL_MASK;
    }

    retVal = dw_dmac_checkChannelBusy(dev, ch_num);
    if(retVal != 0) return retVal;
    
    for(int i=0; i<param->num_channels; i++)
            if((ch_num & (0x1u<<i)) && (param->ch_hc_llp[i] == 1 || param->ch_multi_blk_en[i] == 0)) {
            retVal = -DW_ENOSYS;
                break;
            }
        
    if(retVal != 0) return retVal;

    for(int i=0; i<param->num_channels; i++) {
            if(ch_num & (0x1u<<i)) {
                writeReg = 0;
                reg = DMAC_INP(portmap->ch[i].ctl_l);
                if((sd_sel == Dmac_src || sd_sel == Dmac_src_dst) && ((enum dw_state)DW_BIT_GET(reg, DMAC_CTL_L_LLP_SRC_EN) != value)) {
                        DW_BIT_SET(reg, DMAC_CTL_L_LLP_SRC_EN, value);
                        writeReg = 1;
                }
                if((sd_sel == Dmac_dst || sd_sel == Dmac_src_dst) && ((enum dw_state)DW_BIT_GET(reg, DMAC_CTL_L_LLP_DST_EN) != value)) {
                        DW_BIT_SET(reg, DMAC_CTL_L_LLP_DST_EN, value);
                        writeReg = 1;
                }
                if(writeReg == 1) {
                    DMAC_OUTP(reg, portmap->ch[i].ctl_l);
                }
            }
    }
    
    return retVal;
}

int dw_dmac_setReload(struct dw_device *dev, enum dw_dmac_channel_number ch_num, enum dw_dmac_src_dst_select sd_sel, enum dw_state value)
{
    struct dw_dmac_param *param;
    struct dw_dmac_portmap *portmap;
    uint32_t reg;
    int retVal = 0, writeReg;

    DMAC_COMMON_REQUIREMENTS(dev);

    param = (struct dw_dmac_param  *) dev->comp_param;
    portmap = (struct dw_dmac_portmap *) dev->base_address;

    if(ch_num == Dmac_all_channels) {
        ch_num &= DMAC_CH_ALL_MASK;
    }

    retVal = dw_dmac_checkChannelRange(dev, ch_num);
    if(retVal != 0) return retVal;
    
    // Check for hard-coded values
    for(int i=0; i<param->num_channels; i++)
            if(ch_num & (0x1u<<i)) {
                if(param->ch_multi_blk_en[i] == 0) {
                    retVal = -DW_ENOSYS;
                    break;
                }
                if(param->ch_multi_blk_type[i] != 0) {
                    if(sd_sel == Dmac_src || sd_sel == Dmac_src_dst) {
                        if(value == Dw_set && (param->ch_multi_blk_type[i] != Dmac_transfer_row3) && (param->ch_multi_blk_type[i] != Dmac_transfer_row7)) {
                            retVal = -DW_ENOSYS;
                            break;
                        }
                        if(value == Dw_clear && ((param->ch_multi_blk_type[i] == Dmac_transfer_row3) || (param->ch_multi_blk_type[i] == Dmac_transfer_row7))) {
                            retVal = -DW_ENOSYS;
                            break;
                        }
                    }
                    if(sd_sel == Dmac_dst || sd_sel == Dmac_src_dst) {
                        if(value == Dw_set && (param->ch_multi_blk_type[i] != Dmac_transfer_row2) && (param->ch_multi_blk_type[i] != Dmac_transfer_row4)
			                   && (param->ch_multi_blk_type[i] != Dmac_transfer_row9)) {
                            retVal = -DW_ENOSYS;
                            break;
                        }
                        if(value == Dw_clear && (param->ch_multi_blk_type[i] == Dmac_transfer_row2 || param->ch_multi_blk_type[i] == Dmac_transfer_row4
			                     ||  param->ch_multi_blk_type[i] == Dmac_transfer_row9)) {
                            retVal = -DW_ENOSYS;
                            break;
                        }
                    }
                }
            }
	    
    if(retVal != 0) return retVal;

    for(int i=0; i<param->num_channels; i++)
    	if(ch_num & (0x1u<<i)) {
    	    writeReg = 0;
    	    reg = DMAC_INP(portmap->ch[i].cfg_l);
    	    if((sd_sel == Dmac_src || sd_sel == Dmac_src_dst) && ((enum dw_state)DW_BIT_GET(reg, DMAC_CFG_L_RELOAD_SRC) != value)) {
    		DW_BIT_SET(reg, DMAC_CFG_L_RELOAD_SRC, value);
    		writeReg = 1;
    	    }
    	    if((sd_sel == Dmac_dst || sd_sel == Dmac_src_dst) && ((enum dw_state)DW_BIT_GET(reg, DMAC_CFG_L_RELOAD_DST) != value)) {
    		DW_BIT_SET(reg, DMAC_CFG_L_RELOAD_DST, value);
    		writeReg = 1;
    	    }
    	    if(writeReg == 1) {
    		DMAC_OUTP(reg, portmap->ch[i].cfg_l);
    	    }
    	}

    return retVal;
}

enum dw_state dw_dmac_getReload(struct dw_device *dev, enum dw_dmac_channel_number ch_num, enum dw_dmac_src_dst_select sd_sel)
{
    struct dw_dmac_param *param;
    struct dw_dmac_portmap *portmap;
    uint8_t chanIndex;
    uint32_t reg;
    enum dw_state retVal;

    DMAC_COMMON_REQUIREMENTS(dev);

    param = (struct dw_dmac_param  *) dev->comp_param;
    portmap = (struct dw_dmac_portmap *) dev->base_address;
    
    DW_REQUIRE(!(DMAC_CH_MASK & ch_num));
    DW_REQUIRE(sd_sel == Dmac_src || sd_sel == Dmac_dst);
    
    chanIndex = dw_dmac_getChannelIndex(ch_num);

    DW_REQUIRE(chanIndex != DMAC_MAX_CHANNELS);

    reg = DMAC_INP(portmap->ch[chanIndex].cfg_l);
    if(sd_sel == Dmac_src) retVal = (enum dw_state) DW_BIT_GET(reg, DMAC_CFG_L_RELOAD_SRC);
    else retVal = (enum dw_state) DW_BIT_GET(reg, DMAC_CFG_L_RELOAD_DST);

    return retVal;
}

uint8_t dw_dmac_getChannelIndex(enum dw_dmac_channel_number ch_num)
{
    uint8_t retIndex=0;
    
    ch_num &= DMAC_MAX_CH_MASK;
    for(unsigned enumVal = 1; retIndex<DMAC_MAX_CHANNELS; retIndex++, enumVal*=2)
        if(enumVal == ch_num) break;
        
    return retIndex;
}  

int dw_dmac_irqHandler(struct dw_device *dev)
{
    struct dw_dmac_param *param;
    struct dw_dmac_portmap *portmap;
    dw_callback userCallback;
    struct dw_dmac_instance *instance;
    uint32_t reg;
    int retVal= true, chanIndex, callbackArg;
    uint32_t activeIntReq, mask;

    DMAC_COMMON_REQUIREMENTS(dev);

    param = (struct dw_dmac_param  *) dev->comp_param;
    instance = (struct dw_dmac_instance *) dev->instance;
    portmap = (struct dw_dmac_portmap *) dev->base_address;

    activeIntReq = DMAC_INP(portmap->status_int_l);

    if(DW_BIT_GET(activeIntReq, DMAC_STATUSINT_L_ERR)) {
        callbackArg = Dmac_irq_err;
	
        reg = DMAC_INP(portmap->status_err_l);

        for(int i=0; i<param->num_channels; i++) {
            mask = 0x1u << instance->ch_order[i];
            if(reg & mask) {
                chanIndex = instance->ch_order[i];
                break;
            }
        }

        DW_REQUIRE(instance->ch[chanIndex].userListener != NULL);
        userCallback = instance->ch[chanIndex].userListener;
        userCallback(dev, callbackArg);

        DMAC_OUTP(mask, portmap->clear_err_l);
    }
    else if(DW_BIT_GET(activeIntReq, DMAC_STATUSINT_L_TFR)) {
        reg = DMAC_INP(portmap->status_tfr_l);
        
        for(int i=0; i<param->num_channels; i++) {
            mask = 0x1u << instance->ch_order[i];
            if(reg & mask) {
                chanIndex = instance->ch_order[i];
                break;
            }
        }

        instance->ch[chanIndex].block_cnt++;

        if(instance->ch[chanIndex].userCallback != NULL) {
            callbackArg = instance->ch[chanIndex].block_cnt;
            userCallback = instance->ch[chanIndex].userCallback;
            userCallback(dev, callbackArg);
        }

        dw_dmac_disableChannelIrq(dev, instance->ch[chanIndex].ch_num);
        instance->ch[chanIndex].src_byte_cnt = 0;
        instance->ch[chanIndex].dst_byte_cnt = 0;
        instance->ch[chanIndex].src_state = Dmac_idle;
        instance->ch[chanIndex].dst_state = Dmac_idle;
        DMAC_OUTP(mask, portmap->clear_srctran_l);
        DMAC_OUTP(mask, portmap->clear_dsttran_l);
        DMAC_OUTP(mask, portmap->clear_block_l);
        
        dw_dmac_maskIrq(dev, instance->ch[chanIndex].ch_num, Dmac_irq_all);

        DMAC_OUTP(mask, portmap->clear_tfr_l);
    }
    else if(DW_BIT_GET(activeIntReq, DMAC_STATUSINT_L_BLOCK)) {
        callbackArg = Dmac_irq_block;
	
        reg = DMAC_INP(portmap->status_block_l);

        for(int i=0; i<param->num_channels; i++) {
            mask = 0x1u << instance->ch_order[i];
            if(reg & mask) {
                chanIndex = instance->ch_order[i];
                break;
            }
        }

        instance->ch[chanIndex].block_cnt++;

        if(instance->ch[chanIndex].block_cnt == instance->ch[chanIndex].total_blocks-1)
            switch(instance->ch[chanIndex].trans_type) {
                case Dmac_transfer_row2 :
                case Dmac_transfer_row9 :
                    dw_dmac_setReload(dev, instance->ch[chanIndex].ch_num, Dmac_dst, Dw_clear);
                    break;
                case Dmac_transfer_row3 :
                case Dmac_transfer_row7 :
                    dw_dmac_setReload(dev, instance->ch[chanIndex].ch_num, Dmac_src, Dw_clear);
                    break;
                case Dmac_transfer_row4 :
                    dw_dmac_setReload(dev, instance->ch[chanIndex].ch_num, Dmac_src_dst, Dw_clear);
                    break;
                default :
                    break;
            }

        DW_REQUIRE(instance->ch[chanIndex].userListener != NULL);
        userCallback = instance->ch[chanIndex].userListener;
        userCallback(dev, callbackArg);

        instance->ch[chanIndex].src_byte_cnt = 0;
        instance->ch[chanIndex].dst_byte_cnt = 0;
        instance->ch[chanIndex].src_state = Dmac_burst_region;
        instance->ch[chanIndex].dst_state = Dmac_burst_region;
        DMAC_OUTP(mask, portmap->clear_srctran_l);
        DMAC_OUTP(mask, portmap->clear_dsttran_l);
        
        DMAC_OUTP(mask, portmap->clear_block_l);
    }
    else if(DW_BIT_GET(activeIntReq, DMAC_STATUSINT_L_SRCTRAN)) {
        callbackArg = Dmac_irq_srctran;
	
        reg = DMAC_INP(portmap->status_srctran_l);
	
        for(int i=0; i<param->num_channels; i++) {
            mask = 0x1u << instance->ch_order[i];
            if(reg & mask) {
                chanIndex = instance->ch_order[i];
                break;
            }
        }

        if(instance->ch[chanIndex].src_state == Dmac_single_region)
            instance->ch[chanIndex].src_byte_cnt += instance->ch[chanIndex].src_single_inc;
        if(instance->ch[chanIndex].src_state == Dmac_burst_region)
            instance->ch[chanIndex].src_byte_cnt += instance->ch[chanIndex].src_burst_inc;

        DW_REQUIRE(instance->ch[chanIndex].userListener != NULL);
        userCallback = instance->ch[chanIndex].userListener;
        userCallback(dev, callbackArg);

        DMAC_OUTP(mask, portmap->clear_srctran_l);
    }
    else if(DW_BIT_GET(activeIntReq, DMAC_STATUSINT_L_DSTTRAN)) {
        callbackArg = Dmac_irq_dsttran;
	
        reg = DMAC_INP(portmap->status_dsttran_l);
	
        for(int i=0; i<param->num_channels; i++) {
            mask = 0x1u << instance->ch_order[i];
            if(reg & mask) {
                chanIndex = instance->ch_order[i];
                break;
            }
        }

        if(instance->ch[chanIndex].dst_state == Dmac_single_region)
            instance->ch[chanIndex].dst_byte_cnt += instance->ch[chanIndex].dst_single_inc;
        if(instance->ch[chanIndex].dst_state == Dmac_burst_region)
            instance->ch[chanIndex].dst_byte_cnt += instance->ch[chanIndex].dst_burst_inc;

        DW_REQUIRE(instance->ch[chanIndex].userListener != NULL);
        userCallback = instance->ch[chanIndex].userListener;
        userCallback(dev, callbackArg);

        DMAC_OUTP(mask, portmap->clear_dsttran_l);
    }
    else {
        retVal = false;
    }

    return retVal;
}
    
int dw_dmac_prepareTransfer(struct dw_device *dev, enum dw_dmac_channel_number ch_num, int num_blocks, dw_callback cb_func)
{
    struct dw_dmac_instance *instance;
    uint8_t chanIndex;
    int retVal = 0;

    DMAC_COMMON_REQUIREMENTS(dev);

    instance = (struct dw_dmac_instance *) dev->instance;

    chanIndex = dw_dmac_getChannelIndex(ch_num);
        
    if(chanIndex == DMAC_MAX_CHANNELS) retVal = -DW_ECHRNG;
    else retVal = dw_dmac_checkChannelBusy(dev, ch_num);

    if(retVal == 0) retVal = dw_dmac_disableChannelIrq(dev, ch_num);

    if(retVal != 0) return retVal;

    instance->ch[chanIndex].src_byte_cnt = 0;
    instance->ch[chanIndex].dst_byte_cnt = 0;
    instance->ch[chanIndex].block_cnt = 0;
    
    instance->ch[chanIndex].src_state = Dmac_burst_region;
    instance->ch[chanIndex].dst_state = Dmac_burst_region;
    
    instance->ch[chanIndex].userCallback = cb_func;
    instance->ch[chanIndex].total_blocks = num_blocks;

    instance->ch[chanIndex].trans_type = dw_dmac_getTransferType(dev, ch_num);

    instance->ch[chanIndex].src_single_inc = (POW2(dw_dmac_getTransWidth(dev, ch_num, Dmac_src)+3)/8);
    instance->ch[chanIndex].src_burst_inc = (instance->ch[chanIndex].src_single_inc * POW2(dw_dmac_getBurstTransLength(dev, ch_num, Dmac_src)+1));
    
    instance->ch[chanIndex].dst_single_inc = (POW2(dw_dmac_getTransWidth(dev, ch_num, Dmac_dst)+3)/8);
    instance->ch[chanIndex].dst_burst_inc  = (instance->ch[chanIndex].dst_single_inc * POW2(dw_dmac_getBurstTransLength(dev, ch_num, Dmac_dst)+1));
    
    dw_dmac_unmaskIrq(dev, ch_num, Dmac_irq_err);
    dw_dmac_unmaskIrq(dev, ch_num, Dmac_irq_tfr);
    dw_dmac_unmaskIrq(dev, ch_num, Dmac_irq_block);

    instance->ch[chanIndex].tt_fc = dw_dmac_getMemPeriphFlowCtl(dev, ch_num);

    if((dw_dmac_getHandshakingMode(dev, ch_num, Dmac_src) == Dmac_hs_software) &&
       (instance->ch[chanIndex].tt_fc == Dmac_prf2prf_srcprf || instance->ch[chanIndex].tt_fc == Dmac_prf2prf_dstprf ||
	instance->ch[chanIndex].tt_fc == Dmac_prf2mem_dma    || instance->ch[chanIndex].tt_fc == Dmac_prf2mem_prf))
        dw_dmac_unmaskIrq(dev, ch_num, Dmac_irq_srctran);
    	
    if((dw_dmac_getHandshakingMode(dev, ch_num, Dmac_dst) == Dmac_hs_software) &&
       (instance->ch[chanIndex].tt_fc == Dmac_prf2prf_srcprf || instance->ch[chanIndex].tt_fc == Dmac_prf2prf_dstprf ||
        instance->ch[chanIndex].tt_fc == Dmac_mem2prf_dma    || instance->ch[chanIndex].tt_fc == Dmac_mem2prf_prf))
    	    dw_dmac_unmaskIrq(dev, ch_num, Dmac_irq_dsttran);
 
    retVal = dw_dmac_enableChannelIrq(dev, ch_num);

    return retVal;
}

int dw_dmac_startCommonTransfer(struct dw_device *dev, enum dw_dmac_channel_number ch_num, int num_blocks, dw_callback cb_func)
{
    struct dw_dmac_instance *instance;
    uint8_t chanIndex;
    int retVal = 0;

    DMAC_COMMON_REQUIREMENTS(dev);

    instance = (struct dw_dmac_instance *) dev->instance;

    chanIndex = dw_dmac_getChannelIndex(ch_num);
        
    if(chanIndex == DMAC_MAX_CHANNELS) retVal = -DW_ECHRNG;
    else retVal = dw_dmac_checkChannelBusy(dev, ch_num);

    if(retVal == 0) retVal = dw_dmac_disableChannelIrq(dev, ch_num);

    if(retVal != 0) return retVal;

    instance->ch[chanIndex].src_byte_cnt = 0;
    instance->ch[chanIndex].dst_byte_cnt = 0;
    instance->ch[chanIndex].block_cnt = 0;
    
    instance->ch[chanIndex].src_state = Dmac_burst_region;
    instance->ch[chanIndex].dst_state = Dmac_burst_region;
    
    instance->ch[chanIndex].userCallback = cb_func;
    instance->ch[chanIndex].total_blocks = num_blocks;

    instance->ch[chanIndex].trans_type = dw_dmac_getTransferType(dev, ch_num);

    instance->ch[chanIndex].src_single_inc = (POW2(dw_dmac_getTransWidth(dev, ch_num, Dmac_src)+3)/8);
    instance->ch[chanIndex].src_burst_inc = (instance->ch[chanIndex].src_single_inc * POW2(dw_dmac_getBurstTransLength(dev, ch_num, Dmac_src)+1));
    
    instance->ch[chanIndex].dst_single_inc = (POW2(dw_dmac_getTransWidth(dev, ch_num, Dmac_dst)+3)/8);
    instance->ch[chanIndex].dst_burst_inc  = (instance->ch[chanIndex].dst_single_inc * POW2(dw_dmac_getBurstTransLength(dev, ch_num, Dmac_dst)+1));
    
    dw_dmac_unmaskIrq(dev, ch_num, Dmac_irq_err);
    dw_dmac_unmaskIrq(dev, ch_num, Dmac_irq_tfr);
    dw_dmac_maskIrq(dev, ch_num, Dmac_irq_block);

    instance->ch[chanIndex].tt_fc = dw_dmac_getMemPeriphFlowCtl(dev, ch_num);

    if((dw_dmac_getHandshakingMode(dev, ch_num, Dmac_src) == Dmac_hs_software) &&
       (instance->ch[chanIndex].tt_fc == Dmac_prf2prf_srcprf || instance->ch[chanIndex].tt_fc == Dmac_prf2prf_dstprf ||
	instance->ch[chanIndex].tt_fc == Dmac_prf2mem_dma    || instance->ch[chanIndex].tt_fc == Dmac_prf2mem_prf))
        dw_dmac_unmaskIrq(dev, ch_num, Dmac_irq_srctran);
    	
    if((dw_dmac_getHandshakingMode(dev, ch_num, Dmac_dst) == Dmac_hs_software) &&
       (instance->ch[chanIndex].tt_fc == Dmac_prf2prf_srcprf || instance->ch[chanIndex].tt_fc == Dmac_prf2prf_dstprf ||
        instance->ch[chanIndex].tt_fc == Dmac_mem2prf_dma    || instance->ch[chanIndex].tt_fc == Dmac_mem2prf_prf))
    	    dw_dmac_unmaskIrq(dev, ch_num, Dmac_irq_dsttran);
 
    retVal = dw_dmac_enableChannelIrq(dev, ch_num);

    if(retVal == 0) retVal = dw_dmac_enableChannel(dev, ch_num);

    return retVal;
}
    
int dw_dmac_startTransfer(struct dw_device *dev, enum dw_dmac_channel_number ch_num, int num_blocks, dw_callback cb_func)
{
    struct dw_dmac_instance *instance;
    uint8_t chanIndex;
    int retVal = 0;

    DMAC_COMMON_REQUIREMENTS(dev);

    instance = (struct dw_dmac_instance *) dev->instance;

    chanIndex = dw_dmac_getChannelIndex(ch_num);
        
    if(chanIndex == DMAC_MAX_CHANNELS) retVal = -DW_ECHRNG;
    else retVal = dw_dmac_checkChannelBusy(dev, ch_num);

    if(retVal == 0) retVal = dw_dmac_disableChannelIrq(dev, ch_num);

    if(retVal != 0) return retVal;

    instance->ch[chanIndex].src_byte_cnt = 0;
    instance->ch[chanIndex].dst_byte_cnt = 0;
    instance->ch[chanIndex].block_cnt = 0;
    
    instance->ch[chanIndex].src_state = Dmac_burst_region;
    instance->ch[chanIndex].dst_state = Dmac_burst_region;
    
    instance->ch[chanIndex].userCallback = cb_func;
    instance->ch[chanIndex].total_blocks = num_blocks;

    instance->ch[chanIndex].trans_type = dw_dmac_getTransferType(dev, ch_num);

    instance->ch[chanIndex].src_single_inc = (POW2(dw_dmac_getTransWidth(dev, ch_num, Dmac_src)+3)/8);
    instance->ch[chanIndex].src_burst_inc = (instance->ch[chanIndex].src_single_inc * POW2(dw_dmac_getBurstTransLength(dev, ch_num, Dmac_src)+1));
    
    instance->ch[chanIndex].dst_single_inc = (POW2(dw_dmac_getTransWidth(dev, ch_num, Dmac_dst)+3)/8);
    instance->ch[chanIndex].dst_burst_inc  = (instance->ch[chanIndex].dst_single_inc * POW2(dw_dmac_getBurstTransLength(dev, ch_num, Dmac_dst)+1));
    
    dw_dmac_unmaskIrq(dev, ch_num, Dmac_irq_err);
    dw_dmac_unmaskIrq(dev, ch_num, Dmac_irq_tfr);
    dw_dmac_unmaskIrq(dev, ch_num, Dmac_irq_block);

    instance->ch[chanIndex].tt_fc = dw_dmac_getMemPeriphFlowCtl(dev, ch_num);

    if((dw_dmac_getHandshakingMode(dev, ch_num, Dmac_src) == Dmac_hs_software) &&
       (instance->ch[chanIndex].tt_fc == Dmac_prf2prf_srcprf || instance->ch[chanIndex].tt_fc == Dmac_prf2prf_dstprf ||
	instance->ch[chanIndex].tt_fc == Dmac_prf2mem_dma    || instance->ch[chanIndex].tt_fc == Dmac_prf2mem_prf))
        dw_dmac_unmaskIrq(dev, ch_num, Dmac_irq_srctran);
    	
    if((dw_dmac_getHandshakingMode(dev, ch_num, Dmac_dst) == Dmac_hs_software) &&
       (instance->ch[chanIndex].tt_fc == Dmac_prf2prf_srcprf || instance->ch[chanIndex].tt_fc == Dmac_prf2prf_dstprf ||
        instance->ch[chanIndex].tt_fc == Dmac_mem2prf_dma    || instance->ch[chanIndex].tt_fc == Dmac_mem2prf_prf))
    	    dw_dmac_unmaskIrq(dev, ch_num, Dmac_irq_dsttran);
 
    retVal = dw_dmac_enableChannelIrq(dev, ch_num);

    if(retVal == 0) retVal = dw_dmac_enableChannel(dev, ch_num);

    return retVal;
}

void dw_dmac_setListener(struct dw_device *dev, enum dw_dmac_channel_number ch_num, dw_callback userFunction)
{
    struct dw_dmac_param *param;
    struct dw_dmac_instance *instance;
    uint8_t chanIndex;

    DW_REQUIRE(userFunction != NULL);

    param = (struct dw_dmac_param  *) dev->comp_param;
    instance = (struct dw_dmac_instance *) dev->instance;

    DW_REQUIRE(!(DMAC_CH_MASK & ch_num));
    
    chanIndex = dw_dmac_getChannelIndex(ch_num);

    DW_REQUIRE(chanIndex != DMAC_MAX_CHANNELS);

    instance->ch[chanIndex].userListener = userFunction;
}

void dw_dmac_resetInstance(struct dw_device *dev)
{
    struct dw_dmac_param *param;
    struct dw_dmac_instance *instance;

    DMAC_COMMON_REQUIREMENTS(dev);

    param = (struct dw_dmac_param  *) dev->comp_param;
    instance = (struct dw_dmac_instance *) dev->instance;

    for(int i=0; i<param->num_channels; i++) {

        switch(i) {
            case 0: instance->ch[i].ch_num = Dmac_channel0; break;
            case 1: instance->ch[i].ch_num = Dmac_channel1; break;
            case 2: instance->ch[i].ch_num = Dmac_channel2; break;
            case 3: instance->ch[i].ch_num = Dmac_channel3; break;
            case 4: instance->ch[i].ch_num = Dmac_channel4; break;
            case 5: instance->ch[i].ch_num = Dmac_channel5; break;
            case 6: instance->ch[i].ch_num = Dmac_channel6; break;
            case 7: instance->ch[i].ch_num = Dmac_channel7; break;
        }

        instance->ch[i].src_state      = Dmac_idle;
        instance->ch[i].dst_state      = Dmac_idle;
	
        instance->ch[i].block_cnt      = 0;
        instance->ch[i].total_blocks   = 0;
        instance->ch[i].src_byte_cnt   = 0;
        instance->ch[i].dst_byte_cnt   = 0;
        instance->ch[i].src_single_inc = 0;
        instance->ch[i].src_burst_inc  = 0;
        instance->ch[i].dst_single_inc = 0;
        instance->ch[i].dst_burst_inc  = 0;
	
        instance->ch[i].trans_type     = Dmac_transfer_row1;
        instance->ch[i].tt_fc          = Dmac_mem2mem_dma;
        instance->ch[i].userCallback   = NULL;
        instance->ch[i].userListener   = NULL;
    }

    dw_dmac_setChannelPriorityOrder(dev);
}

int dw_dmac_checkChannelRange(struct dw_device *dev, enum dw_dmac_channel_number ch_num)
{
    struct dw_dmac_param *param;
    int retVal = 0;

    DMAC_COMMON_REQUIREMENTS(dev);

    param = (struct dw_dmac_param  *) dev->comp_param;

    if (ch_num & DMAC_CH_EN_MASK)  retVal = 0;
    else retVal = -DW_ECHRNG;
    
    return retVal;
}

int dw_dmac_checkChannelBusy(struct dw_device *dev, enum dw_dmac_channel_number ch_num)
{
    struct dw_dmac_param *param;
    struct dw_dmac_portmap *portmap;
    uint32_t reg;
    int retVal = 0;

    DMAC_COMMON_REQUIREMENTS(dev);

    param = (struct dw_dmac_param  *) dev->comp_param;
    portmap = (struct dw_dmac_portmap *) dev->base_address;

    if((DMAC_CH_MASK & ch_num) || (ch_num == Dmac_no_channel)) retVal = -DW_ECHRNG;
    else {
        reg = DMAC_INP(portmap->ch_en_reg_l);
        if(reg & ch_num)
            retVal = -DW_EBUSY;
    }
    
    return retVal;
}

void dw_dmac_waitForLastBlock(struct dw_device *dev, uint32_t block_num, uint32_t ch_num, uint8_t clr_src)
{
    //struct dw_dmac_param *param;
    struct dw_dmac_portmap *portmap;
    uint32_t blocks_transferred=0;
    //uint32_t reg;
    
    //param = (struct dw_dmac_param  *) dev->comp_param;
    portmap = (struct dw_dmac_portmap *) dev->base_address;
    
    while(blocks_transferred < block_num-1) {
        while(!DMAC_INP(portmap->raw_block_l));
	DMAC_OUTP(1<<ch_num, portmap->clear_block_l);
        blocks_transferred++;
    }
    //reg = DMAC_INP(portmap->ch[ch_num].cfg_l);
    if(clr_src) DW_BIT_SET(portmap->ch[ch_num].cfg_l, DMAC_CFG_L_RELOAD_SRC, 0);
    else DW_BIT_SET(portmap->ch[ch_num].cfg_l, DMAC_CFG_L_RELOAD_DST, 0);
}
 
void dw_dmac_setChannelPriorityOrder(struct dw_device *dev)
{
    struct dw_dmac_param *param;
    struct dw_dmac_portmap *portmap;
    struct dw_dmac_instance *instance;
    
    int index, chanPriority, chanPriorityOrder[8];


    DMAC_COMMON_REQUIREMENTS(dev);

    param = (struct dw_dmac_param  *) dev->comp_param;
    portmap = (struct dw_dmac_portmap *) dev->base_address;
    instance = (struct dw_dmac_instance *) dev->instance;

    for(int i=0; i<DMAC_MAX_CHANNELS; i++) {
        chanPriorityOrder[i] = 0;
        instance->ch_order[i] = i;
    }

    for(int j=0, k; j<param->num_channels; j++) {

        chanPriority = DW_BIT_GET(DMAC_INP(portmap->ch[j].cfg_l), DMAC_CFG_L_CH_PRIOR);
        
        for(k=0; k<=param->num_channels; k++)
            if(A_MAXEQ_B(chanPriority, chanPriorityOrder[k]) || k == param->num_channels) {
                index = k;
                break;
            }
	
        for(k=j; k>index; k--) {
            chanPriorityOrder[k] = chanPriorityOrder[k-1];
            instance->ch_order[k] = instance->ch_order[k-1];
        }
	
        chanPriorityOrder[index] = chanPriority;
        instance->ch_order[index] = j;
    }
}
