/* --------------------------------------------------------------------
** 
** Synopsys DesignWare DW_apb_i2c Software Driver Kit and
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

#ifndef DW_APB_I2C_PUBLIC_H
#define DW_APB_I2C_PUBLIC_H

#ifdef __cplusplus
extern "C" {    // allow C++ to use these headers
#endif

/****h* drivers.i2c/i2c.api
 * NAME
 *  DW_apb_i2c API overview
 * DESCRIPTION
 *  This section gives an overview of the DW_apb_i2c software driver
 *  Application Programming Interface (API).
 * SEE ALSO
 *  i2c.data, i2c.functions
 ***/

/****h* drivers.i2c/i2c.data
 * NAME
 *  DW_apb_i2c driver data types and definitions
 * DESCRIPTION
 *  This section details all the public data types and definitions used
 *  with the DW_apb_i2c software driver.
 * SEE ALSO
 *  i2c.api, i2c.functions
 ***/

/****h* drivers.i2c/i2c.functions
 * NAME
 *  DW_apb_i2c driver data types and definitions
 * DESCRIPTION
 *  This section details all the public functions available for use with
 *  the DW_apb_i2c software driver.
 * SEE ALSO
 *  i2c.api, i2c.data
 ***/

/****h* i2c.api/i2c.data_types
 * NAME
 *  DW_apb_i2c driver data types and definitions
 * DESCRIPTION
 *  enum dw_i2c_address_mode    -- 7-bit or 10-bit addressing
 *  enum dw_i2c_irq             -- I2C interrupts
 *  enum dw_i2c_scl_phase       -- scl phase (low or high)
 *  enum dw_i2c_speed_mode      -- standard-, fast- or high-speed mode
 *  enum dw_i2c_tx_abort        -- reasons for transmit aborts
 *  enum dw_i2c_tx_mode         -- use target address, start byte
 *                                 protocol or general call
 * SEE ALSO
 *  i2c.configuration, i2c.command, i2c.status, i2c.interrupt,
 ***/

/****h* i2c.api/i2c.configuration
 * NAME
 *  DW_apb_i2c driver configuration functions
 * DESCRIPTION
 *  dw_i2c_enableRestart        -- enable restart conditions
 *  dw_i2c_disableRestart       -- disable restart conditions
 *  dw_i2c_setSpeedMode         -- set speed mode
 *  dw_i2c_setMasterAddressMode -- set master address mode
 *  dw_i2c_setSlaveAddressMode  -- set slave address mode
 *  dw_i2c_setTargetAddress     -- set target slave address
 *  dw_i2c_setSlaveAddress      -- set slave device address
 *  dw_i2c_setTxMode            -- set transmit transfer mode
 *  dw_i2c_setMasterCode        -- set high speed master code
 *  dw_i2c_setSclCount          -- set clock counts
 *  dw_i2c_setNotifier_destinationReady -- set DMA Tx notifier
 *  dw_i2c_setNotifier_sourceReady      -- set DMA Rx notifier
 *  dw_i2c_unmaskIrq            -- unmask interrupt(s)
 *  dw_i2c_maskIrq              -- mask interrupt(s)
 *  dw_i2c_setTxThreshold       -- set transmit FIFO threshold
 *  dw_i2c_setRxThreshold       -- set receive FIFO threshold
 *  dw_i2c_setDmaTxMode         -- set DMA transmit mode
 *  dw_i2c_setDmaRxMode         -- set DMA receive mode
 *  dw_i2c_setDmaTxLevel        -- set DMA transmit data level threshold
 *  dw_i2c_setDmaRxLevel        -- set DMA receive data level threshold
 * SEE ALSO
 *  i2c.data_types, i2c.command, i2c.status, i2c.interrupt
 ***/

/****h* i2c.api/i2c.command
 * NAME
 *  DW_apb_i2c driver command functions
 * DESCRIPTION
 *  dw_i2c_init             -- initialize device driver
 *  dw_i2c_enable           -- enable I2C
 *  dw_i2c_disable          -- disable I2C
 *  dw_i2c_enableMaster     -- enable I2C master
 *  dw_i2c_disableMaster    -- disable I2C master
 *  dw_i2c_enableSlave      -- enable I2C slave
 *  dw_i2c_disableSlave     -- disable I2C slave
 *  dw_i2c_read             -- read byte from the receive FIFO
 *  dw_i2c_write            -- write byte to the transmit FIFO
 *  dw_i2c_issueRead        -- write a read command to the transmit FIFO
 *  dw_i2c_clearIrq         -- clear interrupt(s)
 * SEE ALSO
 *  i2c.data_types, i2c.configuration, i2c.status, i2c.interrupt
 ***/

/****h* i2c.api/i2c.status
 * NAME
 *  DW_apb_i2c driver status functions
 * DESCRIPTION
 *  dw_i2c_isEnabled            -- is device enabled?
 *  dw_i2c_isBusy               -- is device busy?
 *  dw_i2c_isMasterEnabled      -- is master enabled?
 *  dw_i2c_isSlaveEnabled       -- is slave enabled?
 *  dw_i2c_isRestartEnabled     -- are restart conditions enabled?
 *  dw_i2c_isTxFifoFull         -- is transmit FIFO full?
 *  dw_i2c_isTxFifoEmpty        -- is transmit FIFO empty?
 *  dw_i2c_isRxFifoFull         -- is receive FIFO full?
 *  dw_i2c_isRxFifoEmpty        -- is receive FIFO empty?
 *  dw_i2c_isIrqMasked          -- is interrupt masked?
 *  dw_i2c_isIrqActive          -- is interrupt status active?
 *  dw_i2c_isRawIrqActive       -- is raw interrupt status active?
 *  dw_i2c_getIrqMask           -- get the interrupt mask
 *  dw_i2c_getDmaTxMode         -- get DMA transmit mode
 *  dw_i2c_getDmaRxMode         -- get DMA receive mode
 *  dw_i2c_getDmaTxLevel        -- get DMA transmit data level threshold
 *  dw_i2c_getDmaRxLevel        -- get DMA receive data level threshold
 *  dw_i2c_getSpeedMode         -- get speed mode
 *  dw_i2c_getTxMode            -- get transmit transfer mode
 *  dw_i2c_getMasterAddressMode -- get master address mode
 *  dw_i2c_getSlaveAddressMode  -- get slave address mode
 *  dw_i2c_getTargetAddress     -- get target slave address
 *  dw_i2c_getSlaveAddress      -- get slave device address
 *  dw_i2c_getSclCount          -- get clock counts
 *  dw_i2c_getTxThreshold       -- get transmit FIFO threshold
 *  dw_i2c_getRxThreshold       -- get receive FIFO threshold
 *  dw_i2c_getTxFifoLevel       -- number of valid entries in transmit
 *                                 FIFO
 *  dw_i2c_getRxFifoLevel       -- number of valid entries in receive
 *                                 FIFO
 *  dw_i2c_getTxAbortSource     -- get reason for last transmit abort
 *  dw_i2c_getTxFifoDepth       -- get transmit FIFO depth
 *  dw_i2c_getRxFifoDepth       -- get receive FIFO depth
 *  dw_i2c_getMasterCode        -- get high speed master code
 * SEE ALSO
 *  i2c.data_types, i2c.configuration, i2c.command, i2c.interrupt
 ***/

/****h* i2c.api/i2c.interrupt
 * NAME
 *  DW_apb_i2c driver interrupt interface functions
 * DESCRIPTION
 *  dw_i2c_setListener          -- set the user listener function
 *  dw_i2c_masterBack2Back      -- master back-to-back transfer
 *  dw_i2c_masterTransmit       -- master-transmitter transfer
 *  dw_i2c_masterReceive        -- master-receiver transfer
 *  dw_i2c_slaveTransmit        -- slave-transmitter transfer
 *  dw_i2c_slaveBulkTransmit    -- slave bulk transmit transfer
 *  dw_i2c_slaveReceive         -- slave-receiver transfer
 *  dw_i2c_terminate            -- terminate current transfer(s)
 *  dw_i2c_irqHandler           -- I2C interrupt handler
 *  dw_i2c_userIrqHandler       -- minimal I2C interrupt handler
 * SEE ALSO
 *  i2c.data_types, i2c.configuration, i2c.command, i2c.status
 ***/

/****d* i2c.data/dw_i2c_irq
 * DESCRIPTION
 *  This is the data type used for specifying I2C interrupts.  One of
 *  these is passed at a time to the user listener function which should
 *  deal with it accordingly.  The exceptions to this, which are handled
 *  by the driver, are: I2c_irq_tx_empty, I2c_irq_rx_done, and
 *  I2c_irq_all.
 * NOTES
 *  This data type relates to the following register bit field(s):
 *   - ic_intr_mask/all bits
 *   - ic_intr_stat/all bits
 *   - ic_raw_intr_stat/all bits
 *   - ic_clr_intr/clr_intr
 *   - ic_clr_rx_under/clr_rx_under
 *   - ic_clr_rx_over/clr_rx_over
 *   - ic_clr_tx_over/clr_tx_over
 *   - ic_clr_rd_req/clr_rd_req
 *   - ic_clr_tx_abrt/clr_tx_abrt
 *   - ic_clr_rx_done/clr_rx_done
 *   - ic_clr_activity/clr_activity
 *   - ic_clr_stop_det/clr_stop_det
 *   - ic_clr_start_det/clr_start_det
 *   - ic_clr_gen_call/clr_gen_call
 * SEE ALSO
 *  dw_i2c_unmaskIrq(), dw_i2c_maskIrq(), dw_i2c_clearIrq(),
 *  dw_i2c_isIrqMasked(), dw_i2c_setListener()
 * SOURCE
 */
enum dw_i2c_irq {
    I2c_irq_none = 0x000,       // Specifies no interrupt
    I2c_irq_rx_under = 0x001,   // Set if the processor attempts to read
                                // the receive FIFO when it is empty.
    I2c_irq_rx_over = 0x002,    // Set if the receive FIFO was
                                // completely filled and more data
                                // arrived.  That data is lost.
    I2c_irq_rx_full = 0x004,    // Set when the transmit FIFO reaches or
                                // goes above the receive FIFO
                                // threshold. It is automatically
                                // cleared by hardware when the receive
                                // FIFO level goes below the threshold.
    I2c_irq_tx_over = 0x008,    // Set during transmit if the transmit
                                // FIFO is filled and the processor
                                // attempts to issue another I2C command
                                // (read request or write).
    I2c_irq_tx_empty = 0x010,   // Set when the transmit FIFO is at or
                                // below the transmit FIFO threshold
                                // level. It is automatically cleared by
                                // hardware when the transmit FIFO level
                                // goes above the threshold.
    I2c_irq_rd_req = 0x020,     // Set when the I2C is acting as a slave
                                // and another I2C master is attempting
                                // to read data from the slave.
    I2c_irq_tx_abrt = 0x040,    // In general, this is set when the I2C
                                // acting as a master is unable to
                                // complete a command that the processor
                                // has sent.
    I2c_irq_rx_done = 0x080,    // When the I2C is acting as a
                                // slave-transmitter, this is set if the
                                // master does not acknowledge a
                                // transmitted byte. This occurs on the
                                // last byte of the transmission,
                                // indicating that the transmission is
                                // done.
    I2c_irq_activity = 0x100,   // This is set whenever the I2C is busy
                                // (reading from or writing to the I2C
                                // bus).
    I2c_irq_stop_det = 0x200,   // Indicates whether a stop condition
                                // has occurred on the I2C bus.
    I2c_irq_start_det = 0x400,  // Indicates whether a start condition
                                // has occurred on the I2C bus.
    I2c_irq_gen_call = 0x800,   // Indicates that a general call request
                                // was received. The I2C stores the
                                // received data in the receive FIFO.
    I2c_irq_all = 0xfff         // Specifies all I2C interrupts.  This
                                // combined enumeration that can be
                                // used with some functions such as
                                // dw_i2c_clearIrq(), dw_i2c_maskIrq(),
                                // and so on.
};

/*****/

/****d* i2c.data/dw_i2c_tx_abort
 * DESCRIPTION
 *  This is the data type used for reporting one or more transmit
 *  transfer aborts.  The value returned by dw_i2c_getTxAbortSource()
 *  should be compared to these enumerations to determine what caused
 *  the last transfer to abort.
 * NOTES
 *  This data type relates to the following register bit field(s):
 *   - ic_tx_abrt_source/all bits
 *
 *  If I2c_abrt_slv_arblost is true, I2c_abrt_arb_lost is also
 *  true.  The way to distinguish between slave and master arbitration
 *  loss is to first check I2c_abrt_slv_arblost (slave arbitration loss)
 *  and then check I2c_abrt_arb_lost (master or slave arbitration loss).
 * SEE ALSO
 *  dw_i2c_getTxAbortSource()
 * SOURCE
 */
enum dw_i2c_tx_abort {
    // Master in 7-bit address mode and the address sent was not
    // acknowledged by any slave.
    I2c_abrt_7b_addr_noack = 0x0001,
    // Master in 10-bit address mode and the first address byte of the
    // 10-bit address was not acknowledged by the slave.
    I2c_abrt_10addr1_noack = 0x0002,
    // Master in 10-bit address mode and the second address byte of the
    // 10-bit address was not acknowledged by the slave.
    I2c_abrt_10addr2_noack = 0x0004,
    // Master has received an acknowledgement for the address, but when
    // it sent data byte(s) following the address, it did not receive
    // and acknowledge from the remote slave(s).
    I2c_abrt_txdata_noack = 0x0008,
    // Master sent a general call address and no slave on the bus
    // responded with an ack.
    I2c_abrt_gcall_noack = 0x0010,
    // Master sent a general call but the user tried to issue a read
    // following this call.
    I2c_abrt_gcall_read = 0x0020,
    // Master is in high-speed mode and the high speed master code was
    // acknowledged (wrong behavior).
    I2c_abrt_hs_ackdet = 0x0040,
    // Master sent a start byte and the start byte was acknowledged
    // (wrong behavior).
    I2c_abrt_sbyte_ackdet = 0x0080,
    // The restart is disabled and the user is trying to use the master
    // to send data in high speed mode.
    I2c_abrt_hs_norstrt = 0x0100,
    // The restart is disabled and the user is trying to send a start
    // byte.
    I2c_abrt_sbyte_norstrt = 0x0200,
    // The restart is disabled and the master sends a read command in
    // the 10-bit addressing mode.
    I2c_abrt_10b_rd_norstrt = 0x0400,
    // User attempted to use disabled master.
    I2c_abrt_master_dis = 0x0800,
    // Arbitration lost.
    I2c_abrt_arb_lost = 0x1000,
    // Slave has received a read command and some data exists in the
    // transmit FIFO so that the slave issues a TX_ABRT to flush old
    // data in the transmit FIFO.
    I2c_abrt_slvflush_txfifo = 0x2000,
    // Slave lost bus while it is transmitting data to a remote master.
    I2c_abrt_slv_arblost = 0x5000,
    // Slave requests data to transfer and the user issues a read.
    I2c_abrt_slvrd_intx = 0x8000
};
/*****
 * Note that I2c_abrt_slv_arblost represents two bits.  This is because
 * of the hardware implementaion of the transmit abort status register.
 *****/

/****d* i2c.data/dw_i2c_address_mode
 * DESCRIPTION
 *  This is the data type used for specifying the addressing mode used
 *  for transfers.  An I2C master begins all transfer with the
 *  specified addressing mode.  An I2C slave only responds to
 *  transfers of the same type as its addressing mode.
 * NOTES
 *  This data type relates to the following register bit field(s):
 *   - ic_con/ic_10bitaddr_slave
 *   - ic_con/ic_10bitaddr_master
 * SEE ALSO
 *  dw_i2c_setMasterAddressMode(), dw_i2c_getMasterAddressMode(),
 *  dw_i2c_setSlaveAddressMode(), dw_i2c_getSlaveAddressMode()
 * SOURCE
 */
enum dw_i2c_address_mode {
    I2c_7bit_address = 0x0,     // 7-bit address mode.  Only the 7 LSBs
                                // of the slave and/or target address
                                // are relevant.
    I2c_10bit_address = 0x1     // 10-bit address mode.  The 10 LSBs of
                                // the slave and/or target address are
                                // relevant.
};
/*****/

/****d* i2c.data/dw_i2c_speed_mode
 * DESCRIPTION
 *  This is the data type used for setting and getting the speed mode.
 *  It is also used when specifying the scl count values.
 * NOTES
 *  This data type relates to the following register bit field(s):
 *   - ic_con/speed
 *   - ic_ss_scl_lcnt/ic_ss_scl_lcnt
 *   - ic_ss_scl_hcnt/ic_ss_scl_hcnt
 *   - ic_fs_scl_lcnt/ic_fs_scl_lcnt
 *   - ic_fs_scl_hcnt/ic_fs_scl_hcnt
 *   - ic_hs_scl_lcnt/ic_hs_scl_lcnt
 *   - ic_hs_scl_hcnt/ic_hs_scl_hcnt
 * SEE ALSO
 *  dw_i2c_setSpeedMode(), dw_i2c_getSpeedMode(), dw_i2c_setSclCount(),
 *  dw_i2c_getSclCount()
 * SOURCE
 */
enum dw_i2c_speed_mode {
    I2c_speed_standard = 0x1,   // standard speed (100 kbps)
    I2c_speed_fast = 0x2,       // fast speed (400 kbps)
    I2c_speed_high = 0x3        // high speed (3400 kbps)
};
/*****/

/****d* i2c.data/dw_i2c_tx_mode
 * DESCRIPTION
 *  This is the data type used for specifying what type of the transfer
 *  is initiated upon the next write to the transmit FIFO.  There are
 *  three possible types of transfers that may be initiated by an I2C
 *  master:
 *  
 *  - Start condition followed by the programmed target address.
 *  - Start byte protocol. This is identical to the start condition
 *    except that a start byte is issued before the target address.
 *  - General call. Addresses every slave attached to the I2C bus.
 * NOTES
 *  This data type relates to the following register bit field(s):
 *   - ic_tar/gc_or_start
 *   - ic_tar/special
 * SEE ALSO
 *  dw_i2c_setTxMode(), dw_i2c_getTxMode(), dw_i2c_setTargetAddress()
 * SOURCE
 */
enum dw_i2c_tx_mode {
    I2c_tx_target = 0x0,        // normal transfer using target address
    I2c_tx_gen_call = 0x2,      // issue a general call
    I2c_tx_start_byte = 0x3     // issue a start byte I2C command
};
/*****/

/****d* i2c.data/dw_i2c_scl_phase
 * DESCRIPTION
 *  This is the data type used for specifying whether the high or low
 *  count of the scl clock for whatever speed is being read/modified.
 * NOTES
 *  This data type relates to the following register bit field(s):
 *   - ic_ss_scl_lcnt/ic_ss_scl_lcnt
 *   - ic_ss_scl_hcnt/ic_ss_scl_hcnt
 *   - ic_fs_scl_lcnt/ic_fs_scl_lcnt
 *   - ic_fs_scl_hcnt/ic_fs_scl_hcnt
 *   - ic_hs_scl_lcnt/ic_hs_scl_lcnt
 *   - ic_hs_scl_hcnt/ic_hs_scl_hcnt
 * SEE ALSO
 *  dw_i2c_setSclCount(), dw_i2c_getSclCount()
 * SOURCE
 */
enum dw_i2c_scl_phase {
    I2c_scl_low = 0x0,          // SCL clock count low phase
    I2c_scl_high = 0x1          // SCL clock count high phase
};
/*****/

/****f* i2c.functions/dw_i2c_init
 * DESCRIPTION
 *  This function initializes the I2C driver.  It disables and
 *  clears all interrupts, sets the DMA mode to software handshaking,
 *  sets the DMA transmit and receive notifier function pointers to NULL
 *  and disables the I2C.  It also attempts to determine the hardware
 *  parameters of the device, if supported by the device.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- if hardware parameters for the device could not be
 *                 automatically determined
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_intr_mask/all bits
 *   - ic_clr_intr/clr_intr
 *   - ic_rx_tl/rx_tl
 *   - ic_enable/enable
 * 
 *  This function is affected by the ADD_ENCODED_PARAMS hardware
 *  parameter.  If set to false, it is necessary for the user to create
 *  an appropriate dw_i2c_param structure as part of the dw_device
 *  structure.  If set to true, dw_i2c_init() will automatically
 *  initialize this structure (space for which must have been already
 *  allocated).
 * SOURCE
 */
int dw_i2c_init(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_enable
 * DESCRIPTION
 *  This function enables the I2C.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  none
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_enable/enable
 * SEE ALSO
 *  dw_i2c_disable(), dw_i2c_isEnabled()
 * SOURCE
 */
void dw_i2c_enable(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_disable
 * DESCRIPTION
 *  This functions disables the I2C, if it is not busy (determined by
 *  the activity interrupt bit).  The I2C should not be disabled during
 *  interrupt-driven transfers as the resulting driver behavior is
 *  undefined.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EBUSY   -- if the I2C is busy
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_enable/enable
 * SEE ALSO
 *  dw_i2c_enable(), dw_i2c_isEnabled()
 * SOURCE
 */
int dw_i2c_disable(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_isEnabled
 * DESCRIPTION
 *  This function returns whether the I2C is enabled or not.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  true        -- the I2C is enabled
 *  false       -- the I2C is disabled
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_enable/enable
 * SEE ALSO
 *  dw_i2c_enable(), dw_i2c_disable()
 * SOURCE
 */
bool dw_i2c_isEnabled(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_isBusy
 * DESCRIPTION
 *  This function returns whether the I2C is busy (transmitting
 *  or receiving) or not.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  true        -- the I2C device is busy
 *  false       -- the I2C device is not busy
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_status/activity
 * SOURCE
 */
bool dw_i2c_isBusy(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_setSpeedMode
 * DESCRIPTION
 *  This function sets the speed mode used for I2C transfers.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  mode        -- the speed mode to set
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EPERM   -- if the I2C is enabled
 *  -DW_ENOSYS  -- if the specified speed is not supported
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_con/speed
 *
 *  The I2C must be disabled in order to change the speed mode.
 *  This function is affected by the MAX_SPEED_MODE hardware parameter.
 *  It is not possible to set the speed higher than the maximum speed
 *  mode.
 * SEE ALSO
 *  dw_i2c_getSpeedMode(), enum dw_i2c_speed_mode
 * SOURCE
 */
int dw_i2c_setSpeedMode(struct dw_device *dev, enum dw_i2c_speed_mode
        mode);
/*****/

/****f* i2c.functions/dw_i2c_getSpeedMode
 * DESCRIPTION
 *  This function returns the speed mode currently in use by the I2C.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  The current I2C speed mode.
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_con/speed
 * SEE ALSO
 *  dw_i2c_setSpeedMode(), enum dw_i2c_speed_mode
 * SOURCE
 */
enum dw_i2c_speed_mode dw_i2c_getSpeedMode(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_setMasterAddressMode
 * DESCRIPTION
 *  This function sets the master addressing mode (7-bit or 10-bit).
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  mode        -- the addressing mode to set
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EPERM   -- if the I2C is enabled
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_con/ic_10bitaddr_master
 * 
 *  The I2C must be disabled in order to change the master addressing
 *  mode.
 * SEE ALSO
 *  dw_i2c_getMasterAddressMode(), enum dw_i2c_address_mode
 * SOURCE
 */
int dw_i2c_setMasterAddressMode(struct dw_device *dev, enum
        dw_i2c_address_mode mode);
/*****/

/****f* i2c.functions/dw_i2c_getMasterAddressMode
 * DESCRIPTION
 *  This function returns the current master addressing mode (7-bit or
 *  10-bit).
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  The current master addresing mode.
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_con/ic_10bitaddr_master
 * SEE ALSO
 *  dw_i2c_setSlaveAddressMode(), enum dw_i2c_address_mode
 * SOURCE
 */
enum dw_i2c_address_mode dw_i2c_getMasterAddressMode(struct dw_device
        *dev);
/*****/

/****f* i2c.functions/dw_i2c_setSlaveAddressMode
 * DESCRIPTION
 *  This function sets the I2C slave addressing mode (7-bit or 10-bit).
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  mode        -- the addressing mode to set
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EPERM   -- if the I2C is enabled
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_con/ic_10bitaddr_slave
 * 
 *  The I2C must be disabled in order to change the slave addressing
 *  mode.
 * SEE ALSO
 *  dw_i2c_getSlaveAddressMode(), enum dw_i2c_address_mode
 * SOURCE
 */
int dw_i2c_setSlaveAddressMode(struct dw_device *dev, enum
        dw_i2c_address_mode mode);
/*****/

/****f* i2c.functions/dw_i2c_getSlaveAddressMode
 * DESCRIPTION
 *  This function returns the current slave addressing mode (7-bit or
 *  10-bit).
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  The current slave addressing mode.
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_con/ic_10bitaddr_slave
 * SEE ALSO
 *  dw_i2c_setSlaveAddressMode(), enum dw_i2c_address_mode
 * SOURCE
 */
enum dw_i2c_address_mode dw_i2c_getSlaveAddressMode(struct dw_device
        *dev);
/*****/

/****f* i2c.functions/dw_i2c_enableSlave
 * DESCRIPTION
 *  This function enables the I2C slave.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EPERM   -- if the I2C is enabled
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_con/ic_slave_disable
 * 
 *  The I2C must be disabled in order to enable the slave.
 * SEE ALSO
 *  dw_i2c_disableSlave(), dw_i2c_isSlaveEnabled()
 * SOURCE
 */
int dw_i2c_enableSlave(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_disableSlave
 * DESCRIPTION
 *  This function disables the I2C slave.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EPERM   -- if the I2C is enabled
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_con/ic_slave_disable
 * 
 *  The I2C must be disabled in order to disable the slave.
 * SEE ALSO
 *  dw_i2c_enableSlave(), dw_i2c_isSlaveEnabled()
 * SOURCE
 */
int dw_i2c_disableSlave(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_isSlaveEnabled
 * DESCRIPTION
 *  This function returns whether the I2C slave is enabled or not.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  true        -- slave is enabled
 *  false       -- slave is disabled
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_con/ic_slave_disable
 * SEE ALSO
 *  dw_i2c_enableSlave(), dw_i2c_disableSlave()
 * SOURCE
 */
bool dw_i2c_isSlaveEnabled(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_enableMaster
 * DESCRIPTION
 *  This function enables the I2C master.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EPERM   -- if the I2C is enabled
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_con/master_mode
 * 
 *  The I2C must be disabled in order to enable the master.
 * SEE ALSO
 *  dw_i2c_disableMaster(), dw_i2c_isMasterEnabled()
 * SOURCE
 */
int dw_i2c_enableMaster(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_disableMaster
 * DESCRIPTION
 *  This function disables the I2C master.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EPERM   -- if the I2C is enabled
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_con/master_mode
 * 
 *  The I2C must be disabled in order to disable the master.
 * SEE ALSO
 *  dw_i2c_enableMaster(), dw_i2c_isMasterEnabled()
 * SOURCE
 */
int dw_i2c_disableMaster(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_isMasterEnabled
 * DESCRIPTION
 *  This function returns whether the I2C master is enabled or not.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  true        -- master is enabled
 *  false       -- master is disabled
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_con/master_mode
 * SEE ALSO
 *  dw_i2c_enableMaster(), dw_i2c_disableMaster()
 * SOURCE
 */
bool dw_i2c_isMasterEnabled(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_enableRestart
 * DESCRIPTION
 *  This function enables the use of restart conditions.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EPERM   -- if the I2C is enabled
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_con/ic_restart_en
 *
 *  The I2C must be disabled in order to enable restart conditions.
 * SEE ALSO
 *  dw_i2c_disableRestart()
 * SOURCE
 */
int dw_i2c_enableRestart(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_disableRestart
 * DESCRIPTION
 *  This function disables the use of restart conditions.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EPERM   -- if the I2C is enabled
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_con/ic_restart_en
 *
 *  The I2C must be disabled in order to disable restart conditions.
 * SEE ALSO
 *  dw_i2c_enableRestart()
 * SOURCE
 */
int dw_i2c_disableRestart(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_isRestartEnabled
 * DESCRIPTION
 *  This function returns whether restart conditions are currently in
 *  use or not by the I2C.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  true        -- restart conditions are enabled
 *  false       -- restart conditions are disabled
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_con/ic_restart_en
 * SEE ALSO
 *  dw_i2c_enableRestart(), dw_i2c_disableRestart()
 * SOURCE
 */
bool dw_i2c_isRestartEnabled(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_setTargetAddress
 * DESCRIPTION
 *  This function sets the target address used by the I2C master.  When
 *  not issuing a general call or using a start byte, this is the
 *  address the master uses when performing transfers over the I2C bus.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  address     -- target address to set
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EPERM   -- if the I2C is enabled
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_tar/ic_tar
 * 
 *  The I2C must be disabled in order to set the target address.  Only
 *  the 10 least significant bits of the address are relevant.
 * SEE ALSO
 *  dw_i2c_getTargetAddress()
 * SOURCE
 */
int dw_i2c_setTargetAddress(struct dw_device *dev, uint16_t address);
/*****/

/****f* i2c.functions/dw_i2c_getTargetAddress
 * DESCRIPTION
 *  This function returns the current target address in use by the I2C
 *  master.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  The current target address.
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_tar/ic_tar
 * SEE ALSO
 *  dw_i2c_setTargetAddress()
 * SOURCE
 */
uint16_t dw_i2c_getTargetAddress(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_setSlaveAddress
 * DESCRIPTION
 *  This function sets the slave address to which the I2C slave
 *  responds, when enabled.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  address     -- slave address to set
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EPERM   -- if the I2C is enabled
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_sar/ic_sar
 * 
 *  The I2C must be disabled in order to set the target address.  Only
 *  the 10 least significant bits of the address are relevant.
 * SEE ALSO
 *  dw_i2c_getSlaveAddress()
 * SOURCE
 */
int dw_i2c_setSlaveAddress(struct dw_device *dev, uint16_t address);
/*****/

/****f* i2c.functions/dw_i2c_getSlaveAddress
 * DESCRIPTION
 *  This function returns the current address in use by the I2C slave.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  The current I2C slave address.
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_sar/ic_sar
 * SEE ALSO
 *  dw_i2c_setSlaveAddress()
 * SOURCE
 */
uint16_t dw_i2c_getSlaveAddress(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_setTxMode
 * DESCRIPTION
 *  This function sets the master transmit mode.  That is, whether to
 *  use a start byte, general call, or the programmed target address.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  mode        -- transfer mode to set
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EPERM   -- if the I2C is enabled
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_tar/special
 *   - ic_tar/gc_or_start
 *
 *  The I2C must be disabled in order to set the master transmit mode.
 * SEE ALSO
 *  dw_i2c_getTxMode(), enum dw_i2c_tx_mode
 * SOURCE
 */
int dw_i2c_setTxMode(struct dw_device *dev, enum dw_i2c_tx_mode mode);
/*****/

/****f* i2c.functions/dw_i2c_getTxMode
 * DESCRIPTION
 *  This function returns the current transmit mode in use by an I2C
 *  master.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  The current master transmit mode.
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_tar/special
 *   - ic_tar/gc_or_start
 * SEE ALSO
 *  dw_i2c_setTxMode(), enum dw_i2c_tx_mode
 * SOURCE
 */
enum dw_i2c_tx_mode dw_i2c_getTxMode(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_setMasterCode
 * DESCRIPTION
 *  This function sets the master code, used during high-speed mode
 *  transfers.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  code        -- master code to set
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EPERM   -- if the I2C is enabled
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_hs_maddr/ic_hs_mar
 *
 *  The I2C must be disabled in order to set the high-speed mode master
 *  code.
 * SEE ALSO
 *  dw_i2c_getMasterCode()
 * SOURCE
 */
int dw_i2c_setMasterCode(struct dw_device *dev, uint8_t code);
/*****/

/****f* i2c.functions/dw_i2c_getMasterCode
 * DESCRIPTION
 *  Initializes an I2C peripheral.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  The current high-speed mode master code.
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_hs_maddr/ic_hs_mar
 * SEE ALSO
 *  dw_i2c_setMasterCode()
 * SOURCE
 */
uint8_t dw_i2c_getMasterCode(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_setSclCount
 * DESCRIPTION
 *  This function set the scl count value for a particular speed mode
 *  (standard, fast, high) and clock phase (low, high).
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  mode        -- speed mode of count value to set
 *  phase       -- scl phase of count value to set
 *  value       -- count value to set
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EPERM   -- if the I2C is enabled
 *  -DW_ENOSYS  -- if the scl count registers are hardcoded
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_ss_scl_lcnt/ic_ss_scl_lcnt
 *   - ic_ss_scl_hcnt/ic_ss_scl_hcnt
 *   - ic_fs_scl_lcnt/ic_fs_scl_lcnt
 *   - ic_fs_scl_hcnt/ic_fs_scl_hcnt
 *   - ic_hs_scl_lcnt/ic_hs_scl_lcnt
 *   - ic_hs_scl_hcnt/ic_hs_scl_hcnt
 *
 *  The I2C must be disabled in order to set any of the scl count
 *  values.  The minimum programmable value for any of these registers
 *  is 6.
 *  This function is affected by the HC_COUNT_VALUES hardware parameter.
 * SEE ALSO
 *  dw_i2c_getSclCount(), enum dw_i2c_speed_mode, enum dw_i2c_scl_phase
 * SOURCE
 */
int dw_i2c_setSclCount(struct dw_device *dev, enum dw_i2c_speed_mode
        mode, enum dw_i2c_scl_phase phase, uint16_t value);
/*****/

/****f* i2c.functions/dw_i2c_getSclCount
 * DESCRIPTION
 *  This function returns the current scl count value for all speed
 *  modes (standard, fast, high) and phases (low, high).
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  mode        -- speed mode to get count value of
 *  phase       -- scl phase to get count value of
 * RETURN VALUE
 *  The current specified scl count value.
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_ss_scl_lcnt/ic_ss_scl_lcnt
 *   - ic_ss_scl_hcnt/ic_ss_scl_hcnt
 *   - ic_fs_scl_lcnt/ic_fs_scl_lcnt
 *   - ic_fs_scl_hcnt/ic_fs_scl_hcnt
 *   - ic_hs_scl_lcnt/ic_hs_scl_lcnt
 *   - ic_hs_scl_hcnt/ic_hs_scl_hcnt
 *
 *  This function returns 0x0000 for any non-existent scl count
 *  registers.
 * SEE ALSO
 *  dw_i2c_setSclCount(), enum dw_i2c_speed_mode, enum dw_i2c_scl_phase
 * SOURCE
 */
uint16_t dw_i2c_getSclCount(struct dw_device *dev, enum
        dw_i2c_speed_mode mode, enum dw_i2c_scl_phase phase);
/*****/

/****f* i2c.functions/dw_i2c_read
 * DESCRIPTION
 *  This function reads a single byte from the I2C receive FIFO.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  The character read from the I2C FIFO
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_data_cmd/dat
 *
 *  This function also does not check whether there is valid data or not
 *  in the FIFO beforehand.  As such, it can cause an receive underflow
 *  error if used improperly.
 * SEE ALSO
 *  dw_i2c_write(), dw_i2c_issueRead(), dw_i2c_masterReceive(),
 *  dw_i2c_slaveReceive()
 * SOURCE
 */
uint8_t dw_i2c_read(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_write
 * DESCRIPTION
 *  This function writes a single byte to the I2C transmit FIFO.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  datum       -- byte to write to FIFO
 * RETURN VALUE
 *  none
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_data_cmd/dat
 *
 *  This function does not check whether the I2C transmit FIFO is full
 *  or not beforehand.  As such, it can cause a transmit overflow error
 *  if used improperly.
 * SEE ALSO
 *  dw_i2c_read(), dw_i2c_issueRead(), dw_i2c_masterTransmit(),
 *  dw_i2c_slaveTransmit()
 * SOURCE
 */
void dw_i2c_write(struct dw_device *dev, uint32_t datum);
/*****/

/****f* i2c.functions/dw_i2c_issueRead
 * DESCRIPTION
 *  This function writes a read command to the I2C transmit FIFO.  This
 *  is used during master-receiver/slave-transmitter transfers and is
 *  typically followed by a read from the master receive FIFO after the
 *  slave responds with data.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  none
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_data_cmd/cmd
 * 
 *  This function does not check whether the I2C FIFO is full or not
 *  before writing to it.  As such, it can result in a transmit FIFO
 *  overflow if used improperly.
 * SEE ALSO
 *  dw_i2c_read(), dw_i2c_write(), dw_i2c_masterReceive()
 * SOURCE
 */
void dw_i2c_issueRead(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_getTxAbortSource
 * DESCRIPTION
 *  This function returns the current value of the I2C transmit abort
 *  status register.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  The current transmit abort status.
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_tx_abrt_source/all bits
 * 
 *  The transmit abort status register is cleared by the I2C upon
 *  reading.  Note that it is possible for more than one bit of this
 *  register to be active simultaneously and this should be dealt with
 *  properly by any function operating on this return value.
 * SEE ALSO
 *  enum dw_i2c_tx_abort
 * SOURCE
 */
enum dw_i2c_tx_abort dw_i2c_getTxAbortSource(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_getTxFifoDepth
 * DESCRIPTION
 *  Returns how many bytes deep the  I2C transmit FIFO is.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  FIFO depth in bytes (from 2 to 256)
 * NOTES
 *  This function is affected by the TX_BUFFER_DEPTH hardware parameter.
 * SEE ALSO
 *  dw_i2c_getRxFifoDepth(), dw_i2c_getTxFifoLevel()
 * SOURCE
 */
uint16_t dw_i2c_getTxFifoDepth(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_getRxFifoDepth
 * DESCRIPTION
 *  Returns how many bytes deep the I2C transmit FIFO is.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  FIFO depth in bytes (from 2 to 256)
 * NOTES
 *  This function is affected by the RX_BUFFER_DEPTH hardware parameter.
 * SEE ALSO
 *  dw_i2c_getTxFifoDepth(), dw_i2c_getRxFifoLevel()
 * SOURCE
 */
uint16_t dw_i2c_getRxFifoDepth(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_isTxFifoFull
 * DESCRIPTION
 *  Returns whether the transmitter FIFO is full or not.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  true        -- the transmit FIFO is full
 *  false       -- the transmit FIFO is not full
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_status/tfnf
 * SEE ALSO
 *  dw_i2c_isTxFifoEmpty()
 * SOURCE
 */
bool dw_i2c_isTxFifoFull(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_isTxFifoEmpty
 * DESCRIPTION
 *  Returns whether the transmitter FIFO is empty or not.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  true        -- the transmit FIFO is empty
 *  false       -- the transmit FIFO is not empty
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_status/tfe
 * SEE ALSO
 *  dw_i2c_isTxFifoFull()
 * SOURCE
 */
bool dw_i2c_isTxFifoEmpty(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_isRxFifoFull
 * DESCRIPTION
 *  This function returns whether the receive FIFO is full or not.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  true        -- the receive FIFO is full
 *  false       -- the receive FIFO is not full
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_status/rff
 * SEE ALSO
 *  dw_i2c_isRxFifoEmpty()
 * SOURCE
 */
bool dw_i2c_isRxFifoFull(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_isRxFifoEmpty
 * DESCRIPTION
 *  This function returns whether the receive FIFO is empty or not.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  true        -- the receive FIFO is empty
 *  false       -- the receive FIFO is not empty
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_status/rfne
 * SEE ALSO
 *  dw_i2c_isRxFifoFull()
 * SOURCE
 */
bool dw_i2c_isRxFifoEmpty(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_getTxFifoLevel
 * DESCRIPTION
 *  This function returns the number of valid data entries currently
 *  present in the transmit FIFO.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  Number of valid data entries in the transmit FIFO.
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_txflr/txflr
 * SEE ALSO
 *  dw_i2c_getRxFifoLevel(), dw_i2c_isTxFifoFull(),
 *  dw_i2c_isTxFifoEmpty(), dw_i2c_setTxThreshold(),
 *  dw_i2c_getTxThreshold()
 * SOURCE
 */
uint16_t dw_i2c_getTxFifoLevel(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_getRxFifoLevel
 * DESCRIPTION
 *  This function returns the number of valid data entries currently
 *  present in the receiver FIFO.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  Number of valid data entries in the receive FIFO.
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_rxflr/rxflr
 * SEE ALSO
 *  dw_i2c_getTxFifoLevel(), dw_i2c_isRxFifoFull(),
 *  dw_i2c_isRxFifoEmpty(), dw_i2c_setRxThreshold(),
 *  dw_i2c_getRxThreshold()
 * SOURCE
 */
uint16_t dw_i2c_getRxFifoLevel(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_setTxThreshold
 * DESCRIPTION
 *  This function sets the threshold level for the transmit FIFO.  When
 *  the number of data entries in the transmit FIFO is at or below this
 *  level, the tx_empty interrupt is triggered.  If an interrupt-driven
 *  transfer is already in progress, the transmit threshold level is not
 *  updated until the end of the transfer.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  level       -- level at which to set threshold
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EINVAL  -- if the level specified is greater than the transmit
 *                 FIFO depth; the threshold is set to the transmit FIFO
 *                 depth.
 *  -DW_EBUSY   -- if an interrupt-driven transfer is currently in
 *                 progress; the requested level will be written to the
 *                 transmit threshold register when the current transfer
 *                 completes.
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_tx_tl/tx_tl
 *
 *  The driver keeps a copy of the last transmit threshold specified by
 *  the user as it manipulates the transmit threshold level at the end
 *  of interrupt-driven transmit transfers.  This copy is used to
 *  restore the transmit threshold upon completion of a transmit
 *  transfer.
 * SEE ALSO
 *  dw_i2c_getTxThreshold(), dw_i2c_setRxThreshold(),
 *  dw_i2c_getRxThreshold(), dw_i2c_getTxFifoLevel()
 * SOURCE
 */
int dw_i2c_setTxThreshold(struct dw_device *dev, uint8_t level);
/*****/

/****f* i2c.functions/dw_i2c_getTxThreshold
 * DESCRIPTION
 *  This function returns the current threshold level for the transmit
 *  FIFO.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  The transmit FIFO threshold level.
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_tx_tl/tx_tl
 * SEE ALSO
 *  dw_i2c_setTxThreshold(), dw_i2c_setRxThreshold(),
 *  dw_i2c_getRxThreshold(), dw_i2c_getTxFifoLevel()
 * SOURCE
 */
uint8_t dw_i2c_getTxThreshold(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_setRxThreshold
 * DESCRIPTION
 *  This function sets the threshold level for the receive FIFO.  When
 *  the number of data entries in the receive FIFO is at or above this
 *  level, the rx_full interrupt is triggered.  If an interrupt-driven
 *  transfer is already in progress, the receive threshold level is not
 *  updated until the end of the transfer.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  level       -- level at which to set threshold
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EINVAL  -- if the level specified is greater than the receive
 *                 FIFO depth, the threshold is set to the receive FIFO
 *                 depth.
 *  -DW_EBUSY   -- if an interrupt-driven transfer is currently in
 *                 progress, the requested level is written to the
 *                 receive threshold register when the current transfer
 *                 completes.
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_rx_tl/rx_tl
 *
 *  The driver keeps a copy of the last receive threshold specified by
 *  the user as it manipulates the receive threshold level at the end of
 *  interrupt-driven receive transfers.  This copy is used to restore
 *  the receive threshold upon completion of an receive transfer.
 * WARNINGS
 *  When this function is called, if the following slave receive
 *  transfer is less than the threshold set, the rx_full interrupt is
 *  not immediately triggered.
 * SEE ALSO
 *  dw_i2c_getRxThreshold(), dw_i2c_setTxThreshold(),
 *  dw_i2c_getTxThreshold(), dw_i2c_terminate(), dw_i2c_getRxFifoLevel()
 * SOURCE
 */
int dw_i2c_setRxThreshold(struct dw_device *dev, uint8_t level);
/*****/

/****f* i2c.functions/dw_i2c_getRxThreshold
 * DESCRIPTION
 *  This function returns the current threshold level for the receive
 *  FIFO.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  The receive FIFO threshold level.
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_rx_tl/rx_tl
 * 
 *  It is possible that the returned value is not equal to what the user
 *  has previously set the receive threshold to be.  This is because the
 *  driver manipulates the threshold value in order to complete receive
 *  transfers.  The previous user-specified threshold is restored upon
 *  completion of each transfer.
 * SEE ALSO
 *  dw_i2c_setRxThreshold(), dw_i2c_setTxThreshold(),
 *  dw_i2c_getTxThreshold(), dw_i2c_getRxFifoLevel()
 * SOURCE
 */
uint8_t dw_i2c_getRxThreshold(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_setListener
 * DESCRIPTION
 *  This function is used to set a user listener function.  The listener
 *  function is responsible for handling all interrupts that are not
 *  handled by the Driver Kit interrupt handler.  This encompasses all
 *  error interrupts, general calls, read requests, and receive full
 *  when no receive buffer is available.  There is no need to clear any
 *  interrupts in the listener as this is handled automatically by the
 *  Driver Kit interrupt handlers.
 *
 *  A listener must be setup up before using any of the other functions
 *  of the Interrupt API.  Note that if the dw_i2c_userIrqHandler
 *  interrupt handler is being used, none of the other Interrupt API
 *  functions can be used with it.  This is because they are symbiotic
 *  with the dw_i2c_irqHandler() interrupt handler.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  listener    -- function pointer to user listener function
 * RETURN VALUE
 *  none
 * NOTES
 *  This function enables the following interrupts: I2c_irq_rx_under,
 *  I2c_irq_rx_over, I2c_irq_tx_over, I2c_irq_rd_req, I2c_irq_tx_abrt,
 *  I2c_irq_rx_done, and I2c_irq_gen_call.  It also enables
 *  I2c_irq_rx_full is the DmaRxMode is not set to hardware handshaking.
 *  The dw_callback function pointer typedef is defined in the common
 *  header files.
 *
 *  Whether the dw_i2c_userIrqHandler() or dw_i2c_irqHandler() interrupt
 *  handler is being used, this function is used to set the user
 *  listener function that is called by both of them.
 * EXAMPLE
 *  In the case of new data being received, the irq handler
 *  (dw_i2c_irqHandler) would call the user listener function as
 *  follows:
 *
 *  userListener(dev, I2c_irq_rx_full);
 *
 *  It is the listener function's responsibility to properly handle
 *  this. For example:
 *
 *  dw_i2c_slaveReceive(dev, buffer, length, callback);
 *
 * SEE ALSO
 *  dw_i2c_setDmaRxMode(), dw_i2c_userIrqHanler(), dw_i2c_irqHandler(),
 *  dw_i2c_irq, dw_callback
 * SOURCE
 */
void dw_i2c_setListener(struct dw_device *dev, dw_callback listener);
/*****/

/****f* i2c.functions/dw_i2c_masterBack2Back
 * DESCRIPTION
 *  This function initiates an interrupt-driven master back-to-back
 *  transfer.  To do this, the I2C must first be properly configured,
 *  enabled and a transmit buffer must be setup which contains the
 *  sequential reads and writes to perform.  An associated receive
 *  buffer of suitable size must also be specified when issuing the
 *  transfer.  As data is received, it is written to the receive buffer.
 *  The callback function is called (if it is not NULL) when the final
 *  byte is received and there is no more data to send.
 *
 *  A transfer may be stopped at any time by calling dw_i2c_terminate(),
 *  which returns the number of bytes that are sent before the transfer
 *  is interrupted.  A terminated transfer's callback function is never
 *  called.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  txBuffer    -- buffer from which to send data
 *  txLength    -- length of transmit buffer/number of bytes to send
 *  rxBuffer    -- buffer to write received data to
 *  rxLength    -- length of receive buffer/number of bytes to receive
 *  callback    -- function to call when transfer is complete
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EBUSY   -- if the I2C is busy (transfer already in progress)
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_intr_mask/all bits
 *   - ic_rx_tl/rx_tl
 *
 *  This function enables the tx_empty and tx_abrt interrupts.
 *  The dw_callback function pointer typedef is defined in the common
 *  header files.
 *  The transmit buffer must be 16 bits wide as a read command is 9 bits
 *  long (0x100).  Restart conditions must be enabled in order to
 *  perform back-to-back transfers.
 *
 *  This function is part of the Interrupt API and should not be called
 *  when using the I2C in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_i2c_irqHandler().
 * SEE ALSO
 *  dw_i2c_masterTransmit(), dw_i2c_masterReceive(),
 *  dw_i2c_slaveTransmit(), dw_i2c_slaveBulkTransmit(),
 *  dw_i2c_slaveReceive(), dw_i2c_terminate()
 * SOURCE
 */
int dw_i2c_masterBack2Back(struct dw_device *dev, uint16_t *txBuffer,
        unsigned txLength, uint8_t *rxBuffer, unsigned rxLength,
        dw_callback callback);
/*****/

/****f* i2c.functions/dw_i2c_masterTransmit
 * DESCRIPTION
 *  This function initiates an interrupt-driven master transmit
 *  transfer.  To do this, the I2C must first be properly configured and
 *  enabled.  This function configures a master transmit transfer and
 *  enables the transmit interrupt to keep the transmit FIFO filled.
 *  Upon completion, the callback function is called (if it is not
 *  NULL).
 *
 *  A transfer may be stopped at any time by calling dw_i2c_terminate(),
 *  which returns the number of bytes that are sent before the transfer
 *  is interrupted.  A terminated transfer's callback function is never
 *  called.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  buffer      -- buffer from which to send data
 *  length      -- length of buffer/number of bytes to send
 *  callback    -- function to call when transfer is complete
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EBUSY   -- if the I2C is busy (transfer already in progress)
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_intr_mask/all bits
 * 
 *  This function enables the tx_empty and tx_abrt interrupts.
 *  The dw_callback function pointer typedef is defined in the common
 *  header files.
 *
 *  This function is part of the Interrupt API and should not be called
 *  when using the I2C in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_i2c_irqHandler().
 * SEE ALSO
 *  dw_i2c_masterBack2Back(), dw_i2c_masterReceive(),
 *  dw_i2c_slaveTransmit(), dw_i2c_slaveBulkTransmit(),
 *  dw_i2c_slaveReceive(), dw_i2c_terminate()
 * SOURCE
 */
int dw_i2c_masterTransmit(struct dw_device *dev, uint8_t *buffer,
        unsigned length, dw_callback callback);
/*****/

/****f* i2c.functions/dw_i2c_slaveTransmit
 * DESCRIPTION
 *  This function initiates an interrupt-driven slave transmit transfer.
 *  To do this, the I2C must first be properly configured, enabled and
 *  must also receive a read request (I2c_irq_rd_req) from an I2C
 *  master.  This function fills the transmit FIFO and, if there is more
 *  data to send, sets up and enables the transmit interrupts to keep
 *  the FIFO filled.  Upon completion, the callback function is called
 *  (if it is not NULL).
 *
 *  A transfer may be stopped at any time by calling dw_i2c_terminate(),
 *  which returns the number of bytes that were sent before the transfer
 *  was interrupted.  A terminated transfer's callback function is never
 *  called.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  buffer      -- buffer from which to send data
 *  length      -- length of buffer/number of bytes to send
 *  callback    -- function to call when transfer is complete
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EPROTO  -- if a read request was not received
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_intr_mask/all bits
 * 
 *  This function enables the tx_empty and tx_abrt interrupts.
 *  This function may only be called from the user listener function
 *  after a read request has been received.
 *  The dw_callback function pointer typedef is defined in the common
 *  header files.
 *
 *  This function is part of the Interrupt API and should not be called
 *  when using the I2C in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_i2c_irqHandler().
 * SEE ALSO
 *  dw_i2c_masterBack2Back(), dw_i2c_masterTransmit(),
 *  dw_i2c_slaveBulkTransmit(), dw_i2c_masterReceive(),
 *  dw_i2c_slaveReceive(), dw_i2c_terminate()
 * SOURCE
 */
int dw_i2c_slaveTransmit(struct dw_device *dev, uint8_t *buffer,
        unsigned length, dw_callback callback);
/*****/

/****f* i2c.functions/dw_i2c_slaveBulkTransmit
 * DESCRIPTION
 *  This function initiates an interrupt-driven slave transmit transfer.
 *  To do this, the I2C must first be properly configured, enabled and
 *  must also receive a read request (I2c_irq_rd_req) from an I2C
 *  master.  This function fills the transmit FIFO and, if there is more
 *  data to send, sets up and enables the transmit interrupts to keep
 *  the FIFO filled.  Upon completion, the callback function is called
 *  (if it is not NULL).
 *
 *  A transfer may be stopped at any time by calling dw_i2c_terminate(),
 *  which returns the number of bytes that were sent before the transfer
 *  was interrupted.  A terminated transfer's callback function is never
 *  called.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  buffer      -- buffer from which to send data
 *  length      -- length of buffer/number of bytes to send
 *  callback    -- function to call when transfer is complete
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EPROTO  -- if a read request was not received
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_intr_mask/all bits
 * 
 *  This function enables the tx_empty and tx_abrt interrupts.
 *  This function may only be called from the user listener function
 *  after a read request has been received.
 *  The dw_callback function pointer typedef is defined in the common
 *  header files.
 *
 *  This function is part of the Interrupt API and should not be called
 *  when using the I2C in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_i2c_irqHandler().
 * SEE ALSO
 *  dw_i2c_masterBack2Back(), dw_i2c_masterTransmit(),
 *  dw_i2c_slaveTransmit(), dw_i2c_masterReceive(),
 *  dw_i2c_slaveReceive(), dw_i2c_terminate()
 * SOURCE
 */
int dw_i2c_slaveBulkTransmit(struct dw_device *dev, uint8_t *buffer,
        unsigned length, dw_callback callback);
/*****/

/****f* i2c.functions/dw_i2c_masterReceive
 * DESCRIPTION
 *  This function initiates an interrupt-driven master receive transfer.
 *  To do this, the I2C must first be properly configured and enabled.
 *  This function sets up the transmit FIFO to be loaded with read
 *  commands.  In parallel, this function sets up and enables the
 *  receive interrupt to fill the buffer from the receive FIFO (the same
 *  number of times as writes to the transmit FIFO).  Upon completion,
 *  the callback function is called (if it is not NULL).
 *
 *  A transfer may be stopped at any time by calling dw_i2c_terminate(),
 *  which returns the number of bytes that were received before the
 *  transfer was interrupted.  A terminated transfer's callback function
 *  is never called.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  buffer      -- buffer to write received data to
 *  length      -- length of buffer/max number of bytes to receive
 *  callback    -- function to call when transfer is complete
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EBUSY   -- if the I2C is busy (transfer already in progress)
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_intr_mask/all bits
 *   - ic_rx_tl/rx_tl
 * 
 *  This function enables the tx_empty, tx_abrt & rx_full interrupts.
 *  The dw_callback function pointer typedef is defined in the common
 *  header files.
 *
 *  This function is part of the Interrupt API and should not be called
 *  when using the I2C in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_i2c_irqHandler().
 * SEE ALSO
 *  dw_i2c_masterBack2Back(), dw_i2c_masterTransmit(),
 *  dw_i2c_slaveTransmit(), dw_i2c_slaveBulkTransmit(),
 *  dw_i2c_slaveReceive(), dw_i2c_terminate()
 * SOURCE
 */
int dw_i2c_masterReceive(struct dw_device *dev, uint8_t *buffer,
        unsigned length, dw_callback callback);
/*****/

/****f* i2c.functions/dw_i2c_slaveReceive
 * DESCRIPTION
 *  This function initiates an interrupt-driven slave receive transfer.
 *  To do this, the I2C must first be properly configured and enabled.
 *  This function sets up and enables the receive interrupt to fill the
 *  buffer from the receive FIFO.  Upon completion, the callback
 *  function is called (if it is not NULL).
 *
 *  A transfer may be stopped at any time by calling dw_i2c_terminate(),
 *  which returns the number of bytes that were received before the
 *  transfer was interrupted.  A terminated transfer's callback function
 *  is never called.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  buffer      -- buffer to write received data to
 *  length      -- length of buffer/max number of bytes to receive
 *  callback    -- function to call when transfer is complete
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_EBUSY   -- if the I2C is busy (transfer already in progress)
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_intr_mask/all bits
 *   - ic_rx_tl/rx_tl
 *
 *  This function enables the rx_full interrupt.
 *  The dw_callback function pointer typedef is defined in the common
 *  header files.
 *
 *  This function is part of the Interrupt API and should not be called
 *  when using the I2C in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_i2c_irqHandler().
 * SEE ALSO
 *  dw_i2c_masterBack2Back(), dw_i2c_masterTransmit(),
 *  dw_i2c_masterReceive(), dw_i2c_slaveTransmit(),
 *  dw_i2c_slaveBulkTransmit(), dw_i2c_terminate()
 * SOURCE
 */
int dw_i2c_slaveReceive(struct dw_device *dev, uint8_t *buffer, unsigned
        length, dw_callback callback);
/*****/

/****f* i2c.functions/dw_i2c_terminate
 * DESCRIPTION
 *  This function terminates the current I2C interrupt-driven transfer
 *  in progress, if any.  This function must be called to end an
 *  unfinished interrupt-driven transfer as driver instability would
 *  ensue otherwise.
 *  Any data received after calling this function is treated as a new
 *  transfer by the driver.  Therefore, it would be prudent to wait
 *  until the next detected stop condition when receiving data in order
 *  to avoid a misalignment between the device and driver.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  The number of bytes sent/received during the interrupted transfer,
 *  if any.
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_intr_mask/all bits
 *   - ic_rxflr/rxflr
 *   - ic_data_cmd/dat
 *   - ic_rx_tl/rx_tl
 * 
 *  This function is part of the interrupt-driven interface and should
 *  not be called when using the I2C in a poll-driven manner.
 *  This function disables the tx_empty and enables the rx_full
 *  interrupts.
 *  This function restores the receive FIFO threshold to the previously
 *  user-specified value.
 *
 *  This function is part of the Interrupt API and should not be called
 *  when using the I2C in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_i2c_irqHandler().
 * SEE ALSO
 *  dw_i2c_masterBack2Back(), dw_i2c_masterTransmit(),
 *  dw_i2c_masterReceive(), dw_i2c_slaveTransmit(),
 *  dw_i2c_slaveBulkTransmit(), dw_i2c_slaveReceive()
 * SOURCE
 */
int dw_i2c_terminate(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_unmaskIrq
 * DESCRIPTION
 *  Unmasks specified I2C interrupt(s).
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  interrupts  -- interrupt(s) to enable
 * RETURN VALUE
 *  none
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_intr_mask/all bits
 * SEE ALSO
 *  dw_i2c_maskIrq(), dw_i2c_clearIrq(), dw_i2c_isIrqMasked(),
 *  enum dw_i2c_irq
 * SOURCE
 */
void dw_i2c_unmaskIrq(struct dw_device *dev, enum dw_i2c_irq
        interrupts);
/*****/

/****f* i2c.functions/dw_i2c_maskIrq
 * DESCRIPTION
 *  Masks specified I2C interrupt(s).
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  interrupts  -- interrupt(s) to disable
 * RETURN VALUE
 *  none
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_intr_mask/all bits
 * SEE ALSO
 *  dw_i2c_unmaskIrq(), dw_i2c_clearIrq(), dw_i2c_isIrqMasked(),
 *  enum dw_i2c_irq
 * SOURCE
 */
void dw_i2c_maskIrq(struct dw_device *dev, enum dw_i2c_irq
        interrupts);
/*****/

/****f* i2c.functions/dw_i2c_clearIrq
 * DESCRIPTION
 *  Clears specified I2C interrupt(s).  Only the following interrupts
 *  can be cleared in this fashion: rx_under, rx_over, tx_over, rd_req,
 *  tx_abrt, rx_done, activity, stop_det, start_det, gen_call.  Although
 *  they can be specified, the tx_empty and rd_req interrupts cannot be
 *  cleared using this function.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  interrupts  -- interrupt(s) to clear
 * RETURN VALUE
 *  none
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_clr_intr/clr_intr
 *   - ic_clr_rx_under/clr_rx_under
 *   - ic_clr_rx_over/clr_rx_over
 *   - ic_clr_tx_over/clr_tx_over
 *   - ic_clr_rd_req/clr_rd_req
 *   - ic_clr_tx_abrt/clr_tx_abrt
 *   - ic_clr_rx_done/clr_rx_done
 *   - ic_clr_activity/clr_activity
 *   - ic_clr_stop_det/clr_stop_det
 *   - ic_clr_start_det/clr_start_det
 *   - ic_clr_gen_call/clr_gen_call
 * SEE ALSO
 *  dw_i2c_unmaskIrq(), dw_i2c_maskIrq(), dw_i2c_isIrqMasked(),
 *  enum dw_i2c_irq
 * SOURCE
 */
void dw_i2c_clearIrq(struct dw_device *dev, enum dw_i2c_irq interrupts);
/*****/

/****f* i2c.functions/dw_i2c_isIrqMasked
 * DESCRIPTION
 *  Returns whether the specified I2C interrupt is masked or not.  Only
 *  one interrupt can be specified at a time.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  interrupt   -- interrupt to check
 * RETURN VALUE
 *  true        -- interrupt is enabled
 *  false       -- interrupt is disabled
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_intr_mask/all bits
 * SEE ALSO
 *  dw_i2c_unmaskIrq(), dw_i2c_maskIrq(), dw_i2c_clearIrq(),
 *  dw_i2c_getIrqMask(), enum dw_i2c_irq
 * SOURCE
 */
bool dw_i2c_isIrqMasked(struct dw_device *dev, enum dw_i2c_irq
        interrupt);
/*****/

/****f* i2c.functions/dw_i2c_getIrqMask
 * DESCRIPTION
 *  Returns the current interrupt mask.  For each bitfield, a value of
 *  '0' indicates that an interrupt is masked while a value of '1'
 *  indicates that an interrupt is enabled.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  The DW_apb_i2c interrupt mask.
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_intr_mask/all bits
 * SEE ALSO
 *  dw_i2c_unmaskIrq(), dw_i2c_maskIrq(), dw_i2c_clearIrq(),
 *  dw_i2c_isIrqMasked(), enum dw_i2c_irq
 * SOURCE
 */
uint32_t dw_i2c_getIrqMask(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_isIrqActive
 * DESCRIPTION
 *  Returns whether an I2C interrupt is active or not, after the masking
 *  stage.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  interrupt   -- interrupt to check
 * RETURN VALUE
 *  true        -- irq is active
 *  false       -- irq is inactive
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_intr_stat/all bits
 * SEE ALSO
 *  dw_i2c_isRawIrqActive(), dw_i2c_unmaskIrq(), dw_i2c_maskIrq(),
 *  dw_i2c_isIrqMasked(), dw_i2c_clearIrq(), enum dw_i2c_irq
 * SOURCE
 */
bool dw_i2c_isIrqActive(struct dw_device *dev, enum dw_i2c_irq
        interrupt);
/*****/

/****f* i2c.functions/dw_i2c_isRawIrqActive
 * DESCRIPTION
 *  Returns whether an I2C raw interrupt is active or not, regardless of
 *  masking.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  interrupt   -- interrupt to check
 * RETURN VALUE
 *  true        -- raw irq is active
 *  false       -- raw irq is inactive
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_raw_intr_stat/all bits
 * SEE ALSO
 *  dw_i2c_isIrqActive(), dw_i2c_unmaskIrq(), dw_i2c_maskIrq(),
 *  dw_i2c_isIrqMasked(), dw_i2c_clearIrq(), enum dw_i2c_irq
 * SOURCE
 */
bool dw_i2c_isRawIrqActive(struct dw_device *dev, enum dw_i2c_irq
        interrupt);
/*****/

/****f* i2c.functions/dw_i2c_setDmaTxMode
 * DESCRIPTION
 *  This function is used to set the DMA mode for transmit transfers.
 *  Possible options are none (disabled), software or hardware
 *  handshaking.  For software handshaking, a transmit notifier function
 *  (notifies the DMA that the I2C is ready to accept more data) must
 *  first be set via the dw_i2c_setNotifier_destinationReady() function.
 *  The transmitter empty interrupt is masked for hardware handshaking
 *  and unmasked (and managed) for software handshaking or when the DMA
 *  mode is set to none.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  mode        -- DMA mode to set (none, hw or sw handshaking)
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- if device does not have a DMA interface
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_dma_cr/tdmae
 *
 *  This function is affected by the HAS_DMA hardware parameter.
 *
 *  This function is part of the Interrupt API and should not be called
 *  when using the I2C in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_i2c_irqHandler().
 * SEE ALSO
 *  dw_i2c_getDmaTxMode(), dw_i2c_getDmaTxLevel(),
 *  dw_i2c_setNotifier_destinationReady(), dw_i2c_setDmaRxMode()
 * SOURCE
 */
int dw_i2c_setDmaTxMode(struct dw_device *dev, enum dw_dma_mode mode);
/*****/

/****f* i2c.functions/dw_i2c_getDmaTxMode
 * DESCRIPTION
 *  This function returns the current DMA mode for I2C transmit
 *  transfers.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  The current DMA transmit mode.
 * NOTES
 *  This function is part of the Interrupt API and should not be called
 *  when using the I2C in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_i2c_irqHandler().
 * SEE ALSO
 *  dw_i2c_setDmaTxMode(), dw_i2c_getDmaRxMode()
 * SOURCE
 */
enum dw_dma_mode dw_i2c_getDmaTxMode(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_setDmaRxMode
 * DESCRIPTION
 *  This function is used to set the DMA mode for receive transfers.
 *  Possible options are none (disabled), software or hardware
 *  handshaking.  For software handshaking, a receive notifier function
 *  (notifies the DMA that the I2C is ready to accept more data) must
 *  first be setup via the dw_i2c_setNotifier_sourceReady() function.
 *  The receiver full interrupt is masked for hardware handshaking and
 *  unmasked for software handshaking or when the DMA mode is set to
 *  none.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  mode        -- DMA mode to set (none, hw or sw handshaking)
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- if device does not have a DMA interface
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_dma_cr/tdmae
 *
 *  This function is affected by the HAS_DMA hardware parameter.
 *
 *  This function is part of the Interrupt API and should not be called
 *  when using the I2C in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_i2c_irqHandler().
 * SEE ALSO
 *  dw_i2c_getDmaRxMode(), dw_i2c_getDmaTxLevel(), dw_dma_mode,
 *  dw_i2c_setNotifier_sourceReady(), dw_i2c_setDmaRxMode()
 * SOURCE
 */
int dw_i2c_setDmaRxMode(struct dw_device *dev, enum dw_dma_mode
        mode);
/*****/

/****f* i2c.functions/dw_i2c_getDmaRxMode
 * DESCRIPTION
 *  This function returns the current DMA mode for I2C transmit
 *  transfers.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  The current DMA transmit mode.
 * NOTES
 *  This function is part of the Interrupt API and should not be called
 *  when using the I2C in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_i2c_irqHandler().
 * SEE ALSO
 *  dw_i2c_setDmaTxMode(), dw_i2c_getDmaRxMode()
 * SOURCE
 */
enum dw_dma_mode dw_i2c_getDmaRxMode(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_setDmaTxLevel
 * DESCRIPTION
 *  This function sets the threshold level at which new data is
 *  requested from the DMA.  This is used for DMA hardware handshaking
 *  mode only.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  level       -- DMA request threshold level
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- if device does not have a DMA interface
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_dma_tdlr/dmatdl
 *
 *  This function is affected by the HAS_DMA hardware parameter.
 * SEE ALSO
 *  dw_i2c_getDmaTxLevel(), dw_i2c_setDmaTxMode()
 * SOURCE
 */
int dw_i2c_setDmaTxLevel(struct dw_device *dev, uint8_t level);
/*****/

/****f* i2c.functions/dw_i2c_getDmaTxLevel
 * DESCRIPTION
 *  This functions gets the current DMA transmit data threshold level.
 *  This is the FIFO level at which the DMA is requested to send more
 *  data from the I2C.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  The current DMA transmit data level threshold.
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_dma_tdlr/dmatdl
 * SEE ALSO
 *  dw_i2c_setDmaTxLevel(), dw_i2c_setDmaTxMode()
 * SOURCE
 */
uint8_t dw_i2c_getDmaTxLevel(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_setDmaRxLevel
 * DESCRIPTION
 *  This function sets the threshold level at which the DMA is requested
 *  to receive data from the I2C.  This is used for DMA hardware
 *  handshaking mode only.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  level       -- DMA request threshold level
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- if device does not have a DMA interface
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_dma_rdlr/dmardl
 *
 *  This function is affected by the HAS_DMA hardware parameter.
 * SEE ALSO
 *  dw_i2c_getDmaRxLevel(), dw_i2c_setDmaRxMode()
 * SOURCE
 */
int dw_i2c_setDmaRxLevel(struct dw_device *dev, uint8_t level);
/*****/

/****f* i2c.functions/dw_i2c_getDmaRxLevel
 * DESCRIPTION
 *  This functions gets the current DMA receive data threshold level.
 *  This is the FIFO level at which the DMA is requested to receive from
 *  the I2C.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  The current DMA receive data level threshold.
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_dma_rdlr/dmardl
 * SEE ALSO
 *  dw_i2c_setDmaRxLevel(), dw_i2c_setDmaRxMode()
 * SOURCE
 */
uint8_t dw_i2c_getDmaRxLevel(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_setNotifier_destinationReady
 * DESCRIPTION
 *  This function sets the user DMA transmit notifier function.  This
 *  function is required when the DMA transmit mode is software
 *  handshaking.  The I2C driver calls this function at a predefined
 *  threshold to request the DMA to send more data to the I2C.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  funcptr     -- called to request more data from the DMA
 *  dmac        -- associated DW_ahb_dmac device handle
 *  channel     -- channel number used for the transfer
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- if device does not have a DMA interface
 * NOTES
 *  This function is affected by the HAS_DMA hardware parameter.
 *
 *  This function is part of the Interrupt API and should not be called
 *  when using the I2C in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_i2c_irqHandler().
 * SEE ALSO
 *  dw_i2c_setNotifier_sourceReady(), dw_i2c_setDmaTxMode(),
 *  dw_i2c_setTxThreshold()
 * SOURCE
 */
int dw_i2c_setNotifier_destinationReady(struct dw_device *dev,
        dw_dma_notifier_func funcptr, struct dw_device *dmac, unsigned
        channel);
/*****/

/****f* i2c.functions/dw_i2c_setNotifier_sourceReady
 * DESCRIPTION
 *  This function sets the user DMA receive notifier function.  This
 *  function is required when the DMA receive mode is software
 *  handshaking.  The I2C driver calls this function at a predefined
 *  threshold to inform the DMA that data is ready to be read from the
 *  I2C.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 *  funcptr     -- called to inform the DMA to fetch more data
 *  dmac        -- associated DMA device handle
 *  channel     -- channel number used for the transfer
 * RETURN VALUE
 *  0           -- if successful
 *  -DW_ENOSYS  -- if device does not have a DMA interface
 * NOTES
 *  This function is affected by the HAS_DMA hardware parameter.
 *
 *  This function is part of the Interrupt API and should not be called
 *  when using the I2C in a poll-driven manner.
 *
 *  This function cannot be used when using an interrupt handler other
 *  than dw_i2c_irqHandler().
 * SEE ALSO
 *  dw_i2c_setNotifier_destinationReady(), dw_i2c_setDmaRxMode(),
 *  dw_i2c_setRxThreshold()
 * SOURCE
 */
int dw_i2c_setNotifier_sourceReady(struct dw_device *dev,
        dw_dma_notifier_func funcptr, struct dw_device *dmac, unsigned
        channel);
/*****/

/****f* i2c.functions/dw_i2c_irqHandler
 * DESCRIPTION
 *  This function handles and processes I2C interrupts.  It works in
 *  conjunction with the Interrupt API and a user listener function
 *  to manage interrupt-driven transfers.  When fully using the
 *  Interrupt API, this function should be called whenever a DW_apb_i2c
 *  interrupt occurs.  There is an alternate interrupt handler
 *  available, dw_i2c_userIrqHandler(), but this cannot be used in
 *  conjunction with the other Interrupt API functions.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  true        -- an interrupt was processed
 *  false       -- no interrupt was processed
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_intr_mask/all bits
 *   - ic_intr_stat/all bits
 *   - ic_clr_rx_under/clr_rx_under
 *   - ic_clr_rx_over/clr_rx_over
 *   - ic_clr_tx_over/clr_tx_over
 *   - ic_clr_rd_req/clr_rd_req
 *   - ic_clr_tx_abrt/clr_tx_abrt
 *   - ic_clr_rx_done/clr_rx_done
 *   - ic_clr_activity/clr_activity
 *   - ic_clr_stop_det/clr_stop_det
 *   - ic_clr_start_det/clr_start_det
 *   - ic_clr_gen_call/clr_gen_call
 *   - ic_status/rfne
 *   - ic_rxflr/rxflr
 *   - ic_data_cmd/cmd
 *   - ic_data_cmd/dat
 *   - ic_rx_tl/rx_tl
 *   - ic_txflr/txflr
 *
 *  This function is part of the Interrupt API and should not be called
 *  when using the I2C in a poll-driven manner.
 * WARNINGS
 *  The user listener function is run in interrupt context and, as such,
 *  care must be taken to ensure that any data shared between it and
 *  normal code is adequately protected from corruption.  Depending on
 *  the target platform, spinlocks, mutexes or semaphores may be used to
 *  achieve this.  The other Interrupt API functions disable I2C
 *  interrupts before entering critical sections of code to avoid any
 *  shared data issues.
 * SEE ALSO
 *  dw_i2c_masterTransmit(), dw_i2c_masterReceive(),
 *  dw_i2c_masterBack2Back(),  dw_i2c_slaveTransmit(),
 *  dw_i2c_slaveReceive(), dw_i2c_terminate(),
 *  dw_i2c_slaveBulkTransmit()
 * SOURCE
 */
int dw_i2c_irqHandler(struct dw_device *dev);
/*****/

/****f* i2c.functions/dw_i2c_userIrqHandler
 * DESCRIPTION
 *  This function identifies the current highest priority active
 *  interrupt, if any, and forwards it to a user-provided listener
 *  function for processing.  This allows a user absolute control over
 *  how each I2C interrupt is processed.
 *
 *  None of the other Interrupt API functions can be used with this
 *  interrupt handler.  This is because they are symbiotic with the
 *  dw_i2c_irqHandler() interrupt handler.  All Command and Status API
 *  functions, however, can be used within the user listener function.
 *  This is in contrast to dw_i2c_irqHandler(), where dw_i2c_read(),
 *  dw_i2c_write() and dw_i2c_issueRead() cannot be used within the user
 *  listener function.
 * ARGUMENTS
 *  dev         -- DW_apb_i2c device handle
 * RETURN VALUE
 *  true        -- an interrupt was processed
 *  false       -- no interrupt was processed
 * NOTES
 *  Accesses the following DW_apb_i2c register(s)/bit field(s):
 *   - ic_intr_stat/all bits
 *   - ic_clr_rx_under/clr_rx_under
 *   - ic_clr_rx_over/clr_rx_over
 *   - ic_clr_tx_over/clr_tx_over
 *   - ic_clr_rd_req/clr_rd_req
 *   - ic_clr_tx_abrt/clr_tx_abrt
 *   - ic_clr_rx_done/clr_rx_done
 *   - ic_clr_activity/clr_activity
 *   - ic_clr_stop_det/clr_stop_det
 *   - ic_clr_start_det/clr_start_det
 *   - ic_clr_gen_call/clr_gen_call
 *
 *  This function is part of the Interrupt API and should not be called
 *  when using the I2C in a poll-driven manner.
 * WARNINGS
 *  The user listener function is run in interrupt context and, as such,
 *  care must be taken to ensure that any data shared between it and
 *  normal code is adequately protected from corruption.  Depending on
 *  the target platform, spinlocks, mutexes or semaphores may be used to
 *  achieve this.
 * SEE ALSO
 *  dw_i2c_setListener()
 * SOURCE
 */
int dw_i2c_userIrqHandler(struct dw_device *dev);
/*****/

#ifdef __cplusplus
}
#endif

#endif  // DW_APB_I2C_PUBLIC_H

