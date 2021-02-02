======================
Minion Firmware
======================

The ETSoC device consists 32 Compute Shires, and 1 special shire called the Master Shire.
Each Shire consist 32 Minions, and each Minion consists 2 hardware threads. Each hardware
thread is identified by a unique HART ID.

As shown in Figure 1 below, the device firmware running on Minions implement a control domain,
and a compute domain.

**Control Domain:** The control domain executes the **Master Minion Runtime** firmware whose
role is to; initialize the system, receive commands from host, decode and process commands.
Processing of commands could occur on the control domain or dispatched to compute domain.
The Master Minion runtime implements a software architecture that attempts to maximize
parallelization of command processing on the ETSoC Device. The **Master Minion Runtime** ,
is a baremetal software environment that runs across the first 16 Minions (i.e., the first
32 hardware threads) in the Master Shire to support the functionality required of the control
domain. Figure 1 shows the control domain in green.

**Compute Domain:** The compute domain executes the **Compute Runtime** firmware whose role is
to field and process commands from **Master Minion Runtime**, and to properly transfer control
and execute/manage compute kernels offloaded from host for compute acceleration. The compute
kernels execute in U-Mode. The **Compute Runtime** is baremetal software that runs on the second
16 Minions in the Master Shire, and the 32 Compute Shires dedicated for compute to support the
functionality required of the compute domain. Figure 1 shows the compute domain in purple.

.. figure:: mm-runtime.png

**Figure 1: Master Minion Runtime - Architecture, Components, Data Flow**

**Master Minion Runtime - Interfaces and Theory of Operations**

**Interfaces:**

Figure 1 shows the key components in the system architecture. The key interfaces used by the
**Master Minion Runtime** are listed as follows. Except the Master Minion to Compute Minion
interface all other interfaces are implemented using a circular buffer data structure.

**Host Interface:**

Host submits commands to device using the Host to Device Submission Queue
interface using [device-ops-api] specification conformant commands. The number of Submission
Queus in the System is a build time defined parameter in the *Master Minion Runtime* configuration
header. Host uses PCIe interrupt to notify Device of commands posted to Submission Queue.

Device response to commands from Host using the Device to Host Completion Queue interface using
[device-ops-api] specification conformant commands. A single completion queue is used in the
system for all Device to Host communications. Device uses MSIx interrupts to notify Host of
Command responses posted to Completion Queue.

**Service Processor (SP) to Master Minion (MM) Interface**

Service processor submits commands to *Master Minion Runtime* using the SP to MM Submission Queue
using [sp-mm-commands] specification conformant bindings. A single queue is used for SP to MM
communications and Interprocessor Interrupts (IPIs) are used for SP to MM command-post
notifications.

**Master Minion Runtime** submits commands to the Service Processor using the MM to SP Completion
Queue interface using [sp-mm-commands] specification conformant bindings. A single queue is used
for SP to MM communications and Interprocessor Interrupts (IPIs) are used for SP to MM command-post
notifications.

**Master Minion (MM) to Compute Minion (CM) Interface**

**MM to CM Interface:** **Master Minion Runtime** Submission Queue Worker (SQW) submits multicast
commands to Compute Minions (CM) using [MM-CM-commands] specification conformant bindings. Based on
shire mask specified by Kernel Launch or Kernel Abort command the Submission Queue Worker (SQW) uses
the MM to CM interface to manage compute minions executing user kernels. A special single slot buffer
serves as the transport mechanism for this interface and IPIs are used for notification.

**CM to MM Interface:** **Compute Minion Runtime** transmit Kernel Command responses to the Kernel
Worker (KW) using [MM-CM-commands] specification conformant bindings. A unicast circular buffer is
used as transport and IPIs are used as notification mechanism. On completion of user kernel execution
or in case of a error or exception scenario the last minion in each CM shire transmits command response
to the KW using this interface.

**Operations**

On boot, **Master Minion Runtime** firmware's C runtime entry point launches all software threads
in the system. In this implementation, a software thread is an independent baremetal execution
context with a dedicated stack that executes on one or both hardware threads on any given minion.
The software threads present in the **Master Minion Runtime** are listed below. The master minion
build configuration defines the HART ID to software thread assignments. A system spinlock synchronizes
the start of software threads in the system. The Dispatcher thread acquires the system lock and starts
execution, while the rest of the threads halt progress waiting to acquire the system lock.

	- **Dispatcher**
	- **Submission Queue Worker (SQW)**
	- **Kernel Worker (KW)**
	- **DMA Worker (DMAW)**

**Dispatcher**

.. figure:: dispatcher.png
	:align: center

**Figure 2: Dispatcher Flow**

This thread is responsible for initializing the device resources (serial, trace, etc), other
software threads (workers), interfaces, Device Interface Registers (DIR). Post initialization,
the Dispatcher sets the MMRDY bit in the DIR to indicate to host that **Master Minion Runtime**
is ready to accept commands from host and spins in an inifinite loop blocked waiting on a WFI
to receive and process following interrupts; PCIe interrupts from host to notify device of
commands available to process in Submission Queues, Software Interrupt (SWI) from machine mode
used to convey Inter Processor Interrupts (IPIs) from Service processor (SP).

**Submission Queue Worker**

Responsible for servicing commands in the associated Host to MM Submission Queue. For ''n''
Host to MM Submission Queues there are ''n'' SQWs. This thread is launched by main(). Post
initialization, it blocks waiting on a FCC event from the Dispatcher thread.

On receiving a FCC event from Dispatcher; the SQW pops available commands from the corresponding
Submission Queue, decodes the command, and routes the command for further processing.

Compatibility, FW version, echo commands, and trace related commands are processed by the
submission queue worker. Command response is constructed and transmitted to host by pushing
command responses to host completion queue, and the host is notified using a MSI (Message-signalled
Interrupt).

DMA commands are processed by SQW as follows; A shared software data structure called the
Global DMA Channel Status, located in L2 cache, enables the SQW and DMAW to synchronize DMA
command processing. On receiving a DMA command, the SQW obtains lock on the next available DMA
channel by updating the Global DMA Channel Status, and programs the DMA controller to initiate
the DMA transaction requested.

Kernel commands are processed by translating the host kernel command to the corresponding Compute
Minion command. Kernel Launch Command is handled by posting a kernel Launch MM to CM command, and
a multicast notification is sent to compute minions identified from the shire mask specified in
Kernel Launch command. Kernel Abort Command is handled by looking up the Kernel Slot for the tag ID
specified in the Kernel Abort Command. Based on the shire mask associated with the identified Kernel
Slot, an Abort MM to CM command is posted, and a multicast notification is sent to compute minions.

.. figure:: sq-worker-flow.png
	:align: center

**Figure 3: Submission Worker execution flow**

**DMA Worker**

Two DMA workers are launched at startup. Each DMA worker thread is responsible for monitoring the
assigned DMA controller status registers, and updates the "DMA Hardware Channel Status" to support
usage of DMA resources by the Submission Queue Worker Threads. As illustrated in Figure 1. A DMA
worker thread is allocated to monitor completion of transaction on all DMA read channels, and a DMA
worker is allocated to monitor completion of transaction on write channels.

.. figure:: dma-worker-flow.png
	:align: center

**Figure 4: DMA Read and Write Worker execution flow**

**Kernel Worker**

The Kernel Worker thread is launched at startup. Kernel Worker is responsible for monitoring completion
events from Compute Shires. Based on shire mask specified in the Kernel Launch Command, one to many
compute shires may be operational executing the user kernel. On completion of user kernel execution the
last minion in each shire pushes a completion message to an unicast circular buffer and notifies the
kernel Worker. Kernel Worker waits till all shires associated with the Kernel Launch report completion.
On completion of user kernel execution, the kernel worker creates and transmits a Kernel completion
response to the host. If any of the Compute Shires report error during execution, the Kernel Worker
initiates an Abort sequence of the Compute Shires associated with that kernel Launch and frees up
resources.

.. figure:: kernel-worker-flow.png
	:align: center

**Figure 5: Kernel Worker execution flow**

**Compute Runtime - Theory of operations**
@Sergi to fill this part
