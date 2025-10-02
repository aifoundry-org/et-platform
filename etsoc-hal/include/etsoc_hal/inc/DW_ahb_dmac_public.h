/* --------------------------------------------------------------------
** 
** Synopsys DesignWare DW_ahb_dmac Software Driver Kit and
** documentation (hereinafter, "Software") is an Unsupported
** proprietary work of Synopsys, Inc. unless otherwise expressly
** agreed to in writing between Synopsys and you.
** 
** The Software IS NOT an item of Licensed Software or Licensed
** Product under any End User Software License Agreement or Agreement
** for Licensed Product with Synopsys or any supplement thereto. You
** are permitted to use and redistribute this Software in source and
** binary forms, with or without modification, provided that
** redistributions of source code must retain this notice. You may not
** view, use, disclose, copy or distribute this file or any information
** contained herein except pursuant to this license grant from Synopsys.
** If you do not agree with this notice, including the disclaimer
** below, then you are not authorized to use the Software.
** 
** THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS"
** BASIS AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
** FOR A PARTICULAR PURPOSE ARE HEREBY DISCLAIMED. IN NO EVENT SHALL
** SYNOPSYS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
** OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
** USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
** DAMAGE.
** 
** --------------------------------------------------------------------
*/

#ifndef DW_AHB_DMAC_PUBLIC_H
#define DW_AHB_DMAC_PUBLIC_H

/* Allow C++ to use this header */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/****h* drivers.dmac/dmac.api
 * NAME
 *  DW_ahb_dmac API overview
 * DESCRIPTION
 *  This section gives an overview of the DW_ahb_dmac software driver
 *  Application Programming Interface (API).
 * SEE ALSO
 *  dmac.data, dmac.functions
 ***/

/****h* drivers.dmac/dmac.data
 * NAME
 *  DMAC data types and definitions
 * DESCRIPTION
 *  This section details all the public data types and definitions used
 *  with the DW_ahb_dmac software driver.
 * SEE ALSO
 *  dmac.api, dmac.functions
 ***/

/****h* drivers.dmac/dmac.functions
 * NAME
 *  DMAC data types and definitions
 * DESCRIPTION
 *  This section details all the public functions available for use with
 *  the DW_ahb_dmac software driver.
 * SEE ALSO
 *  dmac.api, dmac.data
 ***/

/****h* dmac.api/dmac.data_types
 * NAME
 *  DW_ahb_dmac data types and definitions
 * DESCRIPTION
 *  The data types below are used as function arguments for the DMA
 *  Controller API. Users of this driver must pass the relevant data
 *  types below to the API function that is being used.
 *
 *    - enum dw_dmac_channel_number
 *    - enum dw_dmac_src_dst_select
 *    - enum dw_dmac_lock_bus_ch
 *    - enum dw_dmac_sw_hw_hs_select
 *    - enum dw_dmac_scatter_gather_param
 *    - enum dw_dmac_irq
 *    - enum dw_dmac_software_req
 *    - enum dw_dmac_master_number
 *    - enum dw_dmac_transfer_type
 *    - enum dw_dmac_transfer_flow
 *    - enum dw_dmac_burst_trans_length
 *    - enum dw_dmac_address_increment
 *    - enum dw_dmac_transfer_width
 *    - enum dw_dmac_hs_interface
 *    - enum dw_dmac_prot_level
 *    - enum dw_dmac_fifo_mode
 *    - enum dw_dmac_flow_ctl_mode
 *    - enum dw_dmac_polarity_level
 *    - enum dw_dmac_lock_level
 *    - enum dw_dmac_channel_priority
 *    - struct dw_dmac_channel_config
 * SEE ALSO
 *  dmac.api, dmac.configuration, dmac.command, dmac.status,
 *  dmac.interrupt
 ***/

/****h* dmac.api/dmac.configuration
 * NAME
 *  DW_ahb_dmac configuration functions
 * DESCRIPTION
 *  An API configuration function configures the DMA Controller
 *  and/or the driver software in preparation for some functional
 *  operation to be performed by the DMA Controller. The effect on
 *  the DW_ahb_dmac hardware device of running an API configuration
 *  function is minimal. That is to say NO functional operation is
 *  started or terminated by the DMA Controller. The available
 *  API configuration functions for the DW_ahb_dmac driver are listed
 *  below:
 *
 *    - dw_dmac_init()
 *    - dw_dmac_setChannelConfig()
 *    - dw_dmac_setTransferType()
 *    - dw_dmac_setAddress()
 *    - dw_dmac_setBlockTransSize()
 *    - dw_dmac_setMstSelect()
 *    - dw_dmac_setMemPeriphFlowCtl()
 *    - dw_dmac_setScatterEnable()
 *    - dw_dmac_setGatherEnable()
 *    - dw_dmac_setBurstTransLength()
 *    - dw_dmac_setAddressInc()
 *    - dw_dmac_setTransWidth()
 *    - dw_dmac_setHsInterface()
 *    - dw_dmac_setStatUpdate()
 *    - dw_dmac_setProtCtl()
 *    - dw_dmac_setFifoMode()
 *    - dw_dmac_setFlowCtlMode()
 *    - dw_dmac_setMaxAmbaBurstLength()
 *    - dw_dmac_setHsPolarity()
 *    - dw_dmac_setLockLevel()
 *    - dw_dmac_setLockEnable()
 *    - dw_dmac_setHandshakingMode()
 *    - dw_dmac_setChannelPriority()
 *    - dw_dmac_setListMstSelect()
 *    - dw_dmac_setListPointerAddress()
 *    - dw_dmac_setLlpEnable()
 *    - dw_dmac_setStatus()
 *    - dw_dmac_setStatusAddress()
 *    - dw_dmac_setGatherParam()
 *    - dw_dmac_setScatterParam()
 *    - dw_dmac_addLliItem()
 *
 *  Although the function listed below can have an immediate functional
 *  effect on the DMA Controller hardware, this only happens after
 *  the channel is enabled. Under all other conditions, this function
 *  call does not cause any functional operations on the DMA Controller
 *  hardware to begin or halt.
 *
 *    - dw_dmac_setReload()
 * SEE ALSO
 *  dmac.api, dmac.data_types, dmac.command, dmac.status,
 *  dmac.interrupt
 ***/

/****h* dmac.api/dmac.command
 * NAME
 *  DW_ahb_dmac command functions
 * DESCRIPTION
 *  An API command function causes some functional behavior
 *  within the DMA Controller to begin or halt. This is the
 *  effect of writing to a command register/bit field or interrupt-
 *  related register within the DMA Controller. The available API
 *  command functions for the DW_ahb_dmac driver are list below:
 *
 *    - dw_dmac_enable()
 *    - dw_dmac_disable()
 *    - dw_dmac_enableChannel()
 *    - dw_dmac_disableChannel()
 *    - dw_dmac_enableChannelIrq()
 *    - dw_dmac_disableChannelIrq()
 *    - dw_dmac_maskIrq()
 *    - dw_dmac_unmaskIrq()
 *    - dw_dmac_clearIrq()
 *    - dw_dmac_suspendChannel()
 *    - dw_dmac_resumeChannel()
 *    - dw_dmac_setSoftwareRequest()
 *    - dw_dmac_setTestMode()
 * SEE ALSO
 *  dmac.api, dmac.data_types, dmac.configuration, dmac.status,
 *  dmac.interrupt
 ***/

/****h* dmac.api/dmac.status
 * NAME
 *  DW_ahb_dmac status functions
 * DESCRIPTION
 *  An API status function returns the status information about
 *  the current state of the DMA Controller and/or the driver. This
 *  can be a direct register read or more general information about
 *  the status of a DMA transfer. The available API status functions
 *  for the DW_ahb_dmac driver are listed below:
 *
 *    - dw_dmac_isEnabled()
 *    - dw_dmac_getChannelEnableReg()
 *    - dw_dmac_isChannelEnabled()
 *    - dw_dmac_isChannelIrqEnabled()
 *    - dw_dmac_isBlockTransDone()
 *    - dw_dmac_isFifoEmpty()
 *    - dw_dmac_isChannelSuspended()
 *    - dw_dmac_isIrqActive()
 *    - dw_dmac_isRawIrqActive()
 *    - dw_dmac_isIrqMasked()
 *    - dw_dmac_getFreeChannel()
 *    - dw_dmac_getChannelConfig()
 *    - dw_dmac_getTestMode()
 *    - dw_dmac_getSoftwareRequest()
 *    - dw_dmac_getTransferType()
 *    - dw_dmac_getAddress()
 *    - dw_dmac_getBlockTransSize()
 *    - dw_dmac_getMstSelect()
 *    - dw_dmac_getMemPeriphFlowCtl()
 *    - dw_dmac_getScatterEnable()
 *    - dw_dmac_getGatherEnable()
 *    - dw_dmac_getBurstTransLength()
 *    - dw_dmac_getAddressInc()
 *    - dw_dmac_getTransWidth()
 *    - dw_dmac_getHsInterface()
 *    - dw_dmac_getStatUpdate()
 *    - dw_dmac_getProtCtl()
 *    - dw_dmac_getFifoMode()
 *    - dw_dmac_getFlowCtlMode()
 *    - dw_dmac_getMaxAmbaBurstLength()
 *    - dw_dmac_getHsPolarity()
 *    - dw_dmac_getLockLevel()
 *    - dw_dmac_getLockEnable()
 *    - dw_dmac_getHandshakingMode()
 *    - dw_dmac_getChannelPriority()
 *    - dw_dmac_getListMstSelect()
 *    - dw_dmac_getListPointerAddress()
 *    - dw_dmac_getLlpEnable()
 *    - dw_dmac_getReload()
 *    - dw_dmac_getStatus()
 *    - dw_dmac_getStatusAddress()
 *    - dw_dmac_getGatherParam()
 *    - dw_dmac_getScatterParam()
 *    - dw_dmac_getChannelIndex()
 *    - dw_dmac_getNumChannels()
 *    - dw_dmac_getChannelFifoDepth()
 *    - dw_dmac_getBlockCount()
 *    - dw_dmac_getBlockByteCount()
 * SEE ALSO
 *  dmac.api, dmac.data_types, dmac.configuration, dmac.command,
 *  dmac.interrupt
 ***/

/****h* dmac.api/dmac.interrupt
 * NAME
 *  DW_ahb_dmac interrupt interface functions
 * DESCRIPTION
 *  The Driver Kit provides two ways of managing interrupt-driven DMA
 *  transfers using two different interrupt handlers,
 *  dw_dmac_userIrqHandler() and dw_dmac_irqHandler().  The former does
 *  not perform any interrupt processing, apart from clearing
 *  interrupts, and simply forwards the current highest priority active
 *  interrupt to a user-provided listener function.
 *
 *  The latter uses the entire Interrupt API to actively manage the
 *  complete DMA transfer on a given channel.  These functions free the
 *  user from having to micro-manage the DMAC transfer and deal with
 *  enabling/disabling interrupts. Any interrupt not handled by the
 *  Driver Kit (i.e. Dmac_irq_err) is passed to a listener function to
 *  be handled (though the Driver Kit still handles the clearing of
 *  interrupt(s)).  The available Interrupt API functions for the
 *  DW_ahb_dmac Driver Kit are listed below:
 *
 *    - dw_dmac_setListener()
 *    - dw_dmac_startTransfer()
 *    - dw_dmac_sourceReady()
 *    - dw_dmac_destinationReady()
 *    - dw_dmac_setSingleRegion()
 *    - dw_dmac_nextBlockIsLast()
 *    - dw_dmac_irqHandler()
 *    - dw_dmac_userIrqHandler()
 * NOTES
 *  It is not possible to use any Interrupt API functions, apart from
 *  dw_dmac_setListener(), with the dw_dmac_userIrqHandler() interrupt
 *  handler.  This is because they are symbiotic with the
 *  dw_dmac_irqHandler() interrupt handler.
 * SEE ALSO
 *  dmac.api, dmac.data_types, dmac.configuration, dmac.command,
 *  dmac.status
 ***/

/****d* dmac.data/dw_dmac_channel_number
 * DESCRIPTION
 *  This data type is used to describe the DMA controller's channel
 *  number. The assigned enumerated value matches the register
 *  value that needs to be written to enable the channel. This data
 *  type is used by many of the API functions in the driver, a
 *  select few are listed:
 * SEE ALSO
 *  dw_dmac_enableChannel(), dw_dmac_disableChannel(),
 *  dw_dmac_suspendChannel(), dw_dmac_setAddress()
 * SOURCE
 */
enum dw_dmac_channel_number {
    Dmac_no_channel   = 0x0000,
    Dmac_channel0     = 0x0101,
    Dmac_channel1     = 0x0202,
    Dmac_channel2     = 0x0404,
    Dmac_channel3     = 0x0808,
    Dmac_channel4     = 0x1010,
    Dmac_channel5     = 0x2020,
    Dmac_channel6     = 0x4040,
    Dmac_channel7     = 0x8080,
    Dmac_all_channels = 0xffff
};
/*****/

/****d* dmac.data/dw_dmac_src_dst_select
 * DESCRIPTION
 *  This data type is used to select the source and/or the
 *  destination for a specific DMA channel when using some
 *  of the driver's API functions.
 *  This data type is used by many of the API functions in
 *  the driver, a select few are listed:
 * SEE ALSO
 *  dw_dmac_setAddress(), dw_dmac_setMstSelect(),
 *  dw_dmac_setBurstTransLength(), dw_dmac_setHsPolarity()
 * SOURCE
 */
enum dw_dmac_src_dst_select {
    Dmac_src     = 0x1,
    Dmac_dst     = 0x2,
    Dmac_src_dst = 0x3
};
/*****/

/****d* dmac.data/dw_dmac_lock_bus_ch
 * DESCRIPTION
 *  This data type is used to select the bus and/or the channel
 *  when using the specified driver API functions to lock DMA
 *  transfers. The selection affects which DMA Controller
 *  bit field within the CFGx register is accessed.
 * SEE ALSO
 *  dw_dmac_setLockLevel(), dw_dmac_getLockLevel(),
 *  dw_dmac_setLockEnable(), dw_dmac_getLockEnable()
 * SOURCE
 */
enum dw_dmac_lock_bus_ch {
    Dmac_lock_bus     = 0x1,
    Dmac_lock_channel = 0x2,
    Dmac_lock_bus_ch  = 0x4
};
/*****/

/****d* dmac.data/dw_dmac_sw_hw_hs_select
 * DESCRIPTION
 *  This data type is used to select a software or hardware interface
 *  when using the specified driver API functions to access the
 *  handshaking interface on a specified DMA channel.
 * NOTES
 *  This data type relates directly to the following DMA Controller
 *  register(s) / bit field(s): (x = channel number)
 *    - CFGx.HS_SEL_SRC, CFGx.HS_SEL_DST
 * SEE ALSO
 *  dw_dmac_setHandshakingMode(), dw_dmac_getHandshakingMode()
 * SOURCE
 */
enum dw_dmac_sw_hw_hs_select {
    Dmac_hs_hardware   = 0x0,
    Dmac_hs_software   = 0x1
};
/*****/

/****d* dmac.data/dw_dmac_scatter_gather_param
 * DESCRIPTION
 *  This data type is used to select the count or interval bit field
 *  when using the specified driver API functions to access the
 *  scatter or gather registers on the DMA controller.
 * NOTES
 *  This data type relates directly to the following DMA Controller
 *  register(s) / bit field(s): (x = channel number)
 *    - SGRx.SGC, SGRx.SGI, DSRx.DSC, DSRx.DSI
 * SEE ALSO
 *  dw_dmac_setScatterParam(), dw_dmac_getScatterParam(),
 *  dw_dmac_setGatherParam(), dw_dmac_getGatherParam()
 * SOURCE
 */
enum dw_dmac_scatter_gather_param {
    Dmac_sg_count    = 0x0,
    Dmac_sg_interval = 0x1
};
/*****/

/****d* dmac.data/dw_dmac_irq
 * DESCRIPTION
 *  This data type is used to select the interrupt type on a specified
 *  DMA channel when using the specified driver API functions to access
 *  interrupt registers within the DMA Controller.
 * SEE ALSO
 *  dw_dmac_maskIrq(), dw_dmac_unmaskIrq(), dw_dmac_clearIrq(),
 *  dw_dmac_isIrqActive(), dw_dmac_isRawIrqActive()
 * SOURCE
 */
enum dw_dmac_irq {
    Dmac_irq_none       = 0x00,     // no interrupts
    Dmac_irq_tfr        = 0x01,     // transfer complete
    Dmac_irq_block      = 0x02,     // block transfer complete
    Dmac_irq_srctran    = 0x04,     // source transaction complete
    Dmac_irq_dsttran    = 0x08,     // destination transaction complete
    Dmac_irq_err        = 0x10,     // error
    Dmac_irq_all        = 0x1f      // all interrupts
};
/*****/

/****d* dmac.data/dw_dmac_software_req
 * DESCRIPTION
 *  This data type is used to select which of the software request
 *  registers are accessed within the DMA Controller when using the
 *  specified driver API functions.
 * SEE ALSO
 *  dw_dmac_setSoftwareRequest(), dw_dmac_getSoftwareRequest()
 * SOURCE
 */
enum dw_dmac_software_req {
    Dmac_request        = 0x1, /* ReqSrcReq/ReqDstReq */
    Dmac_single_request = 0x2, /* SglReqSrcReq/SglReqDstReq */
    Dmac_last_request   = 0x4  /* LstReqSrcReq/LstReqDstReq */
};
/*****/

/****d* dmac.data/dw_dmac_master_number
 * DESCRIPTION
 *  This data type is used to select the master interface number
 *  on the DMA Controller when using the specified driver API
 *  functions.
 * NOTES
 *  This data type relates directly to the following DMA Controller
 *  register(s) / bit-field(s): (x = channel number)
 *    - CTLx.SMS, CTLx.DMS, LLPx.LMS
 * SEE ALSO
 *  dw_dmac_setMstSelect(), dw_dmac_getMstSelect(),
 *  dw_dmac_setListMstSelect(), dw_dmac_getListMstSelect()
 * SOURCE
 */
enum dw_dmac_master_number {
    Dmac_master1 = 0x0,
    Dmac_master2 = 0x1,
    Dmac_master3 = 0x2,
    Dmac_master4 = 0x3
};
/*****/

/****d* dmac.data/dw_dmac_transfer_type
 * DESCRIPTION
 *  This data type is used for selecting the transfer type for a
 *  specified DMA channel when using the specified driver API
 *  functions. See the DW_ahb_dmac databook for a detailed
 *  description on these transfer types.
 * SEE ALSO
 *  dw_dmac_setTransferType(), dw_dmac_getTransferType()
 * SOURCE
 */
enum dw_dmac_transfer_type {
    Dmac_transfer_row1  = 0x1, /* single block or last multi-block */
                               /*  no write back                   */
    Dmac_transfer_row2  = 0x2, /* multi-block auto-reload DAR      */
                               /*  contiguous SAR no write back    */
    Dmac_transfer_row3  = 0x3, /* multi-block auto reload SAR      */
                               /*  contiguous DAR no write back    */
    Dmac_transfer_row4  = 0x4, /* multi-block auto-reload SAR DAR  */
                               /*  no write back                   */
    Dmac_transfer_row5  = 0x5, /* single block or last multi-block */
                               /*  with write back                 */
    Dmac_transfer_row6  = 0x6, /* multi-block linked list DAR      */
                               /*  contiguous SAR with write back  */
    Dmac_transfer_row7  = 0x7, /* multi-block linked list DAR auto */
                               /*  reload SAR  with write back     */
    Dmac_transfer_row8  = 0x8, /* multi-block linked list SAR      */
                               /*  contiguous DAR with write back  */
    Dmac_transfer_row9  = 0x9, /* multi-block linked list SAR auto */
                               /*  reload DAR with write back      */
    Dmac_transfer_row10 = 0xa  /* multi-block linked list SAR DAR  */
                               /*  with write back                 */
};
/*****/

/****d* dmac.data/dw_dmac_transfer_flow
 * DESCRIPTION
 *  This data type is used for selecting the transfer flow device
 *  (memory or peripheral device) and for setting the flow control
 *  device for the DMA transfer when using the specified driver
 *  API functions.
 * NOTES
 *  This data type relates directly to the following DMA Controller
 *  register(s) / bit field(s): (x = channel number)
 *    - CTLx.TT_FC
 * SEE ALSO
 *  dw_dmac_setMemPeriphFlowCtl(), dw_dmac_getMemPeriphFlowCtl()
 * SOURCE
 */
enum dw_dmac_transfer_flow {
    Dmac_mem2mem_dma    = 0x0, /* mem to mem - DMAC   flow ctlr */
    Dmac_mem2prf_dma    = 0x1, /* mem to prf - DMAC   flow ctlr */
    Dmac_prf2mem_dma    = 0x2, /* prf to mem - DMAC   flow ctlr */
    Dmac_prf2prf_dma    = 0x3, /* prf to prf - DMAC   flow ctlr */
    Dmac_prf2mem_prf    = 0x4, /* prf to mem - periph flow ctlr */
    Dmac_prf2prf_srcprf = 0x5, /* prf to prf - source flow ctlr */
    Dmac_mem2prf_prf    = 0x6, /* mem to prf - periph flow ctlr */
    Dmac_prf2prf_dstprf = 0x7  /* prf to prf - dest   flow ctlr */
};
/*****/

/****d* dmac.data/dw_dmac_burst_trans_length
 * DESCRIPTION
 *  This data type is used for selecting the burst transfer length
 *  on the source and/or destination of a DMA channel when using the
 *  specified driver API functions. These transfer length values do
 *  not relate to the AMBA HBURST parameter.
 * NOTES
 *  This data type relates directly to the following DMA Controller
 *  register(s) / bit field(s): (x = channel number)
 *    - CTLx.SRC_MSIZE, CTLx.DEST_MSIZE
 * SEE ALSO
 *  dw_dmac_setBurstTransLength(), dw_dmac_getBurstTransLength()
 * SOURCE
 */
enum dw_dmac_burst_trans_length {
    Dmac_msize_1   = 0x0,
    Dmac_msize_4   = 0x1,
    Dmac_msize_8   = 0x2,
    Dmac_msize_16  = 0x3,
    Dmac_msize_32  = 0x4,
    Dmac_msize_64  = 0x5,
    Dmac_msize_128 = 0x6,
    Dmac_msize_256 = 0x7
};
/*****/

/****d* dmac.data/dw_dmac_address_increment
 * DESCRIPTION
 *  This data type is used for selecting the address increment
 *  type for the source and/or destination on a DMA channel when using
 *  the specified driver API functions.
 * NOTES
 *  This data type relates directly to the following DMA Controller
 *  register(s) / bit-field(s): (x = channel number)
 *    - CTLx.SINC, CTLx.DINC
 * SEE ALSO
 *  dw_dmac_setAddressInc(), dw_dmac_getAddressInc()
 * SOURCE
 */
enum dw_dmac_address_increment {
    Dmac_addr_increment = 0x0,
    Dmac_addr_decrement = 0x1,
    Dmac_addr_nochange  = 0x2
};
/*****/

/****d* dmac.data/dw_dmac_transfer_width
 * DESCRIPTION
 *  This data type is used for selecting the transfer width for the
 *  source and/or destination on a DMA channel when using the specified
 *  driver API functions. This data type maps directly to the AMBA AHB
 *  HSIZE parameter.
 * NOTES
 *  This data type relates directly to the following DMA Controller
 *  register(s) / bit field(s): (x = channel number)
 *    - CTLx.SRC_TR_WIDTH, CTLx.DST_TR_WIDTH
 * SEE ALSO
 *  dw_dmac_setTransWidth(), dw_dmac_getTransWidth()
 * SOURCE
 */
enum dw_dmac_transfer_width {
    Dmac_trans_width_8   = 0x0,
    Dmac_trans_width_16  = 0x1,
    Dmac_trans_width_32  = 0x2,
    Dmac_trans_width_64  = 0x3,
    Dmac_trans_width_128 = 0x4,
    Dmac_trans_width_256 = 0x5
};
/*****/

/****d* dmac.data/dw_dmac_hs_interface
 * DESCRIPTION
 *  This data type is used for selecting the handshaking interface
 *  number for the source and/or destination on a DMA channel when
 *  using the specified driver API functions.
 * NOTES
 *  This data type relates directly to the following DMA Controller
 *  register(s) / bit-field(s): (x = channel number)
 *    - CFGx.DEST_PER, CFGx.SRC_PER
 * SEE ALSO
 *  dw_dmac_setHsInterface(), dw_dmac_getHsInterface()
 * SOURCE
 */
enum dw_dmac_hs_interface {
    Dmac_hs_if0  = 0x0,
    Dmac_hs_if1  = 0x1,
    Dmac_hs_if2  = 0x2,
    Dmac_hs_if3  = 0x3,
    Dmac_hs_if4  = 0x4,
    Dmac_hs_if5  = 0x5,
    Dmac_hs_if6  = 0x6,
    Dmac_hs_if7  = 0x7,
    Dmac_hs_if8  = 0x8,
    Dmac_hs_if9  = 0x9,
    Dmac_hs_if10 = 0xa,
    Dmac_hs_if11 = 0xb,
    Dmac_hs_if12 = 0xc,
    Dmac_hs_if13 = 0xd,
    Dmac_hs_if14 = 0xe,
    Dmac_hs_if15 = 0xf
};
/*****/

/****d* dmac.data/dw_dmac_prot_level
 * DESCRIPTION
 *  This data type is used for selecting the tranfer protection level
 *  on a DMA channel when using the specified driver API functions.
 *  This data type maps directly to the AMBA AHB HPROT parameter.
 * NOTES
 *  This data type relates directly to the following DMA Controller
 *  register(s) / bit-field(s): (x = channel number)
 *    - CFGx.PROTCTL
 * SEE ALSO
 *  dw_dmac_setProtCtl(), dw_dmac_getProtCtl()
 * SOURCE
 */
enum dw_dmac_prot_level {
   Dmac_noncache_nonbuff_nonpriv_opcode = 0x0, /* default prot level */
   Dmac_noncache_nonbuff_nonpriv_data   = 0x1,
   Dmac_noncache_nonbuff_priv_opcode    = 0x2,
   Dmac_noncache_nonbuff_priv_data      = 0x3,
   Dmac_noncache_buff_nonpriv_opcode    = 0x4,
   Dmac_noncache_buff_nonpriv_data      = 0x5,
   Dmac_noncache_buff_priv_opcode       = 0x6,
   Dmac_noncache_buff_priv_data         = 0x7,
   Dmac_cache_nonbuff_nonpriv_opcode    = 0x8,
   Dmac_cache_nonbuff_nonpriv_data      = 0x9,
   Dmac_cache_nonbuff_priv_opcode       = 0xa,
   Dmac_cache_nonbuff_priv_data         = 0xb,
   Dmac_cache_buff_nonpriv_opcode       = 0xc,
   Dmac_cache_buff_nonpriv_data         = 0xd,
   Dmac_cache_buff_priv_opcode          = 0xe,
   Dmac_cache_buff_priv_data            = 0xf
};
/*****/

/****d* dmac.data/dw_dmac_fifo_mode
 * DESCRIPTION
 *  This data type is used for selecting the FIFO mode on a DMA
 *  channel when using the specified driver API functions.
 * NOTES
 *  This data type relates directly to the following DMA Controller
 *  register(s) / bit field(s): (x = channel number)
 *    - CFGx.FIFO_MODE
 * SEE ALSO
 *  dw_dmac_setFifoMode(), dw_dmac_getFifoMode
 * SOURCE
 */
enum dw_dmac_fifo_mode {
    Dmac_fifo_mode_single = 0x0,
    Dmac_fifo_mode_half   = 0x1
};
/*****/

/****d* dmac.data/dw_dmac_flow_ctl_mode
 * DESCRIPTION
 *  This data type is used for selecting the flow control mode on a
 *  DMA channel when using the specified driver API functions.
 * NOTES
 *  This data type relates directly to the following DMA Controller
 *  register(s) / bit-field(s): (x = channel number)
 *    - CFGx.FCMODE
 * SEE ALSO
 *  dw_dmac_setFlowCtlMode(), dw_dmac_getFlowCtlMode()
 * SOURCE
 */
enum dw_dmac_flow_ctl_mode {
    Dmac_data_prefetch_enabled  = 0x0,
    Dmac_data_prefetch_disabled = 0x1
};
/*****/

/****d* dmac.data/dw_dmac_polarity_level
 * DESCRIPTION
 *  This data type is used for selecting the polarity level for the
 *  source and/or destination on a DMA channel's handshaking interface
 *  when using the specified driver API functions.
 * NOTES
 *  This data type relates directly to the following DMA Controller
 *  register(s) / bit-field(s): (x = channel number)
 *    - CFGx.SRC_HS_POL, CFGx.DST_HS_POL
 * SEE ALSO
 *  dw_dmac_setHsPolarity(), dw_dmac_getHsPolarity()
 * SOURCE
 */
enum dw_dmac_polarity_level {
    Dmac_active_high = 0x0,
    Dmac_active_low  = 0x1
};
/*****/

/****d* dmac.data/dw_dmac_lock_level
 * DESCRIPTION
 *  This data type is used for selecting the lock level on a DMA
 *  channel when using the specified driver API functions.
 * NOTES
 *  This data type relates directly to the following DMA Controller
 *  register(s)/bit field(s): (x = channel number)
 *    - CFGx.LOCK_B_L, CFGx.LOCK_CH_L
 * SEE ALSO
 *  dw_dmac_setLevel(), dw_dmac_getLockLevel()
 * SOURCE
 */
enum dw_dmac_lock_level {
    Dmac_lock_level_dma_transfer   = 0x0,
    Dmac_lock_level_block_transfer = 0x1,
    Dmac_lock_level_transaction    = 0x2
};
/*****/

/****d* dmac.data/dw_dmac_channel_priority
 * DESCRIPTION
 *  This data type is used for selecting the priority level of a DMA
 *  channel when using the specified driver API functions.
 * NOTES
 *  This data type relates directly to the following DMA Controller
 *  register(s)/bit field(s): (x = channel number)
 *    - CFGx.CH_PRIOR
 * SEE ALSO
 *  dw_dmac_setChannelPriority(), dw_dmac_getChannelPriority()
 * SOURCE
 */
enum dw_dmac_channel_priority {
    Dmac_priority_0 = 0x0,
    Dmac_priority_1 = 0x1,
    Dmac_priority_2 = 0x2,
    Dmac_priority_3 = 0x3,
    Dmac_priority_4 = 0x4,
    Dmac_priority_5 = 0x5,
    Dmac_priority_6 = 0x6,
    Dmac_priority_7 = 0x7
};
/*****/

/****s* dmac.data/dw_dmac_channel_config
 * DESCRIPTION
 *  This structure is used to set configuration parameters for
 *  a channel in the DMA Controller. All of these configuration
 *  parameters must be programmed into the DMA controller before
 *  enabling the channel. The members of this structure map directly
 *  to a channel's register/bit field within the DMAC device. 
 * NOTES
 *  To initialize the structure, the user should call the
 *  dw_dmac_getChannelConfig() API function after reset of the DMA
 *  controller. This sets the dw_dmac_channel_config structure members
 *  to the DMA controllers reset values. This allows the user to
 *  change only the structure members that need to be different from
 *  the default values and then call the dw_dmac_setChannelConfig()
 *  function to set up the DMA channel transfer.
 * SEE ALSO
 *  dw_dmac_setChannelConfig(), dw_dmac_getChannelConfig()
 * SOURCE
 */
struct dw_dmac_channel_config {
    uint32_t                        sar;
    uint32_t                        dar;
    enum dw_state                   ctl_llp_src_en;
    enum dw_state                   ctl_llp_dst_en;
    enum dw_dmac_master_number      ctl_sms;
    enum dw_dmac_master_number      ctl_dms;
    enum dw_dmac_burst_trans_length ctl_src_msize;
    enum dw_dmac_burst_trans_length ctl_dst_msize;
    enum dw_dmac_address_increment  ctl_sinc;
    enum dw_dmac_address_increment  ctl_dinc;
    enum dw_dmac_transfer_width     ctl_src_tr_width;
    enum dw_dmac_transfer_width     ctl_dst_tr_width;
    uint32_t                        sstat;
    uint32_t                        dstat;
    uint32_t                        sstatar;
    uint32_t                        dstatar;
    enum dw_dmac_hs_interface       cfg_dst_per;
    enum dw_dmac_hs_interface       cfg_src_per;
    enum dw_state                   cfg_ss_upd_en;
    enum dw_state                   cfg_ds_upd_en;
    enum dw_state                   cfg_reload_src;
    enum dw_state                   cfg_reload_dst;
    enum dw_dmac_polarity_level     cfg_src_hs_pol;
    enum dw_dmac_polarity_level     cfg_dst_hs_pol;
    enum dw_dmac_sw_hw_hs_select    cfg_hs_sel_src;
    enum dw_dmac_sw_hw_hs_select    cfg_hs_sel_dst;

    uint32_t                        llp_loc;
    enum dw_dmac_master_number      llp_lms;
    enum dw_state                   ctl_done;
    uint32_t                        ctl_block_ts;
    enum dw_dmac_transfer_flow      ctl_tt_fc;
    enum dw_state                   ctl_dst_scatter_en;
    enum dw_state                   ctl_src_gather_en;
    enum dw_state                   ctl_int_en;
    enum dw_dmac_prot_level         cfg_protctl;
    enum dw_dmac_fifo_mode          cfg_fifo_mode;
    enum dw_dmac_flow_ctl_mode      cfg_fcmode;
    uint32_t                        cfg_max_abrst;
    enum dw_state                   cfg_lock_b;
    enum dw_state                   cfg_lock_ch;
    enum dw_dmac_lock_level         cfg_lock_b_l;
    enum dw_dmac_lock_level         cfg_lock_ch_l;
    enum dw_dmac_channel_priority   cfg_ch_prior;
    uint32_t                        sgr_sgc;
    uint32_t                        sgr_sgi;
    uint32_t                        dsr_dsc;
    uint32_t                        dsr_dsi;
};
/*****/

/****s* dmac.data/dw_dmac_lli_item
 * DESCRIPTION
 *  This structure is used when creating Linked List Items.
 * SEE ALSO
 *  dw_dmac_addLliItem()
 * SOURCE
 */
struct dw_dmac_lli_item {
    uint32_t  sar;
    uint32_t  dar;
    uint32_t  llp;
    uint32_t  ctl_l;
    uint32_t  ctl_h;
    uint32_t  sstat;
    uint32_t  dstat;
    struct    dw_list_head list;
};
/*****/

/****f* dmac.functions/dw_dmac_init
 * DESCRIPTION
 *  This function is used to initialize the DMA controller. All
 *  interrupts are cleared and disabled; DMA channels are disabled; and
 *  the device instance structure is reset.
 * ARGUMENTS
 *  dev         -- DMA controller device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EIO     -- if an error occurred
 * SEE ALSO
 *  dw_device
 * SOURCE
 */
int dw_dmac_init(struct dw_device *dev);
/*****/

/****f* dmac.functions/dw_dmac_enable
 * DESCRIPTION
 *  Function will enable the DMA controller
 * ARGUMENTS
 *  dev         -- DMA controller device handle
 * RETURN VALUE
 *  none
 * NOTES
 *  This function accesses directly the following DMA Controller
 *  register(s) / bit field(s)
 *    - DmaCfgReg.DMA_EN
 * SEE ALSO
 *  dw_dmac_disable(), dw_dmac_isEnabled()
 * SOURCE
 */
void dw_dmac_enable(struct dw_device *dev);
/*****/

/****f* dmac.functions/dw_dmac_disable
 * DESCRIPTION
 *  Function will disable the dma controller
 * ARGUMENTS
 *  dev         -- DMA controller device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EBUSY   -- if failed to disable the DMA controller
 * NOTES
 *  This function accesses directly the following DMA Controller
 *  register(s)/bit field(s)
 *    - DmaCfgReg.DMA_EN
 *
 *  The function returns -DW_EBUSY if a DMA channel is not able
 *  to be disabled. This may happen in the case of a SPLIT response
 *  on the AHB bus.
 * SEE ALSO
 *  dw_dmac_disableChannel(), dw_dmac_enable(), dw_dmac_isEnabled()
 * SOURCE
 */
int dw_dmac_disable(struct dw_device *dev);
/*****/

/****f* dmac.functions/dw_dmac_isEnabled
 * DESCRIPTION
 *  This function returns when the DMA controller is enabled.
 * ARGUMENTS
 *  dev         -- DMA controller device handle
 * RETURN VALUE
 *  true        -- if the DMA controller is enabled
 *  false       -- if the DMA controller is not enabled
 * NOTES
 *  This function directly accesses the following DMA Controller
 *  register(s)/bit field(s):
 *    - DmaCfgReg.DMA_EN
 * SEE ALSO
 *  dw_dmac_enable(), dw_dmac_disable()
 * SOURCE
 */
bool dw_dmac_isEnabled(struct dw_device *dev);
/*****/

/****f* dmac.functions/dw_dmac_enableChannel
 * DESCRIPTION
 *  This function enables the specified DMA channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA controller device handle
 *  ch_num      -- Enumerated DMA channel number
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 *  -DW_EPERM   -- if the DMA controller is not enabled
 * NOTES
 *  This function directly accesses the following DMA Controller
 *  register(s)/bit field(s): (x = channel number)
 *    - ChEnReg.CH_EN[x]
 *
 *  If enabling a DMA channel for interrupt driven transfers, this
 *  function should NOT be used. Instead use the
 *  dw_dmac_startTransfer() API function.
 * SEE ALSO
 *  dw_dmac_disableChannel(), dw_dmac_isChannelEnabled(),
 *  dw_dmac_startTransfer(), dw_dmac_getChannelEnableReg(),
 *  dw_dmac_channel_number
 * SOURCE
 */
int dw_dmac_enableChannel(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num);
/*****/

/****f* dmac.functions/dw_dmac_disableChannel
 * DESCRIPTION
 *  This function disables the specified DMA channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA controller device handle
 *  ch_num      -- Enumerated DMA channel number
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 *  -DW_EBUSY   -- if the DMA channel did not disable
 * NOTES
 *  This function directly accesses the following DMA Controller
 *  register(s)/bit field(s): (x = channel number)
 *    - ChEnReg.CH_EN[x]
 *
 *  The function returns -DW_EBUSY if the DMA channel is not able
 *  to be disabled. This may happen in the case of a SPLIT response
 *  on the AHB bus.
 * SEE ALSO
 *  dw_dmac_enableChannel(), dw_dmac_isChannelEnabled(),
 *  dw_dmac_getChannelEnableReg(),
 *  dw_dmac_channel_number
 * SOURCE
 */
int dw_dmac_disableChannel(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num);
/*****/

/****f* dmac.functions/dw_dmac_isChannelEnabled
 * DESCRIPTION
 *  This function returns whether the specified DMA channel is
 *  enabled. Only ONE DMA channel can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA controller device handle
 *  ch_num      -- Enumerated DMA channel number
 * RETURN VALUE
 *  true        -- if the channel is enabled
 *  false       -- if the channel is not enabled
 * NOTES
 *  This function directly accesses the following DMA Controller
 *  register(s)/bit field(s): (x = channel number)
 *    - ChEnReg.CH_EN[x]
 * SEE ALSO
 *  dw_dmac_enableChannel(), dw_dmac_disableChannel(),
 *  dw_dmac_getChannelEnableReg(),
 *  dw_dmac_channel_number
 * SOURCE
 */
bool dw_dmac_isChannelEnabled(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num);
/*****/

/****f* dmac.functions/dw_dmac_getChannelEnableReg
 * DESCRIPTION
 *  This function returns the lower byte of the channel enable register
 *  (ChEnReg).
 * ARGUMENTS
 *  dev         -- DMA controller device handle
 * RETURN VALUE
 *  Contents of the lower byte of the ChEnReg.
 * NOTES
 *  This function directly accesses the following DMA Controller
 *  register(s)/bit field(s):
 *    - ChEnReg.CH_EN[7:0]
 * SEE ALSO
 *  dw_dmac_enableChannel(), dw_dmac_disableChannel(),
 *  dw_dmac_isChannelEnabled()
 * SOURCE
 */
uint8_t dw_dmac_getChannelEnableReg(struct dw_device *dev);
/*****/

/****f* dmac.functions/dw_dmac_enableChannelIrq
 * DESCRIPTION
 *  This function enables interrupts for the selected channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA controller device handle
 *  ch_num      -- Enumerated DMA channel number
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 *  -DW_EBUSY   -- if the any of the specified DMA channels are enabled
 * NOTES
 *  This function directly accesses the following DMA Controller
 *  register(s)/bit field(s): (x = channel number)
 *    - CTLx.INT_EN
 * SEE ALSO
 *  dw_dmac_disableChannelIrq(), dw_dmac_isChannelIrqEnabled(),
 *  dw_dmac_channel_number
 * SOURCE
 */
int dw_dmac_enableChannelIrq(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num);
/*****/

/****f* dmac.functions/dw_dmac_disableChannelIrq
 * DESCRIPTION
 *  This function disables interrupts for the selected channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA controller device handle
 *  ch_num      -- Enumerated DMA channel number
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 *  -DW_EBUSY   -- if any of the specified DMA channels are enabled
 * NOTES
 *  This function directly accesses the following DMA Controller
 *  register(s) / bit field(s): (x = channel number)
 *    - CTLx.INT_EN
 * SEE ALSO
 *  dw_dmac_enableChannelIrq(), dw_dmac_isChannelIrqEnabled(),
 *  dw_dmac_channel_number
 * SOURCE
 */
int dw_dmac_disableChannelIrq(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num);
/*****/

/****f* dmac.functions/dw_dmac_isChannelIrqEnabled
 * DESCRIPTION
 *  This function returns whether interrupts are enabled for the
 *  specified DMA channel. Only ONE DMA channel can be specified for
 *  the dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA controller device handle
 *  ch_num      -- Enumerated DMA channel number
 * RETURN VALUE
 *  true        -- channel interrupts are enabled
 *  false       -- channel interrupts are not enabled
 * NOTES
 *  This function directly accesses the following DMA Controller
 *  register(s)/bit-field(s): (x = channel number)
 *    - CTLx.INT_EN
 * SEE ALSO
 *  dw_dmac_enableChannelIrq(), dw_dmac_disableChannelIrq()
 *  dw_dmac_channel_number
 * SOURCE
 */
bool dw_dmac_isChannelIrqEnabled(struct dw_device *dev,
     enum dw_dmac_channel_number ch_num);
/*****/

/****f* dmac.functions/dw_dmac_getFreeChannel
 * DESCRIPTION
 *  This function returns a DMA channel number (enumerated) that is
 *  disabled. The function starts at channel 0 and increments up
 *  through the channels until a free channel is found.
 * ARGUMENTS
 *  dev         -- DMA controller device handle
 * RETURN VALUE
 *  DMA channel number, as an enumerated type.
 * NOTES
 *  This function directly accesses the following DMA Controller
 *  register(s)/bit field(s):
 *    - ChEnReg.CH_EN
 * SEE ALSO
 *  dw_dmac_channel_number
 * SOURCE
 */
enum dw_dmac_channel_number dw_dmac_getFreeChannel(
     struct dw_device *dev);
/*****/

/****f* dmac.functions/dw_dmac_suspendChannel
 * DESCRIPTION
 *  This function suspends transfers on the specified channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA controller device handle
 *  ch_num      -- Enumerated DMA channel number
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 * NOTES
 *  This function directly accesses the following DMA Controller
 *  register(s) / bit-field(s): (x = channel number)
 *    - CFGx.CH_SUSP
 * SEE ALSO
 *  dw_dmac_resumeChannel(), dw_dmac_isChannelSuspended(),
 *  de_device, dw_dmac_channel_number
 * SOURCE
 */
int dw_dmac_suspendChannel(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num);
/*****/

/****f* dmac.functions/dw_dmac_resumeChannel
 * DESCRIPTION
 *  This function resumes (remove suspend) on the specified channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA controller device handle
 *  ch_num      -- Enumerated DMA channel number
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 * NOTES
 *  This function directly accesses the following DMA Controller
 *  register(s)/bit field(s): (x = channel number)
 *    - CFGx.CH_SUSP
 * SEE ALSO
 *  dw_dmac_suspendChannel(), dw_dmac_isChannelSuspended(),
 *  dw_dmac_channel_number
 * SOURCE
 */
int dw_dmac_resumeChannel(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num);
/*****/

/****f* dmac.functions/dw_dmac_isChannelSuspended
 * DESCRIPTION
 *  This function returns whether the specified channel is suspended.
 *  Only ONE DMA channel can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA controller device handle
 *  ch_num      -- Enumerated DMA channel number
 * RETURN VALUE
 *  true        -- if the channel is suspended
 *  false       -- if the channel is not suspended
 * NOTES
 *  This function directly accesses the following DMA Controller
 *  register(s)/bit field(s): (x = channel number)
 *    - CFGx.CH_SUSP
 * SEE ALSO
 *  dw_dmac_suspendChannel(), dw_dmac_resumeChannel(),
 *  dw_dmac_channel_number
 * SOURCE
 */
bool dw_dmac_isChannelSuspended(struct dw_device *dev,
     enum dw_dmac_channel_number ch_num);
/*****/

/****f* dmac.functions/dw_dmac_clearIrq
 * DESCRIPTION
 *  This function clears the specified interrupt(s) on the specified
 *  DMA channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument. Multiple interrupt types can
 *  be specified for the dw_dmac_irq argument.
 * ARGUMENTS
 *  dev         -- DMA controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  ch_irq      -- Enumerated interrupt type
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 * NOTES
 *  This function directly accesses the following DMA Controller
 *  register(s) / bit-field(s): (x = channel number)
 *    - ClearTfr.CLEAR[x]     (when Dmac_irq_tfr is specified)
 *    - ClearBlock.CLEAR[x]   (when Dmac_irq_block is specified)
 *    - ClearSrcTran.CLEAR[x] (when Dmac_irq_srctran is specified)
 *    - ClearDstTran.CLEAR[x] (when Dmac_irq_dsttran is specified)
 *    - ClearErr.CLEAR[x]     (when Dmac_irq_err is specified)
 * SEE ALSO
 *  dw_dmac_enableChannelIrq(), dw_dmac_isIrqMasked(),
 *  dw_dmac_isIrqActive(), dw_dmac_isRawIrqActive(),
 *  dw_dmac_maskIrq(), dw_dmac_unmaskIrq(),
 *  dw_dmac_channel_number, dw_dmac_irq
 *  
 * SOURCE
 */
int dw_dmac_clearIrq(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_irq ch_irq);
/*****/

/****f* dmac.functions/dw_dmac_maskIrq
 * DESCRIPTION
 *  This function masks the specified interrupt(s) on the specified
 *  channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument. Multiple interrupt types can
 *  be specified for the dw_dmac_irq argument.
 * ARGUMENTS
 *  dev         -- DMA controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  ch_irq      -- Enumerated interrupt type
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if the any of the specified DMA channels are out of
 *                 range (not present)
 * NOTE
 *  This function directly accesses the following DMA Controller
 *  register(s)/bit field(s): (x = channel number)
 *    - MaskTfr.INT_MASK[x]     (when Dmac_irq_tfr is specified)
 *    - MaskBlock.INT_MASK[x]   (when Dmac_irq_block is specified)
 *    - MaskSrcTran.INT_MASK[x] (when Dmac_irq_srctran is specified)
 *    - MaskDstTran.INT_MASK[x] (when Dmac_irq_dsttran is specified)
 *    - MaskErr.INT_MASK[x]     (when Dmac_irq_err is specified)
 * SEE ALSO
 *  dw_dmac_enableChannelIrq(), dw_dmac_clearIrq(),
 *  dw_dmac_isIrqActive(), dw_dmac_isRawIrqActive(),
 *  dw_dmac_unmaskIrq(), dw_dmac_isIrqMasked(),
 *  dw_dmac_channel_number, dw_dmac_irq
 * SOURCE
 */
int dw_dmac_maskIrq(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_irq ch_irq);
/*****/

/****f* dmac.functions/dw_dmac_unmaskIrq
 * DESCRIPTION
 *  This function unmasks the specified interrupt(s)
 *  on the specified channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument. Multiple interrupt types can
 *  be specified for the dw_dmac_irq argument.
 * ARGUMENTS
 *  dev         -- DMA controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  ch_irq      -- Enumerated interrupt type
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if the any of the specified DMA channels are out of
 *                 range (not present)
 * NOTES
 *  This function directly accesses the following DMA Controller
 *  register(s)/bit field(s): (x = channel number)
 *    - MaskTfr.INT_MASK[x]     (when Dmac_irq_tfr is specified)
 *    - MaskBlock.INT_MASK[x]   (when Dmac_irq_block is specified)
 *    - MaskSrcTran.INT_MASK[x] (when Dmac_irq_srctran is specified)
 *    - MaskDstTran.INT_MASK[x] (when Dmac_irq_dsttran is specified)
 *    - MaskErr.INT_MASK[x]     (when Dmac_irq_err is specified)
 * SEE ALSO
 *  dw_dmac_enableChannelIrq(), dw_dmac_clearIrq(),
 *  dw_dmac_isIrqActive(), dw_dmac_isRawIrqActive(),
 *  dw_dmac_maskIrq(), dw_dmac_isIrqMasked(),
 *  dw_dmac_channel_number, dw_dmac_irq
 * SOURCE
 */
int dw_dmac_unmaskIrq(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_irq ch_irq);
/*****/

/****f* dmac.functions/dw_dmac_isIrqMasked
 * DESCRIPTION
 *  This function returns whether the specified interrupt on the
 *  specified channel is masked.
 *  Only 1 DMA channel can be specified for the dw_dmac_channel_number
 *  argument. Only 1 interrupt type can be specified for the
 *  dw_dmac_irq argument.
 * ARGUMENTS
 *  dev         -- DMA controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  ch_irq      -- Enumerated interrupt type
 * RETURN VALUE
 *  true        -- if the interrupt is masked
 *  false       -- if the interrupt is NOT masked
 * NOTES
 *  This function directly accesses the following DMA Controller
 *  register(s)/bit field(s): (x = channel number)
 *    - MaskTfr.INT_MASK[x]     (when Dmac_irq_tfr is specified)
 *    - MaskBlock.INT_MASK[x]   (when Dmac_irq_block is specified)
 *    - MaskSrcTran.INT_MASK[x] (when Dmac_irq_srctran is specified)
 *    - MaskDstTran.INT_MASK[x] (when Dmac_irq_dsttran is specified)
 *    - MaskErr.INT_MASK[x]     (when Dmac_irq_err is specified)
 * SEE ALSO
 *  dw_dmac_enableChannelIrq(), dw_dmac_clearIrq(),
 *  dw_dmac_isIrqActive(), dw_dmac_isRawIrqActive(),
 *  dw_dmac_maskIrq(), dw_dmac_unmaskIrq(),
 *  dw_dmac_channel_number, dw_dmac_irq
 * SOURCE
 */
bool dw_dmac_isIrqMasked(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_irq ch_irq);
/*****/

/****f* dmac.functions/dw_dmac_isRawIrqActive
 * DESCRIPTION
 *  This function returns whether the specified raw interrupt on the
 *  specified channel is active.
 *  Only ONE DMA channel can be specified for the dw_dmac_channel_number
 *  argument. Only 1 interrupt type can be specified for the
 *  dw_dmac_irq argument.
 * ARGUMENTS
 *  dev         -- DMA controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  ch_irq      -- Enumerated interrupt type
 * RETURN VALUE
 *  true        -- if the interrupt is active
 *  false       -- if the interrupt is not active
 * NOTES
 *  This function directly accesses the following DMA Controller
 *  register(s)/bit field(s): (x = channel number)
 *    - RawTfr.RAW[x]     (when Dmac_irq_tfr is specified)
 *    - RawBlock.RAW[x]   (when Dmac_irq_block is specified)
 *    - RawSrcTran.RAW[x] (when Dmac_irq_srctran is specified)
 *    - RawDstTran.RAW[x] (when Dmac_irq_dsttran is specified)
 *    - RawErr.RAW[x]     (when Dmac_irq_err is specified)
 * SEE ALSO
 *  dw_dmac_enableChannelIrq(), dw_dmac_clearIrq(),
 *  dw_dmac_isIrqActive(), dw_dmac_maskIrq(),
 *  dw_dmac_unmaskIrq(), dw_dmac_isIrqMasked(),
 *  dw_dmac_channel_number, dw_dmac_irq
 * SOURCE
 */
bool dw_dmac_isRawIrqActive(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_irq ch_irq);
/*****/

/****f* dmac.functions/dw_dmac_isIrqActive
 * DESCRIPTION
 *  This function returns whether the specified interrupt on the
 *  specified channel is active after masking.
 *  All DMA channels OR only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument. Only 1 interrupt type can be
 *  specified for the dw_dmac_irq argument.
 * ARGUMENTS
 *  dev         -- DMA controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  ch_irq      -- Enumerated interrupt type.
 * RETURN VALUE
 *  true        -- if the interrupt is active
 *  false       -- if the interrupt is not active
 * NOTES
 *  This function directly accesses the following DMA Controller
 *  register(s)/bit field(s): (x = channel number)
 *    - StatusTfr.STATUS[x]     (when Dmac_irq_tfr is specified)
 *    - StatusBlock.STATUS[x]   (when Dmac_irq_block is specified)
 *    - StatusSrcTran.STATUS[x] (when Dmac_irq_srctran is specified)
 *    - StatusDstTran.STATUS[x] (when Dmac_irq_dsttran is specified)
 *    - StatusErr.STATUS[x]     (when Dmac_irq_err is specified)
 *    - StatusInt.STATUS[x]     (when Dmac_all_channels is specified)
 * SEE ALSO
 *  dw_dmac_enableChannelIrq(), dw_dmac_clearIrq(),
 *  dw_dmac_isRawIrqActive(), dw_dmac_maskIrq(), dw_dmac_unmaskIrq(),
 *  dw_dmac_isIrqMasked(), dw_dmac_channel_number,
 *  dw_dmac_irq
 * SOURCE
 */
bool dw_dmac_isIrqActive(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_irq ch_irq);
/*****/

/****f* dmac.functions/dw_dmac_setChannelConfig
 * DESCRIPTION
 *  This function sets configuration parameters in the DMAC's
 *  channel registers on the specified DMA channel.
 *  Only 1 DMA channel can be specified for the dw_dmac_channel_number
 *  argument.
 * ARGUMENTS
 *  dev         -- DMA controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  ch_config   -- Configuration structure handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if the DMA channel is out of range (not present) or
 *                 if multiple DMA channels are specified
 *  -DW_EBUSY   -- if the DMA channel is enabled
 *  -DW_EINVAL  -- if an attempt is made to set a configuration
 *                 parameter value outside the legal range
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CTLx (all bits), CFGx (all bits bar CH_SUSP, FIFO_EMPTY),
 *    - LLPx (all bits), SARx (all bits), DARx (all bits),
 *    - SGRx (all bits), DSRx (all bits), SSTATx (all bits),
 *    - DSTATx (all bits), SSTATARx (all bits), DSTATARx (all bits)
 * SEE ALSO
 *  dw_dmac_getChannelConfig(), dw_dmac_channel_number,
 *  dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setChannelConfig(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    struct dw_dmac_channel_config *ch_config);
/*****/

/****f* dmac.functions/dw_dmac_getChannelConfig
 * DESCRIPTION
 *  This function gets configuration parameters in the DMAC's
 *  channel registers for the specified DMA channel.
 *  Only 1 DMA channel can be specified for the dw_dmac_channel_number
 *  argument.
 * ARGUMENTS
 *  dev         -- DMA controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  ch_config   -- Configuration structure handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if the DMA channel is out of range (not present) or
 *                 if multiple DMA channels are specified
 *  -DW_EBUSY   -- if the DMA channel is enabled
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CTLx (all bits), CFGx (all bits bar CH_SUSP, FIFO_EMPTY),
 *    - LLPx (all bits), SARx (all bits), DARx (all bits),
 *    - SGRx (all bits), DSRx (all bits), SSTATx (all bits),
 *    - DSTATx (all bits), SSTATARx (all bits), DSTATARx (all bits)
 *  
 *  The configuration structure that is passed into this function
 *  has its members loaded with the contents of the following DMAC
 *  registers: CTLx, CFGx, LLPx, SARx, DARx, SGRx, DSR, SSTAT, DSTAT,
 *  SSTATAR, and DSTATAR.
 * SEE ALSO
 *  dw_dmac_setChannelConfig(), dw_dmac_channel_number,
 *  dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_getChannelConfig(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    struct dw_dmac_channel_config *ch_config);
/*****/

/****f* dmac.functions/dw_dmac_setTransferType
 * DESCRIPTION
 *  This function sets up the specified DMA channel(s) for the specified
 *  transfer type. The dw_dmac_transfer_type enumerated type describes
 *  all of the transfer types supported by the DMA controller.
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  trans_type  -- Enumerated DMA transfer type
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if the any of the specified DMA channels are out of
 *                 range (not present)
 *  -DW_EBUSY   -- if the any of the specified DMA channels are enabled
 *  -DW_EPERM   -- if hardware parameter DMAH_CHx_MULTI_BLK_EN == 0
 *  -DW_EINVAL  -- if an invalid transfer type argument is specified
 *  -DW_ENOSYS  -- if the transfer type is unsupported by the specified
 *                 DMA channel
 * NOTES
 *  This function directly accesses the following DMAC
 *  register(s)/bit field(s): (x = channel number)
 *    - CTLx.LLP_SRC_EN, CTLx.LLP_DST_EN, CFGx.RELOAD_SRC,
 *    - CFGx.RELOAD_DST, LLPx.LOC
 *
 *  If tranfer types row 5 to row 10 are being set, the user must also
 *  use the setListPointerAddress() API function to set the LLP.LOC
 *  field. This can be set before or after this function call, but must
 *  be done before the channel is enabled.
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_getTransferType(), dw_dmac_setReload(),
 *  dw_dmac_getReload(), dw_dmac_setLlpEnable(),
 *  dw_dmac_getLlpEnable(), dw_dmac_setListPointerAddress(),
 *  dw_dmac_getListPointerAddress(), dw_dmac_setChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_transfer_type,
 *  dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setTransferType(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_transfer_type trans_type);
/*****/

/****f* dmac.functions/dw_dmac_getTransferType
 * DESCRIPTION
 *  This function returns the DMA transfer type for the specified
 *  DMA channel. The dw_dmac_transfer_type enumerated type
 *  describes all of the transfer types supported by the DMA
 *  controller.
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 * RETURN VALUE
 *  Enumerated DMA transfer type.
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CTLx.LLP_SRC_EN, CTLx.LLP_DST_EN, CFGx.RELOAD_SRC,
 *    - CFGx.RELOAD_DST, LLPx.LOC
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setTransferType(), dw_dmac_setReload(),
 *  dw_dmac_getReload(), dw_dmac_setLlpEnable(),
 *  dw_dmac_getLlpEnable(), dw_dmac_setListPointerAddress(),
 *  dw_dmac_getListPointerAddress(), dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_transfer_type,
 *  dw_dmac_channel_config
 * SOURCE
 */
enum dw_dmac_transfer_type dw_dmac_getTransferType(
    struct dw_device *dev, enum dw_dmac_channel_number ch_num);
/*****/

/****f* dmac.functions/dw_dmac_isBlockTransDone
 * DESCRIPTION
 *  This function returns whether the block transfer of the selected
 *  channel has completed.
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA controller device handle
 *  ch_num      -- Enumerated DMA channel number
 * RETURN VALUE
 *  true        -- if the block transfer is done
 *  false       -- if the block transfer is not done
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CTLx.DONE
 *
 *  For linked list transfers (rows 5-10), this function 
 *  should not be used.
 * SEE ALSO
 *  dw_dmac_channel_number
 * SOURCE
 */
bool dw_dmac_isBlockTransDone(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num);
/*****/

/****f* dmac.functions/dw_dmac_isFifoEmpty
 * DESCRIPTION
 *  This function returns whether the FIFO is empty on the 
 *  specified channel. Only 1 DMA channel can be specified 
 *  for the dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA controller device handle
 *  ch_num      -- Enumerated DMA channel number
 * RETURN VALUE
 *  true        -- channel FIFO is empty
 *  false       -- channel FIFO is not empty
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CFGx.FIFO_EMPTY
 * SEE ALSO
 *  dw_dmac_channel_number
 * SOURCE
 */
bool dw_dmac_isFifoEmpty(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num);
/*****/

/****f* dmac.functions/dw_dmac_setTestMode
 * DESCRIPTION
 *  This function enables/disables test mode in the DMAC.
 * ARGUMENTS
 *  dev         -- DMA controller device handle
 *  dw_state    -- Enumerated Enabled/Disabled state
 * RETURN VALUE
 *  none
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - DmaTestReg.TEST_SLV_IF
 * SEE ALSO
 *  dw_dmac_getTestMode(), dw_state
 * SOURCE
 */
void dw_dmac_setTestMode(struct dw_device *dev,
    enum dw_state state);
/*****/

/****f* dmac.functions/dw_dmac_getTestMode
 * DESCRIPTION
 *  This function returns whether test mode is enabled or disabled
 *  in the DMA controller.
 * ARGUMENTS
 *  dev          - DMA controller device handle
 * RETURN VALUE
 *  Enumerated Enabled/Disabled state.
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - DmaTestReg.TEST_SLV_IF
 * SEE ALSO
 *  dw_dmac_setTestMode(), dw_state
 * SOURCE
 */
enum dw_state dw_dmac_getTestMode(struct dw_device *dev);
/*****/

/****f* dmac.functions/dw_dmac_setSoftwareRequest
 * DESCRIPTION
 *  This function is used to activate/de-activate the source and 
 *  destination software request registers. Three registers exist 
 *  for software requests on the source and destination. 
 *  These are: Request, Single Request, Last Request. 
 *  Use the dw_dmac_software_req enum to select which of the three
 *  registers is accessed. Use the dw_dmac_src_dst_select
 *  enum to select either the source or destination register.
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument. Both source and destination can be
 *  specified for the dw_dmac_src_dst_select argument. Only 1 request
 *  register can be specified for the dw_dmac_software_req argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated source/destination select
 *  sw_req      -- Enumerated request register select
 *  state       -- Enumerated enabled/disabled state
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if the any of the specified DMA channels are
 *                 out of range (not present)
 *  -DW_EPERM   -- if the channel is not enabled
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - ReqSrcReg.SRC_REQ[x]       (when Dmac_request and Dmac_src are
 *                                  specified) 
 *    - ReqDstReq.DST_REQ[x]       (when Dmac_request and Dmac_dst are
 *                                  specified)
 *    - SglReqSrcReg.SRC_SGLREQ[x] (when Dmac_single_request and
 *                                  Dmac_src are specified)
 *    - SglReqDstReg.DST_SGLREQ[x] (when Dmac_single_request and
 *                                  Dmac_dst are specified)
 *    - LstSrcReg.LSTSRC[x]        (when Dmac_last_request and
 *                                  Dmac_src are specified)
 *    - LstDstReg.LSTDST[x]        (when Dmac_last_request and
 *                                  Dmac_dst are specified)
 * SEE ALSO
 *  dw_dmac_getSoftwareRequest(), dw_dmac_channel_number
 *  dw_dmac_src_dst_select, dw_dmac_software_req, dw_state
 * SOURCE
 */
int dw_dmac_setSoftwareRequest(struct dw_device *dev,
   enum dw_dmac_channel_number ch_num,
   enum dw_dmac_src_dst_select sd_sel,
   enum dw_dmac_software_req sw_req, enum dw_state state);
/*****/

/****f* dmac.functions/dw_dmac_getSoftwareRequest
 * DESCRIPTION
 *  This function is used to return the activate/in-activate status 
 *  of the source and destination software request registers. 
 *  Three registers exist for software requests on the source and
 *  destination. These are: Request, Single Request, Last Request.
 *  Use the dw_dmac_software_req enum to select which of the three
 *  registers is accessed. Use the dw_dmac_src_dst_select enum to
 *  select either the source or destination register.
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument. Only 1, source or destination,
 *  can be specified for the dw_dmac_src_dst_select argument. Only 1
 *  request register can be specified for the dw_dmac_software_req
 *  argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated source/destination select
 *  sw_req      -- Enumerated request register select
 * RETURN VALUE
 *  Enumerated enabled/disabled state.
 * NOTES
 *  This function directly accesses the following DMA Controller
 *  register(s)/bit field(s): (x = channel number)
 *    - ReqSrcReg.SRC_REQ[x]       (when Dmac_request and Dmac_src
 *                                  are specified) 
 *    - ReqDstReq.DST_REQ[x]       (when Dmac_request and Dmac_dst
 *                                  are specified)
 *    - SglReqSrcReg.SRC_SGLREQ[x] (when Dmac_single_request and
 *                                  Dmac_src are specified)
 *    - SglReqDstReg.DST_SGLREQ[x] (when Dmac_single_request and
 *                                  Dmac_dst are specified)
 *    - LstSrcReg.LSTSRC[x]        (when Dmac_last_request and
 *                                  Dmac_src are specified)
 *    - LstDstReg.LSTDST[x]        (when Dmac_last_request and
 *                                  Dmac_dst are specified)
 * SEE ALSO
 *  dw_dmac_setSoftwareRequest(), dw_dmac_channel_number
 *  dw_dmac_src_dst_select, dw_dmac_software_req, dw_state
 * SOURCE
 */
enum dw_state dw_dmac_getSoftwareRequest(struct dw_device *dev,
   enum dw_dmac_channel_number ch_num,
   enum dw_dmac_src_dst_select sd_sel,
   enum dw_dmac_software_req sw_req);
/*****/

/****f* dmac.functions/dw_dmac_setAddress
 * DESCRIPTION
 *  This function sets the address on the specified source or/and
 *  destination register of the specified channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument. Both source and destination can
 *  be specified for the dw_dmac_src_dst_select argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated source/destination select
 *  address     -- 32-bit address value
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 *  -DW_EBUSY   -- if any of the specified DMA channels are enabled
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - SARx.SAR   (when Dmac_src is specified)
 *    - DARx.DAR   (when Dmac_dst is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_getAddress(), dw_dmac_setChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_src_dst_select,
 *  dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setAddress(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_src_dst_select sd_sel,
    uint32_t address);
/*****/

/****f* dmac.functions/dw_dmac_getAddress
 * DESCRIPTION
 *  This function returns the address on the specified source or
 *  destination register of the specified channel.
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument. Only 1, source or destination,
 *  can be specified for the dw_dmac_src_dst_select argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated source/destination select
 * RETURN VALUE
 *  32-bit contents of source/destination address register of the
 *  specified DMA channel.
 * NOTES
 *  This function directly accesses the following DMA Controller
 *  register(s)/bit field(s): (x = channel number)
 *    - SARx.SAR   (when Dmac_src is specified)
 *    - DARx.DAR   (when Dmac_dst is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_getChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setAddress(), dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_src_dst_select,
 *  dw_dmac_channel_config
 * SOURCE
 */
uint32_t dw_dmac_getAddress(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_src_dst_select sd_sel);
/*****/

/****f* dmac.functions/dw_dmac_setBlockTransSize
 * DESCRIPTION
 *  This function sets the block size of a transfer on the specified
 *  DMA channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  block_size  -- Size of the transfers block
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 *  -DW_EBUSY   -- if any of the specified DMA channels are enabled
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CTLx.BLOCK_TS
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_getBlockTransSize(), dw_dmac_setChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setBlockTransSize(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num, uint16_t block_size);
/*****/

/****f* dmac.functions/dw_dmac_getBlockTransSize
 * DESCRIPTION
 *  This function returns the block size of a transfer
 *  on the specified DMA channel.
 *  Only ONE DMA channel can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 * RETURN VALUE
 *  16-bit value of the transfer block size.
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CTLx.BLOCK_TS
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_getChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_getBlockTransSize(), dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_channel_config
 * SOURCE
 */
uint16_t dw_dmac_getBlockTransSize(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num);
/*****/

/****f* dmac.functions/dw_dmac_setMstSelect
 * DESCRIPTION
 *  This function sets the specified source and/or destination master
 *  select interface on the specified DMA channel(s).
 *  Multiple DMA channels can be specified for the
 *  the dw_dmac_channel_number argument. Both source and destination
 *  can be specified for the dw_dmac_src_dst_select argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated source/destination select
 *  mst_num     -- Enumerated master interface number
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 *  -DW_EBUSY   -- if any of the specified DMA channels are enabled
 *  -DW_EINVAL  -- if the master interface specified exceeds the
 *                 DMAH_NUM_MASTER_INT hardware parameter. For
 *                 example if Dmac_master2 is specified when
 *                 DMAH_NUM_MASTER_INT == 1.
 *  -DW_EPERM   -- if this field is hardcoded. Field is hardcoded
 *                 on the source if the DMAH_CHx_SMS != 0x4. Field is
 *                 hardcoded on the destination if
 *                 DMAH_CHx_DMS != 0x4. (x indicates channel number).
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CTLx.SMS   (when Dmac_src is specified)
 *    - CTLx.DMS   (when Dmac_dst is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_getMstSelect(), dw_dmac_setChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_src_dst_select,
 *  dw_dmac_master_number, dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setMstSelect(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_src_dst_select sd_sel,
    enum dw_dmac_master_number mst_num);
/*****/

/****f* dmac.functions/dw_dmac_getMstSelect
 * DESCRIPTION
 *  This function returns the specified source or destination master
 *  select interface on the specified DMA channel.
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument. Only 1, source or destination,
 *  can be specified for the dw_dmac_src_dst_select argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated source/destination select
 * RETURN VALUE
 *  Enumerated master interface number.
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CTLx.SMS   (when Dmac_src is specified)
 *    - CTLx.DMS   (when Dmac_dst is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_getChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setMstSelect(), dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_src_dst_select,
 *  dw_dmac_master_number, dw_dmac_channel_config
 * SOURCE
 */
enum dw_dmac_master_number dw_dmac_getMstSelect(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_src_dst_select sd_sel);
/*****/

/****f* dmac.functions/dw_dmac_setMemPeriphFlowCtl
 * DESCRIPTION
 *  This function sets the transfer device type and flow control
 *  (TT_FC) for the specified DMA channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  tt_fc       -- Enumerated transfer device type and flow control
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 *  -DW_EBUSY   -- if any of the specified DMA channels are enabled
 *  -DW_EPERM   -- if the flow control device is hardcoded and an
 *                 invalid dw_dmac_trnasfer_type is specified. See the
 *                 DMAH_CHx_FC hardware parameter
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CTLx.TT_FC
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_getMemPeriphFlowCtl(), dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_transfer_flow,
 *  dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setMemPeriphFlowCtl(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_transfer_flow tt_fc);
/*****/

/****f* dmac.functions/dw_dmac_getMemPeriphFlowCtl
 * DESCRIPTION
 *  This function returns the transfer device type and flow control
 *  (TT_FC) for the specified DMA channel.
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 * RETURN VALUE
 *  Enumerated transfer device type and flow control
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CTLx.TT_FC
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_getChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setMemPeriphFlowCtl(), dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_transfer_flow,
 *  dw_dmac_channel_config
 * SOURCE
 */
enum dw_dmac_transfer_flow dw_dmac_getMemPeriphFlowCtl(
    struct dw_device *dev, enum dw_dmac_channel_number ch_num);
/*****/

/****f* dmac.functions/dw_dmac_setScatterEnable
 * DESCRIPTION
 *  This function enables or disables the destination scatter mode
 *  on the specified DMA channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  dw_state    -- Enumerated Enable/Disable state
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 *  -DW_EBUSY   -- if any of the specified DMA channels are enabled
 *  -DW_EPERM   -- if the hardware parameter DMAH_CHx_DST_SCA_EN == 0
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CTLx.DST_SCATTER_EN
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_getScatterEnable(), dw_dmac_setChannelConfig(),
 *  dw_dmac_channel_number, dw_state, dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setScatterEnable(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num, enum dw_state state);
/*****/

/****f* dmac.functions/dw_dmac_getScatterEnable
 * DESCRIPTION
 *  This function returns whether scatter mode is enabled or disabled
 *  on the specified DMA channel.
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 * RETURN VALUE
 *  Enumerated Enabled/Disabled state
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CTLx.DST_SCATTER_EN
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_getChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setScatterEnable(), dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_state, dw_dmac_channel_config
 * SOURCE
 */
enum dw_state dw_dmac_getScatterEnable(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num);
/*****/

/****f* dmac.functions/dw_dmac_setGatherEnable
 * DESCRIPTION
 *  This function enables or disables the source gather mode on the
 *  specified DMA channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  dw_state    -- Enumerated Enable/Disable state
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if the any of the specified DMA channels are out of
 *                 range (not present)
 *  -DW_EBUSY   -- if the any of the specified DMA channels are enabled
 *  -DW_EPERM   -- if the hardware parameter DMAH_CHx_SRC_GAT_EN == 0
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CTLx.SRC_GATHER_EN
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_getGatherEnable(), dw_dmac_setChannelConfig(),
 *  dw_dmac_channel_number, dw_state, dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setGatherEnable(struct dw_device *dev,
   enum dw_dmac_channel_number ch_num, enum dw_state state);
/*****/

/****f* dmac.functions/dw_dmac_getGatherEnable
 * DESCRIPTION
 *  This function returns whether gather mode is enabled or disabled
 *  on the specified DMA channel.
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 * RETURN VALUE
 *  Enumerated Enabled/Disabled state
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CTLx.SRC_GATHER_EN
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_getChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setGatherEnable(), dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_state, dw_dmac_channel_config
 * SOURCE
 */
enum dw_state dw_dmac_getGatherEnable(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num);
/*****/

/****f* dmac.functions/dw_dmac_setBurstTransLength
 * DESCRIPTION
 *  This function sets the specified source and/or destination
 *  burst size on the specified DMA channel(s). 
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument. Both source and destination
 *  can be specified for the dw_dmac_src_dst_select argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated source/destination select
 *  xf_length   -- Enumerated burst size
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 *  -DW_EBUSY   -- if any of the specified DMA channels are enabled
 *  -DW_EINVAL  -- if the specified burst length is greater than the
 *                 hardware parameter DAMH_CHx_MAX_MULTI_SIZE
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CTLx.SRC_MSIZE   (when Dmac_src is specified)
 *    - CTLx.DST_MSIZE   (when Dmac_dst is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_getBurstTransLength(), dw_dmac_setChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_src_dst_select,
 *  dw_dmac_burst_trans_length, dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setBurstTransLength(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_src_dst_select sd_sel,
    enum dw_dmac_burst_trans_length xf_length);
/*****/

/****f* dmac.functions/dw_dmac_getBurstTransLength
 * DESCRIPTION
 *  This function returns the specified source or destination
 *  burst size on the specified DMA channel. 
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument. Only 1, source or destination,
 *  can be specified for the dw_dmac_src_dst_select argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated source/destination select
 * RETURN VALUE
 *  Enumerated burst size
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CTLx.SRC_MSIZE   (when Dmac_src is specified)
 *    - CTLx.DST_MSIZE   (when Dmac_dst is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_getChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setBurstTransLength(), dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_src_dst_select,
 *  dw_dmac_burst_trans_length, dw_dmac_channel_config
 * SOURCE
 */
enum dw_dmac_burst_trans_length dw_dmac_getBurstTransLength(
    struct dw_device *dev, enum dw_dmac_channel_number ch_num,
    enum dw_dmac_src_dst_select sd_sel);
/*****/

/****f* dmac.functions/dw_dmac_setAddressInc
 * DESCRIPTION
 *  This function sets the address increment type on the specified
 *  source and/or destination on the specified DMA channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument. Both source and destination
 *  can be specified for the dw_dmac_src_dst_select argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated source/destination select
 *  addr_inc    -- Enumerated increment type
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 *  -DW_EBUSY   -- if any of the specified DMA channels are enabled
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CTLx.SINC   (when Dmac_src is specified)
 *    - CTLx.DINC   (when Dmac_dst is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_getAddressInc(), dw_dmac_setChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_src_dst_select,
 *  dw_dmac_address_increment, dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setAddressInc(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_src_dst_select sd_sel,
    enum dw_dmac_address_increment addr_inc);
/*****/

/****f* dmac.functions/dw_dmac_getAddressInc
 * DESCRIPTION
 *  This function returns the address increment type on the specified
 *  source or destination on the specified DMA channel.
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument. Only 1, source or destination,
 *  can be specified for the dw_dmac_src_dst_select argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated source/destination select
 * RETURN VALUE
 *  Enumerated address increment type
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CTLx.SINC   (when Dmac_src is specified)
 *    - CTLx.DINC   (when Dmac_dst is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_getChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setAddressInc(), dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_src_dst_select,
 *  dw_dmac_address_increment, dw_dmac_channel_config
 * SOURCE
 */
enum dw_dmac_address_increment dw_dmac_getAddressInc(
    struct dw_device *dev, enum dw_dmac_channel_number ch_num,
    enum dw_dmac_src_dst_select sd_sel);
/*****/

/****f* dmac.functions/dw_dmac_setTransWidth
 * DESCRIPTION
 *  This function sets the specified source and/or destination
 *  transfer width on the specified DMA channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument. Both source and destination
 *  can be specified for the dw_dmac_src_dst_select argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated source/destination select
 *  xf_width    -- Enumerated transfer width
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 *  -DW_EBUSY   -- if any of the specified DMA channels are enabled
 *  -DW_EPERM   -- if hardware parameter DMAH_CHx_STW != 0 and the
 *                 source is specified, or hardware parameter
 *                 DMAH_CHx_DTW != 0 and the destination is specified
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CTLx.SRC_TR_WIDTH   (when Dmac_src is specified)
 *    - CTLx.DST_TR_WIDTH   (when Dmac_dst is specified)
 *
 *  The value programmed must be less than or equal to the
 *  hardware parameter DMAH_Mx_HDATA_WIDTH, where x is the AMBA
 *  layer 1 to 4 where the source/destination resides.
 *  
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_getTransWidth(), dw_dmac_setChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_src_dst_select,
 *  dw_dmac_transfer_width, dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setTransWidth(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_src_dst_select sd_sel,
    enum dw_dmac_transfer_width xf_width);
/*****/

/****f* dmac.functions/dw_dmac_getTransWidth
 * DESCRIPTION
 *  This function returns the specified source or destination
 *  transfer width on the specified DMA channel.
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument. Only 1, source or destination,
 *  can be specified for the dw_dmac_src_dst_select argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated source/destination select
 * RETURN VALUE
 *  Enumerated transfer width
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CTLx.SRC_TR_WIDTH   (when Dmac_src is specified)
 *    - CTLx.DST_TR_WIDTH   (when Dmac_dst is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_getChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setTransWidth(), dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_src_dst_select,
 *  dw_dmac_transfer_width, dw_dmac_channel_config
 * SOURCE
 */
enum dw_dmac_transfer_width dw_dmac_getTransWidth(
    struct dw_device *dev, enum dw_dmac_channel_number ch_num,
    enum dw_dmac_src_dst_select sd_sel);
/*****/

/****f* dmac.functions/dw_dmac_setHsInterface
 * DESCRIPTION
 *  This function sets the handshaking interface on the specified
 *  source or destination on the specified DMA channel.
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument. Both source and destination
 *  can be specified for the dw_dmac_src_dst_select argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated source/destination select
 *  hs_inter    -- Enumerated handshaking interface number
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if the any of the specified DMA channels are out of
 *                 range (not present)
 *  -DW_EINVAL  -- if the specified handshaking interface is great than
 *                 or equal to the hardware parameter DMAH_NUM_HS_INT
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CFGx.SRC_PER   (when Dmac_src is specified)
 *    - CFGx.DEST_PER  (when Dmac_dst is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_getHsInterface(), dw_dmac_setChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_src_dst_select,
 *  dw_dmac_hs_interface, dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setHsInterface(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_src_dst_select sd_sel,
    enum dw_dmac_hs_interface hs_inter);
/*****/

/****f* dmac.functions/dw_dmac_getHsInterface
 * DESCRIPTION
 *  This function returns the handshaking interface on the specified
 *  source or destination on the specified DMA channel.
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument. Only 1, source or destination,
 *  can be specified for the dw_dmac_src_dst_select argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated source/destination select
 * RETURN VALUE
 *  Enumerated handshaking interface number
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CFGx.SRC_PER   (when Dmac_src is specified)
 *    - CFGx.DEST_PER  (when Dmac_dst is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_getChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setHsInterface(), dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_src_dst_select,
 *  dw_dmac_hs_interface, dw_dmac_channel_config
 * SOURCE
 */
enum dw_dmac_hs_interface dw_dmac_getHsInterface(
    struct dw_device *dev, enum dw_dmac_channel_number ch_num,
    enum dw_dmac_src_dst_select sd_sel);
/*****/

/****f* dmac.functions/dw_dmac_setStatUpdate
 * DESCRIPTION
 *  This function enables/disables the specified source and/or
 *  destination status update feature on the specified DMA channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument. Both source and destination
 *  can be specified for the dw_dmac_src_dst_select argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated source/destination select
 *  state       -- Enumerated Enable/Disable state
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 *  -DW_EPERM   -- if the hardware parameter DMAH_CHx_STAT_SRC == 0
 *                 and the source is specified, or hardware parameter
 *                 DMAH_CHx_STAT_DST == 0 and the destination is
 *                 specified
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CFGx.SS_UPD_EN   (when Dmac_src is specified)
 *    - CFGx.DS_UPD_EN   (when Dmac_dst is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_getStatUpdate(), dw_dmac_setChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_src_dst_select,
 *  dw_state, dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setStatUpdate(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_src_dst_select sd_sel, enum dw_state state);
/*****/

/****f* dmac.functions/dw_dmac_getStatUpdate
 * DESCRIPTION
 *  This function returns whether the status update feature is
 *  enabled or disabled for the specified source or destination
 *  on the specified DMA channel.
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument. Only 1, source or destination,
 *  can be specified for the dw_dmac_src_dst_select argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated source/destination select
 * RETURN VALUE
 *  Enumerated Enabled/Disabled state
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CFGx.SS_UPD_EN   (when Dmac_src is specified)
 *    - CFGx.DS_UPD_EN   (when Dmac_dst is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_getChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setStatUpdate(), dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_src_dst_select,
 *  dw_state, dw_dmac_channel_config
 * SOURCE
 */
enum dw_state dw_dmac_getStatUpdate(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_src_dst_select sd_sel);
/*****/

/****f* dmac.functions/dw_dmac_setProtCtl
 * DESCRIPTION
 *  This function sets the prot level for the AMBA bus
 *   on the specified DMA channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  prot_lvl    -- Enumerated prot level
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CFGx.PROTCTL
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_getProtCtl(), dw_dmac_setChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_prot_level,
 *  dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setProtCtl(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_prot_level prot_lvl);
/*****/

/****f* dmac.functions/dw_dmac_getProtCtl
 * DESCRIPTION
 *  This function returns the prot level for the AMBA bus on the
 *  specified DMA channel.
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 * RETURN VALUE
 *  Enumerated protection level
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CFGx.PROTCTL
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_getChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setProtCtl(), dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_prot_level,
 *  dw_dmac_channel_config
 * SOURCE
 */
enum dw_dmac_prot_level dw_dmac_getProtCtl(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num);
/*****/

/****f* dmac.functions/dw_dmac_setFifoMode
 * DESCRIPTION
 *  This function sets the FIFO mode on the specified DMA channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  fifo_mode   -- Enumerated fifo mode for the DMA
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CFGx.FIFO_MODE
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_getFifoMode(), dw_dmac_setChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_fifo_mode,
 *  dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setFifoMode(struct dw_device *dev,
   enum dw_dmac_channel_number ch_num,
   enum dw_dmac_fifo_mode fifo_mode);
/*****/

/****f* dmac.functions/dw_dmac_getFifoMode
 * DESCRIPTION
 *  This function returns the FIFO mode on the specified DMA channel(s)
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 * RETURN VALUE
 *  Enumerated FIFO mode for the DMA
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CFGx.FIFO_MODE
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_getChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setFifoMode(), dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_fifo_mode,
 *  dw_dmac_channel_config
 * SOURCE
 */
enum dw_dmac_fifo_mode dw_dmac_getFifoMode(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num);
/*****/

/****f* dmac.functions/dw_dmac_setFlowCtlMode
 * DESCRIPTION
 *  This function sets the flow control mode on the specified DMA
 *  channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  fc_mode     -- Enumerated flow control mode
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CFGx.FC_MODE
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_getFlowCtlMode(), dw_dmac_setChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_flow_ctl_mode,
 *  dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setFlowCtlMode(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_flow_ctl_mode fc_mode);
/*****/

/****f* dmac.functions/dw_dmac_getFlowCtlMode
 * DESCRIPTION
 *  This function returns the flow control mode on the specified DMA
 *  channel.
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 * RETURN VALUE
 *  Enumerated flow control mode
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CFGx.FC_MODE
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_getChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setFlowCtlMode(), dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_flow_ctl_mode,
 *  dw_dmac_channel_config
 * SOURCE
 */
enum  dw_dmac_flow_ctl_mode dw_dmac_getFlowCtlMode(
    struct dw_device *dev, enum dw_dmac_channel_number ch_num);
/*****/

/****f* dmac.functions/dw_dmac_setMaxAmbaBurstLength
 * DESCRIPTION
 *  This function sets the maximum amba burst length on the specified
 *  DMA channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument.
 * USES
 *  Accesses the following DMA controller register/bit_field:
 *  x refers to the channel index
 *   - CFGx.MAX_ABRST
 * ARGUMENTS
 *  dev             -- DMA Controller device handle
 *  ch_num          -- Enumerated DMA channel number
 *  burst_length    -- AMBA burst length value
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 *  -DW_EPERM   -- if hardware parameter DMAH_MABRST == 0
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CFGx.MAX_ABRST
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_getMaxAmbaBurstLength(), dw_dmac_setChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setMaxAmbaBurstLength(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num, uint16_t burst_length);
/*****/

/****f* dmac.functions/dw_dmac_getMaxAmbaBurstLength
 * DESCRIPTION
 *  This function returns the maximum amba burst length 
 *  on the specified DMA channel.
 *  Only ONE DMA channel can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 * RETURN VALUE
 *  AMBA burst length value
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CFGx.MAX_ABRST
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_getChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setMaxAmbaBurstLength(), dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_channel_config
 * SOURCE
 */
uint16_t dw_dmac_getMaxAmbaBurstLength(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num);
/*****/

/****f* dmac.functions/dw_dmac_setHsPolarity
 * DESCRIPTION
 *  This function sets the handshaking interface polarity on the
 *  specified source and/or destination on the specified DMA
 *  channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument. Both source and destination
 *  can be specified for the dw_dmac_src_dst_select argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated source/destination select
 *  pol_level   -- Enumerated polarity level
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CFGx.SRC_HS_POL   (when Dmac_src is specified)
 *    - CFGx.DST_HS_POL   (when Dmac_dst is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_getHsPolarity(), dw_dmac_setChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_src_dst_select,
 *  dw_dmac_polarity_level, dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setHsPolarity(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_src_dst_select sd_sel,
    enum dw_dmac_polarity_level pol_level);
/*****/

/****f* dmac.functions/dw_dmac_getHsPolarity
 * DESCRIPTION
 *  This function returns the handshaking interface polarity on the
 *  specified source or destination on the specified DMA channel.
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument. Only 1, source or destination,
 *  can be specified for the dw_dmac_src_dst_select argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated source/destination select
 * RETURN VALUE
 *  Enumerated polarity level
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CFGx.SRC_HS_POL   (when Dmac_src is specified)
 *    - CFGx.DST_HS_POL   (when Dmac_dst is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_getChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setHsPolarity(), dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_src_dst_select,
 *  dw_dmac_polarity_level, dw_dmac_channel_config
 * SOURCE
 */
enum dw_dmac_polarity_level dw_dmac_getHsPolarity(
    struct dw_device *dev, enum dw_dmac_channel_number ch_num,
    enum dw_dmac_src_dst_select sd_sel);
/*****/

/****f* dmac.functions/dw_dmac_setLockLevel
 * DESCRIPTION
 *  This function sets the lock level for the specified bus and/or
 *  channel on the specified DMA channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  bus_ch      -- Enumerated channel or bus lock select
 *  lock_l      -- Enumerated level for the lock feature
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 *  -DW_EPERM   -- if hardware parameter DMAH_CHx_LOCK_EN == 0
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CFGx.LOCK_B_L    (when Dmac_lock_bus is specified)
 *    - CFGx.LOCK_CH_L   (when Dmac_lock_channel is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_getLockLevel(), dw_dmac_setLockEnable(),
 *  dw_dmac_getLockEnable(), dw_dmac_setChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_lock_level,
 *  dw_dmac_lock_bus_ch, dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setLockLevel(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_lock_bus_ch bus_ch,
    enum dw_dmac_lock_level lock_l);
/*****/

/****f* dmac.functions/dw_dmac_getLockLevel
 * DESCRIPTION
 *  This function returns the lock level for the specified bus or
 *  channel on the specified DMA channel.  Only 1 DMA channel can be
 *  specified for the dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  bus_ch      -- Enumerated channel or bus lock select
 * RETURN VALUE
 *  Enumerated level for the lock feature
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CFGx.LOCK_B_L    (when Dmac_lock_bus is specified)
 *    - CFGx.LOCK_CH_L   (when Dmac_lock_channel is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_getChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setLockLevel(), dw_dmac_setLockEnable(),
 *  dw_dmac_getLockEnable(), dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_lock_level,
 *  dw_dmac_lock_bus_ch, dw_dmac_channel_config
 * SOURCE
 */
enum  dw_dmac_lock_level dw_dmac_getLockLevel(
    struct dw_device *dev, enum dw_dmac_channel_number ch_num,
    enum dw_dmac_lock_bus_ch bus_ch);
/*****/

/****f* dmac.functions/dw_dmac_setLockEnable
 * DESCRIPTION
 *  This function enables/disables the lock feature on the specified
 *  bus and/or channel on the specified DMA channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  bus_ch      -- Enumerated channel or bus lock select
 *  state       -- Enumerated Enable/Disable state
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if the any of the specified DMA channels are out of
 *                 range (not present)
 *  -DW_EPERM   -- if hardware parameter DMAH_CHx_LOCK_EN == 0
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CFGx.LOCK_B    (when Dmac_lock_bus is specified)
 *    - CFGx.LOCK_CH   (when Dmac_lock_channel is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_getLockEnable(), dw_dmac_setLockLevel(),
 *  dw_dmac_getLockLevel(),  dw_dmac_setChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_lock_bus_ch,
 *  dw_state, dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setLockEnable(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_lock_bus_ch bus_ch, enum dw_state state);
/*****/

/****f* dmac.functions/dw_dmac_getLockEnable
 * DESCRIPTION
 *  This function returns the enabled or disabled the lock status 
 *  on the specified bus or channel on the specified DMA channel.
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  bus_ch      -- Enumerated channel or bus lock select
 * RETURN VALUE
 *  Enumerated Enabled/Disabled state
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CFGx.LOCK_B    (when Dmac_lock_bus is specified)
 *    - CFGx.LOCK_CH   (when Dmac_lock_channel is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_getChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setLockEnable(), dw_dmac_setLockLevel(),
 *  dw_dmac_getLockLevel(),  dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_lock_bus_ch,
 *  dw_state, dw_dmac_channel_config
 * SOURCE
 */
enum dw_state dw_dmac_getLockEnable(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_lock_bus_ch bus_ch);
/*****/

/****f* dmac.functions/dw_dmac_setHandshakingMode
 * DESCRIPTION
 *  This function sets the handshaking mode from hardware to software
 *  on the specified source and/or destination on the specified DMA
 *  channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument. Both source and destination
 *  can be specified for the dw_dmac_src_dst_select argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated source/destination select
 *  hs_hwsw_sel -- Enumerated software/hardware handshaking select
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CFGx.HS_SEL_SRC   (when Dmac_src is specified)
 *    - CFGx.HS_SEL_DST   (when Dmac_dst is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_getHandshakingMode(),  dw_dmac_setChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_src_dst_select,
 *  dw_dmac_sw_hw_hs_select, dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setHandshakingMode(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_src_dst_select sd_sel,
    enum dw_dmac_sw_hw_hs_select hs_hwsw_sel);
/*****/

/****f* dmac.functions/dw_dmac_getHandshakingMode
 * DESCRIPTION
 *  This function returns the handshaking mode hardware or software
 *  on the specified source or destination on the specified DMA
 *  channel.
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument. Only 1, source or destination,
 *  can be specified for the dw_dmac_src_dst_select argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated source/destination select
 * RETURN VALUE
 *  Enumerated software/hardware handshaking select
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CFGx.HS_SEL_SRC   (when Dmac_src is specified)
 *    - CFGx.HS_SEL_DST   (when Dmac_dst is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_getChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setHandshakingMode(),  dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_src_dst_select,
 *  dw_dmac_sw_hw_hs_select, dw_dmac_channel_config
 * SOURCE
 */
enum dw_dmac_sw_hw_hs_select dw_dmac_getHandshakingMode(
    struct dw_device *dev, enum dw_dmac_channel_number ch_num,
    enum dw_dmac_src_dst_select sd_sel);
/*****/

/****f* dmac.functions/dw_dmac_setChannelPriority
 * DESCRIPTION
 *  This function sets the priority level on the specified DMA
 *  channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  ch_priority -- Enumerated priority level
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 *  -DW_EINVAL  -- if the specified priority level is greater than or
 *                 equal to the hardware parameter DMAH_NUM_CHANNELS
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CFGx.CH_PRIOR
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_getChannelPriority(),  dw_dmac_setChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_channel_priority,
 *  dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setChannelPriority(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_channel_priority ch_priority);
/*****/

/****f* dmac.functions/dw_dmac_getChannelPriority
 * DESCRIPTION
 *  This function returns the priority level on the specified DMA
 *  channel.
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 * RETURN VALUE
 *  Enumerated channel priority level
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CFGx.CH_PRIOR
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_getChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setChannelPriority(),  dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_channel_priority,
 *  dw_dmac_channel_config
 * SOURCE
 */
enum dw_dmac_channel_priority dw_dmac_getChannelPriority(
    struct dw_device *dev, enum dw_dmac_channel_number ch_num);
/*****/

/****f* dmac.functions/dw_dmac_setListMstSelect
 * DESCRIPTION
 *  This function sets the list master select interface on the specified
 *  DMA channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  mst_num     -- Enumerated master interface number
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 *  -DW_EBUSY   -- if any of the specified DMA channels are enabled
 *  -DW_EPERM   -- if hardware parameter DMAH_CHx_LMS != 4
 *  -DW_ENOSYS  -- if the DMA is configured to have no LLP register on
 *                 any of the specified channels
 *  -DW_EINVAL  -- if the specified master number is out of range
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - LLPx.LMS
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_getListMstSelect(),  dw_dmac_setChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_master_number,
 *  dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setListMstSelect(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_master_number mst_num);
/*****/

/****f* dmac.functions/dw_dmac_getListMstSelect
 * DESCRIPTION
 *  This function returns the list master select interface on the
 *  specified DMA channel.
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 * RETURN VALUE
 *  Enumerated master interface number
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - LLPx.LMS
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_getChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setListMstSelect(),  dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_master_number,
 *  dw_dmac_channel_config
 * SOURCE
 */
enum dw_dmac_master_number dw_dmac_getListMstSelect(
    struct dw_device *dev, enum dw_dmac_channel_number ch_num);
/*****/

/****f* dmac.functions/dw_dmac_setListPointerAddress
 * DESCRIPTION
 *  This function sets the address for the first linked list item
 *  in the system memory for the specified DMA channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  address     -- Linked list item address
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 *  -DW_EBUSY   -- if any of the specified DMA channels are enabled
 *  -DW_ENOSYS  -- if the DMA is configured to have no LLP register on
 *                 any of the specified channels
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - LLPx.LOC
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_getListPointerAddress(),  dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setListPointerAddress(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num, uint32_t address);
/*****/

/****f* dmac.functions/dw_dmac_getListPointerAddress
 * DESCRIPTION
 *  This function returns the address for the first linked list item
 *  in the system memory for the specified DMA channel.
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 * RETURN VALUE
 *  Linked list item address
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - LLPx.LOC
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_getChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setListPointerAddress(),  dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_channel_config
 * SOURCE
 */
uint32_t dw_dmac_getListPointerAddress(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num);
/*****/

/****f* dmac.functions/dw_dmac_setLlpEnable
 * DESCRIPTION
 *  This function enables or disables the block chaining on the 
 *  specified source and/or destination on the specified DMA channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument. Both source and destination
 *  can be specified for the dw_dmac_src_dst_select argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated source/destination select
 *  state       -- Enumerated enable/disable state
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 *  -DW_EBUSY   -- if any of the specified DMA channels are enabled
 *  -DW_EPERM   -- if hardware parameterDMAH_CHx_MULTI_BLK_EN == 0
 *                 or if hardware parameter DMAH_CHx_HC_LLP == 1
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CTLx.LLP_SRC_EN  (when Dmac_src is specified)
 *    - CTLx.LLP_DST_EN  (when Dmac_dst is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_getLlpEnable(),  dw_dmac_setTransferType(),
 *  dw_dmac_getChannelConfig(), dw_dmac_channel_number,
 *  dw_dmac_src_dst_select, dw_state, dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setLlpEnable(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_src_dst_select sd_sel,
    enum dw_state state);
/*****/

/****f* dmac.functions/dw_dmac_getLlpEnable
 * DESCRIPTION
 *  This function returns whether block chaining is enabled or disabled
 *  on the specified source or destination on the specified DMA
 *  channel.
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument. Only 1, source or destination,
 *  can be specified for the dw_dmac_src_dst_select argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated source/destination select
 * RETURN VALUE
 *  Enumerated Enabled/Disabled state
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CTLx.LLP_SRC_EN  (when Dmac_src is specified)
 *    - CTLx.LLP_DST_EN  (when Dmac_dst is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_getChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setLlpEnable(),  dw_dmac_getTransferType(),
 *  dw_dmac_getChannelConfig(), dw_dmac_channel_number,
 *  dw_dmac_src_dst_select, dw_state, dw_dmac_channel_config
 * SOURCE
 */
enum dw_state dw_dmac_getLlpEnable(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_src_dst_select sd_sel);
/*****/

/****f* dmac.functions/dw_dmac_setReload
 * DESCRIPTION
 *  This function enables or disables the reload feature on the
 *  specified source and/or destination on the specified DMA channel(s).
 *  Multiple DMA channels can be specified for the ch_num argument. Both
 *  source and destination can be specified for the sd_sel argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated source/destination select
 *  state       -- Enumerated enable/disable state
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 *  -DW_ENOSYS  -- if either the src or dst reload setting is not
 *                 supported by the specified DMA channel (due to
 *                 hardcoding of the CHx_MULTI_BLK_TYPE hardware
 *                 parameter).  This is also returned if multi-block
 *                 transfers are not supported by the channel (when the
 *                 CHx_MULTI_BLK_EN hardware parameter is false).
 * NOTES
 *  This function directly accesses the following DMAC
 *  register(s)/bit field(s): (x = channel number)
 *    - CFGx.RELOAD_SRC  (when Dmac_src or Dmac_src_dst is specified)
 *    - CFGx.RELOAD_DST  (when Dmac_dst or Dmac_src_dst is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_getReload(),  dw_dmac_setTransferType(),
 *  dw_dmac_setChannelConfig(), dw_dmac_channel_number,
 *  dw_dmac_src_dst_select, dw_state, dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setReload(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_src_dst_select sd_sel,
    enum dw_state state);
/*****/

/****f* dmac.functions/dw_dmac_getReload
 * DESCRIPTION
 *  This function returns whether the reload feature is enabled or
 *  disabled on the specified source or destination on the specified
 *  DMA channel.
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument. Only 1, source or destination,
 *  can be specified for the dw_dmac_src_dst_select argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated source/destination select
 * RETURN VALUE
 *  Enumerated Enabled/Disabled state
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - CFGx.RELOAD_SRC  (when Dmac_src is specified)
 *    - CFGx.RELOAD_DST  (when Dmac_dst is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_getChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setReload(),  dw_dmac_getTransferType(),
 *  dw_dmac_getChannelConfig(), dw_dmac_channel_number,
 *  dw_dmac_src_dst_select, dw_state, dw_dmac_channel_config
 * SOURCE
 */
enum dw_state dw_dmac_getReload(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_src_dst_select sd_sel);
/*****/

/****f* dmac.functions/dw_dmac_setStatus
 * DESCRIPTION
 *  This function sets the status registers on the specified source
 *  and/or destination on the specified DMA channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument. Both source and destination
 *  can be specified for the dw_dmac_src_dst_select argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated source/destination select
 *  value       -- 32-bit status value
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 *  -DW_EBUSY   -- if any of the specified DMA channels are enabled
 *  -DW_ENOSYS  -- if hardware parmeter DMAH_CHx_STAT_SRC == 0 and
 *                 the source is specified, or if hardware parameter
 *                 DMAH_CHx_STAT_DST == 0 and the destination is
 *                 specified
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - SSTATx.SSTAT  (when Dmac_src is specified)
 *    - DSTATx.DSTAT  (when Dmac_dst is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_getStatus(), dw_dmac_setStatusAddress(),
 *  dw_dmac_getStatusAddress(), dw_dmac_setChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_src_dst_select,
 *  dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setStatus(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_src_dst_select sd_sel, uint32_t value);
/*****/

/****f* dmac.functions/dw_dmac_getStatus
 * DESCRIPTION
 *  This function returns the status registers on the specified 
 *  source or destination on the specified DMA channel.
 *  Only ONE DMA channel can be specified for the
 *  dw_dmac_channel_number argument. Only ONE, source or destination,
 *  can be specified for the dw_dmac_src_dst_select argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated source/destination select
 * RETURN VALUE
 *  32-bit status value
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - SSTATx.SSTAT  (when Dmac_src is specified)
 *    - DSTATx.DSTAT  (when Dmac_dst is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_getChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setStatus(), dw_dmac_setStatusAddress(),
 *  dw_dmac_getStatusAddress(), dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_src_dst_select,
 *  dw_dmac_channel_config
 * SOURCE
 */
uint32_t dw_dmac_getStatus(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_src_dst_select sd_sel);
/*****/

/****f* dmac.functions/dw_dmac_setStatusAddress
 * DESCRIPTION
 *  This function sets the status address registers on the specified
 *  source and/or destination on the specified DMA channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument. Both source and destination
 *  can be specified for the dw_dmac_src_dst_select argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated source/destination select
 *  address     -- 32-bit address from where status is fetched
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 *  -DW_EBUSY   -- if any of the specified DMA channels are enabled
 *  -DW_ENOSYS  -- if hardware parmeter DMAH_CHx_STAT_SRC == 0 and
 *                 the source is specified, or if hardware parameter
 *                 DMAH_CHx_STAT_DST == 0 and the destination is
 *                 specified
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - SSTATARx.SSTATAR  (when Dmac_src is specified)
 *    - DSTATARx.DSTATAR  (when Dmac_dst is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_getStatusAddress(), dw_dmac_setStatus(),
 *  dw_dmac_getStatus(), dw_dmac_setChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_src_dst_select,
 *  dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setStatusAddress(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_src_dst_select sd_sel, uint32_t address);
/*****/

/****f* dmac.functions/dw_dmac_getStatusAddress
 * DESCRIPTION
 *  This function returns the status address register on the
 *  specified source or destination on the specified DMA channel.
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument. Only 1, source or destination,
 *  can be specified for the dw_dmac_src_dst_select argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated source/destination select
 * RETURN VALUE
 *  32-bit address from where status is fetched
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - SSTATARx.SSTATAR  (when Dmac_src is specified)
 *    - DSTATARx.DSTATAR  (when Dmac_dst is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_getChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setStatusAddress(), dw_dmac_setStatus(),
 *  dw_dmac_getStatus(), dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_src_dst_select,
 *  dw_dmac_channel_config
 * SOURCE
 */
uint32_t dw_dmac_getStatusAddress(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_src_dst_select sd_sel);
/*****/

/****f* dmac.functions/dw_dmac_setGatherParam
 * DESCRIPTION
 *  This function sets the specified gather interval or count
 *  on the specified DMA channel(s).
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  cnt_int     -- Enumerated count/interval select
 *  value       -- Count or interval value
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 *  -DW_EBUSY   -- if any of the specified DMA channels are enabled
 *  -DW_ENOSYS  -- if hardware parameter DMAH_CHx_SRC_GAT_EN == 0
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - SGRx.SGC          (when Dmac_sg_count is specified)
 *    - SGRx.SGI          (when Dmac_sg_interval is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setGatherEn(), dw_dmac_getGatherEn(),
 *  dw_dmac_getGatherParam(), dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_scatter_gather_param,
 *  dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setGatherParam(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_scatter_gather_param cnt_int, uint32_t value);
/*****/

/****f* dmac.functions/dw_dmac_getGatherParam
 * DESCRIPTION
 *  This function returns the specified gather interval or count
 *  on the specified DMA channel. Only 1 DMA channel can be specified
 *  for the dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  cnt_int     -- Enumerated count/interval select
 * RETURN VALUE
 *  Count or interval value
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - SGRx.SGC          (when Dmac_sg_count is specified)
 *    - SGRx.SGI          (when Dmac_sg_interval is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_getChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setGatherEn(), dw_dmac_getGatherEn(),
 *  dw_dmac_setGatherParam(), dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_scatter_gather_param,
 *  dw_dmac_channel_config
 * SOURCE
 */
uint32_t dw_dmac_getGatherParam(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_scatter_gather_param cnt_int);
/*****/

/****f* dmac.functions/dw_dmac_setScatterParam
 * DESCRIPTION
 *  This function sets the specified scatter interval or count on the
 *  specified DMA channel.
 *  Multiple DMA channels can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  cnt_int     -- Enumerated count/interval select
 *  value       -- Count or interval value
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if any of the specified DMA channels are out of
 *                 range (not present)
 *  -DW_EBUSY   -- if any of the specified DMA channels are enabled
 *  -DW_ENOSYS  -- if hardware parameter DMAH_CHx_DST_SCA_EN == 0
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - DSRx.DSC          (when Dmac_sg_count is specified)
 *    - DSRx.DSI          (when Dmac_sg_interval is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setScatterEn(), dw_dmac_getScatterEn(),
 *  dw_dmac_getScatterParam(), dw_dmac_setChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_scatter_gather_param,
 *  dw_dmac_channel_config
 * SOURCE
 */
int dw_dmac_setScatterParam(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_scatter_gather_param cnt_int, uint32_t value);
/*****/

/****f* dmac.functions/dw_dmac_getScatterParam
 * DESCRIPTION
 *  This function returns the specified scatter interval or count
 *  on the specified DMA channel.
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  ch_int      -- Enumerated count/interval select
 * RETURN VALUE
 *  Count or interval value
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - DSRx.DSC          (when Dmac_sg_count is specified)
 *    - DSRx.DSI          (when Dmac_sg_interval is specified)
 *
 *  The operation covered by this function is also performed as part
 *  of the dw_dmac_setChannelConfig() function.
 * SEE ALSO
 *  dw_dmac_setScatterEn(), dw_dmac_getScatterEn(),
 *  dw_dmac_setScatterParam(), dw_dmac_getChannelConfig(),
 *  dw_dmac_channel_number, dw_dmac_scatter_gather_param,
 *  dw_dmac_channel_config
 * SOURCE
 */
uint32_t  dw_dmac_getScatterParam(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_scatter_gather_param cnt_int);
/*****/

/****f* dmac.functions/dw_dmac_getChannelIndex
 * DESCRIPTION
 *  This function returns the channel index from the specified channel
 *  enumerated type.
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  ch_num      -- Enumerated DMA channel number
 * RETURN VALUE
 *  The DMA channel index.
 * SOURCE
 */
uint8_t dw_dmac_getChannelIndex(enum dw_dmac_channel_number ch_num);
/*****/

/****f* dmac.functions/dw_dmac_getNumChannels
 * DESCRIPTION
 *  This function returns the number of channels that the DMA controller
 *  is configured to have. This function returns the value on the
 *  DMAH_NUM_CHANNELS hardware parameter for the specified DMA
 *  controller device.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 * RETURN VALUE
 *  Number of channels that the DMA controller was configured to have.
 * SOURCE
 */
uint8_t dw_dmac_getNumChannels(struct dw_device *dev);
/*****/

/****f* dmac.functions/dw_dmac_getChannelFifoDepth
 * DESCRIPTION
 *  This function returns the FIFO depth of the specified DMA channel
 *  that the DMA controller is configured to have. This function
 *  returns the value on the DMAH_CHx_FIFO_DEPTH hardware
 *  parameter for the specified DMA controller device.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 * RETURN VALUE
 *  Depth of the FIFO for the specified DMA channel
 * SOURCE
 */
int dw_dmac_getChannelFifoDepth(struct dw_device *dev,
       enum dw_dmac_channel_number ch_num);
/*****/

/****f* dmac.functions/dw_dmac_addLliItem
 * DESCRIPTION
 *  This function creates a Linked List Item or appends a current linked
 *  list with a new item. The dw_dmac_channel_config structure handle
 *  contains the values for the dw_dmac_lli_item structure members.
 * ARGUMENTS
 *  lhead       -- Handle to a dw_list_head structure
 *  lli_item    -- Handle to a dw_dmac_lli_item structure
 *  config      -- Handle to a dw_dmac_channel_config structure
 * RETURN VALUE
 *  none
 * NOTES
 *  Before passing in the dw_list_head structure handle, it should be
 *  initialized (for the first call to create the linked list).
 *  The dw_list_head structure is prototyped in the DW_common_list.h
 *  file and the macro to initialize the dw_list_head structure
 *  (DW_INIT_LIST_HEAD) is also defined in the DW_common_list.h file.
 *
 *  To use this function, the user should create and initialize a
 *  pointer to a dw_list_head structure, and then create a pointer to a
 *  DW_dmac_lli_item structure. The pointer to the DW_dmac_lli_item
 *  structure should point to the address in memory where the lli item
 *  is going to reside. Then call this function with these arguments
 *  along with a pointer to a DW_dmac_channelConfig structure to create
 *  or append a linked list item.
 * SEE ALSO
 *  dw_dmac_setChannelConfig(), dw_dmac_getChannelConfig(),
 *  dw_list_head, dw_dmac_lli_item, dw_dmac_channel_config,
 *  DW_INIT_LIST_HEAD()
 * SOURCE
 */
void dw_dmac_addLliItem(struct dw_list_head *lhead,
    struct dw_dmac_lli_item *lli_item,
    struct dw_dmac_channel_config *config);
/*****/

/****f* dmac.functions/dw_dmac_userIrqHandler
 * DESCRIPTION
 *  This function identifies the current highest priority active
 *  interrupt, if any, and forwards it to a user-specified listener
 *  function for processing.  This allows a user absolute control over
 *  how each DMAC interrupt is processed.
 *
 *  None of the other Interrupt API functions can be used with this
 *  interrupt handler.  This is because they are symbiotic withe the
 *  dw_dmac_irqHandler() interrupt handler.  All Command and Status API
 *  functions, however, can be used within the user listener functions.
 *  This is in contrast to dw_dmac_irqHandler(), where
 *  dw_dmac_enableChannel() cannot be used within the listener function.
 * ARGUMENTS
 *  dev         -- DMA controller device handle
 * RETURN VALUE
 *  true        -- an interrupt was processed
 *  false       -- no interrupt was processed
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *   - StatusInt
 *   - StatusBlock
 *   - StatusTfr
 *   - StatusSrcTran
 *   - StatusDstTran
 *   - StatusErr
 *   - ClearBlock
 *   - ClearTfr
 *   - ClearSrcTran
 *   - ClearDstTran
 *   - ClearErr
 * WARNINGS
 *  The user listener function is run in interrupt context and, as such,
 *  care must be taken to ensure that any data shared between it and
 *  normal code is adequately protected from corruption.  Depending on
 *  the target platform, spinlocks, mutexes or semaphores may be used to
 *  achieve this.
 * SEE ALSO
 *  dw_dmac_setListener(), dw_dmac_irqHandler()
 * SOURCE
 */
int dw_dmac_userIrqHandler(struct dw_device *dev);
/*****/

/****f* dmac.functions/dw_dmac_irqHandler
 * DESCRIPTION
 *  This function handles and processes any DMA controller interrupts.
 *  It works in conjuntion with the Interrupt API and user listener
 *  functions to manage interrupt-driven DMA transfers.  Before using
 *  this function, the user must set up a listener function using
 *  dw_dmac_setListener() for the relevant channel(s). When fully using
 *  the Interrupt API, this function should be called whenever a
 *  DW_ahb_dmac interrupt occurs.
 *
 *  There is also an alternate interrupt handler available,
 *  dw_dmac_userIrqHandler(), but this cannot be used in conjunction
 *  with the other Interrupt API functions, apart from
 *  dw_dmac_setListener().
 * ARGUMENTS
 *  dev         -- DMA controller device handle
 * RETURN VALUE
 *  true        -- an interrupt was processed
 *  false       -- no interrupt was processed
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *   - StatusInt
 *   - StatusBlock
 *   - StatusTfr
 *   - StatusSrcTran
 *   - StatusDstTran
 *   - StatusErr
 *   - ClearBlock
 *   - ClearTfr
 *   - ClearSrcTran
 *   - ClearDstTran
 *   - ClearErr
 *   - CFGx.RELOAD_SRC
 *   - CFGx.RELOAD_DST
 * WARNINGS
 *  The user listener function is run in interrupt context and, as such,
 *  care must be taken to ensure that any data shared between it and
 *  normal code is adequately protected from corruption.  Depending on
 *  the target platform, spinlocks, mutexes or semaphores may be used to
 *  achieve this.
 * SEE ALSO
 *  dw_dmac_startTransfer(), dw_dmac_setListener(),
 *  dw_dmac_userIrqHandler()
 * SOURCE
 */
int dw_dmac_irqHandler(struct dw_device *dev);
/*****/

/*****/
int dw_dmac_prepareTransfer(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num, int num_blocks,
    dw_callback cb_func);

/****f* dmac.functions/dw_dmac_startTransfer
 * DESCRIPTION
 *  This function is used to start an interrupt-driven transfer on a
 *  DMA channel.  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument.
 *  
 *  The function enables DMA channel interrupts and stores
 *  information needed by the IRQ Handler to control the transfer.
 *  The DMA channel is also enabled to begin the DMA transfer. The
 *  following channel interrupts are enabled and unmasked by this
 *  function:
 *
 *  - IntTfr - transfer complete interrupt
 *  - IntBlock - block transfer complete interrupt
 *  - IntErr - error response on the AMBA AHB bus
 *
 *  If software handshaking is used on the source and the source
 *  device is a peripheral, the following interrupt is unmasked. If
 *  the transfer set up does not match that described and the user
 *  wants to use this interrupt, the user should unmask the
 *  interrupt using the dw_dmac_unmaskIrq() function prior to calling
 *  this function.
 *  
 *    IntSrcTran - source burst/single tranfer completed
 *
 *  If software handshaking is used on the destination and the
 *  destination device is a peripheral, the following interrupt
 *  is unmasked. If the transfer setup does not match that described
 *  and the user wants to use this interrupt, the user should
 *  unmask the interrupt using the dw_dmac_unmaskIrq() function prior
 *  to calling this function.
 *
 *    IntDstTran - destination burst/single tranfer completed
 *  
 *  All channel interrupts are masked and disabled on completion of
 *  the DMA transfer.
 *
 *  If the number of blocks that make up the DMA transfer is not known,
 *  the user should enter 0 for the num_blocks argument. The user's
 *  listener function is called by the dw_dmac_irqHandler() function
 *  each time a block interrupt occurs. The user can use the
 *  dw_dmac_getBlockCount() API function to fetch the number of blocks
 *  completed by the DMA Controller from within the listener function.
 *  When the total number of blocks is known, the user should call the
 *  dw_dmac_nextBlockIsLast() function also from within the Listener
 *  function.
 *  The listener function has two arguments, the DMAC device handle
 *  and the interrupt type (dw_dmac_irq).
 *  
 *  At the end of the DMA transfer, the dw_dmac_irqHandler() calls
 *  the user's callback function if the user has specified one. The
 *  callback function has two arguments: the DMAC device handle and
 *  the number of blocks transferred by the DMA Controller.
 * ARGUMENTS
 *  dev         -- DMA controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  num_blocks  -- Number of blocks in the DMA transfer
 *  cb_func     -- User callback function (can be NULL) - called by ISR
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ECHRNG  -- if the DMA channel is out of range (not present) or
 *                 if multiple DMA channels are specified.
 *  -DW_EBUSY   -- if the DMA channel is already enabled
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - ChEnReg.CH_EN[x]
 *    - CTLx.INT_EN
 *    - MaskTfr.INT_MASK
 *    - MaskBlock.INT_MASK
 *    - MaskErr.INT_MASK
 *    - MaskSrcTran.INT_MASK - if source is peripheral and setup for
 *                             software handshaking
 *    - MaskDstTran.INT_MASK - if destination is peripheral and setup
 *                             for software handshaking
 *
 *  If enabling a DMA channel for polled transfers, the
 *  dw_dmac_enableChannel() function is better suited as there is no
 *  need to pass the extra information into the driver.
 *
 *  This function is part of the Interrupt API and should not be called
 *  when using the DMAC in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_dmac_irqHandler().
 * SEE ALSO
 *  dw_dmac_enableChannel(), dw_dmac_disableChannel(),
 *  dw_dmac_sourceReady(), dw_dmac_destinationReady(),
 *  dw_dmac_setSingleRegion(), dw_dmac_nextBlockIsLast(),
 *  dw_dmac_irqHandler(), dw_dmac_channel_number,
 *  dw_dmac_transfer_type, dw_callback
 * SOURCE
 */
int dw_dmac_startTransfer(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num, int num_blocks,
    dw_callback cb_func);
int dw_dmac_startCommonTransfer(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num, int num_blocks,
    dw_callback cb_func);

/*****/

/****f* dmac.functions/dw_dmac_sourceReady
 * DESCRIPTION
 *  This function is part of the interrupt-driven interface for the
 *  DMA controller driver. This function writes to the source
 *  software request registers on the DMA controller.
 *
 *  This function is ONLY useful when the source device is a
 *  peripheral (non-memory) AND that source device is interfacing to
 *  the DMA controller via software handshaking. Under all other
 *  transfer conditions, this function should NOT be used.
 *
 *  This function should ideally be called inside an ISR for the
 *  source peripheral device to indicate that it is ready for a
 *  DMA transfer.
 *
 *  As an example, if the DW_apb_ssi device is the source peripheral,
 *  the ISR for the ssi_rxf_intr interrupt (receive FIFO full
 *  interrupt on the DW_apb_ssi) should call this function to request
 *  a DMA transfer.
 *
 *  If the source peripheral is the flow control device in the DMA
 *  transfer, the single and last arguments should be used to relate
 *  transfer information back to the DMA controller. If a single
 *  transfer is required, set the single argument to 'true'. If it is
 *  the last burst/single transfer of the block, set the last argument
 *  to 'true'.
 *
 *  If the source peripheral is not the flow control device, the
 *  single and last arguments are ignored and should be left at 'false'.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_index    -- DMA channel index (0 to DMAC_MAX_CHANNELS)
 *
 *  The following arguments are only useful when the source peripheral
 *  is the flow control device
 *  
 *  single      -- when 'true' requests a single transfer
 *                 when 'false' requests a burst transfer
 *  last        -- when 'true' the next single/burst transfer is the
 *                 last in the current block;
 *                 when 'false' the next single/burst transfer is NOT
 *                 the last in the current block
 * RETURN VALUE
 *  none
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - ReqSrcReg.SRC_REQ[x]
 *    - SglReqSrcReg.SRC_SGLREQ[x]
 *    - LstSrcReg.LSTSRC[x]
 *
 *  This function is part of the Interrupt API and should not be called
 *  when using the DMAC in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_dmac_irqHandler().
 * SEE ALSO
 *  dw_dmac_irqHandler(), dw_dmac_destinationReady(),
 *  dw_dmac_startTransfer(), dw_dmac_setSingleRegion(),
 *  dw_dmac_nextBlockIsLast(), dw_dmac_setSoftwareRequest(),
 *  dw_dmac_getSoftwareRequest(), dw_dmac_channel_number
 * SOURCE
 */
void dw_dmac_sourceReady(struct dw_device *dev, unsigned ch_index,
        bool single, bool last);
/*****/

/****f* dmac.functions/dw_dmac_destinationReady
 * DESCRIPTION
 *  This function is part of the interrupt-driven interface for the
 *  DMA controller driver. This function writes to the destination
 *  software request registers on the DMA controller.
 *
 *  This function is ONLY useful when the destination device is a
 *  peripheral (non-memory) AND that destination device is
 *  interfacing to the DMA controller via software handshaking.
 *  Under all other transfer conditions, this function should NOT be
 *  used.
 *
 *  This function should ideally be called inside an ISR for the
 *  destination peripheral device to indicate that it is ready for a
 *  DMA transfer.
 *
 *  As an example, if the DW_apb_ssi device is the destination
 *  peripheral, the ISR for the ssi_txe_intr interrupt (transmit FIFO
 *  empty interrupt on the DW_apb_ssi) should call this function to
 *  request a DMA transfer.
 *
 *  If the destination peripheral is the flow control device in the
 *  DMA transfer, the single and last arguments should be used to
 *  relate transfer information back to the DMA controller. If a
 *  single transfer is required, set the single argument to 'true'. If
 *  it is the last burst/single transfer of the block, set the last
 *  argument to 'true'.
 *
 *  If the destination peripheral is not the flow control device, the
 *  single and last arguments are ignored and should be left at 'false'.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_index    -- DMA channel index (0 to DMAC_MAX_CHANNELS)
 *
 *  The following argument are only useful when the destination
 *  peripheral is the flow control device:
 *  
 *  single      -- when 'true' requests a single transfer
 *                 when 'false' requests a burst transfer
 *  last        -- when 'true' the next single/burst transfer is the
 *                 last in the current block;
 *                 when 'false' the next single/burst transfer is NOT
 *                 the last in the current block
 * RETURN VALUE
 *  none
 * NOTES
 *  This function directly accesses the following DMAC's
 *  register(s)/bit field(s): (x = channel number)
 *    - ReqDstReg.DST_REQ[x]
 *    - SglReqDstReg.DST_SGLREQ[x]
 *    - LstDstReg.LSTDST[x]
 *
 *  This function is part of the Interrupt API and should not be called
 *  when using the DMAC in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_dmac_irqHandler().
 * SEE ALSO
 *  dw_dmac_irqHandler(), dw_dmac_sourceReady(),
 *  dw_dmac_startTransfer(), dw_dmac_setSingleRegion(),
 *  dw_dmac_nextBlockIsLast(), dw_dmac_setSoftwareRequest(),
 *  dw_dmac_getSoftwareRequest(), dw_dmac_channel_number
 * SOURCE
 */
void dw_dmac_destinationReady(struct dw_device *dev, unsigned ch_index,
        bool single, bool last);
/*****/

/****f* dmac.functions/dw_dmac_setSingleRegion
 * DESCRIPTION
 *  This function is part of the interrupt-driven interface for the
 *  DMA controller driver. The function is used to instruct the
 *  driver that subsequent transfers in a block are to be completed
 *  using single transfers.
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument.
 *  
 *  This function is ONLY useful when either the source or
 *  destination device is a peripheral (non-memory) AND that source or
 *  destination device is interfacing to the DMA controller via
 *  software handshaking. Under all other transfer conditions this
 *  function should NOT be used.
 *  
 *  This function is only be needed if the source or destination
 *  transfer can enter a single transaction region. (See the 
 *  DW_ahb_dmac databook for descrption single transaction region).
 *
 *  If the source or destination enters a single transaction region,
 *  the user has the choice of completing the block transaction using
 *  a single transfer (in which case this function should be called) 
 *  or completing the block transaction using a burst transfer, and
 *  allowing the DMA controller to early terminate.
 *  Care should be taken here to set the threshold levels in the
 *  peripheral device to match the requested transfer (single/burst).
 * NOTES
 *  This function is part of the Interrupt API and should not be called
 *  when using the DMAC in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_dmac_irqHandler().
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated DMA channel number
 * RETURN VALUE
 *  none
 * SEE ALSO
 *  dw_dmac_irqHandler(), dw_dmac_sourceReady(),
 *  dw_dmac_destinationReady(), dw_dmac_startTransfer(),
 *  dw_dmac_nextBlockIsLast(), dw_dmac_channel_number,
 *  dw_dmac_src_dst_select
 * SOURCE
 */
void dw_dmac_setSingleRegion(struct dw_device *dev,
     enum dw_dmac_channel_number ch_num,
     enum dw_dmac_src_dst_select sd_sel);
/*****/

/****f* dmac.functions/dw_dmac_nextBlockIsLast
 * DESCRIPTION
 *  This function is part of the interrupt-driven interface in
 *  the DMAC driver. This function is only needed for the
 *  special case where the number of blocks that make up the DMAC
 *  transfer is not known when the transfer is initiated. If this
 *  is the case, the user can monitor the block count in the listener
 *  function and call this function when the last block of the DMAC
 *  transfer is known. Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument.
 * NOTES
 *  This function is part of the Interrupt API and should not be called
 *  when using the DMAC in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_dmac_irqHandler().
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 * RETURN VALUE
 *  none
 * SEE ALSO
 *  dw_dmac_irqHandler(), dw_dmac_sourceReady(),
 *  dw_dmac_destinationReady(), dw_dmac_startTransfer(),
 *  dw_dmac_setSingleRegion(), dw_dmac_channel_number
 * SOURCE
 */
void dw_dmac_nextBlockIsLast(struct dw_device *dev,
     enum dw_dmac_channel_number ch_num);
/*****/

/****f* dmac.functions/dw_dmac_setListener
 * DESCRIPTION
 *  This function is used to set up a listener function for the
 *  interrupt handler of the DMAC driver. The listener function is
 *  responsible for handling all interrupts that are not handled
 *  by the Driver Kit interrupt handler.  A listener function need to be
 *  setup for each channel that is being used.
 *
 *  A listener must be setup up before using any of the other functions
 *  of the Interrupt API.  Note that if the dw_dmac_userIrqHandler()
 *  interrupt handler is being used, none of the other Interrupt API
 *  functions can be used with it.  This is because they are symbiotic
 *  with the dw_dmac_irqHandler() interrupt handler.  The priority in
 *  which interrupt are passed to the listener function is listed below:
 *
 *   - Dmac_irq_err
 *   - Dmac_irq_tfr
 *   - Dmac_irq_block
 *   - Dmac_irq_srctran
 *   - Dmac_irq_dsttran
 *  
 *  Only 1 DMA channel can be specified for the ch_num argument.  There
 *  is no need to clear any interrupts in the listener as this is
 *  handled automatically by the Driver Kit interrupt handlers.  Note
 *  that when using the dw_dmac_irqHandler() interrupt handler, the
 *  Dmac_irq_tfr interrupt is never passed to the listener function.
 *  Instead, an optional user-provided callback function is called.
 * ARGUMENTS
 *  dev             - DMA Controller device handle
 *  ch_num          - Enumerated DMA channel number
 *  userFunction    - function pointer to user listener function
 * RETURN VALUE
 *  none
 * NOTE
 *  Calling this function does NOT enable or unmask any of the channel
 *  interrupts in the DMAC device.
 *
 *  Whether the dw_dmac_userIrqHandler() or dw_dmac_irqHandler()
 *  interrupt handler is being used, this function is used to set the
 *  user listener function(s) that are called by both of them.
 * SEE ALSO
 *  dw_dmac_irqHandler(), dw_dmac_userIrqHandler(),
 *  dw_dmac_sourceReady(), dw_dmac_destinationReady(),
 *  dw_dmac_startTransfer(), dw_dmac_setSingleRegion(),
 *  dw_dmac_channel_number
 * SOURCE
 */
void dw_dmac_setListener(struct dw_device *dev,
     enum dw_dmac_channel_number ch_num, dw_callback userFunction);
/*****/

/****f* dmac.functions/dw_dmac_getBlockCount
 * DESCRIPTION
 *  This function returns the number of blocks that a DMA
 *  channel has completed transferring. This function should only be
 *  used for interrupt driven transfers.
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 * RETURN VALUE
 *  Number of blocks completed by the DMA controller
 * SEE ALSO
 *  dw_dmac_getBlockByteCount(), dw_dmac_channel_number
 * SOURCE
 */
int dw_dmac_getBlockCount(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num);
/*****/

/****f* dmac.functions/dw_dmac_getBlockByteCount
 * DESCRIPTION
 *  This function returns the number of bytes within a DMA block
 *  that have been transferred by the DMA controller on the specified
 *  source or destination. This function should only be used for
 *  interrupt driven transfers, where the SRCTRAN or DSTTRAN
 *  interrupts are enabled and unmasked.
 *  Only 1 DMA channel can be specified for the
 *  dw_dmac_channel_number argument. Only 1, source or destination,
 *  can be specified for the dw_dmac_src_dst_select argument.
 * ARGUMENTS
 *  dev         -- DMA Controller device handle
 *  ch_num      -- Enumerated DMA channel number
 *  sd_sel      -- Enumerated source/destination select
 * RETURN VALUE
 *  Number of bytes completed on source or destination
 * SEE ALSO
 *  dw_dmac_getBlockCount(), dw_dmac_channel_number,
 *  dw_dmac_src_dst_select
 * SOURCE
 */
int dw_dmac_getBlockByteCount(struct dw_device *dev,
    enum dw_dmac_channel_number ch_num,
    enum dw_dmac_src_dst_select sd_sel);
/*****/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* DW_AHB_DMAC_PUBLIC_H */

