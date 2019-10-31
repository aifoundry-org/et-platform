#ifndef DDR_CONFIG_H
#define DDR_CONFIG_H

#include <stdint.h>

static inline void write_reg(uint32_t memshire_id, uint64_t address, uint32_t value)
{
   if (address & 0x100000000)
      *(volatile uint64_t*)(address | ((0xe8 + memshire_id) << 22)) = value;
   else
      *(volatile uint32_t*)(address | (memshire_id << 26)) = value;
}

static inline uint32_t read_reg(uint32_t memshire_id, uint64_t address)
{
   if (address & 0x100000000)
      return (uint32_t)*(volatile uint64_t*)(address | ((0xe8 + memshire_id) << 22));
   else
      return *(volatile uint32_t*)(address | (memshire_id << 26));
}

static void ddr_init(uint32_t memshire_id ){
  // set power-ok and turn off apb reset
  // keep other resets asserted

  uint32_t value_read;

//#################################################################################################################################
//# file: cc_boot_seq.tcl
//#
//# This file is used to initialize the Synopsys Memory Controller IP
//#
//#  ===>>> this version does not include DDR interface training <<<===
//#
//# The general flow is:
//# 1) Turn off the APB reset and set Power OK by writing to the the ddrc_reset_ctl (a memshire/DDRC ESR)
//# 2) Configure the memory controller main blocks by writing to various registers in both controllers
//# 3) Turn off the core and AXI resets by writing to the ddrc_reset_ctl
//# 4) Turn off the PUB reset by writing to the ddrc_reset_ctl
//# 5) Configure the PUB (PHY) by writing to various PUB registers
//# 6) Wait for PHY init to complete by polling a memory controller register
//# 7) finalize initialzation  by writing a few memory controller registers
//#
//#################################################################################################################################
//#
//# Key:
//#
//# write_esr  <register_name> <value> : write an esr in the memshire
//# read_esr  <register_name>          : read an esr in the memshire
//#
//# write_both_ddrc_reg <reg> <value>  : write a value to a memory controller register in both memory controllers
//# write_ddrc_reg <blk> <reg> <value> : write a value to particular memory controller block:
//#                                    : 0 = mem controller 0, 1 = mem controller 1, 2 = PUB (PHY)
//# read_ddrc_reg <blk> <reg>          : read a value to particular memory controller block:
//#                                    : 0 = mem controller 0, 1 = mem controller 1, 2 = PUB (PHY)
//#
//# poll_ddrc_reg <blk> <reg> <value> <mask> <timeout> : continues to issue read_ddrc_reg <blk> <reg> until
//#                                                    :   (returned value & <mask>) == <value>.
//#                                                    : If this match is not acheived after <timeout> tries, a dut_error is flagged
//#
//#################################################################################################################################
//
//#####################################
//# set power-ok and turn off apb reset
//# keep other resets asserted
//###################################
//
#ifdef CONFIG_REAL_PLL
//# behavorial PLL model so no need to init the PLL
//
//// "TCL: Doing PLL register setup."
//
  write_reg( memshire_id, 0x61000000, 0x00000234);// write_ddrc_reg 0 mem_pll_ctl 0x0234
//# Check that what you wrote is there
//poll_ddrc_reg 0  mem_pll_ctl  0x0234 0xffffffff
//
//// "TCL: Program PLL to generate 1067Mhz and turn it on!"
//# write_ddrc_reg routines are all "raw", meaning must manually set agent
  write_reg( memshire_id, 0x61000000, 0x000001f8);// write_ddrc_reg 0 mem_pll_ctl            0x01f8
  write_reg( memshire_id, 0x61000004, 0x00000001);// write_ddrc_reg 0 mem_pll_fcw_prediv     0x0001
  write_reg( memshire_id, 0x61000008, 0x00000028);// write_ddrc_reg 0 mem_pll_fcw_int        0x0028
  write_reg( memshire_id, 0x6100000c, 0x00000000);// write_ddrc_reg 0 mem_pll_fcw_frac       0x0000
  write_reg( memshire_id, 0x61000010, 0x000001b0);// write_ddrc_reg 0 mem_pll_dlf_locked_kp  0x01b0
  write_reg( memshire_id, 0x61000014, 0x00000add);// write_ddrc_reg 0 mem_pll_dlf_locked_ki  0x0add
  write_reg( memshire_id, 0x61000018, 0x00000000);// write_ddrc_reg 0 mem_pll_dlf_locked_kb  0x0000
  write_reg( memshire_id, 0x6100001c, 0x000001c3);// write_ddrc_reg 0 mem_pll_dlf_track_kp   0x01c3
  write_reg( memshire_id, 0x61000020, 0x00000988);// write_ddrc_reg 0 mem_pll_dlf_track_ki   0x0988
  write_reg( memshire_id, 0x61000024, 0x00000000);// write_ddrc_reg 0 mem_pll_dlf_track_kb   0x0000
  write_reg( memshire_id, 0x61000028, 0x000001f0);// write_ddrc_reg 0 mem_pll_lock_count     0x01f0
  write_reg( memshire_id, 0x6100002c, 0x0000003d);// write_ddrc_reg 0 mem_pll_lock_threshold 0x003d
  write_reg( memshire_id, 0x61000030, 0x00000026);// write_ddrc_reg 0 mem_pll_dco_gain       0x0026
  write_reg( memshire_id, 0x61000034, 0x00000000);// write_ddrc_reg 0 mem_pll_dsm_dither     0x0000
  write_reg( memshire_id, 0x61000038, 0x00000004);// write_ddrc_reg 0 mem_pll_postdiv        0x0004
  write_reg( memshire_id, 0x6100003c, 0x00000000);// write_ddrc_reg 0 mem_pll_reserved1      0x0000
  write_reg( memshire_id, 0x61000040, 0x00000000);// write_ddrc_reg 0 mem_pll_reserved2      0x0000
  write_reg( memshire_id, 0x61000044, 0x00000000);// write_ddrc_reg 0 mem_pll_reserved3      0x0000
  write_reg( memshire_id, 0x61000048, 0x00000000);// write_ddrc_reg 0 mem_pll_postdiv_ctl    0x0000
  write_reg( memshire_id, 0x6100004c, 0x00000000);// write_ddrc_reg 0 mem_pll_hidden_13      0x0000
  write_reg( memshire_id, 0x61000050, 0x00000000);// write_ddrc_reg 0 mem_pll_open_loop_code 0x0000
  write_reg( memshire_id, 0x61000054, 0x00000000);// write_ddrc_reg 0 mem_pll_ldo_ctl        0x0000
  write_reg( memshire_id, 0x61000058, 0x00000000);// write_ddrc_reg 0 mem_pll_hidden_16      0x0000
  write_reg( memshire_id, 0x6100005c, 0x00000000);// write_ddrc_reg 0 mem_pll_hidden_17      0x0000
  write_reg( memshire_id, 0x61000060, 0x00000000);// write_ddrc_reg 0 mem_pll_hidden_18      0x0000
  write_reg( memshire_id, 0x61000064, 0x00000000);// write_ddrc_reg 0 mem_pll_hidden_19      0x0000
  write_reg( memshire_id, 0x61000068, 0x00000001);// write_ddrc_reg 0 mem_pll_hidden_1a      0x0001
  write_reg( memshire_id, 0x61000080, 0x00000000);// write_ddrc_reg 0 mem_pll_hidden_20      0x0000
  write_reg( memshire_id, 0x61000084, 0x00000000);// write_ddrc_reg 0 mem_pll_hidden_21      0x0000
  write_reg( memshire_id, 0x61000088, 0x00000000);// write_ddrc_reg 0 mem_pll_hidden_22      0x0000
  write_reg( memshire_id, 0x6100008c, 0x00000000);// write_ddrc_reg 0 mem_pll_hidden_23      0x0000
  write_reg( memshire_id, 0x61000090, 0x00000000);// write_ddrc_reg 0 mem_pll_hidden_24      0x0000
  write_reg( memshire_id, 0x6100009c, 0x0000000c);// write_ddrc_reg 0 mem_pll_hidden_27      0x000c
  write_reg( memshire_id, 0x610000a0, 0x00000000);// write_ddrc_reg 0 mem_pll_hidden_28      0x0000
//
//# strobe the regs:
  write_reg( memshire_id, 0x610000e0, 0x00000001);// write_ddrc_reg 0 mem_pll_reg_update_strobe 0x1
//
//# poll for complete
  do
    value_read = read_reg( memshire_id, 0x610000e4);// poll_ddrc_reg 0  mem_pll_lock_detect_status 0x1 0x1 100 500
  while(!((value_read & 0x1) == 0x1));
//
//// "TCL: Finished PLL register setup."
//
//# HPDPLL version 1.2.1 settings for mode 3:
//#
//#.offsets = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
//#                   0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11,
//#                   0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a,
//#                   0x20, 0x21, 0x22, 0x23, 0x24, 0x27, 0x28, 0x38, 0x39
//#      },
//#      .values = { 0x01f8, 0x0001, 0x0028, 0x0000, 0x01b0, 0x0add, 0x0000, 0x01c3, 0x0988,
//#                  0x0000, 0x01f0, 0x003d, 0x0026, 0x0000, 0x0004, 0x0000, 0x0000, 0x0000,
//#                  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001,
//#                  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x000c, 0x0000, 0x0000, 0x0000
  write_reg( memshire_id, 0x180000200, 0x0000010d);// write_esr ddrc_reset_ctl 0x10d
#else
  write_reg( memshire_id, 0x180000200, 0x0000000d);// write_esr ddrc_reset_ctl 0x0d
#endif
//
//set val [read_esr ddrc_reset_ctl]
//// "ddrc_reset_ctl is [format "%8x" $val]"
//wait_cycles 100;
//
//###################################
//# start to initialze the memory controllers
//###################################
  write_reg( memshire_id, 0x60000304, 0x00000001);// (controller #0) write_both_ddrc_reg DBG1 0x00000001
  write_reg( memshire_id, 0x60001304, 0x00000001);// (controller #1) write_both_ddrc_reg DBG1 0x00000001
  write_reg( memshire_id, 0x60000030, 0x00000001);// (controller #0) write_both_ddrc_reg PWRCTL 0x00000001
  write_reg( memshire_id, 0x60001030, 0x00000001);// (controller #1) write_both_ddrc_reg PWRCTL 0x00000001
//
//# make sure operating mode is Init
  do
    value_read = read_reg( memshire_id, 0x60000004); // poll_ddrc_reg 0 STAT 0x0 0x7 10
  while(!((value_read & 0x7) == 0x0));

  do
    value_read = read_reg( memshire_id, 0x60001004);// poll_ddrc_reg 1 STAT 0x0 0x7 10
  while(!((value_read & 0x7) == 0x0));
//
//###################################
//# configure the memory controllers
//###################################
  write_reg( memshire_id, 0x60000000, 0x80080020);// (controller #0) write_both_ddrc_reg MSTR 0x80080020;            # set device config to x16 parts
  write_reg( memshire_id, 0x60001000, 0x80080020);// (controller #1) write_both_ddrc_reg MSTR 0x80080020;            # set device config to x16 parts
  write_reg( memshire_id, 0x60000010, 0x0000a010);// (controller #0) write_both_ddrc_reg MRCTRL0 0x0000a010
  write_reg( memshire_id, 0x60001010, 0x0000a010);// (controller #1) write_both_ddrc_reg MRCTRL0 0x0000a010
  write_reg( memshire_id, 0x60000014, 0x0001008c);// (controller #0) write_both_ddrc_reg MRCTRL1 0x0001008c
  write_reg( memshire_id, 0x60001014, 0x0001008c);// (controller #1) write_both_ddrc_reg MRCTRL1 0x0001008c
  write_reg( memshire_id, 0x60000020, 0x00000404);// (controller #0) write_both_ddrc_reg DERATEEN 0x00000404
  write_reg( memshire_id, 0x60001020, 0x00000404);// (controller #1) write_both_ddrc_reg DERATEEN 0x00000404
  write_reg( memshire_id, 0x60000024, 0xc0c188dd);// (controller #0) write_both_ddrc_reg DERATEINT 0xc0c188dd
  write_reg( memshire_id, 0x60001024, 0xc0c188dd);// (controller #1) write_both_ddrc_reg DERATEINT 0xc0c188dd
  write_reg( memshire_id, 0x6000002c, 0x00000000);// (controller #0) write_both_ddrc_reg DERATECTL 0x00000000
  write_reg( memshire_id, 0x6000102c, 0x00000000);// (controller #1) write_both_ddrc_reg DERATECTL 0x00000000
  write_reg( memshire_id, 0x60000030, 0x00000000);// (controller #0) write_both_ddrc_reg PWRCTL 0x00000000
  write_reg( memshire_id, 0x60001030, 0x00000000);// (controller #1) write_both_ddrc_reg PWRCTL 0x00000000
  write_reg( memshire_id, 0x60000034, 0x0040d104);// (controller #0) write_both_ddrc_reg PWRTMG 0x0040d104
  write_reg( memshire_id, 0x60001034, 0x0040d104);// (controller #1) write_both_ddrc_reg PWRTMG 0x0040d104
  write_reg( memshire_id, 0x60000038, 0x00af0003);// (controller #0) write_both_ddrc_reg HWLPCTL 0x00af0002
  write_reg( memshire_id, 0x60001038, 0x00af0003);// (controller #1) write_both_ddrc_reg HWLPCTL 0x00af0002
  write_reg( memshire_id, 0x60000050, 0x00210000);// (controller #0) write_both_ddrc_reg RFSHCTL0 0x00210000
  write_reg( memshire_id, 0x60001050, 0x00210000);// (controller #1) write_both_ddrc_reg RFSHCTL0 0x00210000
  write_reg( memshire_id, 0x60000060, 0x00000001);// (controller #0) write_both_ddrc_reg RFSHCTL3 0x00000001;      # dis_auto_refressh = 1
  write_reg( memshire_id, 0x60001060, 0x00000001);// (controller #1) write_both_ddrc_reg RFSHCTL3 0x00000001;      # dis_auto_refressh = 1
  write_reg( memshire_id, 0x60000064, 0x0082008c);// (controller #0) write_both_ddrc_reg RFSHTMG 0x0082008c
  write_reg( memshire_id, 0x60001064, 0x0082008c);// (controller #1) write_both_ddrc_reg RFSHTMG 0x0082008c
  write_reg( memshire_id, 0x60000068, 0x00410000);// (controller #0) write_both_ddrc_reg RFSHTMG1 0x00410000
  write_reg( memshire_id, 0x60001068, 0x00410000);// (controller #1) write_both_ddrc_reg RFSHTMG1 0x00410000
#ifdef CONFIG_ECC
  write_reg( memshire_id, 0x60000070, 0x003f7f14);// (controller #0) write_both_ddrc_reg ECCCFG0 0x003f7f14;       # enable ECC
  write_reg( memshire_id, 0x60001070, 0x003f7f14);// (controller #1) write_both_ddrc_reg ECCCFG0 0x003f7f14;       # enable ECC
#else
  write_reg( memshire_id, 0x60000070, 0x003f7f10);// (controller #0) write_both_ddrc_reg ECCCFG0 0x003f7f10;       # disable ECC
  write_reg( memshire_id, 0x60001070, 0x003f7f10);// (controller #1) write_both_ddrc_reg ECCCFG0 0x003f7f10;       # disable ECC
#endif
  write_reg( memshire_id, 0x60000074, 0x000007b2);// (controller #0) write_both_ddrc_reg ECCCFG1 0x000007b2
  write_reg( memshire_id, 0x60001074, 0x000007b2);// (controller #1) write_both_ddrc_reg ECCCFG1 0x000007b2
  write_reg( memshire_id, 0x6000007c, 0x00000300);// (controller #0) write_both_ddrc_reg ECCCTL 0x00000300
  write_reg( memshire_id, 0x6000107c, 0x00000300);// (controller #1) write_both_ddrc_reg ECCCTL 0x00000300
  write_reg( memshire_id, 0x600000b8, 0x000002b4);// (controller #0) write_both_ddrc_reg ECCPOISONADDR0 0x000002b4
  write_reg( memshire_id, 0x600010b8, 0x000002b4);// (controller #1) write_both_ddrc_reg ECCPOISONADDR0 0x000002b4
  write_reg( memshire_id, 0x600000bc, 0x0001df60);// (controller #0) write_both_ddrc_reg ECCPOISONADDR1 0x0001df60
  write_reg( memshire_id, 0x600010bc, 0x0001df60);// (controller #1) write_both_ddrc_reg ECCPOISONADDR1 0x0001df60
  write_reg( memshire_id, 0x600000c0, 0x00000000);// (controller #0) write_both_ddrc_reg CRCPARCTL0 0x00000000
  write_reg( memshire_id, 0x600010c0, 0x00000000);// (controller #1) write_both_ddrc_reg CRCPARCTL0 0x00000000
  write_reg( memshire_id, 0x600000d0, 0x00020002);// (controller #0) write_both_ddrc_reg INIT0 0x00020002
  write_reg( memshire_id, 0x600010d0, 0x00020002);// (controller #1) write_both_ddrc_reg INIT0 0x00020002
  write_reg( memshire_id, 0x600000d4, 0x00020002);// (controller #0) write_both_ddrc_reg INIT1 0x00020002
  write_reg( memshire_id, 0x600010d4, 0x00020002);// (controller #1) write_both_ddrc_reg INIT1 0x00020002
  write_reg( memshire_id, 0x600000d8, 0x0000f405);// (controller #0) write_both_ddrc_reg INIT2 0x0000f405
  write_reg( memshire_id, 0x600010d8, 0x0000f405);// (controller #1) write_both_ddrc_reg INIT2 0x0000f405
  write_reg( memshire_id, 0x600000dc, 0x0074007f);// (controller #0) write_both_ddrc_reg INIT3 0x0074007f
  write_reg( memshire_id, 0x600010dc, 0x0074007f);// (controller #1) write_both_ddrc_reg INIT3 0x0074007f
  write_reg( memshire_id, 0x600000e0, 0x00330000);// (controller #0) write_both_ddrc_reg INIT4 0x00330000
  write_reg( memshire_id, 0x600010e0, 0x00330000);// (controller #1) write_both_ddrc_reg INIT4 0x00330000
  write_reg( memshire_id, 0x600000e4, 0x0005000c);// (controller #0) write_both_ddrc_reg INIT5 0x0005000c
  write_reg( memshire_id, 0x600010e4, 0x0005000c);// (controller #1) write_both_ddrc_reg INIT5 0x0005000c
  write_reg( memshire_id, 0x600000e8, 0x0000004d);// (controller #0) write_both_ddrc_reg INIT6 0x0000004d
  write_reg( memshire_id, 0x600010e8, 0x0000004d);// (controller #1) write_both_ddrc_reg INIT6 0x0000004d
  write_reg( memshire_id, 0x600000ec, 0x0000004d);// (controller #0) write_both_ddrc_reg INIT7 0x0000004d
  write_reg( memshire_id, 0x600010ec, 0x0000004d);// (controller #1) write_both_ddrc_reg INIT7 0x0000004d
  write_reg( memshire_id, 0x600000f0, 0x00000000);// (controller #0) write_both_ddrc_reg DIMMCTL 0x00000000
  write_reg( memshire_id, 0x600010f0, 0x00000000);// (controller #1) write_both_ddrc_reg DIMMCTL 0x00000000
  write_reg( memshire_id, 0x60000100, 0x2921242d);// (controller #0) write_both_ddrc_reg DRAMTMG0 0x2921242d; # set TRasMax = 'd36 due to 2:1 clock
  write_reg( memshire_id, 0x60001100, 0x2921242d);// (controller #1) write_both_ddrc_reg DRAMTMG0 0x2921242d; # set TRasMax = 'd36 due to 2:1 clock
  write_reg( memshire_id, 0x60000104, 0x00090901);// (controller #0) write_both_ddrc_reg DRAMTMG1 0x00090901
  write_reg( memshire_id, 0x60001104, 0x00090901);// (controller #1) write_both_ddrc_reg DRAMTMG1 0x00090901
  write_reg( memshire_id, 0x60000108, 0x11120a21);// (controller #0) write_both_ddrc_reg DRAMTMG2 0x11120a21
  write_reg( memshire_id, 0x60001108, 0x11120a21);// (controller #1) write_both_ddrc_reg DRAMTMG2 0x11120a21
  write_reg( memshire_id, 0x6000010c, 0x00f0f000);// (controller #0) write_both_ddrc_reg DRAMTMG3 0x00f0f000
  write_reg( memshire_id, 0x6000110c, 0x00f0f000);// (controller #1) write_both_ddrc_reg DRAMTMG3 0x00f0f000
  write_reg( memshire_id, 0x60000110, 0x14040914);// (controller #0) write_both_ddrc_reg DRAMTMG4 0x14040914
  write_reg( memshire_id, 0x60001110, 0x14040914);// (controller #1) write_both_ddrc_reg DRAMTMG4 0x14040914
  write_reg( memshire_id, 0x60000114, 0x02061111);// (controller #0) write_both_ddrc_reg DRAMTMG5 0x02061111
  write_reg( memshire_id, 0x60001114, 0x02061111);// (controller #1) write_both_ddrc_reg DRAMTMG5 0x02061111
  write_reg( memshire_id, 0x60000118, 0x0101000a);// (controller #0) write_both_ddrc_reg DRAMTMG6 0x0101000a
  write_reg( memshire_id, 0x60001118, 0x0101000a);// (controller #1) write_both_ddrc_reg DRAMTMG6 0x0101000a
  write_reg( memshire_id, 0x6000011c, 0x00000602);// (controller #0) write_both_ddrc_reg DRAMTMG7 0x00000602
  write_reg( memshire_id, 0x6000111c, 0x00000602);// (controller #1) write_both_ddrc_reg DRAMTMG7 0x00000602
  write_reg( memshire_id, 0x60000120, 0x00000101);// (controller #0) write_both_ddrc_reg DRAMTMG8 0x00000101
  write_reg( memshire_id, 0x60001120, 0x00000101);// (controller #1) write_both_ddrc_reg DRAMTMG8 0x00000101
  write_reg( memshire_id, 0x60000130, 0x00020000);// (controller #0) write_both_ddrc_reg DRAMTMG12 0x00020000
  write_reg( memshire_id, 0x60001130, 0x00020000);// (controller #1) write_both_ddrc_reg DRAMTMG12 0x00020000
  write_reg( memshire_id, 0x60000134, 0x16100002);// (controller #0) write_both_ddrc_reg DRAMTMG13 0x16100002
  write_reg( memshire_id, 0x60001134, 0x16100002);// (controller #1) write_both_ddrc_reg DRAMTMG13 0x16100002
  write_reg( memshire_id, 0x60000138, 0x00000093);// (controller #0) write_both_ddrc_reg DRAMTMG14 0x00000093
  write_reg( memshire_id, 0x60001138, 0x00000093);// (controller #1) write_both_ddrc_reg DRAMTMG14 0x00000093
  write_reg( memshire_id, 0x60000180, 0xc42f0021);// (controller #0) write_both_ddrc_reg ZQCTL0 0xc42f0021
  write_reg( memshire_id, 0x60001180, 0xc42f0021);// (controller #1) write_both_ddrc_reg ZQCTL0 0xc42f0021
  write_reg( memshire_id, 0x60000184, 0x036a3055);// (controller #0) write_both_ddrc_reg ZQCTL1 0x036a3055
  write_reg( memshire_id, 0x60001184, 0x036a3055);// (controller #1) write_both_ddrc_reg ZQCTL1 0x036a3055
  write_reg( memshire_id, 0x60000188, 0x00000000);// (controller #0) write_both_ddrc_reg ZQCTL2 0x00000000
  write_reg( memshire_id, 0x60001188, 0x00000000);// (controller #1) write_both_ddrc_reg ZQCTL2 0x00000000
  write_reg( memshire_id, 0x60000190, 0x049f821e);// (controller #0) write_both_ddrc_reg DFITMG0 0x049f821e
  write_reg( memshire_id, 0x60001190, 0x049f821e);// (controller #1) write_both_ddrc_reg DFITMG0 0x049f821e
  write_reg( memshire_id, 0x60000194, 0x00090303);// (controller #0) write_both_ddrc_reg DFITMG1 0x00090303
  write_reg( memshire_id, 0x60001194, 0x00090303);// (controller #1) write_both_ddrc_reg DFITMG1 0x00090303
  write_reg( memshire_id, 0x60000198, 0x0321b010);// (controller #0) write_both_ddrc_reg DFILPCFG0 0x0321b010
  write_reg( memshire_id, 0x60001198, 0x0321b010);// (controller #1) write_both_ddrc_reg DFILPCFG0 0x0321b010
  write_reg( memshire_id, 0x600001a0, 0x80400018);// (controller #0) write_both_ddrc_reg DFIUPD0 0x80400018
  write_reg( memshire_id, 0x600011a0, 0x80400018);// (controller #1) write_both_ddrc_reg DFIUPD0 0x80400018
  write_reg( memshire_id, 0x600001a4, 0x003c00b1);// (controller #0) write_both_ddrc_reg DFIUPD1 0x003c00b1
  write_reg( memshire_id, 0x600011a4, 0x003c00b1);// (controller #1) write_both_ddrc_reg DFIUPD1 0x003c00b1
  write_reg( memshire_id, 0x600001a8, 0x80000000);// (controller #0) write_both_ddrc_reg DFIUPD2 0x80000000
  write_reg( memshire_id, 0x600011a8, 0x80000000);// (controller #1) write_both_ddrc_reg DFIUPD2 0x80000000
  write_reg( memshire_id, 0x600001b0, 0x00000081);// (controller #0) write_both_ddrc_reg DFIMISC 0x00000081
  write_reg( memshire_id, 0x600011b0, 0x00000081);// (controller #1) write_both_ddrc_reg DFIMISC 0x00000081
  write_reg( memshire_id, 0x600001b4, 0x00001f1e);// (controller #0) write_both_ddrc_reg DFITMG2 0x00001f1e
  write_reg( memshire_id, 0x600011b4, 0x00001f1e);// (controller #1) write_both_ddrc_reg DFITMG2 0x00001f1e
  write_reg( memshire_id, 0x600001c0, 0x00000001);// (controller #0) write_both_ddrc_reg DBICTL 0x00000001
  write_reg( memshire_id, 0x600011c0, 0x00000001);// (controller #1) write_both_ddrc_reg DBICTL 0x00000001
  write_reg( memshire_id, 0x600001c4, 0x00000000);// (controller #0) write_both_ddrc_reg DFIPHYMSTR 0x00000000
  write_reg( memshire_id, 0x600011c4, 0x00000000);// (controller #1) write_both_ddrc_reg DFIPHYMSTR 0x00000000
  write_reg( memshire_id, 0x60000204, 0x00030303);// (controller #0) write_both_ddrc_reg ADDRMAP1 0x00030303; # 1/2 cache line same bank
  write_reg( memshire_id, 0x60001204, 0x00030303);// (controller #1) write_both_ddrc_reg ADDRMAP1 0x00030303; # 1/2 cache line same bank
  write_reg( memshire_id, 0x60000208, 0x03000000);// (controller #0) write_both_ddrc_reg ADDRMAP2 0x03000000; # 1/2 cache line same bank
  write_reg( memshire_id, 0x60001208, 0x03000000);// (controller #1) write_both_ddrc_reg ADDRMAP2 0x03000000; # 1/2 cache line same bank
  write_reg( memshire_id, 0x60000210, 0x00001f1f);// (controller #0) write_both_ddrc_reg ADDRMAP4 0x00001f1f
  write_reg( memshire_id, 0x60001210, 0x00001f1f);// (controller #1) write_both_ddrc_reg ADDRMAP4 0x00001f1f
#ifdef CONFIG_ECC_RAM4GB
//#write_both_ddrc_reg ADDRMAP3 0x11111103;  # For smallest LPDDR4x part
//#write_both_ddrc_reg ADDRMAP5 0x04040404
//#write_both_ddrc_reg ADDRMAP6 0x0f0f0404;  # For smallest LPDDR4x part
//#write_both_ddrc_reg ADDRMAP7 0x00000f0f;  # For smallest LPDDR4x part
#endif
#ifdef $CONFIG_ECC_RAM32GB
//#write_both_ddrc_reg ADDRMAP3 0x14141403;  # For largest LPDDR4x part
//#write_both_ddrc_reg ADDRMAP5 0x04040404
//#write_both_ddrc_reg ADDRMAP6 0x04040404; # For largest LPDDR4x part
//##write_both_ddrc_reg ADDRMAP7 0x00000f04; # For largest LPDDR4x part
#endif
#ifdef CONFIG_ECC
  write_reg( memshire_id, 0x6000020c, 0x11111103);// (controller #0) write_both_ddrc_reg ADDRMAP3 0x11111103;  # For smallest LPDDR4x part
  write_reg( memshire_id, 0x6000120c, 0x11111103);// (controller #1) write_both_ddrc_reg ADDRMAP3 0x11111103;  # For smallest LPDDR4x part
  write_reg( memshire_id, 0x60000214, 0x04040404);// (controller #0) write_both_ddrc_reg ADDRMAP5 0x04040404
  write_reg( memshire_id, 0x60001214, 0x04040404);// (controller #1) write_both_ddrc_reg ADDRMAP5 0x04040404
  write_reg( memshire_id, 0x60000218, 0x0f0f0404);// (controller #0) write_both_ddrc_reg ADDRMAP6 0x0f0f0404;  # For smallest LPDDR4x part
  write_reg( memshire_id, 0x60001218, 0x0f0f0404);// (controller #1) write_both_ddrc_reg ADDRMAP6 0x0f0f0404;  # For smallest LPDDR4x part
  write_reg( memshire_id, 0x6000021c, 0x00000f0f);// (controller #0) write_both_ddrc_reg ADDRMAP7 0x00000f0f;  # For smallest LPDDR4x part
  write_reg( memshire_id, 0x6000121c, 0x00000f0f);// (controller #1) write_both_ddrc_reg ADDRMAP7 0x00000f0f;  # For smallest LPDDR4x part
#else
  write_reg( memshire_id, 0x6000020c, 0x03030303);// (controller #0) write_both_ddrc_reg ADDRMAP3 0x03030303;  # 1/2 cache line same bank
  write_reg( memshire_id, 0x6000120c, 0x03030303);// (controller #1) write_both_ddrc_reg ADDRMAP3 0x03030303;  # 1/2 cache line same bank
  write_reg( memshire_id, 0x60000214, 0x07070707);// (controller #0) write_both_ddrc_reg ADDRMAP5 0x07070707
  write_reg( memshire_id, 0x60001214, 0x07070707);// (controller #1) write_both_ddrc_reg ADDRMAP5 0x07070707
  write_reg( memshire_id, 0x60000218, 0x07070707);// (controller #0) write_both_ddrc_reg ADDRMAP6 0x07070707;  # assume addressing for largest LPDDR4x part is okay always
  write_reg( memshire_id, 0x60001218, 0x07070707);// (controller #1) write_both_ddrc_reg ADDRMAP6 0x07070707;  # assume addressing for largest LPDDR4x part is okay always
  write_reg( memshire_id, 0x6000021c, 0x00000f07);// (controller #0) write_both_ddrc_reg ADDRMAP7 0x00000f07;  # assume addressing for largest LPDDR4x part is okay always
  write_reg( memshire_id, 0x6000121c, 0x00000f07);// (controller #1) write_both_ddrc_reg ADDRMAP7 0x00000f07;  # assume addressing for largest LPDDR4x part is okay always
#endif
//#write_both_ddrc_reg ADDRMAP8 0x00003f3f;  # unused bg
  write_reg( memshire_id, 0x60000224, 0x07070707);// (controller #0) write_both_ddrc_reg ADDRMAP9 0x07070707;  # unused
  write_reg( memshire_id, 0x60001224, 0x07070707);// (controller #1) write_both_ddrc_reg ADDRMAP9 0x07070707;  # unused
  write_reg( memshire_id, 0x60000228, 0x07070707);// (controller #0) write_both_ddrc_reg ADDRMAP10 0x07070707;  # unused
  write_reg( memshire_id, 0x60001228, 0x07070707);// (controller #1) write_both_ddrc_reg ADDRMAP10 0x07070707;  # unused
  write_reg( memshire_id, 0x6000022c, 0x001f1f07);// (controller #0) write_both_ddrc_reg ADDRMAP11 0x001f1f07;  # unused
  write_reg( memshire_id, 0x6000122c, 0x001f1f07);// (controller #1) write_both_ddrc_reg ADDRMAP11 0x001f1f07;  # unused
  write_reg( memshire_id, 0x60000240, 0x061a0c1c);// (controller #0) write_both_ddrc_reg ODTCFG 0x061a0c1c
  write_reg( memshire_id, 0x60001240, 0x061a0c1c);// (controller #1) write_both_ddrc_reg ODTCFG 0x061a0c1c
  write_reg( memshire_id, 0x60000244, 0x00000000);// (controller #0) write_both_ddrc_reg ODTMAP 0x00000000
  write_reg( memshire_id, 0x60001244, 0x00000000);// (controller #1) write_both_ddrc_reg ODTMAP 0x00000000
  write_reg( memshire_id, 0x60000250, 0x00a01f01);// (controller #0) write_both_ddrc_reg SCHED 0x00a01f01
  write_reg( memshire_id, 0x60001250, 0x00a01f01);// (controller #1) write_both_ddrc_reg SCHED 0x00a01f01
  write_reg( memshire_id, 0x60000254, 0x00000000);// (controller #0) write_both_ddrc_reg SCHED1 0x00000000
  write_reg( memshire_id, 0x60001254, 0x00000000);// (controller #1) write_both_ddrc_reg SCHED1 0x00000000
  write_reg( memshire_id, 0x6000025c, 0x0f000001);// (controller #0) write_both_ddrc_reg PERFHPR1 0x0f000001
  write_reg( memshire_id, 0x6000125c, 0x0f000001);// (controller #1) write_both_ddrc_reg PERFHPR1 0x0f000001
  write_reg( memshire_id, 0x60000264, 0x0f00007f);// (controller #0) write_both_ddrc_reg PERFLPR1 0x0f00007f
  write_reg( memshire_id, 0x60001264, 0x0f00007f);// (controller #1) write_both_ddrc_reg PERFLPR1 0x0f00007f
  write_reg( memshire_id, 0x6000026c, 0x0f00007f);// (controller #0) write_both_ddrc_reg PERFWR1 0x0f00007f
  write_reg( memshire_id, 0x6000126c, 0x0f00007f);// (controller #1) write_both_ddrc_reg PERFWR1 0x0f00007f
  write_reg( memshire_id, 0x60000300, 0x00000000);// (controller #0) write_both_ddrc_reg DBG0 0x00000000
  write_reg( memshire_id, 0x60001300, 0x00000000);// (controller #1) write_both_ddrc_reg DBG0 0x00000000
  write_reg( memshire_id, 0x60000304, 0x00000000);// (controller #0) write_both_ddrc_reg DBG1 0x00000000
  write_reg( memshire_id, 0x60001304, 0x00000000);// (controller #1) write_both_ddrc_reg DBG1 0x00000000
  write_reg( memshire_id, 0x6000030c, 0x00000000);// (controller #0) write_both_ddrc_reg DBGCMD 0x00000000
  write_reg( memshire_id, 0x6000130c, 0x00000000);// (controller #1) write_both_ddrc_reg DBGCMD 0x00000000
  write_reg( memshire_id, 0x60000320, 0x00000001);// (controller #0) write_both_ddrc_reg SWCTL 0x00000001
  write_reg( memshire_id, 0x60001320, 0x00000001);// (controller #1) write_both_ddrc_reg SWCTL 0x00000001
  write_reg( memshire_id, 0x6000036c, 0x00000010);// (controller #0) write_both_ddrc_reg POISONCFG 0x00000010
  write_reg( memshire_id, 0x6000136c, 0x00000010);// (controller #1) write_both_ddrc_reg POISONCFG 0x00000010
  write_reg( memshire_id, 0x60000374, 0x000001d2);// (controller #0) write_both_ddrc_reg ADVECCINDEX 0x000001d2
  write_reg( memshire_id, 0x60001374, 0x000001d2);// (controller #1) write_both_ddrc_reg ADVECCINDEX 0x000001d2
  write_reg( memshire_id, 0x6000037c, 0x00000000);// (controller #0) write_both_ddrc_reg ECCPOISONPAT0 0x00000000
  write_reg( memshire_id, 0x6000137c, 0x00000000);// (controller #1) write_both_ddrc_reg ECCPOISONPAT0 0x00000000
  write_reg( memshire_id, 0x60000384, 0x00000000);// (controller #0) write_both_ddrc_reg ECCPOISONPAT2 0x00000000
  write_reg( memshire_id, 0x60001384, 0x00000000);// (controller #1) write_both_ddrc_reg ECCPOISONPAT2 0x00000000
  write_reg( memshire_id, 0x60000490, 0x00000001);// (controller #0) write_both_ddrc_reg PCTRL_0 0x00000001
  write_reg( memshire_id, 0x60001490, 0x00000001);// (controller #1) write_both_ddrc_reg PCTRL_0 0x00000001
  write_reg( memshire_id, 0x60000540, 0x00000001);// (controller #0) write_both_ddrc_reg PCTRL_1 0x00000001
  write_reg( memshire_id, 0x60001540, 0x00000001);// (controller #1) write_both_ddrc_reg PCTRL_1 0x00000001
//
//###################################
//# make sure STAT == 0
//###################################
  do
    value_read = read_reg( memshire_id, 0x60000004);// poll_ddrc_reg 0 STAT 0x0 0xffffffff 10
  while(!((value_read & 0xffffffff) == 0x0));

  do
    value_read = read_reg( memshire_id, 0x60001004);// poll_ddrc_reg 1 STAT 0x0 0xffffffff 10
  while(!((value_read & 0xffffffff) == 0x0));
//
  write_reg( memshire_id, 0x60000400, 0x00000000);// (controller #0) write_both_ddrc_reg PCCFG 0x00000000
  write_reg( memshire_id, 0x60001400, 0x00000000);// (controller #1) write_both_ddrc_reg PCCFG 0x00000000
  write_reg( memshire_id, 0x60000404, 0x0000400f);// (controller #0) write_both_ddrc_reg PCFGR_0 0x0000400f
  write_reg( memshire_id, 0x60001404, 0x0000400f);// (controller #1) write_both_ddrc_reg PCFGR_0 0x0000400f
  write_reg( memshire_id, 0x600004b4, 0x0000400f);// (controller #0) write_both_ddrc_reg PCFGR_1 0x0000400f
  write_reg( memshire_id, 0x600014b4, 0x0000400f);// (controller #1) write_both_ddrc_reg PCFGR_1 0x0000400f
  write_reg( memshire_id, 0x60000404, 0x0000500f);// (controller #0) write_both_ddrc_reg PCFGR_0 0x0000500f
  write_reg( memshire_id, 0x60001404, 0x0000500f);// (controller #1) write_both_ddrc_reg PCFGR_0 0x0000500f
  write_reg( memshire_id, 0x600004b4, 0x0000500f);// (controller #0) write_both_ddrc_reg PCFGR_1 0x0000500f
  write_reg( memshire_id, 0x600014b4, 0x0000500f);// (controller #1) write_both_ddrc_reg PCFGR_1 0x0000500f
  write_reg( memshire_id, 0x60000404, 0x0000500f);// (controller #0) write_both_ddrc_reg PCFGR_0 0x0000500f
  write_reg( memshire_id, 0x60001404, 0x0000500f);// (controller #1) write_both_ddrc_reg PCFGR_0 0x0000500f
  write_reg( memshire_id, 0x600004b4, 0x0000500f);// (controller #0) write_both_ddrc_reg PCFGR_1 0x0000500f
  write_reg( memshire_id, 0x600014b4, 0x0000500f);// (controller #1) write_both_ddrc_reg PCFGR_1 0x0000500f
  write_reg( memshire_id, 0x60000404, 0x0000100f);// (controller #0) write_both_ddrc_reg PCFGR_0 0x0000100f
  write_reg( memshire_id, 0x60001404, 0x0000100f);// (controller #1) write_both_ddrc_reg PCFGR_0 0x0000100f
  write_reg( memshire_id, 0x600004b4, 0x0000100f);// (controller #0) write_both_ddrc_reg PCFGR_1 0x0000100f
  write_reg( memshire_id, 0x600014b4, 0x0000100f);// (controller #1) write_both_ddrc_reg PCFGR_1 0x0000100f
  write_reg( memshire_id, 0x60000408, 0x0000400f);// (controller #0) write_both_ddrc_reg PCFGW_0 0x0000400f
  write_reg( memshire_id, 0x60001408, 0x0000400f);// (controller #1) write_both_ddrc_reg PCFGW_0 0x0000400f
  write_reg( memshire_id, 0x600004b8, 0x0000400f);// (controller #0) write_both_ddrc_reg PCFGW_1 0x0000400f
  write_reg( memshire_id, 0x600014b8, 0x0000400f);// (controller #1) write_both_ddrc_reg PCFGW_1 0x0000400f
  write_reg( memshire_id, 0x60000408, 0x0000500f);// (controller #0) write_both_ddrc_reg PCFGW_0 0x0000500f
  write_reg( memshire_id, 0x60001408, 0x0000500f);// (controller #1) write_both_ddrc_reg PCFGW_0 0x0000500f
  write_reg( memshire_id, 0x600004b8, 0x0000500f);// (controller #0) write_both_ddrc_reg PCFGW_1 0x0000500f
  write_reg( memshire_id, 0x600014b8, 0x0000500f);// (controller #1) write_both_ddrc_reg PCFGW_1 0x0000500f
  write_reg( memshire_id, 0x60000408, 0x0000500f);// (controller #0) write_both_ddrc_reg PCFGW_0 0x0000500f
  write_reg( memshire_id, 0x60001408, 0x0000500f);// (controller #1) write_both_ddrc_reg PCFGW_0 0x0000500f
  write_reg( memshire_id, 0x600004b8, 0x0000500f);// (controller #0) write_both_ddrc_reg PCFGW_1 0x0000500f
  write_reg( memshire_id, 0x600014b8, 0x0000500f);// (controller #1) write_both_ddrc_reg PCFGW_1 0x0000500f
  write_reg( memshire_id, 0x60000408, 0x0000100f);// (controller #0) write_both_ddrc_reg PCFGW_0 0x0000100f
  write_reg( memshire_id, 0x60001408, 0x0000100f);// (controller #1) write_both_ddrc_reg PCFGW_0 0x0000100f
  write_reg( memshire_id, 0x600004b8, 0x0000100f);// (controller #0) write_both_ddrc_reg PCFGW_1 0x0000100f
  write_reg( memshire_id, 0x600014b8, 0x0000100f);// (controller #1) write_both_ddrc_reg PCFGW_1 0x0000100f
  write_reg( memshire_id, 0x60000304, 0x00000000);// (controller #0) write_both_ddrc_reg DBG1 0x00000000
  write_reg( memshire_id, 0x60001304, 0x00000000);// (controller #1) write_both_ddrc_reg DBG1 0x00000000
//
//########################################
//# make power control reg can be written
//# FIXME tberg: is this needed?
//########################################
  do
    value_read = read_reg( memshire_id, 0x60000030);// poll_ddrc_reg 0 PWRCTL 0 0x1ff 1
  while(!((value_read & 0x1ff) == 0x0));
  do
    value_read = read_reg( memshire_id, 0x60000030);// poll_ddrc_reg 0 PWRCTL 0 0x1ff 1
  while(!((value_read & 0x1ff) == 0x0));
//
  write_reg( memshire_id, 0x60000030, 0x00000000);// (controller #0) write_both_ddrc_reg PWRCTL 0x00000000
  write_reg( memshire_id, 0x60001030, 0x00000000);// (controller #1) write_both_ddrc_reg PWRCTL 0x00000000
  do
    value_read = read_reg( memshire_id, 0x60000030);// poll_ddrc_reg 0 PWRCTL 0 0x1ff 1
  while(!((value_read & 0x1ff) == 0x0));
  do
    value_read = read_reg( memshire_id, 0x60000030);// poll_ddrc_reg 0 PWRCTL 0 0x1ff 1
  while(!((value_read & 0x1ff) == 0x0));
//
  write_reg( memshire_id, 0x60000030, 0x00000000);// (controller #0) write_both_ddrc_reg PWRCTL 0x00000000;       # powerdown_en = 0, selfref_en = 0
  write_reg( memshire_id, 0x60001030, 0x00000000);// (controller #1) write_both_ddrc_reg PWRCTL 0x00000000;       # powerdown_en = 0, selfref_en = 0
  write_reg( memshire_id, 0x60000320, 0x00000000);// (controller #0) write_both_ddrc_reg SWCTL 0x00000000;        # sw_done = 0
  write_reg( memshire_id, 0x60001320, 0x00000000);// (controller #1) write_both_ddrc_reg SWCTL 0x00000000;        # sw_done = 0
  write_reg( memshire_id, 0x600001b0, 0x00000080);// (controller #0) write_both_ddrc_reg DFIMISC 0x00000080;      # dfi_init_complete_en = 0
  write_reg( memshire_id, 0x600011b0, 0x00000080);// (controller #1) write_both_ddrc_reg DFIMISC 0x00000080;      # dfi_init_complete_en = 0
  write_reg( memshire_id, 0x600001b0, 0x00001080);// (controller #0) write_both_ddrc_reg DFIMISC 0x00001080;      # ???
  write_reg( memshire_id, 0x600011b0, 0x00001080);// (controller #1) write_both_ddrc_reg DFIMISC 0x00001080;      # ???
//
//######################################################
//# These are probably optional, but good to make sure
//# the memory controllers are in the expected state
//######################################################
  do
    value_read = read_reg( memshire_id, 0x600000d0);// poll_ddrc_reg 0 INIT0  0x00020002  0xffffffff 1
  while(!((value_read & 0xffffffff) == 0x00020002));

  do
    value_read = read_reg( memshire_id, 0x600001c0);// poll_ddrc_reg 0 DBICTL 0x00000001  0xffffffff 1
  while(!((value_read & 0xffffffff) == 0x00000001));

  do
    value_read = read_reg( memshire_id, 0x60000000);// poll_ddrc_reg 0 MSTR   0x00080020  0xffffffff 1
  while(!((value_read & 0xffffffff) == 0x00080020));

  do
    value_read = read_reg( memshire_id, 0x600000dc);// poll_ddrc_reg 0 INIT3  0x0074007f  0xffffffff 1
  while(!((value_read & 0xffffffff) == 0x0074007f));

  do
    value_read = read_reg( memshire_id, 0x600000e0);// poll_ddrc_reg 0 INIT4  0x00330000  0xffffffff 1
  while(!((value_read & 0xffffffff) == 0x00330000));

  do
    value_read = read_reg( memshire_id, 0x600000e8);// poll_ddrc_reg 0 INIT6  0x0000004d  0xffffffff 1
  while(!((value_read & 0xffffffff) == 0x0000004d));

  do
    value_read = read_reg( memshire_id, 0x600010d0);// poll_ddrc_reg 1 INIT0  0x00020002  0xffffffff 1
  while(!((value_read & 0xffffffff) == 0x00020002));

  do
    value_read = read_reg( memshire_id, 0x600011c0);// poll_ddrc_reg 1 DBICTL 0x00000001  0xffffffff 1
  while(!((value_read & 0xffffffff) == 0x00000001));

  do
    value_read = read_reg( memshire_id, 0x60001000);// poll_ddrc_reg 1 MSTR   0x00080020  0xffffffff 1
  while(!((value_read & 0xffffffff) == 0x00080020));

  do
    value_read = read_reg( memshire_id, 0x600010dc);// poll_ddrc_reg 1 INIT3  0x0074007f  0xffffffff 1
  while(!((value_read & 0xffffffff) == 0x0074007f));

  do
    value_read = read_reg( memshire_id, 0x600010e0);// poll_ddrc_reg 1 INIT4  0x00330000  0xffffffff 1
  while(!((value_read & 0xffffffff) == 0x00330000));

  do
    value_read = read_reg( memshire_id, 0x600010e8);// poll_ddrc_reg 1 INIT6  0x0000004d  0xffffffff 1
  while(!((value_read & 0xffffffff) == 0x0000004d));
//
//######################################################
//# turn off core and axi resets
//######################################################
#ifdef CONFIG_REAL_PLL
  write_reg( memshire_id, 0x180000200, 0x0000010c);// write_esr ddrc_reset_ctl 0x10c
#else
  write_reg( memshire_id, 0x180000200, 0x0000000c);// write_esr ddrc_reset_ctl 0x0c
#endif
//set val [read_esr ddrc_reset_ctl]
//// "ddrc_reset_ctl is [format "%8x" $val]"
//
//# not sure how much time is needed here?
//wait_cycles 100;
//
//######################################################
//# turn off pub reset
//######################################################
#ifdef CONFIG_REAL_PLL
  write_reg( memshire_id, 0x180000200, 0x00000108);// write_esr ddrc_reset_ctl 0x108
#else
  write_reg( memshire_id, 0x180000200, 0x00000008);// write_esr ddrc_reset_ctl 0x08
#endif
//set val [read_esr ddrc_reset_ctl]
//// "ddrc_reset_ctl is [format "%8x" $val]"
//wait_cycles 100;
//
//######################################################
//# Execute routine to initialize the phy
//######################################################
//set my_repo_root [get_env REPOROOT]
  write_reg( memshire_id, 0x6204017c, 0x000005ff);// write_ddrc_reg 2 DBYTE0_TxSlewRate_b0_p0 0x5ff
  write_reg( memshire_id, 0x6204057c, 0x000005ff);// write_ddrc_reg 2 DBYTE0_TxSlewRate_b1_p0 0x5ff
  write_reg( memshire_id, 0x6204417c, 0x000005ff);// write_ddrc_reg 2 DBYTE1_TxSlewRate_b0_p0 0x5ff
  write_reg( memshire_id, 0x6204457c, 0x000005ff);// write_ddrc_reg 2 DBYTE1_TxSlewRate_b1_p0 0x5ff
  write_reg( memshire_id, 0x6204817c, 0x000005ff);// write_ddrc_reg 2 DBYTE2_TxSlewRate_b0_p0 0x5ff
  write_reg( memshire_id, 0x6204857c, 0x000005ff);// write_ddrc_reg 2 DBYTE2_TxSlewRate_b1_p0 0x5ff
  write_reg( memshire_id, 0x6204c17c, 0x000005ff);// write_ddrc_reg 2 DBYTE3_TxSlewRate_b0_p0 0x5ff
  write_reg( memshire_id, 0x6204c57c, 0x000005ff);// write_ddrc_reg 2 DBYTE3_TxSlewRate_b1_p0 0x5ff
  write_reg( memshire_id, 0x62000154, 0x000001ff);// write_ddrc_reg 2 ANIB0_ATxSlewRate 0x1ff
  write_reg( memshire_id, 0x62004154, 0x000001ff);// write_ddrc_reg 2 ANIB1_ATxSlewRate 0x1ff
  write_reg( memshire_id, 0x62008154, 0x000001ff);// write_ddrc_reg 2 ANIB2_ATxSlewRate 0x1ff
  write_reg( memshire_id, 0x6200c154, 0x000001ff);// write_ddrc_reg 2 ANIB3_ATxSlewRate 0x1ff
  write_reg( memshire_id, 0x62010154, 0x000001ff);// write_ddrc_reg 2 ANIB4_ATxSlewRate 0x1ff
  write_reg( memshire_id, 0x62014154, 0x000001ff);// write_ddrc_reg 2 ANIB5_ATxSlewRate 0x1ff
  write_reg( memshire_id, 0x62080314, 0x00000019);// write_ddrc_reg 2 MASTER0_PllCtrl2_p0 0x19
  write_reg( memshire_id, 0x620800b8, 0x00000002);// write_ddrc_reg 2 MASTER0_ARdPtrInitVal_p0 0x2
  write_reg( memshire_id, 0x62240810, 0x00000000);// write_ddrc_reg 2 INITENG0_Seq0BGPR4_p0 0x0
  write_reg( memshire_id, 0x62080090, 0x000000e3);// write_ddrc_reg 2 MASTER0_DqsPreambleControl_p0 0xe3
  write_reg( memshire_id, 0x620800e8, 0x00000002);// write_ddrc_reg 2 MASTER0_DbyteDllModeCntrl 0x2
  write_reg( memshire_id, 0x62080158, 0x00000003);// write_ddrc_reg 2 MASTER0_ProcOdtTimeCtl_p0 0x3
  write_reg( memshire_id, 0x62040134, 0x00000600);// write_ddrc_reg 2 DBYTE0_TxOdtDrvStren_b0_p0 0x600
  write_reg( memshire_id, 0x62040534, 0x00000600);// write_ddrc_reg 2 DBYTE0_TxOdtDrvStren_b1_p0 0x600
  write_reg( memshire_id, 0x62044134, 0x00000600);// write_ddrc_reg 2 DBYTE1_TxOdtDrvStren_b0_p0 0x600
  write_reg( memshire_id, 0x62044534, 0x00000600);// write_ddrc_reg 2 DBYTE1_TxOdtDrvStren_b1_p0 0x600
  write_reg( memshire_id, 0x62048134, 0x00000600);// write_ddrc_reg 2 DBYTE2_TxOdtDrvStren_b0_p0 0x600
  write_reg( memshire_id, 0x62048534, 0x00000600);// write_ddrc_reg 2 DBYTE2_TxOdtDrvStren_b1_p0 0x600
  write_reg( memshire_id, 0x6204c134, 0x00000600);// write_ddrc_reg 2 DBYTE3_TxOdtDrvStren_b0_p0 0x600
  write_reg( memshire_id, 0x6204c534, 0x00000600);// write_ddrc_reg 2 DBYTE3_TxOdtDrvStren_b1_p0 0x600
  write_reg( memshire_id, 0x62040124, 0x00000618);// write_ddrc_reg 2 DBYTE0_TxImpedanceCtrl1_b0_p0 0x618
  write_reg( memshire_id, 0x62040524, 0x00000618);// write_ddrc_reg 2 DBYTE0_TxImpedanceCtrl1_b1_p0 0x618
  write_reg( memshire_id, 0x62044124, 0x00000618);// write_ddrc_reg 2 DBYTE1_TxImpedanceCtrl1_b0_p0 0x618
  write_reg( memshire_id, 0x62044524, 0x00000618);// write_ddrc_reg 2 DBYTE1_TxImpedanceCtrl1_b1_p0 0x618
  write_reg( memshire_id, 0x62048124, 0x00000618);// write_ddrc_reg 2 DBYTE2_TxImpedanceCtrl1_b0_p0 0x618
  write_reg( memshire_id, 0x62048524, 0x00000618);// write_ddrc_reg 2 DBYTE2_TxImpedanceCtrl1_b1_p0 0x618
  write_reg( memshire_id, 0x6204c124, 0x00000618);// write_ddrc_reg 2 DBYTE3_TxImpedanceCtrl1_b0_p0 0x618
  write_reg( memshire_id, 0x6204c524, 0x00000618);// write_ddrc_reg 2 DBYTE3_TxImpedanceCtrl1_b1_p0 0x618
  write_reg( memshire_id, 0x6200010c, 0x000003ff);// write_ddrc_reg 2 ANIB0_ATxImpedance 0x3ff
  write_reg( memshire_id, 0x6200410c, 0x000003ff);// write_ddrc_reg 2 ANIB1_ATxImpedance 0x3ff
  write_reg( memshire_id, 0x6200810c, 0x000003ff);// write_ddrc_reg 2 ANIB2_ATxImpedance 0x3ff
  write_reg( memshire_id, 0x6200c10c, 0x000003ff);// write_ddrc_reg 2 ANIB3_ATxImpedance 0x3ff
  write_reg( memshire_id, 0x6201010c, 0x000003ff);// write_ddrc_reg 2 ANIB4_ATxImpedance 0x3ff
  write_reg( memshire_id, 0x6201410c, 0x000003ff);// write_ddrc_reg 2 ANIB5_ATxImpedance 0x3ff
  write_reg( memshire_id, 0x62080060, 0x00000003);// write_ddrc_reg 2 MASTER0_DfiMode 0x3
  write_reg( memshire_id, 0x620801d4, 0x00000004);// write_ddrc_reg 2 MASTER0_DfiCAMode 0x4
  write_reg( memshire_id, 0x62080140, 0x00000000);// write_ddrc_reg 2 MASTER0_CalDrvStr0 0x0
  write_reg( memshire_id, 0x62080020, 0x00000320);// write_ddrc_reg 2 MASTER0_CalUclkInfo_p0 0x320
  write_reg( memshire_id, 0x62080220, 0x00000009);// write_ddrc_reg 2 MASTER0_CalRate 0x9
  write_reg( memshire_id, 0x620802c8, 0x00000104);// write_ddrc_reg 2 MASTER0_VrefInGlobal_p0 0x104
  write_reg( memshire_id, 0x6204010c, 0x000005a1);// write_ddrc_reg 2 DBYTE0_DqDqsRcvCntrl_b0_p0 0x5a1
  write_reg( memshire_id, 0x6204050c, 0x000005a1);// write_ddrc_reg 2 DBYTE0_DqDqsRcvCntrl_b1_p0 0x5a1
  write_reg( memshire_id, 0x6204410c, 0x000005a1);// write_ddrc_reg 2 DBYTE1_DqDqsRcvCntrl_b0_p0 0x5a1
  write_reg( memshire_id, 0x6204450c, 0x000005a1);// write_ddrc_reg 2 DBYTE1_DqDqsRcvCntrl_b1_p0 0x5a1
  write_reg( memshire_id, 0x6204810c, 0x000005a1);// write_ddrc_reg 2 DBYTE2_DqDqsRcvCntrl_b0_p0 0x5a1
  write_reg( memshire_id, 0x6204850c, 0x000005a1);// write_ddrc_reg 2 DBYTE2_DqDqsRcvCntrl_b1_p0 0x5a1
  write_reg( memshire_id, 0x6204c10c, 0x000005a1);// write_ddrc_reg 2 DBYTE3_DqDqsRcvCntrl_b0_p0 0x5a1
  write_reg( memshire_id, 0x6204c50c, 0x000005a1);// write_ddrc_reg 2 DBYTE3_DqDqsRcvCntrl_b1_p0 0x5a1
  write_reg( memshire_id, 0x620803e8, 0x00000001);// write_ddrc_reg 2 MASTER0_DfiFreqRatio_p0 0x1
  write_reg( memshire_id, 0x62080064, 0x00000001);// write_ddrc_reg 2 MASTER0_TristateModeCA_p0 0x1
  write_reg( memshire_id, 0x620803c0, 0x00000000);// write_ddrc_reg 2 MASTER0_DfiFreqXlat0 0x0
  write_reg( memshire_id, 0x620803c4, 0x00000000);// write_ddrc_reg 2 MASTER0_DfiFreqXlat1 0x0
  write_reg( memshire_id, 0x620803c8, 0x00004444);// write_ddrc_reg 2 MASTER0_DfiFreqXlat2 0x4444
  write_reg( memshire_id, 0x620803cc, 0x00008888);// write_ddrc_reg 2 MASTER0_DfiFreqXlat3 0x8888
  write_reg( memshire_id, 0x620803d0, 0x00005555);// write_ddrc_reg 2 MASTER0_DfiFreqXlat4 0x5555
  write_reg( memshire_id, 0x620803d4, 0x00000000);// write_ddrc_reg 2 MASTER0_DfiFreqXlat5 0x0
  write_reg( memshire_id, 0x620803d8, 0x00000000);// write_ddrc_reg 2 MASTER0_DfiFreqXlat6 0x0
  write_reg( memshire_id, 0x620803dc, 0x0000f000);// write_ddrc_reg 2 MASTER0_DfiFreqXlat7 0xf000
  write_reg( memshire_id, 0x62040128, 0x00000500);// write_ddrc_reg 2 DBYTE0_DqDqsRcvCntrl1 0x500
  write_reg( memshire_id, 0x62044128, 0x00000500);// write_ddrc_reg 2 DBYTE1_DqDqsRcvCntrl1 0x500
  write_reg( memshire_id, 0x62048128, 0x00000500);// write_ddrc_reg 2 DBYTE2_DqDqsRcvCntrl1 0x500
  write_reg( memshire_id, 0x6204c128, 0x00000500);// write_ddrc_reg 2 DBYTE3_DqDqsRcvCntrl1 0x500
  write_reg( memshire_id, 0x62080094, 0x00000000);// write_ddrc_reg 2 MASTER0_MasterX4Config 0x0
  write_reg( memshire_id, 0x620800b4, 0x00000000);// write_ddrc_reg 2 MASTER0_DMIPinPresent_p0 0x0
  write_reg( memshire_id, 0x62040080, 0x00000005);// write_ddrc_reg 2 DBYTE0_DFIMRL_p0 0x5
  write_reg( memshire_id, 0x62044080, 0x00000005);// write_ddrc_reg 2 DBYTE1_DFIMRL_p0 0x5
  write_reg( memshire_id, 0x62048080, 0x00000005);// write_ddrc_reg 2 DBYTE2_DFIMRL_p0 0x5
  write_reg( memshire_id, 0x6204c080, 0x00000005);// write_ddrc_reg 2 DBYTE3_DFIMRL_p0 0x5
  write_reg( memshire_id, 0x62080080, 0x00000005);// write_ddrc_reg 2 MASTER0_HwtMRL_p0 0x5
  write_reg( memshire_id, 0x62040340, 0x00000100);// write_ddrc_reg 2 DBYTE0_TxDqsDlyTg0_u0_p0 0x100
  write_reg( memshire_id, 0x62040740, 0x00000100);// write_ddrc_reg 2 DBYTE0_TxDqsDlyTg0_u1_p0 0x100
  write_reg( memshire_id, 0x62044340, 0x00000100);// write_ddrc_reg 2 DBYTE1_TxDqsDlyTg0_u0_p0 0x100
  write_reg( memshire_id, 0x62044740, 0x00000100);// write_ddrc_reg 2 DBYTE1_TxDqsDlyTg0_u1_p0 0x100
  write_reg( memshire_id, 0x62048340, 0x00000100);// write_ddrc_reg 2 DBYTE2_TxDqsDlyTg0_u0_p0 0x100
  write_reg( memshire_id, 0x62048740, 0x00000100);// write_ddrc_reg 2 DBYTE2_TxDqsDlyTg0_u1_p0 0x100
  write_reg( memshire_id, 0x6204c340, 0x00000100);// write_ddrc_reg 2 DBYTE3_TxDqsDlyTg0_u0_p0 0x100
  write_reg( memshire_id, 0x6204c740, 0x00000100);// write_ddrc_reg 2 DBYTE3_TxDqsDlyTg0_u1_p0 0x100
  write_reg( memshire_id, 0x62040300, 0x00000084);// write_ddrc_reg 2 DBYTE0_TxDqDlyTg0_r0_p0 0x84
  write_reg( memshire_id, 0x62040700, 0x00000084);// write_ddrc_reg 2 DBYTE0_TxDqDlyTg0_r1_p0 0x84
  write_reg( memshire_id, 0x62040b00, 0x00000084);// write_ddrc_reg 2 DBYTE0_TxDqDlyTg0_r2_p0 0x84
  write_reg( memshire_id, 0x62040f00, 0x00000084);// write_ddrc_reg 2 DBYTE0_TxDqDlyTg0_r3_p0 0x84
  write_reg( memshire_id, 0x62041300, 0x00000084);// write_ddrc_reg 2 DBYTE0_TxDqDlyTg0_r4_p0 0x84
  write_reg( memshire_id, 0x62041700, 0x00000084);// write_ddrc_reg 2 DBYTE0_TxDqDlyTg0_r5_p0 0x84
  write_reg( memshire_id, 0x62041b00, 0x00000084);// write_ddrc_reg 2 DBYTE0_TxDqDlyTg0_r6_p0 0x84
  write_reg( memshire_id, 0x62041f00, 0x00000084);// write_ddrc_reg 2 DBYTE0_TxDqDlyTg0_r7_p0 0x84
  write_reg( memshire_id, 0x62042300, 0x00000084);// write_ddrc_reg 2 DBYTE0_TxDqDlyTg0_r8_p0 0x84
  write_reg( memshire_id, 0x62044300, 0x00000084);// write_ddrc_reg 2 DBYTE1_TxDqDlyTg0_r0_p0 0x84
  write_reg( memshire_id, 0x62044700, 0x00000084);// write_ddrc_reg 2 DBYTE1_TxDqDlyTg0_r1_p0 0x84
  write_reg( memshire_id, 0x62044b00, 0x00000084);// write_ddrc_reg 2 DBYTE1_TxDqDlyTg0_r2_p0 0x84
  write_reg( memshire_id, 0x62044f00, 0x00000084);// write_ddrc_reg 2 DBYTE1_TxDqDlyTg0_r3_p0 0x84
  write_reg( memshire_id, 0x62045300, 0x00000084);// write_ddrc_reg 2 DBYTE1_TxDqDlyTg0_r4_p0 0x84
  write_reg( memshire_id, 0x62045700, 0x00000084);// write_ddrc_reg 2 DBYTE1_TxDqDlyTg0_r5_p0 0x84
  write_reg( memshire_id, 0x62045b00, 0x00000084);// write_ddrc_reg 2 DBYTE1_TxDqDlyTg0_r6_p0 0x84
  write_reg( memshire_id, 0x62045f00, 0x00000084);// write_ddrc_reg 2 DBYTE1_TxDqDlyTg0_r7_p0 0x84
  write_reg( memshire_id, 0x62046300, 0x00000084);// write_ddrc_reg 2 DBYTE1_TxDqDlyTg0_r8_p0 0x84
  write_reg( memshire_id, 0x62048300, 0x00000084);// write_ddrc_reg 2 DBYTE2_TxDqDlyTg0_r0_p0 0x84
  write_reg( memshire_id, 0x62048700, 0x00000084);// write_ddrc_reg 2 DBYTE2_TxDqDlyTg0_r1_p0 0x84
  write_reg( memshire_id, 0x62048b00, 0x00000084);// write_ddrc_reg 2 DBYTE2_TxDqDlyTg0_r2_p0 0x84
  write_reg( memshire_id, 0x62048f00, 0x00000084);// write_ddrc_reg 2 DBYTE2_TxDqDlyTg0_r3_p0 0x84
  write_reg( memshire_id, 0x62049300, 0x00000084);// write_ddrc_reg 2 DBYTE2_TxDqDlyTg0_r4_p0 0x84
  write_reg( memshire_id, 0x62049700, 0x00000084);// write_ddrc_reg 2 DBYTE2_TxDqDlyTg0_r5_p0 0x84
  write_reg( memshire_id, 0x62049b00, 0x00000084);// write_ddrc_reg 2 DBYTE2_TxDqDlyTg0_r6_p0 0x84
  write_reg( memshire_id, 0x62049f00, 0x00000084);// write_ddrc_reg 2 DBYTE2_TxDqDlyTg0_r7_p0 0x84
  write_reg( memshire_id, 0x6204a300, 0x00000084);// write_ddrc_reg 2 DBYTE2_TxDqDlyTg0_r8_p0 0x84
  write_reg( memshire_id, 0x6204c300, 0x00000084);// write_ddrc_reg 2 DBYTE3_TxDqDlyTg0_r0_p0 0x84
  write_reg( memshire_id, 0x6204c700, 0x00000084);// write_ddrc_reg 2 DBYTE3_TxDqDlyTg0_r1_p0 0x84
  write_reg( memshire_id, 0x6204cb00, 0x00000084);// write_ddrc_reg 2 DBYTE3_TxDqDlyTg0_r2_p0 0x84
  write_reg( memshire_id, 0x6204cf00, 0x00000084);// write_ddrc_reg 2 DBYTE3_TxDqDlyTg0_r3_p0 0x84
  write_reg( memshire_id, 0x6204d300, 0x00000084);// write_ddrc_reg 2 DBYTE3_TxDqDlyTg0_r4_p0 0x84
  write_reg( memshire_id, 0x6204d700, 0x00000084);// write_ddrc_reg 2 DBYTE3_TxDqDlyTg0_r5_p0 0x84
  write_reg( memshire_id, 0x6204db00, 0x00000084);// write_ddrc_reg 2 DBYTE3_TxDqDlyTg0_r6_p0 0x84
  write_reg( memshire_id, 0x6204df00, 0x00000084);// write_ddrc_reg 2 DBYTE3_TxDqDlyTg0_r7_p0 0x84
  write_reg( memshire_id, 0x6204e300, 0x00000084);// write_ddrc_reg 2 DBYTE3_TxDqDlyTg0_r8_p0 0x84
  write_reg( memshire_id, 0x62040200, 0x00000142);// write_ddrc_reg 2 DBYTE0_RxEnDlyTg0_u0_p0 0x142
  write_reg( memshire_id, 0x62040600, 0x00000142);// write_ddrc_reg 2 DBYTE0_RxEnDlyTg0_u1_p0 0x142
  write_reg( memshire_id, 0x62044200, 0x00000142);// write_ddrc_reg 2 DBYTE1_RxEnDlyTg0_u0_p0 0x142
  write_reg( memshire_id, 0x62044600, 0x00000142);// write_ddrc_reg 2 DBYTE1_RxEnDlyTg0_u1_p0 0x142
  write_reg( memshire_id, 0x62048200, 0x00000142);// write_ddrc_reg 2 DBYTE2_RxEnDlyTg0_u0_p0 0x142
  write_reg( memshire_id, 0x62048600, 0x00000142);// write_ddrc_reg 2 DBYTE2_RxEnDlyTg0_u1_p0 0x142
  write_reg( memshire_id, 0x6204c200, 0x00000142);// write_ddrc_reg 2 DBYTE3_RxEnDlyTg0_u0_p0 0x142
  write_reg( memshire_id, 0x6204c600, 0x00000142);// write_ddrc_reg 2 DBYTE3_RxEnDlyTg0_u1_p0 0x142
  write_reg( memshire_id, 0x62240804, 0x00001a00);// write_ddrc_reg 2 INITENG0_Seq0BGPR1_p0 0x1a00
  write_reg( memshire_id, 0x62240808, 0x00000000);// write_ddrc_reg 2 INITENG0_Seq0BGPR2_p0 0x0
  write_reg( memshire_id, 0x6224080c, 0x00002600);// write_ddrc_reg 2 INITENG0_Seq0BGPR3_p0 0x2600
  write_reg( memshire_id, 0x620801c8, 0x00000001);// write_ddrc_reg 2 MASTER0_HwtLpCsEnA 0x1
  write_reg( memshire_id, 0x620801cc, 0x00000001);// write_ddrc_reg 2 MASTER0_HwtLpCsEnB 0x1
  write_reg( memshire_id, 0x620402b8, 0x00000034);// write_ddrc_reg 2 DBYTE0_PptDqsCntInvTrnTg0_p0 0x34
  write_reg( memshire_id, 0x620442b8, 0x00000034);// write_ddrc_reg 2 DBYTE1_PptDqsCntInvTrnTg0_p0 0x34
  write_reg( memshire_id, 0x620482b8, 0x00000034);// write_ddrc_reg 2 DBYTE2_PptDqsCntInvTrnTg0_p0 0x34
  write_reg( memshire_id, 0x6204c2b8, 0x00000034);// write_ddrc_reg 2 DBYTE3_PptDqsCntInvTrnTg0_p0 0x34
  write_reg( memshire_id, 0x620402bc, 0x00000034);// write_ddrc_reg 2 DBYTE0_PptDqsCntInvTrnTg1_p0 0x34
  write_reg( memshire_id, 0x620442bc, 0x00000034);// write_ddrc_reg 2 DBYTE1_PptDqsCntInvTrnTg1_p0 0x34
  write_reg( memshire_id, 0x620482bc, 0x00000034);// write_ddrc_reg 2 DBYTE2_PptDqsCntInvTrnTg1_p0 0x34
  write_reg( memshire_id, 0x6204c2bc, 0x00000034);// write_ddrc_reg 2 DBYTE3_PptDqsCntInvTrnTg1_p0 0x34
  write_reg( memshire_id, 0x620402a8, 0x00000703);// write_ddrc_reg 2 DBYTE0_PptCtlStatic 0x703
  write_reg( memshire_id, 0x620442a8, 0x0000070f);// write_ddrc_reg 2 DBYTE1_PptCtlStatic 0x70f
  write_reg( memshire_id, 0x620482a8, 0x00000703);// write_ddrc_reg 2 DBYTE2_PptCtlStatic 0x703
  write_reg( memshire_id, 0x6204c2a8, 0x0000070f);// write_ddrc_reg 2 DBYTE3_PptCtlStatic 0x70f
  write_reg( memshire_id, 0x620801dc, 0x00000034);// write_ddrc_reg 2 MASTER0_HwtCAMode 0x34
  write_reg( memshire_id, 0x620801f0, 0x00000054);// write_ddrc_reg 2 MASTER0_DllGainCtl_p0 0x54
  write_reg( memshire_id, 0x620801f4, 0x000003f2);// write_ddrc_reg 2 MASTER0_DllLockParam_p0 0x3f2
  write_reg( memshire_id, 0x62100300, 0x0000010f);// write_ddrc_reg 2 ACSM0_AcsmCtrl23 0x10f
  write_reg( memshire_id, 0x6208032c, 0x000061f0);// write_ddrc_reg 2 MASTER0_PllCtrl3 0x61f0
  write_reg( memshire_id, 0x622400a0, 0x00000000);// write_ddrc_reg 2 INITENG0_PhyInLP3 0x0
  write_reg( memshire_id, 0x62340000, 0x00000000);// write_ddrc_reg 2 APBONLY0_MicroContMuxSel 0x0
  write_reg( memshire_id, 0x62240000, 0x00000010);// write_ddrc_reg 2 INITENG0_PreSequenceReg0b0s0 0x10
  write_reg( memshire_id, 0x62240004, 0x00000400);// write_ddrc_reg 2 INITENG0_PreSequenceReg0b0s1 0x400
  write_reg( memshire_id, 0x62240008, 0x0000010e);// write_ddrc_reg 2 INITENG0_PreSequenceReg0b0s2 0x10e
  write_reg( memshire_id, 0x6224000c, 0x00000000);// write_ddrc_reg 2 INITENG0_PreSequenceReg0b1s0 0x0
  write_reg( memshire_id, 0x62240010, 0x00000000);// write_ddrc_reg 2 INITENG0_PreSequenceReg0b1s1 0x0
  write_reg( memshire_id, 0x62240014, 0x00000008);// write_ddrc_reg 2 INITENG0_PreSequenceReg0b1s2 0x8
  write_reg( memshire_id, 0x622400a4, 0x0000000b);// write_ddrc_reg 2 INITENG0_SequenceReg0b0s0 0xb
  write_reg( memshire_id, 0x622400a8, 0x00000480);// write_ddrc_reg 2 INITENG0_SequenceReg0b0s1 0x480
  write_reg( memshire_id, 0x622400ac, 0x00000109);// write_ddrc_reg 2 INITENG0_SequenceReg0b0s2 0x109
  write_reg( memshire_id, 0x622400b0, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b1s0 0x8
  write_reg( memshire_id, 0x622400b4, 0x00000448);// write_ddrc_reg 2 INITENG0_SequenceReg0b1s1 0x448
  write_reg( memshire_id, 0x622400b8, 0x00000139);// write_ddrc_reg 2 INITENG0_SequenceReg0b1s2 0x139
  write_reg( memshire_id, 0x622400bc, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b2s0 0x8
  write_reg( memshire_id, 0x622400c0, 0x00000478);// write_ddrc_reg 2 INITENG0_SequenceReg0b2s1 0x478
  write_reg( memshire_id, 0x622400c4, 0x00000109);// write_ddrc_reg 2 INITENG0_SequenceReg0b2s2 0x109
  write_reg( memshire_id, 0x622400c8, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b3s0 0x0
  write_reg( memshire_id, 0x622400cc, 0x000000e8);// write_ddrc_reg 2 INITENG0_SequenceReg0b3s1 0xe8
  write_reg( memshire_id, 0x622400d0, 0x00000109);// write_ddrc_reg 2 INITENG0_SequenceReg0b3s2 0x109
  write_reg( memshire_id, 0x622400d4, 0x00000002);// write_ddrc_reg 2 INITENG0_SequenceReg0b4s0 0x2
  write_reg( memshire_id, 0x622400d8, 0x00000010);// write_ddrc_reg 2 INITENG0_SequenceReg0b4s1 0x10
  write_reg( memshire_id, 0x622400dc, 0x00000139);// write_ddrc_reg 2 INITENG0_SequenceReg0b4s2 0x139
  write_reg( memshire_id, 0x622400e0, 0x0000000b);// write_ddrc_reg 2 INITENG0_SequenceReg0b5s0 0xb
  write_reg( memshire_id, 0x622400e4, 0x000007c0);// write_ddrc_reg 2 INITENG0_SequenceReg0b5s1 0x7c0
  write_reg( memshire_id, 0x622400e8, 0x00000139);// write_ddrc_reg 2 INITENG0_SequenceReg0b5s2 0x139
  write_reg( memshire_id, 0x622400ec, 0x00000044);// write_ddrc_reg 2 INITENG0_SequenceReg0b6s0 0x44
  write_reg( memshire_id, 0x622400f0, 0x00000630);// write_ddrc_reg 2 INITENG0_SequenceReg0b6s1 0x630
  write_reg( memshire_id, 0x622400f4, 0x00000159);// write_ddrc_reg 2 INITENG0_SequenceReg0b6s2 0x159
  write_reg( memshire_id, 0x622400f8, 0x0000014f);// write_ddrc_reg 2 INITENG0_SequenceReg0b7s0 0x14f
  write_reg( memshire_id, 0x622400fc, 0x00000630);// write_ddrc_reg 2 INITENG0_SequenceReg0b7s1 0x630
  write_reg( memshire_id, 0x62240100, 0x00000159);// write_ddrc_reg 2 INITENG0_SequenceReg0b7s2 0x159
  write_reg( memshire_id, 0x62240104, 0x00000047);// write_ddrc_reg 2 INITENG0_SequenceReg0b8s0 0x47
  write_reg( memshire_id, 0x62240108, 0x00000630);// write_ddrc_reg 2 INITENG0_SequenceReg0b8s1 0x630
  write_reg( memshire_id, 0x6224010c, 0x00000149);// write_ddrc_reg 2 INITENG0_SequenceReg0b8s2 0x149
  write_reg( memshire_id, 0x62240110, 0x0000004f);// write_ddrc_reg 2 INITENG0_SequenceReg0b9s0 0x4f
  write_reg( memshire_id, 0x62240114, 0x00000630);// write_ddrc_reg 2 INITENG0_SequenceReg0b9s1 0x630
  write_reg( memshire_id, 0x62240118, 0x00000179);// write_ddrc_reg 2 INITENG0_SequenceReg0b9s2 0x179
  write_reg( memshire_id, 0x6224011c, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b10s0 0x8
  write_reg( memshire_id, 0x62240120, 0x000000e0);// write_ddrc_reg 2 INITENG0_SequenceReg0b10s1 0xe0
  write_reg( memshire_id, 0x62240124, 0x00000109);// write_ddrc_reg 2 INITENG0_SequenceReg0b10s2 0x109
  write_reg( memshire_id, 0x62240128, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b11s0 0x0
  write_reg( memshire_id, 0x6224012c, 0x000007c8);// write_ddrc_reg 2 INITENG0_SequenceReg0b11s1 0x7c8
  write_reg( memshire_id, 0x62240130, 0x00000109);// write_ddrc_reg 2 INITENG0_SequenceReg0b11s2 0x109
  write_reg( memshire_id, 0x62240134, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b12s0 0x0
  write_reg( memshire_id, 0x62240138, 0x00000001);// write_ddrc_reg 2 INITENG0_SequenceReg0b12s1 0x1
  write_reg( memshire_id, 0x6224013c, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b12s2 0x8
  write_reg( memshire_id, 0x62240140, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b13s0 0x0
  write_reg( memshire_id, 0x62240144, 0x0000045a);// write_ddrc_reg 2 INITENG0_SequenceReg0b13s1 0x45a
  write_reg( memshire_id, 0x62240148, 0x00000009);// write_ddrc_reg 2 INITENG0_SequenceReg0b13s2 0x9
  write_reg( memshire_id, 0x6224014c, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b14s0 0x0
  write_reg( memshire_id, 0x62240150, 0x00000448);// write_ddrc_reg 2 INITENG0_SequenceReg0b14s1 0x448
  write_reg( memshire_id, 0x62240154, 0x00000109);// write_ddrc_reg 2 INITENG0_SequenceReg0b14s2 0x109
  write_reg( memshire_id, 0x62240158, 0x00000040);// write_ddrc_reg 2 INITENG0_SequenceReg0b15s0 0x40
  write_reg( memshire_id, 0x6224015c, 0x00000630);// write_ddrc_reg 2 INITENG0_SequenceReg0b15s1 0x630
  write_reg( memshire_id, 0x62240160, 0x00000179);// write_ddrc_reg 2 INITENG0_SequenceReg0b15s2 0x179
  write_reg( memshire_id, 0x62240164, 0x00000001);// write_ddrc_reg 2 INITENG0_SequenceReg0b16s0 0x1
  write_reg( memshire_id, 0x62240168, 0x00000618);// write_ddrc_reg 2 INITENG0_SequenceReg0b16s1 0x618
  write_reg( memshire_id, 0x6224016c, 0x00000109);// write_ddrc_reg 2 INITENG0_SequenceReg0b16s2 0x109
  write_reg( memshire_id, 0x62240170, 0x000040c0);// write_ddrc_reg 2 INITENG0_SequenceReg0b17s0 0x40c0
  write_reg( memshire_id, 0x62240174, 0x00000630);// write_ddrc_reg 2 INITENG0_SequenceReg0b17s1 0x630
  write_reg( memshire_id, 0x62240178, 0x00000149);// write_ddrc_reg 2 INITENG0_SequenceReg0b17s2 0x149
  write_reg( memshire_id, 0x6224017c, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b18s0 0x8
  write_reg( memshire_id, 0x62240180, 0x00000004);// write_ddrc_reg 2 INITENG0_SequenceReg0b18s1 0x4
  write_reg( memshire_id, 0x62240184, 0x00000048);// write_ddrc_reg 2 INITENG0_SequenceReg0b18s2 0x48
  write_reg( memshire_id, 0x62240188, 0x00004040);// write_ddrc_reg 2 INITENG0_SequenceReg0b19s0 0x4040
  write_reg( memshire_id, 0x6224018c, 0x00000630);// write_ddrc_reg 2 INITENG0_SequenceReg0b19s1 0x630
  write_reg( memshire_id, 0x62240190, 0x00000149);// write_ddrc_reg 2 INITENG0_SequenceReg0b19s2 0x149
  write_reg( memshire_id, 0x62240194, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b20s0 0x0
  write_reg( memshire_id, 0x62240198, 0x00000004);// write_ddrc_reg 2 INITENG0_SequenceReg0b20s1 0x4
  write_reg( memshire_id, 0x6224019c, 0x00000048);// write_ddrc_reg 2 INITENG0_SequenceReg0b20s2 0x48
  write_reg( memshire_id, 0x622401a0, 0x00000040);// write_ddrc_reg 2 INITENG0_SequenceReg0b21s0 0x40
  write_reg( memshire_id, 0x622401a4, 0x00000630);// write_ddrc_reg 2 INITENG0_SequenceReg0b21s1 0x630
  write_reg( memshire_id, 0x622401a8, 0x00000149);// write_ddrc_reg 2 INITENG0_SequenceReg0b21s2 0x149
  write_reg( memshire_id, 0x622401ac, 0x00000010);// write_ddrc_reg 2 INITENG0_SequenceReg0b22s0 0x10
  write_reg( memshire_id, 0x622401b0, 0x00000004);// write_ddrc_reg 2 INITENG0_SequenceReg0b22s1 0x4
  write_reg( memshire_id, 0x622401b4, 0x00000018);// write_ddrc_reg 2 INITENG0_SequenceReg0b22s2 0x18
  write_reg( memshire_id, 0x622401b8, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b23s0 0x0
  write_reg( memshire_id, 0x622401bc, 0x00000004);// write_ddrc_reg 2 INITENG0_SequenceReg0b23s1 0x4
  write_reg( memshire_id, 0x622401c0, 0x00000078);// write_ddrc_reg 2 INITENG0_SequenceReg0b23s2 0x78
  write_reg( memshire_id, 0x622401c4, 0x00000549);// write_ddrc_reg 2 INITENG0_SequenceReg0b24s0 0x549
  write_reg( memshire_id, 0x622401c8, 0x00000630);// write_ddrc_reg 2 INITENG0_SequenceReg0b24s1 0x630
  write_reg( memshire_id, 0x622401cc, 0x00000159);// write_ddrc_reg 2 INITENG0_SequenceReg0b24s2 0x159
  write_reg( memshire_id, 0x622401d0, 0x00000d49);// write_ddrc_reg 2 INITENG0_SequenceReg0b25s0 0xd49
  write_reg( memshire_id, 0x622401d4, 0x00000630);// write_ddrc_reg 2 INITENG0_SequenceReg0b25s1 0x630
  write_reg( memshire_id, 0x622401d8, 0x00000159);// write_ddrc_reg 2 INITENG0_SequenceReg0b25s2 0x159
  write_reg( memshire_id, 0x622401dc, 0x0000094a);// write_ddrc_reg 2 INITENG0_SequenceReg0b26s0 0x94a
  write_reg( memshire_id, 0x622401e0, 0x00000630);// write_ddrc_reg 2 INITENG0_SequenceReg0b26s1 0x630
  write_reg( memshire_id, 0x622401e4, 0x00000159);// write_ddrc_reg 2 INITENG0_SequenceReg0b26s2 0x159
  write_reg( memshire_id, 0x622401e8, 0x00000441);// write_ddrc_reg 2 INITENG0_SequenceReg0b27s0 0x441
  write_reg( memshire_id, 0x622401ec, 0x00000630);// write_ddrc_reg 2 INITENG0_SequenceReg0b27s1 0x630
  write_reg( memshire_id, 0x622401f0, 0x00000149);// write_ddrc_reg 2 INITENG0_SequenceReg0b27s2 0x149
  write_reg( memshire_id, 0x622401f4, 0x00000042);// write_ddrc_reg 2 INITENG0_SequenceReg0b28s0 0x42
  write_reg( memshire_id, 0x622401f8, 0x00000630);// write_ddrc_reg 2 INITENG0_SequenceReg0b28s1 0x630
  write_reg( memshire_id, 0x622401fc, 0x00000149);// write_ddrc_reg 2 INITENG0_SequenceReg0b28s2 0x149
  write_reg( memshire_id, 0x62240200, 0x00000001);// write_ddrc_reg 2 INITENG0_SequenceReg0b29s0 0x1
  write_reg( memshire_id, 0x62240204, 0x00000630);// write_ddrc_reg 2 INITENG0_SequenceReg0b29s1 0x630
  write_reg( memshire_id, 0x62240208, 0x00000149);// write_ddrc_reg 2 INITENG0_SequenceReg0b29s2 0x149
  write_reg( memshire_id, 0x6224020c, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b30s0 0x0
  write_reg( memshire_id, 0x62240210, 0x000000e0);// write_ddrc_reg 2 INITENG0_SequenceReg0b30s1 0xe0
  write_reg( memshire_id, 0x62240214, 0x00000109);// write_ddrc_reg 2 INITENG0_SequenceReg0b30s2 0x109
  write_reg( memshire_id, 0x62240218, 0x0000000a);// write_ddrc_reg 2 INITENG0_SequenceReg0b31s0 0xa
  write_reg( memshire_id, 0x6224021c, 0x00000010);// write_ddrc_reg 2 INITENG0_SequenceReg0b31s1 0x10
  write_reg( memshire_id, 0x62240220, 0x00000109);// write_ddrc_reg 2 INITENG0_SequenceReg0b31s2 0x109
  write_reg( memshire_id, 0x62240224, 0x00000009);// write_ddrc_reg 2 INITENG0_SequenceReg0b32s0 0x9
  write_reg( memshire_id, 0x62240228, 0x000003c0);// write_ddrc_reg 2 INITENG0_SequenceReg0b32s1 0x3c0
  write_reg( memshire_id, 0x6224022c, 0x00000149);// write_ddrc_reg 2 INITENG0_SequenceReg0b32s2 0x149
  write_reg( memshire_id, 0x62240230, 0x00000009);// write_ddrc_reg 2 INITENG0_SequenceReg0b33s0 0x9
  write_reg( memshire_id, 0x62240234, 0x000003c0);// write_ddrc_reg 2 INITENG0_SequenceReg0b33s1 0x3c0
  write_reg( memshire_id, 0x62240238, 0x00000159);// write_ddrc_reg 2 INITENG0_SequenceReg0b33s2 0x159
  write_reg( memshire_id, 0x6224023c, 0x00000018);// write_ddrc_reg 2 INITENG0_SequenceReg0b34s0 0x18
  write_reg( memshire_id, 0x62240240, 0x00000010);// write_ddrc_reg 2 INITENG0_SequenceReg0b34s1 0x10
  write_reg( memshire_id, 0x62240244, 0x00000109);// write_ddrc_reg 2 INITENG0_SequenceReg0b34s2 0x109
  write_reg( memshire_id, 0x62240248, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b35s0 0x0
  write_reg( memshire_id, 0x6224024c, 0x000003c0);// write_ddrc_reg 2 INITENG0_SequenceReg0b35s1 0x3c0
  write_reg( memshire_id, 0x62240250, 0x00000109);// write_ddrc_reg 2 INITENG0_SequenceReg0b35s2 0x109
  write_reg( memshire_id, 0x62240254, 0x00000018);// write_ddrc_reg 2 INITENG0_SequenceReg0b36s0 0x18
  write_reg( memshire_id, 0x62240258, 0x00000004);// write_ddrc_reg 2 INITENG0_SequenceReg0b36s1 0x4
  write_reg( memshire_id, 0x6224025c, 0x00000048);// write_ddrc_reg 2 INITENG0_SequenceReg0b36s2 0x48
  write_reg( memshire_id, 0x62240260, 0x00000018);// write_ddrc_reg 2 INITENG0_SequenceReg0b37s0 0x18
  write_reg( memshire_id, 0x62240264, 0x00000004);// write_ddrc_reg 2 INITENG0_SequenceReg0b37s1 0x4
  write_reg( memshire_id, 0x62240268, 0x00000058);// write_ddrc_reg 2 INITENG0_SequenceReg0b37s2 0x58
  write_reg( memshire_id, 0x6224026c, 0x0000000a);// write_ddrc_reg 2 INITENG0_SequenceReg0b38s0 0xa
  write_reg( memshire_id, 0x62240270, 0x00000010);// write_ddrc_reg 2 INITENG0_SequenceReg0b38s1 0x10
  write_reg( memshire_id, 0x62240274, 0x00000109);// write_ddrc_reg 2 INITENG0_SequenceReg0b38s2 0x109
  write_reg( memshire_id, 0x62240278, 0x00000002);// write_ddrc_reg 2 INITENG0_SequenceReg0b39s0 0x2
  write_reg( memshire_id, 0x6224027c, 0x00000010);// write_ddrc_reg 2 INITENG0_SequenceReg0b39s1 0x10
  write_reg( memshire_id, 0x62240280, 0x00000109);// write_ddrc_reg 2 INITENG0_SequenceReg0b39s2 0x109
  write_reg( memshire_id, 0x62240284, 0x00000005);// write_ddrc_reg 2 INITENG0_SequenceReg0b40s0 0x5
  write_reg( memshire_id, 0x62240288, 0x000007c0);// write_ddrc_reg 2 INITENG0_SequenceReg0b40s1 0x7c0
  write_reg( memshire_id, 0x6224028c, 0x00000109);// write_ddrc_reg 2 INITENG0_SequenceReg0b40s2 0x109
  write_reg( memshire_id, 0x62100000, 0x00000811);// write_ddrc_reg 2 ACSM0_AcsmSeq0x0 0x811
  write_reg( memshire_id, 0x62100080, 0x00000880);// write_ddrc_reg 2 ACSM0_AcsmSeq1x0 0x880
  write_reg( memshire_id, 0x62100100, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq2x0 0x0
  write_reg( memshire_id, 0x62100180, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq3x0 0x0
  write_reg( memshire_id, 0x62100004, 0x00004008);// write_ddrc_reg 2 ACSM0_AcsmSeq0x1 0x4008
  write_reg( memshire_id, 0x62100084, 0x00000083);// write_ddrc_reg 2 ACSM0_AcsmSeq1x1 0x83
  write_reg( memshire_id, 0x62100104, 0x0000004f);// write_ddrc_reg 2 ACSM0_AcsmSeq2x1 0x4f
  write_reg( memshire_id, 0x62100184, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq3x1 0x0
  write_reg( memshire_id, 0x62100008, 0x00004040);// write_ddrc_reg 2 ACSM0_AcsmSeq0x2 0x4040
  write_reg( memshire_id, 0x62100088, 0x00000083);// write_ddrc_reg 2 ACSM0_AcsmSeq1x2 0x83
  write_reg( memshire_id, 0x62100108, 0x00000051);// write_ddrc_reg 2 ACSM0_AcsmSeq2x2 0x51
  write_reg( memshire_id, 0x62100188, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq3x2 0x0
  write_reg( memshire_id, 0x6210000c, 0x00000811);// write_ddrc_reg 2 ACSM0_AcsmSeq0x3 0x811
  write_reg( memshire_id, 0x6210008c, 0x00000880);// write_ddrc_reg 2 ACSM0_AcsmSeq1x3 0x880
  write_reg( memshire_id, 0x6210010c, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq2x3 0x0
  write_reg( memshire_id, 0x6210018c, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq3x3 0x0
  write_reg( memshire_id, 0x62100010, 0x00000720);// write_ddrc_reg 2 ACSM0_AcsmSeq0x4 0x720
  write_reg( memshire_id, 0x62100090, 0x0000000f);// write_ddrc_reg 2 ACSM0_AcsmSeq1x4 0xf
  write_reg( memshire_id, 0x62100110, 0x00001740);// write_ddrc_reg 2 ACSM0_AcsmSeq2x4 0x1740
  write_reg( memshire_id, 0x62100190, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq3x4 0x0
  write_reg( memshire_id, 0x62100014, 0x00000016);// write_ddrc_reg 2 ACSM0_AcsmSeq0x5 0x16
  write_reg( memshire_id, 0x62100094, 0x00000083);// write_ddrc_reg 2 ACSM0_AcsmSeq1x5 0x83
  write_reg( memshire_id, 0x62100114, 0x0000004b);// write_ddrc_reg 2 ACSM0_AcsmSeq2x5 0x4b
  write_reg( memshire_id, 0x62100194, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq3x5 0x0
  write_reg( memshire_id, 0x62100018, 0x00000716);// write_ddrc_reg 2 ACSM0_AcsmSeq0x6 0x716
  write_reg( memshire_id, 0x62100098, 0x0000000f);// write_ddrc_reg 2 ACSM0_AcsmSeq1x6 0xf
  write_reg( memshire_id, 0x62100118, 0x00002001);// write_ddrc_reg 2 ACSM0_AcsmSeq2x6 0x2001
  write_reg( memshire_id, 0x62100198, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq3x6 0x0
  write_reg( memshire_id, 0x6210001c, 0x00000716);// write_ddrc_reg 2 ACSM0_AcsmSeq0x7 0x716
  write_reg( memshire_id, 0x6210009c, 0x0000000f);// write_ddrc_reg 2 ACSM0_AcsmSeq1x7 0xf
  write_reg( memshire_id, 0x6210011c, 0x00002800);// write_ddrc_reg 2 ACSM0_AcsmSeq2x7 0x2800
  write_reg( memshire_id, 0x6210019c, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq3x7 0x0
  write_reg( memshire_id, 0x62100020, 0x00000716);// write_ddrc_reg 2 ACSM0_AcsmSeq0x8 0x716
  write_reg( memshire_id, 0x621000a0, 0x0000000f);// write_ddrc_reg 2 ACSM0_AcsmSeq1x8 0xf
  write_reg( memshire_id, 0x62100120, 0x00000f00);// write_ddrc_reg 2 ACSM0_AcsmSeq2x8 0xf00
  write_reg( memshire_id, 0x621001a0, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq3x8 0x0
  write_reg( memshire_id, 0x62100024, 0x00000720);// write_ddrc_reg 2 ACSM0_AcsmSeq0x9 0x720
  write_reg( memshire_id, 0x621000a4, 0x0000000f);// write_ddrc_reg 2 ACSM0_AcsmSeq1x9 0xf
  write_reg( memshire_id, 0x62100124, 0x00001400);// write_ddrc_reg 2 ACSM0_AcsmSeq2x9 0x1400
  write_reg( memshire_id, 0x621001a4, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq3x9 0x0
  write_reg( memshire_id, 0x62100028, 0x00000e08);// write_ddrc_reg 2 ACSM0_AcsmSeq0x10 0xe08
  write_reg( memshire_id, 0x621000a8, 0x00000c15);// write_ddrc_reg 2 ACSM0_AcsmSeq1x10 0xc15
  write_reg( memshire_id, 0x62100128, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq2x10 0x0
  write_reg( memshire_id, 0x621001a8, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq3x10 0x0
  write_reg( memshire_id, 0x6210002c, 0x00000623);// write_ddrc_reg 2 ACSM0_AcsmSeq0x11 0x623
  write_reg( memshire_id, 0x621000ac, 0x00000015);// write_ddrc_reg 2 ACSM0_AcsmSeq1x11 0x15
  write_reg( memshire_id, 0x6210012c, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq2x11 0x0
  write_reg( memshire_id, 0x621001ac, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq3x11 0x0
  write_reg( memshire_id, 0x62100030, 0x00004028);// write_ddrc_reg 2 ACSM0_AcsmSeq0x12 0x4028
  write_reg( memshire_id, 0x621000b0, 0x00000080);// write_ddrc_reg 2 ACSM0_AcsmSeq1x12 0x80
  write_reg( memshire_id, 0x62100130, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq2x12 0x0
  write_reg( memshire_id, 0x621001b0, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq3x12 0x0
  write_reg( memshire_id, 0x62100034, 0x00000e08);// write_ddrc_reg 2 ACSM0_AcsmSeq0x13 0xe08
  write_reg( memshire_id, 0x621000b4, 0x00000c1a);// write_ddrc_reg 2 ACSM0_AcsmSeq1x13 0xc1a
  write_reg( memshire_id, 0x62100134, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq2x13 0x0
  write_reg( memshire_id, 0x621001b4, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq3x13 0x0
  write_reg( memshire_id, 0x62100038, 0x00000623);// write_ddrc_reg 2 ACSM0_AcsmSeq0x14 0x623
  write_reg( memshire_id, 0x621000b8, 0x0000001a);// write_ddrc_reg 2 ACSM0_AcsmSeq1x14 0x1a
  write_reg( memshire_id, 0x62100138, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq2x14 0x0
  write_reg( memshire_id, 0x621001b8, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq3x14 0x0
  write_reg( memshire_id, 0x6210003c, 0x00004040);// write_ddrc_reg 2 ACSM0_AcsmSeq0x15 0x4040
  write_reg( memshire_id, 0x621000bc, 0x00000080);// write_ddrc_reg 2 ACSM0_AcsmSeq1x15 0x80
  write_reg( memshire_id, 0x6210013c, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq2x15 0x0
  write_reg( memshire_id, 0x621001bc, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq3x15 0x0
  write_reg( memshire_id, 0x62100040, 0x00002604);// write_ddrc_reg 2 ACSM0_AcsmSeq0x16 0x2604
  write_reg( memshire_id, 0x621000c0, 0x00000015);// write_ddrc_reg 2 ACSM0_AcsmSeq1x16 0x15
  write_reg( memshire_id, 0x62100140, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq2x16 0x0
  write_reg( memshire_id, 0x621001c0, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq3x16 0x0
  write_reg( memshire_id, 0x62100044, 0x00000708);// write_ddrc_reg 2 ACSM0_AcsmSeq0x17 0x708
  write_reg( memshire_id, 0x621000c4, 0x00000005);// write_ddrc_reg 2 ACSM0_AcsmSeq1x17 0x5
  write_reg( memshire_id, 0x62100144, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq2x17 0x0
  write_reg( memshire_id, 0x621001c4, 0x00002002);// write_ddrc_reg 2 ACSM0_AcsmSeq3x17 0x2002
  write_reg( memshire_id, 0x62100048, 0x00000008);// write_ddrc_reg 2 ACSM0_AcsmSeq0x18 0x8
  write_reg( memshire_id, 0x621000c8, 0x00000080);// write_ddrc_reg 2 ACSM0_AcsmSeq1x18 0x80
  write_reg( memshire_id, 0x62100148, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq2x18 0x0
  write_reg( memshire_id, 0x621001c8, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq3x18 0x0
  write_reg( memshire_id, 0x6210004c, 0x00002604);// write_ddrc_reg 2 ACSM0_AcsmSeq0x19 0x2604
  write_reg( memshire_id, 0x621000cc, 0x0000001a);// write_ddrc_reg 2 ACSM0_AcsmSeq1x19 0x1a
  write_reg( memshire_id, 0x6210014c, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq2x19 0x0
  write_reg( memshire_id, 0x621001cc, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq3x19 0x0
  write_reg( memshire_id, 0x62100050, 0x00000708);// write_ddrc_reg 2 ACSM0_AcsmSeq0x20 0x708
  write_reg( memshire_id, 0x621000d0, 0x0000000a);// write_ddrc_reg 2 ACSM0_AcsmSeq1x20 0xa
  write_reg( memshire_id, 0x62100150, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq2x20 0x0
  write_reg( memshire_id, 0x621001d0, 0x00002002);// write_ddrc_reg 2 ACSM0_AcsmSeq3x20 0x2002
  write_reg( memshire_id, 0x62100054, 0x00004040);// write_ddrc_reg 2 ACSM0_AcsmSeq0x21 0x4040
  write_reg( memshire_id, 0x621000d4, 0x00000080);// write_ddrc_reg 2 ACSM0_AcsmSeq1x21 0x80
  write_reg( memshire_id, 0x62100154, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq2x21 0x0
  write_reg( memshire_id, 0x621001d4, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq3x21 0x0
  write_reg( memshire_id, 0x62100058, 0x0000060a);// write_ddrc_reg 2 ACSM0_AcsmSeq0x22 0x60a
  write_reg( memshire_id, 0x621000d8, 0x00000015);// write_ddrc_reg 2 ACSM0_AcsmSeq1x22 0x15
  write_reg( memshire_id, 0x62100158, 0x00001200);// write_ddrc_reg 2 ACSM0_AcsmSeq2x22 0x1200
  write_reg( memshire_id, 0x621001d8, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq3x22 0x0
  write_reg( memshire_id, 0x6210005c, 0x0000061a);// write_ddrc_reg 2 ACSM0_AcsmSeq0x23 0x61a
  write_reg( memshire_id, 0x621000dc, 0x00000015);// write_ddrc_reg 2 ACSM0_AcsmSeq1x23 0x15
  write_reg( memshire_id, 0x6210015c, 0x00001300);// write_ddrc_reg 2 ACSM0_AcsmSeq2x23 0x1300
  write_reg( memshire_id, 0x621001dc, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq3x23 0x0
  write_reg( memshire_id, 0x62100060, 0x0000060a);// write_ddrc_reg 2 ACSM0_AcsmSeq0x24 0x60a
  write_reg( memshire_id, 0x621000e0, 0x0000001a);// write_ddrc_reg 2 ACSM0_AcsmSeq1x24 0x1a
  write_reg( memshire_id, 0x62100160, 0x00001200);// write_ddrc_reg 2 ACSM0_AcsmSeq2x24 0x1200
  write_reg( memshire_id, 0x621001e0, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq3x24 0x0
  write_reg( memshire_id, 0x62100064, 0x00000642);// write_ddrc_reg 2 ACSM0_AcsmSeq0x25 0x642
  write_reg( memshire_id, 0x621000e4, 0x0000001a);// write_ddrc_reg 2 ACSM0_AcsmSeq1x25 0x1a
  write_reg( memshire_id, 0x62100164, 0x00001300);// write_ddrc_reg 2 ACSM0_AcsmSeq2x25 0x1300
  write_reg( memshire_id, 0x621001e4, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq3x25 0x0
  write_reg( memshire_id, 0x62100068, 0x00004808);// write_ddrc_reg 2 ACSM0_AcsmSeq0x26 0x4808
  write_reg( memshire_id, 0x621000e8, 0x00000880);// write_ddrc_reg 2 ACSM0_AcsmSeq1x26 0x880
  write_reg( memshire_id, 0x62100168, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq2x26 0x0
  write_reg( memshire_id, 0x621001e8, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmSeq3x26 0x0
  write_reg( memshire_id, 0x62240290, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b41s0 0x0
  write_reg( memshire_id, 0x62240294, 0x00000790);// write_ddrc_reg 2 INITENG0_SequenceReg0b41s1 0x790
  write_reg( memshire_id, 0x62240298, 0x0000011a);// write_ddrc_reg 2 INITENG0_SequenceReg0b41s2 0x11a
  write_reg( memshire_id, 0x6224029c, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b42s0 0x8
  write_reg( memshire_id, 0x622402a0, 0x000007aa);// write_ddrc_reg 2 INITENG0_SequenceReg0b42s1 0x7aa
  write_reg( memshire_id, 0x622402a4, 0x0000002a);// write_ddrc_reg 2 INITENG0_SequenceReg0b42s2 0x2a
  write_reg( memshire_id, 0x622402a8, 0x00000010);// write_ddrc_reg 2 INITENG0_SequenceReg0b43s0 0x10
  write_reg( memshire_id, 0x622402ac, 0x000007b2);// write_ddrc_reg 2 INITENG0_SequenceReg0b43s1 0x7b2
  write_reg( memshire_id, 0x622402b0, 0x0000002a);// write_ddrc_reg 2 INITENG0_SequenceReg0b43s2 0x2a
  write_reg( memshire_id, 0x622402b4, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b44s0 0x0
  write_reg( memshire_id, 0x622402b8, 0x000007c8);// write_ddrc_reg 2 INITENG0_SequenceReg0b44s1 0x7c8
  write_reg( memshire_id, 0x622402bc, 0x00000109);// write_ddrc_reg 2 INITENG0_SequenceReg0b44s2 0x109
  write_reg( memshire_id, 0x622402c0, 0x00000010);// write_ddrc_reg 2 INITENG0_SequenceReg0b45s0 0x10
  write_reg( memshire_id, 0x622402c4, 0x00000010);// write_ddrc_reg 2 INITENG0_SequenceReg0b45s1 0x10
  write_reg( memshire_id, 0x622402c8, 0x00000109);// write_ddrc_reg 2 INITENG0_SequenceReg0b45s2 0x109
  write_reg( memshire_id, 0x622402cc, 0x00000010);// write_ddrc_reg 2 INITENG0_SequenceReg0b46s0 0x10
  write_reg( memshire_id, 0x622402d0, 0x000002a8);// write_ddrc_reg 2 INITENG0_SequenceReg0b46s1 0x2a8
  write_reg( memshire_id, 0x622402d4, 0x00000129);// write_ddrc_reg 2 INITENG0_SequenceReg0b46s2 0x129
  write_reg( memshire_id, 0x622402d8, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b47s0 0x8
  write_reg( memshire_id, 0x622402dc, 0x00000370);// write_ddrc_reg 2 INITENG0_SequenceReg0b47s1 0x370
  write_reg( memshire_id, 0x622402e0, 0x00000129);// write_ddrc_reg 2 INITENG0_SequenceReg0b47s2 0x129
  write_reg( memshire_id, 0x622402e4, 0x0000000a);// write_ddrc_reg 2 INITENG0_SequenceReg0b48s0 0xa
  write_reg( memshire_id, 0x622402e8, 0x000003c8);// write_ddrc_reg 2 INITENG0_SequenceReg0b48s1 0x3c8
  write_reg( memshire_id, 0x622402ec, 0x000001a9);// write_ddrc_reg 2 INITENG0_SequenceReg0b48s2 0x1a9
  write_reg( memshire_id, 0x622402f0, 0x0000000c);// write_ddrc_reg 2 INITENG0_SequenceReg0b49s0 0xc
  write_reg( memshire_id, 0x622402f4, 0x00000408);// write_ddrc_reg 2 INITENG0_SequenceReg0b49s1 0x408
  write_reg( memshire_id, 0x622402f8, 0x00000199);// write_ddrc_reg 2 INITENG0_SequenceReg0b49s2 0x199
  write_reg( memshire_id, 0x622402fc, 0x00000014);// write_ddrc_reg 2 INITENG0_SequenceReg0b50s0 0x14
  write_reg( memshire_id, 0x62240300, 0x00000790);// write_ddrc_reg 2 INITENG0_SequenceReg0b50s1 0x790
  write_reg( memshire_id, 0x62240304, 0x0000011a);// write_ddrc_reg 2 INITENG0_SequenceReg0b50s2 0x11a
  write_reg( memshire_id, 0x62240308, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b51s0 0x8
  write_reg( memshire_id, 0x6224030c, 0x00000004);// write_ddrc_reg 2 INITENG0_SequenceReg0b51s1 0x4
  write_reg( memshire_id, 0x62240310, 0x00000018);// write_ddrc_reg 2 INITENG0_SequenceReg0b51s2 0x18
  write_reg( memshire_id, 0x62240314, 0x0000000e);// write_ddrc_reg 2 INITENG0_SequenceReg0b52s0 0xe
  write_reg( memshire_id, 0x62240318, 0x00000408);// write_ddrc_reg 2 INITENG0_SequenceReg0b52s1 0x408
  write_reg( memshire_id, 0x6224031c, 0x00000199);// write_ddrc_reg 2 INITENG0_SequenceReg0b52s2 0x199
  write_reg( memshire_id, 0x62240320, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b53s0 0x8
  write_reg( memshire_id, 0x62240324, 0x00008568);// write_ddrc_reg 2 INITENG0_SequenceReg0b53s1 0x8568
  write_reg( memshire_id, 0x62240328, 0x00000108);// write_ddrc_reg 2 INITENG0_SequenceReg0b53s2 0x108
  write_reg( memshire_id, 0x6224032c, 0x00000018);// write_ddrc_reg 2 INITENG0_SequenceReg0b54s0 0x18
  write_reg( memshire_id, 0x62240330, 0x00000790);// write_ddrc_reg 2 INITENG0_SequenceReg0b54s1 0x790
  write_reg( memshire_id, 0x62240334, 0x0000016a);// write_ddrc_reg 2 INITENG0_SequenceReg0b54s2 0x16a
  write_reg( memshire_id, 0x62240338, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b55s0 0x8
  write_reg( memshire_id, 0x6224033c, 0x000001d8);// write_ddrc_reg 2 INITENG0_SequenceReg0b55s1 0x1d8
  write_reg( memshire_id, 0x62240340, 0x00000169);// write_ddrc_reg 2 INITENG0_SequenceReg0b55s2 0x169
  write_reg( memshire_id, 0x62240344, 0x00000010);// write_ddrc_reg 2 INITENG0_SequenceReg0b56s0 0x10
  write_reg( memshire_id, 0x62240348, 0x00008558);// write_ddrc_reg 2 INITENG0_SequenceReg0b56s1 0x8558
  write_reg( memshire_id, 0x6224034c, 0x00000168);// write_ddrc_reg 2 INITENG0_SequenceReg0b56s2 0x168
  write_reg( memshire_id, 0x62240350, 0x00000070);// write_ddrc_reg 2 INITENG0_SequenceReg0b57s0 0x70
  write_reg( memshire_id, 0x62240354, 0x00000788);// write_ddrc_reg 2 INITENG0_SequenceReg0b57s1 0x788
  write_reg( memshire_id, 0x62240358, 0x0000016a);// write_ddrc_reg 2 INITENG0_SequenceReg0b57s2 0x16a
  write_reg( memshire_id, 0x6224035c, 0x00001ff8);// write_ddrc_reg 2 INITENG0_SequenceReg0b58s0 0x1ff8
  write_reg( memshire_id, 0x62240360, 0x000085a8);// write_ddrc_reg 2 INITENG0_SequenceReg0b58s1 0x85a8
  write_reg( memshire_id, 0x62240364, 0x000001e8);// write_ddrc_reg 2 INITENG0_SequenceReg0b58s2 0x1e8
  write_reg( memshire_id, 0x62240368, 0x00000050);// write_ddrc_reg 2 INITENG0_SequenceReg0b59s0 0x50
  write_reg( memshire_id, 0x6224036c, 0x00000798);// write_ddrc_reg 2 INITENG0_SequenceReg0b59s1 0x798
  write_reg( memshire_id, 0x62240370, 0x0000016a);// write_ddrc_reg 2 INITENG0_SequenceReg0b59s2 0x16a
  write_reg( memshire_id, 0x62240374, 0x00000060);// write_ddrc_reg 2 INITENG0_SequenceReg0b60s0 0x60
  write_reg( memshire_id, 0x62240378, 0x000007a0);// write_ddrc_reg 2 INITENG0_SequenceReg0b60s1 0x7a0
  write_reg( memshire_id, 0x6224037c, 0x0000016a);// write_ddrc_reg 2 INITENG0_SequenceReg0b60s2 0x16a
  write_reg( memshire_id, 0x62240380, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b61s0 0x8
  write_reg( memshire_id, 0x62240384, 0x00008310);// write_ddrc_reg 2 INITENG0_SequenceReg0b61s1 0x8310
  write_reg( memshire_id, 0x62240388, 0x00000168);// write_ddrc_reg 2 INITENG0_SequenceReg0b61s2 0x168
  write_reg( memshire_id, 0x6224038c, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b62s0 0x8
  write_reg( memshire_id, 0x62240390, 0x0000a310);// write_ddrc_reg 2 INITENG0_SequenceReg0b62s1 0xa310
  write_reg( memshire_id, 0x62240394, 0x00000168);// write_ddrc_reg 2 INITENG0_SequenceReg0b62s2 0x168
  write_reg( memshire_id, 0x62240398, 0x0000000a);// write_ddrc_reg 2 INITENG0_SequenceReg0b63s0 0xa
  write_reg( memshire_id, 0x6224039c, 0x00000408);// write_ddrc_reg 2 INITENG0_SequenceReg0b63s1 0x408
  write_reg( memshire_id, 0x622403a0, 0x00000169);// write_ddrc_reg 2 INITENG0_SequenceReg0b63s2 0x169
  write_reg( memshire_id, 0x622403a4, 0x0000006e);// write_ddrc_reg 2 INITENG0_SequenceReg0b64s0 0x6e
  write_reg( memshire_id, 0x622403a8, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b64s1 0x0
  write_reg( memshire_id, 0x622403ac, 0x00000068);// write_ddrc_reg 2 INITENG0_SequenceReg0b64s2 0x68
  write_reg( memshire_id, 0x622403b0, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b65s0 0x0
  write_reg( memshire_id, 0x622403b4, 0x00000408);// write_ddrc_reg 2 INITENG0_SequenceReg0b65s1 0x408
  write_reg( memshire_id, 0x622403b8, 0x00000169);// write_ddrc_reg 2 INITENG0_SequenceReg0b65s2 0x169
  write_reg( memshire_id, 0x622403bc, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b66s0 0x0
  write_reg( memshire_id, 0x622403c0, 0x00008310);// write_ddrc_reg 2 INITENG0_SequenceReg0b66s1 0x8310
  write_reg( memshire_id, 0x622403c4, 0x00000168);// write_ddrc_reg 2 INITENG0_SequenceReg0b66s2 0x168
  write_reg( memshire_id, 0x622403c8, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b67s0 0x0
  write_reg( memshire_id, 0x622403cc, 0x0000a310);// write_ddrc_reg 2 INITENG0_SequenceReg0b67s1 0xa310
  write_reg( memshire_id, 0x622403d0, 0x00000168);// write_ddrc_reg 2 INITENG0_SequenceReg0b67s2 0x168
  write_reg( memshire_id, 0x622403d4, 0x00001ff8);// write_ddrc_reg 2 INITENG0_SequenceReg0b68s0 0x1ff8
  write_reg( memshire_id, 0x622403d8, 0x000085a8);// write_ddrc_reg 2 INITENG0_SequenceReg0b68s1 0x85a8
  write_reg( memshire_id, 0x622403dc, 0x000001e8);// write_ddrc_reg 2 INITENG0_SequenceReg0b68s2 0x1e8
  write_reg( memshire_id, 0x622403e0, 0x00000068);// write_ddrc_reg 2 INITENG0_SequenceReg0b69s0 0x68
  write_reg( memshire_id, 0x622403e4, 0x00000798);// write_ddrc_reg 2 INITENG0_SequenceReg0b69s1 0x798
  write_reg( memshire_id, 0x622403e8, 0x0000016a);// write_ddrc_reg 2 INITENG0_SequenceReg0b69s2 0x16a
  write_reg( memshire_id, 0x622403ec, 0x00000078);// write_ddrc_reg 2 INITENG0_SequenceReg0b70s0 0x78
  write_reg( memshire_id, 0x622403f0, 0x000007a0);// write_ddrc_reg 2 INITENG0_SequenceReg0b70s1 0x7a0
  write_reg( memshire_id, 0x622403f4, 0x0000016a);// write_ddrc_reg 2 INITENG0_SequenceReg0b70s2 0x16a
  write_reg( memshire_id, 0x622403f8, 0x00000068);// write_ddrc_reg 2 INITENG0_SequenceReg0b71s0 0x68
  write_reg( memshire_id, 0x622403fc, 0x00000790);// write_ddrc_reg 2 INITENG0_SequenceReg0b71s1 0x790
  write_reg( memshire_id, 0x62240400, 0x0000016a);// write_ddrc_reg 2 INITENG0_SequenceReg0b71s2 0x16a
  write_reg( memshire_id, 0x62240404, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b72s0 0x8
  write_reg( memshire_id, 0x62240408, 0x00008b10);// write_ddrc_reg 2 INITENG0_SequenceReg0b72s1 0x8b10
  write_reg( memshire_id, 0x6224040c, 0x00000168);// write_ddrc_reg 2 INITENG0_SequenceReg0b72s2 0x168
  write_reg( memshire_id, 0x62240410, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b73s0 0x8
  write_reg( memshire_id, 0x62240414, 0x0000ab10);// write_ddrc_reg 2 INITENG0_SequenceReg0b73s1 0xab10
  write_reg( memshire_id, 0x62240418, 0x00000168);// write_ddrc_reg 2 INITENG0_SequenceReg0b73s2 0x168
  write_reg( memshire_id, 0x6224041c, 0x0000000a);// write_ddrc_reg 2 INITENG0_SequenceReg0b74s0 0xa
  write_reg( memshire_id, 0x62240420, 0x00000408);// write_ddrc_reg 2 INITENG0_SequenceReg0b74s1 0x408
  write_reg( memshire_id, 0x62240424, 0x00000169);// write_ddrc_reg 2 INITENG0_SequenceReg0b74s2 0x169
  write_reg( memshire_id, 0x62240428, 0x00000058);// write_ddrc_reg 2 INITENG0_SequenceReg0b75s0 0x58
  write_reg( memshire_id, 0x6224042c, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b75s1 0x0
  write_reg( memshire_id, 0x62240430, 0x00000068);// write_ddrc_reg 2 INITENG0_SequenceReg0b75s2 0x68
  write_reg( memshire_id, 0x62240434, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b76s0 0x0
  write_reg( memshire_id, 0x62240438, 0x00000408);// write_ddrc_reg 2 INITENG0_SequenceReg0b76s1 0x408
  write_reg( memshire_id, 0x6224043c, 0x00000169);// write_ddrc_reg 2 INITENG0_SequenceReg0b76s2 0x169
  write_reg( memshire_id, 0x62240440, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b77s0 0x0
  write_reg( memshire_id, 0x62240444, 0x00008b10);// write_ddrc_reg 2 INITENG0_SequenceReg0b77s1 0x8b10
  write_reg( memshire_id, 0x62240448, 0x00000168);// write_ddrc_reg 2 INITENG0_SequenceReg0b77s2 0x168
  write_reg( memshire_id, 0x6224044c, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b78s0 0x0
  write_reg( memshire_id, 0x62240450, 0x0000ab10);// write_ddrc_reg 2 INITENG0_SequenceReg0b78s1 0xab10
  write_reg( memshire_id, 0x62240454, 0x00000168);// write_ddrc_reg 2 INITENG0_SequenceReg0b78s2 0x168
  write_reg( memshire_id, 0x62240458, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b79s0 0x0
  write_reg( memshire_id, 0x6224045c, 0x000001d8);// write_ddrc_reg 2 INITENG0_SequenceReg0b79s1 0x1d8
  write_reg( memshire_id, 0x62240460, 0x00000169);// write_ddrc_reg 2 INITENG0_SequenceReg0b79s2 0x169
  write_reg( memshire_id, 0x62240464, 0x00000080);// write_ddrc_reg 2 INITENG0_SequenceReg0b80s0 0x80
  write_reg( memshire_id, 0x62240468, 0x00000790);// write_ddrc_reg 2 INITENG0_SequenceReg0b80s1 0x790
  write_reg( memshire_id, 0x6224046c, 0x0000016a);// write_ddrc_reg 2 INITENG0_SequenceReg0b80s2 0x16a
  write_reg( memshire_id, 0x62240470, 0x00000018);// write_ddrc_reg 2 INITENG0_SequenceReg0b81s0 0x18
  write_reg( memshire_id, 0x62240474, 0x000007aa);// write_ddrc_reg 2 INITENG0_SequenceReg0b81s1 0x7aa
  write_reg( memshire_id, 0x62240478, 0x0000006a);// write_ddrc_reg 2 INITENG0_SequenceReg0b81s2 0x6a
  write_reg( memshire_id, 0x6224047c, 0x0000000a);// write_ddrc_reg 2 INITENG0_SequenceReg0b82s0 0xa
  write_reg( memshire_id, 0x62240480, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b82s1 0x0
  write_reg( memshire_id, 0x62240484, 0x000001e9);// write_ddrc_reg 2 INITENG0_SequenceReg0b82s2 0x1e9
  write_reg( memshire_id, 0x62240488, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b83s0 0x8
  write_reg( memshire_id, 0x6224048c, 0x00008080);// write_ddrc_reg 2 INITENG0_SequenceReg0b83s1 0x8080
  write_reg( memshire_id, 0x62240490, 0x00000108);// write_ddrc_reg 2 INITENG0_SequenceReg0b83s2 0x108
  write_reg( memshire_id, 0x62240494, 0x0000000f);// write_ddrc_reg 2 INITENG0_SequenceReg0b84s0 0xf
  write_reg( memshire_id, 0x62240498, 0x00000408);// write_ddrc_reg 2 INITENG0_SequenceReg0b84s1 0x408
  write_reg( memshire_id, 0x6224049c, 0x00000169);// write_ddrc_reg 2 INITENG0_SequenceReg0b84s2 0x169
  write_reg( memshire_id, 0x622404a0, 0x0000000c);// write_ddrc_reg 2 INITENG0_SequenceReg0b85s0 0xc
  write_reg( memshire_id, 0x622404a4, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b85s1 0x0
  write_reg( memshire_id, 0x622404a8, 0x00000068);// write_ddrc_reg 2 INITENG0_SequenceReg0b85s2 0x68
  write_reg( memshire_id, 0x622404ac, 0x00000009);// write_ddrc_reg 2 INITENG0_SequenceReg0b86s0 0x9
  write_reg( memshire_id, 0x622404b0, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b86s1 0x0
  write_reg( memshire_id, 0x622404b4, 0x000001a9);// write_ddrc_reg 2 INITENG0_SequenceReg0b86s2 0x1a9
  write_reg( memshire_id, 0x622404b8, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b87s0 0x0
  write_reg( memshire_id, 0x622404bc, 0x00000408);// write_ddrc_reg 2 INITENG0_SequenceReg0b87s1 0x408
  write_reg( memshire_id, 0x622404c0, 0x00000169);// write_ddrc_reg 2 INITENG0_SequenceReg0b87s2 0x169
  write_reg( memshire_id, 0x622404c4, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b88s0 0x0
  write_reg( memshire_id, 0x622404c8, 0x00008080);// write_ddrc_reg 2 INITENG0_SequenceReg0b88s1 0x8080
  write_reg( memshire_id, 0x622404cc, 0x00000108);// write_ddrc_reg 2 INITENG0_SequenceReg0b88s2 0x108
  write_reg( memshire_id, 0x622404d0, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b89s0 0x8
  write_reg( memshire_id, 0x622404d4, 0x000007aa);// write_ddrc_reg 2 INITENG0_SequenceReg0b89s1 0x7aa
  write_reg( memshire_id, 0x622404d8, 0x0000006a);// write_ddrc_reg 2 INITENG0_SequenceReg0b89s2 0x6a
  write_reg( memshire_id, 0x622404dc, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b90s0 0x0
  write_reg( memshire_id, 0x622404e0, 0x00008568);// write_ddrc_reg 2 INITENG0_SequenceReg0b90s1 0x8568
  write_reg( memshire_id, 0x622404e4, 0x00000108);// write_ddrc_reg 2 INITENG0_SequenceReg0b90s2 0x108
  write_reg( memshire_id, 0x622404e8, 0x000000b7);// write_ddrc_reg 2 INITENG0_SequenceReg0b91s0 0xb7
  write_reg( memshire_id, 0x622404ec, 0x00000790);// write_ddrc_reg 2 INITENG0_SequenceReg0b91s1 0x790
  write_reg( memshire_id, 0x622404f0, 0x0000016a);// write_ddrc_reg 2 INITENG0_SequenceReg0b91s2 0x16a
  write_reg( memshire_id, 0x622404f4, 0x0000001f);// write_ddrc_reg 2 INITENG0_SequenceReg0b92s0 0x1f
  write_reg( memshire_id, 0x622404f8, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b92s1 0x0
  write_reg( memshire_id, 0x622404fc, 0x00000068);// write_ddrc_reg 2 INITENG0_SequenceReg0b92s2 0x68
  write_reg( memshire_id, 0x62240500, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b93s0 0x8
  write_reg( memshire_id, 0x62240504, 0x00008558);// write_ddrc_reg 2 INITENG0_SequenceReg0b93s1 0x8558
  write_reg( memshire_id, 0x62240508, 0x00000168);// write_ddrc_reg 2 INITENG0_SequenceReg0b93s2 0x168
  write_reg( memshire_id, 0x6224050c, 0x0000000f);// write_ddrc_reg 2 INITENG0_SequenceReg0b94s0 0xf
  write_reg( memshire_id, 0x62240510, 0x00000408);// write_ddrc_reg 2 INITENG0_SequenceReg0b94s1 0x408
  write_reg( memshire_id, 0x62240514, 0x00000169);// write_ddrc_reg 2 INITENG0_SequenceReg0b94s2 0x169
  write_reg( memshire_id, 0x62240518, 0x0000000c);// write_ddrc_reg 2 INITENG0_SequenceReg0b95s0 0xc
  write_reg( memshire_id, 0x6224051c, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b95s1 0x0
  write_reg( memshire_id, 0x62240520, 0x00000068);// write_ddrc_reg 2 INITENG0_SequenceReg0b95s2 0x68
  write_reg( memshire_id, 0x62240524, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b96s0 0x0
  write_reg( memshire_id, 0x62240528, 0x00000408);// write_ddrc_reg 2 INITENG0_SequenceReg0b96s1 0x408
  write_reg( memshire_id, 0x6224052c, 0x00000169);// write_ddrc_reg 2 INITENG0_SequenceReg0b96s2 0x169
  write_reg( memshire_id, 0x62240530, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b97s0 0x0
  write_reg( memshire_id, 0x62240534, 0x00008558);// write_ddrc_reg 2 INITENG0_SequenceReg0b97s1 0x8558
  write_reg( memshire_id, 0x62240538, 0x00000168);// write_ddrc_reg 2 INITENG0_SequenceReg0b97s2 0x168
  write_reg( memshire_id, 0x6224053c, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b98s0 0x8
  write_reg( memshire_id, 0x62240540, 0x000003c8);// write_ddrc_reg 2 INITENG0_SequenceReg0b98s1 0x3c8
  write_reg( memshire_id, 0x62240544, 0x000001a9);// write_ddrc_reg 2 INITENG0_SequenceReg0b98s2 0x1a9
  write_reg( memshire_id, 0x62240548, 0x00000003);// write_ddrc_reg 2 INITENG0_SequenceReg0b99s0 0x3
  write_reg( memshire_id, 0x6224054c, 0x00000370);// write_ddrc_reg 2 INITENG0_SequenceReg0b99s1 0x370
  write_reg( memshire_id, 0x62240550, 0x00000129);// write_ddrc_reg 2 INITENG0_SequenceReg0b99s2 0x129
  write_reg( memshire_id, 0x62240554, 0x00000020);// write_ddrc_reg 2 INITENG0_SequenceReg0b100s0 0x20
  write_reg( memshire_id, 0x62240558, 0x000002aa);// write_ddrc_reg 2 INITENG0_SequenceReg0b100s1 0x2aa
  write_reg( memshire_id, 0x6224055c, 0x00000009);// write_ddrc_reg 2 INITENG0_SequenceReg0b100s2 0x9
  write_reg( memshire_id, 0x62240560, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b101s0 0x0
  write_reg( memshire_id, 0x62240564, 0x00000400);// write_ddrc_reg 2 INITENG0_SequenceReg0b101s1 0x400
  write_reg( memshire_id, 0x62240568, 0x0000010e);// write_ddrc_reg 2 INITENG0_SequenceReg0b101s2 0x10e
  write_reg( memshire_id, 0x6224056c, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b102s0 0x8
  write_reg( memshire_id, 0x62240570, 0x000000e8);// write_ddrc_reg 2 INITENG0_SequenceReg0b102s1 0xe8
  write_reg( memshire_id, 0x62240574, 0x00000109);// write_ddrc_reg 2 INITENG0_SequenceReg0b102s2 0x109
  write_reg( memshire_id, 0x62240578, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b103s0 0x0
  write_reg( memshire_id, 0x6224057c, 0x00008140);// write_ddrc_reg 2 INITENG0_SequenceReg0b103s1 0x8140
  write_reg( memshire_id, 0x62240580, 0x0000010c);// write_ddrc_reg 2 INITENG0_SequenceReg0b103s2 0x10c
  write_reg( memshire_id, 0x62240584, 0x00000010);// write_ddrc_reg 2 INITENG0_SequenceReg0b104s0 0x10
  write_reg( memshire_id, 0x62240588, 0x00008138);// write_ddrc_reg 2 INITENG0_SequenceReg0b104s1 0x8138
  write_reg( memshire_id, 0x6224058c, 0x0000010c);// write_ddrc_reg 2 INITENG0_SequenceReg0b104s2 0x10c
  write_reg( memshire_id, 0x62240590, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b105s0 0x8
  write_reg( memshire_id, 0x62240594, 0x000007c8);// write_ddrc_reg 2 INITENG0_SequenceReg0b105s1 0x7c8
  write_reg( memshire_id, 0x62240598, 0x00000101);// write_ddrc_reg 2 INITENG0_SequenceReg0b105s2 0x101
  write_reg( memshire_id, 0x6224059c, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b106s0 0x8
  write_reg( memshire_id, 0x622405a0, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b106s1 0x0
  write_reg( memshire_id, 0x622405a4, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b106s2 0x8
  write_reg( memshire_id, 0x622405a8, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b107s0 0x8
  write_reg( memshire_id, 0x622405ac, 0x00000448);// write_ddrc_reg 2 INITENG0_SequenceReg0b107s1 0x448
  write_reg( memshire_id, 0x622405b0, 0x00000109);// write_ddrc_reg 2 INITENG0_SequenceReg0b107s2 0x109
  write_reg( memshire_id, 0x622405b4, 0x0000000f);// write_ddrc_reg 2 INITENG0_SequenceReg0b108s0 0xf
  write_reg( memshire_id, 0x622405b8, 0x000007c0);// write_ddrc_reg 2 INITENG0_SequenceReg0b108s1 0x7c0
  write_reg( memshire_id, 0x622405bc, 0x00000109);// write_ddrc_reg 2 INITENG0_SequenceReg0b108s2 0x109
  write_reg( memshire_id, 0x622405c0, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b109s0 0x0
  write_reg( memshire_id, 0x622405c4, 0x000000e8);// write_ddrc_reg 2 INITENG0_SequenceReg0b109s1 0xe8
  write_reg( memshire_id, 0x622405c8, 0x00000109);// write_ddrc_reg 2 INITENG0_SequenceReg0b109s2 0x109
  write_reg( memshire_id, 0x622405cc, 0x00000047);// write_ddrc_reg 2 INITENG0_SequenceReg0b110s0 0x47
  write_reg( memshire_id, 0x622405d0, 0x00000630);// write_ddrc_reg 2 INITENG0_SequenceReg0b110s1 0x630
  write_reg( memshire_id, 0x622405d4, 0x00000109);// write_ddrc_reg 2 INITENG0_SequenceReg0b110s2 0x109
  write_reg( memshire_id, 0x622405d8, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b111s0 0x8
  write_reg( memshire_id, 0x622405dc, 0x00000618);// write_ddrc_reg 2 INITENG0_SequenceReg0b111s1 0x618
  write_reg( memshire_id, 0x622405e0, 0x00000109);// write_ddrc_reg 2 INITENG0_SequenceReg0b111s2 0x109
  write_reg( memshire_id, 0x622405e4, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b112s0 0x8
  write_reg( memshire_id, 0x622405e8, 0x000000e0);// write_ddrc_reg 2 INITENG0_SequenceReg0b112s1 0xe0
  write_reg( memshire_id, 0x622405ec, 0x00000109);// write_ddrc_reg 2 INITENG0_SequenceReg0b112s2 0x109
  write_reg( memshire_id, 0x622405f0, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b113s0 0x0
  write_reg( memshire_id, 0x622405f4, 0x000007c8);// write_ddrc_reg 2 INITENG0_SequenceReg0b113s1 0x7c8
  write_reg( memshire_id, 0x622405f8, 0x00000109);// write_ddrc_reg 2 INITENG0_SequenceReg0b113s2 0x109
  write_reg( memshire_id, 0x622405fc, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b114s0 0x8
  write_reg( memshire_id, 0x62240600, 0x00008140);// write_ddrc_reg 2 INITENG0_SequenceReg0b114s1 0x8140
  write_reg( memshire_id, 0x62240604, 0x0000010c);// write_ddrc_reg 2 INITENG0_SequenceReg0b114s2 0x10c
  write_reg( memshire_id, 0x62240608, 0x00000000);// write_ddrc_reg 2 INITENG0_SequenceReg0b115s0 0x0
  write_reg( memshire_id, 0x6224060c, 0x00000001);// write_ddrc_reg 2 INITENG0_SequenceReg0b115s1 0x1
  write_reg( memshire_id, 0x62240610, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b115s2 0x8
  write_reg( memshire_id, 0x62240614, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b116s0 0x8
  write_reg( memshire_id, 0x62240618, 0x00000004);// write_ddrc_reg 2 INITENG0_SequenceReg0b116s1 0x4
  write_reg( memshire_id, 0x6224061c, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b116s2 0x8
  write_reg( memshire_id, 0x62240620, 0x00000008);// write_ddrc_reg 2 INITENG0_SequenceReg0b117s0 0x8
  write_reg( memshire_id, 0x62240624, 0x000007c8);// write_ddrc_reg 2 INITENG0_SequenceReg0b117s1 0x7c8
  write_reg( memshire_id, 0x62240628, 0x00000101);// write_ddrc_reg 2 INITENG0_SequenceReg0b117s2 0x101
  write_reg( memshire_id, 0x62240018, 0x00000000);// write_ddrc_reg 2 INITENG0_PostSequenceReg0b0s0 0x0
  write_reg( memshire_id, 0x6224001c, 0x00000000);// write_ddrc_reg 2 INITENG0_PostSequenceReg0b0s1 0x0
  write_reg( memshire_id, 0x62240020, 0x00000008);// write_ddrc_reg 2 INITENG0_PostSequenceReg0b0s2 0x8
  write_reg( memshire_id, 0x62240024, 0x00000000);// write_ddrc_reg 2 INITENG0_PostSequenceReg0b1s0 0x0
  write_reg( memshire_id, 0x62240028, 0x00000000);// write_ddrc_reg 2 INITENG0_PostSequenceReg0b1s1 0x0
  write_reg( memshire_id, 0x6224002c, 0x00000000);// write_ddrc_reg 2 INITENG0_PostSequenceReg0b1s2 0x0
  write_reg( memshire_id, 0x6234039c, 0x00000400);// write_ddrc_reg 2 APBONLY0_SequencerOverride 0x400
  write_reg( memshire_id, 0x6224005c, 0x00000000);// write_ddrc_reg 2 INITENG0_StartVector0b0 0x0
  write_reg( memshire_id, 0x6224007c, 0x00000029);// write_ddrc_reg 2 INITENG0_StartVector0b8 0x29
  write_reg( memshire_id, 0x62240098, 0x0000006a);// write_ddrc_reg 2 INITENG0_StartVector0b15 0x6a
  write_reg( memshire_id, 0x62100340, 0x00000000);// write_ddrc_reg 2 ACSM0_AcsmCsMapCtrl0 0x0
  write_reg( memshire_id, 0x62100344, 0x00000101);// write_ddrc_reg 2 ACSM0_AcsmCsMapCtrl1 0x101
  write_reg( memshire_id, 0x62100348, 0x00000105);// write_ddrc_reg 2 ACSM0_AcsmCsMapCtrl2 0x105
  write_reg( memshire_id, 0x6210034c, 0x00000107);// write_ddrc_reg 2 ACSM0_AcsmCsMapCtrl3 0x107
  write_reg( memshire_id, 0x62100350, 0x0000010f);// write_ddrc_reg 2 ACSM0_AcsmCsMapCtrl4 0x10f
  write_reg( memshire_id, 0x62100354, 0x00000202);// write_ddrc_reg 2 ACSM0_AcsmCsMapCtrl5 0x202
  write_reg( memshire_id, 0x62100358, 0x0000020a);// write_ddrc_reg 2 ACSM0_AcsmCsMapCtrl6 0x20a
  write_reg( memshire_id, 0x6210035c, 0x0000020b);// write_ddrc_reg 2 ACSM0_AcsmCsMapCtrl7 0x20b
  write_reg( memshire_id, 0x620800e8, 0x00000002);// write_ddrc_reg 2 MASTER0_DbyteDllModeCntrl 0x2
  write_reg( memshire_id, 0x6208002c, 0x00000064);// write_ddrc_reg 2 MASTER0_Seq0BDLY0_p0 0x64
  write_reg( memshire_id, 0x62080030, 0x000000c8);// write_ddrc_reg 2 MASTER0_Seq0BDLY1_p0 0xc8
  write_reg( memshire_id, 0x62080034, 0x000007d0);// write_ddrc_reg 2 MASTER0_Seq0BDLY2_p0 0x7d0
  write_reg( memshire_id, 0x62080038, 0x0000002c);// write_ddrc_reg 2 MASTER0_Seq0BDLY3_p0 0x2c
  write_reg( memshire_id, 0x62240030, 0x00000000);// write_ddrc_reg 2 INITENG0_Seq0BDisableFlag0 0x0
  write_reg( memshire_id, 0x62240034, 0x00000173);// write_ddrc_reg 2 INITENG0_Seq0BDisableFlag1 0x173
  write_reg( memshire_id, 0x62240038, 0x00000060);// write_ddrc_reg 2 INITENG0_Seq0BDisableFlag2 0x60
  write_reg( memshire_id, 0x6224003c, 0x00006110);// write_ddrc_reg 2 INITENG0_Seq0BDisableFlag3 0x6110
  write_reg( memshire_id, 0x62240040, 0x00002152);// write_ddrc_reg 2 INITENG0_Seq0BDisableFlag4 0x2152
  write_reg( memshire_id, 0x62240044, 0x0000dfbd);// write_ddrc_reg 2 INITENG0_Seq0BDisableFlag5 0xdfbd
  write_reg( memshire_id, 0x62240048, 0x0000ffff);// write_ddrc_reg 2 INITENG0_Seq0BDisableFlag6 0xffff
  write_reg( memshire_id, 0x6224004c, 0x00006152);// write_ddrc_reg 2 INITENG0_Seq0BDisableFlag7 0x6152
  write_reg( memshire_id, 0x62100200, 0x000000e0);// write_ddrc_reg 2 ACSM0_AcsmPlayback0x0_p0 0xe0
  write_reg( memshire_id, 0x62100204, 0x00000012);// write_ddrc_reg 2 ACSM0_AcsmPlayback1x0_p0 0x12
  write_reg( memshire_id, 0x62100208, 0x000000e0);// write_ddrc_reg 2 ACSM0_AcsmPlayback0x1_p0 0xe0
  write_reg( memshire_id, 0x6210020c, 0x00000012);// write_ddrc_reg 2 ACSM0_AcsmPlayback1x1_p0 0x12
  write_reg( memshire_id, 0x62100210, 0x000000e0);// write_ddrc_reg 2 ACSM0_AcsmPlayback0x2_p0 0xe0
  write_reg( memshire_id, 0x62100214, 0x00000012);// write_ddrc_reg 2 ACSM0_AcsmPlayback1x2_p0 0x12
  write_reg( memshire_id, 0x621003f4, 0x0000000f);// write_ddrc_reg 2 ACSM0_AcsmCtrl13 0xf
  write_reg( memshire_id, 0x62040044, 0x00000001);// write_ddrc_reg 2 DBYTE0_TsmByte1 0x1
  write_reg( memshire_id, 0x62040048, 0x00000001);// write_ddrc_reg 2 DBYTE0_TsmByte2 0x1
  write_reg( memshire_id, 0x6204004c, 0x00000180);// write_ddrc_reg 2 DBYTE0_TsmByte3 0x180
  write_reg( memshire_id, 0x62040060, 0x00000001);// write_ddrc_reg 2 DBYTE0_TsmByte5 0x1
  write_reg( memshire_id, 0x62040008, 0x00006209);// write_ddrc_reg 2 DBYTE0_TrainingParam 0x6209
  write_reg( memshire_id, 0x620402c8, 0x00000001);// write_ddrc_reg 2 DBYTE0_Tsm0_i0 0x1
  write_reg( memshire_id, 0x620406d0, 0x00000001);// write_ddrc_reg 2 DBYTE0_Tsm2_i1 0x1
  write_reg( memshire_id, 0x62040ad0, 0x00000001);// write_ddrc_reg 2 DBYTE0_Tsm2_i2 0x1
  write_reg( memshire_id, 0x62040ed0, 0x00000001);// write_ddrc_reg 2 DBYTE0_Tsm2_i3 0x1
  write_reg( memshire_id, 0x620412d0, 0x00000001);// write_ddrc_reg 2 DBYTE0_Tsm2_i4 0x1
  write_reg( memshire_id, 0x620416d0, 0x00000001);// write_ddrc_reg 2 DBYTE0_Tsm2_i5 0x1
  write_reg( memshire_id, 0x62041ad0, 0x00000001);// write_ddrc_reg 2 DBYTE0_Tsm2_i6 0x1
  write_reg( memshire_id, 0x62041ed0, 0x00000001);// write_ddrc_reg 2 DBYTE0_Tsm2_i7 0x1
  write_reg( memshire_id, 0x620422d0, 0x00000001);// write_ddrc_reg 2 DBYTE0_Tsm2_i8 0x1
  write_reg( memshire_id, 0x62044044, 0x00000001);// write_ddrc_reg 2 DBYTE1_TsmByte1 0x1
  write_reg( memshire_id, 0x62044048, 0x00000001);// write_ddrc_reg 2 DBYTE1_TsmByte2 0x1
  write_reg( memshire_id, 0x6204404c, 0x00000180);// write_ddrc_reg 2 DBYTE1_TsmByte3 0x180
  write_reg( memshire_id, 0x62044060, 0x00000001);// write_ddrc_reg 2 DBYTE1_TsmByte5 0x1
  write_reg( memshire_id, 0x62044008, 0x00006209);// write_ddrc_reg 2 DBYTE1_TrainingParam 0x6209
  write_reg( memshire_id, 0x620442c8, 0x00000001);// write_ddrc_reg 2 DBYTE1_Tsm0_i0 0x1
  write_reg( memshire_id, 0x620446d0, 0x00000001);// write_ddrc_reg 2 DBYTE1_Tsm2_i1 0x1
  write_reg( memshire_id, 0x62044ad0, 0x00000001);// write_ddrc_reg 2 DBYTE1_Tsm2_i2 0x1
  write_reg( memshire_id, 0x62044ed0, 0x00000001);// write_ddrc_reg 2 DBYTE1_Tsm2_i3 0x1
  write_reg( memshire_id, 0x620452d0, 0x00000001);// write_ddrc_reg 2 DBYTE1_Tsm2_i4 0x1
  write_reg( memshire_id, 0x620456d0, 0x00000001);// write_ddrc_reg 2 DBYTE1_Tsm2_i5 0x1
  write_reg( memshire_id, 0x62045ad0, 0x00000001);// write_ddrc_reg 2 DBYTE1_Tsm2_i6 0x1
  write_reg( memshire_id, 0x62045ed0, 0x00000001);// write_ddrc_reg 2 DBYTE1_Tsm2_i7 0x1
  write_reg( memshire_id, 0x620462d0, 0x00000001);// write_ddrc_reg 2 DBYTE1_Tsm2_i8 0x1
  write_reg( memshire_id, 0x62048044, 0x00000001);// write_ddrc_reg 2 DBYTE2_TsmByte1 0x1
  write_reg( memshire_id, 0x62048048, 0x00000001);// write_ddrc_reg 2 DBYTE2_TsmByte2 0x1
  write_reg( memshire_id, 0x6204804c, 0x00000180);// write_ddrc_reg 2 DBYTE2_TsmByte3 0x180
  write_reg( memshire_id, 0x62048060, 0x00000001);// write_ddrc_reg 2 DBYTE2_TsmByte5 0x1
  write_reg( memshire_id, 0x62048008, 0x00006209);// write_ddrc_reg 2 DBYTE2_TrainingParam 0x6209
  write_reg( memshire_id, 0x620482c8, 0x00000001);// write_ddrc_reg 2 DBYTE2_Tsm0_i0 0x1
  write_reg( memshire_id, 0x620486d0, 0x00000001);// write_ddrc_reg 2 DBYTE2_Tsm2_i1 0x1
  write_reg( memshire_id, 0x62048ad0, 0x00000001);// write_ddrc_reg 2 DBYTE2_Tsm2_i2 0x1
  write_reg( memshire_id, 0x62048ed0, 0x00000001);// write_ddrc_reg 2 DBYTE2_Tsm2_i3 0x1
  write_reg( memshire_id, 0x620492d0, 0x00000001);// write_ddrc_reg 2 DBYTE2_Tsm2_i4 0x1
  write_reg( memshire_id, 0x620496d0, 0x00000001);// write_ddrc_reg 2 DBYTE2_Tsm2_i5 0x1
  write_reg( memshire_id, 0x62049ad0, 0x00000001);// write_ddrc_reg 2 DBYTE2_Tsm2_i6 0x1
  write_reg( memshire_id, 0x62049ed0, 0x00000001);// write_ddrc_reg 2 DBYTE2_Tsm2_i7 0x1
  write_reg( memshire_id, 0x6204a2d0, 0x00000001);// write_ddrc_reg 2 DBYTE2_Tsm2_i8 0x1
  write_reg( memshire_id, 0x6204c044, 0x00000001);// write_ddrc_reg 2 DBYTE3_TsmByte1 0x1
  write_reg( memshire_id, 0x6204c048, 0x00000001);// write_ddrc_reg 2 DBYTE3_TsmByte2 0x1
  write_reg( memshire_id, 0x6204c04c, 0x00000180);// write_ddrc_reg 2 DBYTE3_TsmByte3 0x180
  write_reg( memshire_id, 0x6204c060, 0x00000001);// write_ddrc_reg 2 DBYTE3_TsmByte5 0x1
  write_reg( memshire_id, 0x6204c008, 0x00006209);// write_ddrc_reg 2 DBYTE3_TrainingParam 0x6209
  write_reg( memshire_id, 0x6204c2c8, 0x00000001);// write_ddrc_reg 2 DBYTE3_Tsm0_i0 0x1
  write_reg( memshire_id, 0x6204c6d0, 0x00000001);// write_ddrc_reg 2 DBYTE3_Tsm2_i1 0x1
  write_reg( memshire_id, 0x6204cad0, 0x00000001);// write_ddrc_reg 2 DBYTE3_Tsm2_i2 0x1
  write_reg( memshire_id, 0x6204ced0, 0x00000001);// write_ddrc_reg 2 DBYTE3_Tsm2_i3 0x1
  write_reg( memshire_id, 0x6204d2d0, 0x00000001);// write_ddrc_reg 2 DBYTE3_Tsm2_i4 0x1
  write_reg( memshire_id, 0x6204d6d0, 0x00000001);// write_ddrc_reg 2 DBYTE3_Tsm2_i5 0x1
  write_reg( memshire_id, 0x6204dad0, 0x00000001);// write_ddrc_reg 2 DBYTE3_Tsm2_i6 0x1
  write_reg( memshire_id, 0x6204ded0, 0x00000001);// write_ddrc_reg 2 DBYTE3_Tsm2_i7 0x1
  write_reg( memshire_id, 0x6204e2d0, 0x00000001);// write_ddrc_reg 2 DBYTE3_Tsm2_i8 0x1
  write_reg( memshire_id, 0x62080224, 0x00000001);// write_ddrc_reg 2 MASTER0_CalZap 0x1
  write_reg( memshire_id, 0x62080220, 0x00000019);// write_ddrc_reg 2 MASTER0_CalRate 0x19
  write_reg( memshire_id, 0x62300200, 0x00000002);// write_ddrc_reg 2 DRTUB0_UcclkHclkEnables 0x2
  write_reg( memshire_id, 0x62340000, 0x00000001);// write_ddrc_reg 2 APBONLY0_MicroContMuxSel 0x1
//
  write_reg( memshire_id, 0x600001b0, 0x000010a0);// (controller #0) write_both_ddrc_reg DFIMISC 0x000010a0
  write_reg( memshire_id, 0x600011b0, 0x000010a0);// (controller #1) write_both_ddrc_reg DFIMISC 0x000010a0
//
//######################################################
//# wait for init complete
//######################################################
  do
    value_read = read_reg( memshire_id, 0x600001bc);// poll_ddrc_reg 0 DFISTAT 0x1 0x1 1000
  while(!((value_read & 0x1) == 0x1));

  do
    value_read = read_reg( memshire_id, 0x600011bc);// poll_ddrc_reg 1 DFISTAT 0x1 0x1 1000
  while(!((value_read & 0x1) == 0x1));
//
  write_reg( memshire_id, 0x600001b0, 0x00001080);// (controller #0) write_both_ddrc_reg DFIMISC 0x00001080
  write_reg( memshire_id, 0x600011b0, 0x00001080);// (controller #1) write_both_ddrc_reg DFIMISC 0x00001080
  write_reg( memshire_id, 0x600001b0, 0x00001081);// (controller #0) write_both_ddrc_reg DFIMISC 0x00001081
  write_reg( memshire_id, 0x600011b0, 0x00001081);// (controller #1) write_both_ddrc_reg DFIMISC 0x00001081
  write_reg( memshire_id, 0x600001b0, 0x00001081);// (controller #0) write_both_ddrc_reg DFIMISC 0x00001081
  write_reg( memshire_id, 0x600011b0, 0x00001081);// (controller #1) write_both_ddrc_reg DFIMISC 0x00001081
  write_reg( memshire_id, 0x60000320, 0x00000001);// (controller #0) write_both_ddrc_reg SWCTL 0x00000001
  write_reg( memshire_id, 0x60001320, 0x00000001);// (controller #1) write_both_ddrc_reg SWCTL 0x00000001
//
  do
    value_read = read_reg( memshire_id, 0x60000324);// poll_ddrc_reg 0 SWSTAT 0x1 0x1 10
  while(!((value_read & 0x1) == 0x1));

  do
    value_read = read_reg( memshire_id, 0x60001324);// poll_ddrc_reg 1 SWSTAT 0x1 0x1 10
  while(!((value_read & 0x1) == 0x1));

  do
    value_read = read_reg( memshire_id, 0x60000004);// poll_ddrc_reg 0 STAT 0x1 0x1 1000
  while(!((value_read & 0x1) == 0x1));

  do
    value_read = read_reg( memshire_id, 0x60001004);// poll_ddrc_reg 1 STAT 0x1 0x1 1000
  while(!((value_read & 0x1) == 0x1));
//
  write_reg( memshire_id, 0x60000030, 0x00000000);// (controller #0) write_both_ddrc_reg PWRCTL 0x00000000
  write_reg( memshire_id, 0x60001030, 0x00000000);// (controller #1) write_both_ddrc_reg PWRCTL 0x00000000
  write_reg( memshire_id, 0x60000490, 0x00000001);// (controller #0) write_both_ddrc_reg PCTRL_0 0x00000001
  write_reg( memshire_id, 0x60001490, 0x00000001);// (controller #1) write_both_ddrc_reg PCTRL_0 0x00000001
  write_reg( memshire_id, 0x60000540, 0x00000001);// (controller #0) write_both_ddrc_reg PCTRL_1 0x00000001
  write_reg( memshire_id, 0x60001540, 0x00000001);// (controller #1) write_both_ddrc_reg PCTRL_1 0x00000001
//
//######################################################
//# That's All folks...
//######################################################
//

}

#endif // DDR_CONFIG_H
