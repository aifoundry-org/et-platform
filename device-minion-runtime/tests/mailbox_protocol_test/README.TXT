This directory contains a basic test of the software protocol used by the Service Processor to interact with the Maxions, Master Minion and PCIe host.
The test consists of two native Linux executables (master and slave) that are used to emulated the two endpoints of the mailbox.
The mailbox is simulated using a shared memory region that is accessible by both processes.
In each process, the main thread is used to run the "mailbox driver" code and a number of additional worker threads (producers) are used to emulate the workers.
This test only tries to send high-priority requests and responses.  It prints a message when a response has been received, but does not try to actually pass the response back to the worker thread the sent the original request.

