
Producer_consumer_fc Kernel
===========================

Author: emizan

This test runs 4 iterations of a complex producer-consumer scenario:
First FMA operations produce data that are sent through reduces to the consumers.
Reduce pairs are formed randomly using the send-receiver mapping.
Then that data is sent with tensor stores to memory and consumed by tensor loads.
Load-store minion-pairs are also random.
Tensor load, store and FMA, and reduce parameters are randomly generated.
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
ctest -V -R dv:ComboKernelLaunchTest/TensorKernelLaunch.producer_consumer_fc_kernel
 
This will run back-to-back 10 different seeds.

To run only one seed (eg test 5):
ctest -V -R dv:ComboKernelLaunchTest/TensorKernelLaunch.prodcuer_consumer_fc_kernel_5

The test runs on sysemu and Zebu and the CRC signatures coming back from each shire
are compared. On a mismatch the test fails.

