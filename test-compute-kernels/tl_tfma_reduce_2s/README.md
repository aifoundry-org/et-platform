
Tl_tfma_reduce_2s Kernel
========================

Author: emizan

This test runs 100 loops of 2 non-coop tensor loads, 1 tensor FMA and 1 reduce  on full chip.
At the end it stores data with tensor stores and does a CRC check. Minions use the reduce send/receive
type of reduces and they are paired into 512 random pairs. On every iteration the pairs are the same
but the roles of sender/receiver are inverted
Tensor load, FMA and reduce parameters are randomly generated but with a constant seed.
So running this on checkin regressions will give always the same results.
The test uses 2 compute shires.

This test can currently run on sysemu/Zebu using the full SW stack.

After building sw-platform:
1. Start zebu in one terminal: in build/zebu-regression 
python_venv/bin/python ./zebu.py --prompt --start-zebu --zebu-configuration=bemu

2. Start QEMU in another terminal: In build/zebu-regression:
python_venv/bin/python ./zebu.py --prompt --start-qemu --zebu-configuration=bemu

3. Inside QEMU:
Go to build/tests/DV
ctest -V -R dv:ComboKernelLaunchTest/TensorKernelLaunch.tl_tfma_reduce_2s_kernel/0

The test runs on sysemu and Zebu and the CRC signatures coming back from each shire
are compared. On a mismatch the test fails.
