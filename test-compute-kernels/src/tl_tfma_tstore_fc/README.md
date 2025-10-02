
Tl_tfma_tstore_fc Kernel
========================

Author: emizan

This test runs 100 loops of 25 possibly coop tensor loads, 1 tensor FMA and 
1 tensor store (coop / non-coop or SCP) on full chip. At the end it evicts data to L3 
and does a CRC check.

Tensor load, FMA and ztore parameters are randomly generated.
The test uses all 32 shires.

This test can currently run on sysemu/Zebu using the full SW stack.

After building sw-platform:
1. Start zebu in one terminal: in build/zebu-regression 
python_venv/bin/python ./zebu.py --prompt --start-zebu --zebu-configuration=full

2. Start QEMU in another terminal: In build/zebu-regression:
python_venv/bin/python ./zebu.py --prompt --start-qemu --zebu-configuration=full

3. Inside QEMU:
Go to build/tests/DV
ctest -V -R dv:ComboKernelLaunchTest/TensorKernelLaunch.tl_tfma_tstore_fc_kernel

This will run back-to-back 10 different seeds.

To run only one seed (eg test 5):
ctest -V -R dv:ComboKernelLaunchTest/TensorKernelLaunch.tl_tfma_tstore_fc_kernel_5

The test runs on sysemu and Zebu and the CRC signatures coming back from each shire
are compared. On a mismatch the test fails.
