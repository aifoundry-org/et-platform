
Tl_tfma_reduce_fc Kernel
========================

Author: emizan

This test runs 100 loops of 2 non-coop tensor loads, 1 tensor FMA and 1 reduce  on full chip.
At the end it stores data with tensor stores and does a CRC check. Minions use the reduce send/receive
type of reduces and they are paired into 512 random pairs. On every iteration the pairs are the same
but the roles of sender/receiver are inverted

Tensor load, FMA and reduce parameters are randomly generated.
At the end the test stores data with tensor stores and generates CRCs
The test uses all 32 shires.

This test can currently run on sysemu/Zebu using the full SW stack.

After building sw-platform:
1. Start zebu in one terminal: in build/zebu-regression 
python_venv/bin/python ./zebu.py --prompt --start-zebu --zebu-configuration=full

2. Start QEMU in another terminal: In build/zebu-regression:
python_venv/bin/python ./zebu.py --prompt --start-qemu --zebu-configuration=full

3. Inside QEMU:
Go to build/tests/DV
ctest -V -R dv:ComboKernelLaunchTest/TensorKernelLaunch.tl_tfma_reduce_fc_kernel

This will run back-to-back 10 different seeds.

To run only one seed (eg test 5):
ctest -V -R dv:ComboKernelLaunchTest/TensorKernelLaunch.tl_tfma_reduce_fc_kernel_5

The test runs on sysemu and Zebu and the CRC signatures coming back from each shire
are compared. On a mismatch the test fails.
