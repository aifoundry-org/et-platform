Master Minion Firmware
======================

The Master Shire consists of 32 minions. The Master Minion software stack, also known as Master Minion Runtime, is a baremetal software environment that runs across the minions in the master shire to support the functionality required of the Master Minion. 

The Master Minion runtime implements a software architecture that attempts to maximize parallelization of command processing on the ETSoC Device. 

.. image:: mm-runtime.png
  :width: 400

The key software threads that enable this architecture are as follows:
  - Dispatcher - Responsible for initializing the system and launching other worker threads.
  - Submission Queue Worker (SQW) - Responsible for servicing commands in the associated Host to MM Submission Queue. For n Host to MM Submission Queues there are n SQWs. 
  - Kernel Worker (KW) - Responsible for servicing kernel related commands. SQWs offload kernel command related processing to KW. 
  - DMA Worker (DMAW) - Responsible for monitoring and updating DMA hardware channel status to support usage of DMA resources by SQW.  

The key interface that enable this architecture are as follows:
  - Host Interface 
  	- Submission Queues, for Host to submit commands to Dispatcher/Submission Queue Worker.
	- Completion Queue, for SQW, KW, and DMAW to submit command responses and events to Host.  
  - Worker Interface 
  		- KW FIFO, for SQWs to submit kernel commands to Kernel Worker
  
Each of these software threads execute on an independent hardware context (i.e., a minion hardware thread) in a fully concurrent model. 
  
MM runtime early boot: 
	On release from reset each minion in the master shire executs early C runtime setup which sets up an per minion stack (exclusive to each minion) and jumps to main function. 

Main function: 
	The main function executes on each Master Minon. It fetches the HART ID of the minion it is executing on and launches the corresponding software thread. The software thread to HART ID mapping is defined as a build time configuration in mm_config.h.

Dispatcher: 
	The Dispatcher is the master software thread. Its role is to initialize the necessary system level components and the other resources used by the other worker threads. Post initialization, the Dispatcher blocks waiting on a interrupt from host. On receiving an interrupt from host the Dispatcher identifies the Submission Queues with commands to be processed and notifies the corresponding Submission Queue Workers using an FCC event.   

Submission Queue Worker(SQW): 
	This thread is launched by main. Post initialization it blocks waiting on a FCC event from the Dispatcher thread. On receiving a FCC event from Dispatcher, the SQW pops available commands from the corresponding Submission Queue, decodes the command, and routes the command for further processing. 

	Compatibility, FW version, echo commands, and trace related commands are processed by the submission queue worker. Command response is constructed and transmitted to host by pushing command responses to host completion queue, and the host is notified using a MSI interrupt. 

	DMA commands are processed by SQW as follows - A shared software data structure called the Global DMA Channel Status located in L2 cache enables the SQW and DMAW to synchronize DMA command processing. On receiving a DMA command, the SQW obtains lock on the next available DMA channel by updating the Global DMA Channel Status, and programs the DMA controller to initiate the DMA transaction requested.

	Kernel commands are processed by simply routing them to the kernel worker. i.e., all kernel commands are pushed to the KW FIFO, and KW is notified using a FCC event.

Kernel Worker:
	This thread is launched by main. Post initialization, it blocks waiting on a FCC event from SQWs. On receiving a FCC event from a SQW, the KW pops the next available kernel command from the KW FIFO, and processes it.

	Processing of kernel launch command - details to come
	Processing of kernel abort command - details to come
	Processing of kernel state command - details to come

	Completion of kernel command processing, and Kernel command completion interrupt events are fielded by the kernel worker, and command responses are constructed. Command responses are pushed to the host completion queue, and host is notified using an MSI interrupt.    

DMA Worker:
	This thread is launched by main. Post initialization, this thread infinitely polls the the PCIe DMA HW status registers to detect completion of DMA activity on each supported DMA channel, and updates DMA Channel Status software data structure. When completion of DMA activity on a given channel is detected by the polling loop, DMA channel status isupdates and a corresponding DMA command response is created by the DMA worker. The command response is pushed into the host completion queue, and host is notified using an MSI interrupt. 







Legacy content below .. 









Software Stack
--------------
The Master and Sync Minions software stack looks like follows:

.. image:: Master-Minion-Software-Stack.png
  :width: 400


Execution Sequence (Master Minion Execution Thread)
  - Initialization Sequence
	- Serial Port (PU UART 0)
	- Interupt controller
	- Mailbox (Hand shake message with SP, Worker Minion and Host)
	- Message buffers
	- FCC 0 and 1 counters cleared
	- Kernel Init
	- Enable Interrupts (External, Supervisor, Software)
  - Main Execution Loop
	- if (Debug) Send SP Mbox
	- if (SW Flags set - SP or Worker Minion)
 		- Handle messages from SP and then Worker Minion
	- if (PCI Ex Interrupt flag set)
		- Handle messages from Host
	- Handler Timer Events
	- If no messages - go to sleep

As master/runtime minions are not visible to user kernels,
they should never execute in user mode privilege. In the S-mode there
will be an RTos device runtime software. The RTos can only be
run in the master and runtime minions (minions 0..15 of the
shire 32). On m-mode there will be a thin m-code that will provide
low level services that are not supported directly by hardware (setting SATP, …).

\todo Guillem, From Ioannis: The above sentence is confusing to me.

S-mode calls to M-mode
^^^^^^^^^^^^^^^^^^^^^^

Fill what are the explicit services that the RTos device runtime software will request to m-code.


M-mode “transparent” traps
^^^^^^^^^^^^^^^^^^^^^^^^^^

Same as compute minion


Communication Mechanisms Between Sync and Compute Minions
---------------------------------------------------------

\todo Guillem we need to clarify the following paragraph:

Compute and sync minions can communicate only with the runtime minions (assuming that master minion delegates the execution handling to one or several runtime minions). APIs

* Kernel done: when a kernel yields a minion, the minion goes to S-mode. After the minion state cleanup done by the bare-metal software, it should notify the runtime minion in charge that the minion is available. One option is first checking that all the minions in a shire are done and the last one notifies the runtime minion. Implementation options:

  * Doing an interrupt is likely too slow: there would be a total of 32 interrupts to notify that each shire is done
  *  Using ports is not safe because u-mode code can write to them (with 1GByte pages there’s no shire_id granularity through the page table)
  * Doing an atomic increment and add in a position of the L3 of the master shire and having the runtime minion doing an active poll seems the most reasonable option

* Kernel error: same as before, but need to set a variable to 1 to let the runtime minion that there was a problem in the clean up process (Tensor* not in idle state)

Communication Mechanisms Between Master and Runtime Minions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


\todo Guillem's Questions
* Master to Runtime: spawn a new thread. Done through IPI? Dependent on RTos?
* Runtime to Runtime: can a runtime spawn a new thread?
* Runtime to Compute/Sync: start a new kernel. Done through IPI.
* Runtime to Master: notify that something finished: kernel, mem transfer, ...
