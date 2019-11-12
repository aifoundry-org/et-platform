
Tl_tfma_reduce Kernel
=====================

Author: emizan

This test runs 10K loops of 2 non-coop tensor loads, 1 tensor FMA and 1 reduce  on full chip.
At the end it stores data with tensor stores and does a CRC check. Minions use the reduce send/receive
type of reduces and they are paired into 512 random pairs. On every iteration the pairs are the same
but the roles of sender/receiver are inverted

To run this test you will need the following:

1. Change MasterMinion/main.c:
Enable DEBUG_FAKE_MESSAGE_FORM_HOST, tensor_a = 0x8180000000, tensor_b = 0x200000,
tensor_c = 0x8280000000, tensor_d = 0x100000, shire_mask 0xFFFFFFFF.

2. Use a randomly generated data image for the input data (tensor_a) and for output data (tensor_c)
This should not spill into the output tensor (tensor_c). A 2MB image should be enough.

3. Run with sys_emu:
sys_emu -master_min -mem_desc mem_desc.txt -shires 1ffffffff -minions ffffffff -max_cycles 100000000
This will take ~5mins. See the CRC outputs and copy the correct values into include/crc_vals.h
Build and run again. You should see no errors. Generate the zebu images and send it to Zebu

For a 4-shire (3 shires + master shire), use shire_mask = 0x7, and run sys_emu with -shires 100000007

TBD: This process will become more automated.
