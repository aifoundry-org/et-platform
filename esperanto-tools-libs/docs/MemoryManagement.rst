.. _MemoryManagement:

MemoryManagement
================

Goal / Scope
------------

This Chapter describes how the Host-Runtime manages the available DRAM memory provided in the
system.

We are going to cover:

* The memory requirements of loading and launching a network in our system.

* How DRAM is divided in our system and managed.

Types of Buffers In The System
------------------------------

When running a Machine Learninng model through Glow, we need to
allocate different types of tensors in the device. Some of these
tensors belong to the model (Filters, Weights, Biases, etc), while
other tensors are internally created by Neuralizer when compiling the
code, and thus other Backends do not require them (for example, Buffer
Scales used by RISCV-ET BufferQuant instruction).


The usage that the device is going to do from these tensors is also
different. While some tensors are constants (Biases), and thus can be
reused between runs, others are volatile (Activations), and thus can
be deallocated once consumed. Note that while Activations and Outputs
are Placeholders, Activations can be deallocated once the model
completes (or even when the first layer of the network has executed) ,
while Outputs may remain in the device until their value is consumed.

Most of the information required is in
https://gitlab.esperanto.ai/software/host-sw/blob/master/Neuralizer/inc/EBuffer.h
class and in
https://gitlab.esperanto.ai/software/host-sw/blob/master/Neuralizer/inc/NeuralizerBufferTypes.h
enumerators.

Here a brief description of the properties of the tensors that we
expect to load in the device:

Placeholders
^^^^^^^^^^^^

Placeholders are the inputs and outputs of a network. These tensors
change their value in every execution and thus need to be transferred
from host to device (inputs), and device to host (outputs) for a given
run. A tensor placeholder can also be input and output at the same
time requiring host to device transfer before execution and back to
host after execution.

Constants
^^^^^^^^^

Constants are immutable tensors that can be reused among runs. Hence,
it could make sense to allocate them in different memory regions.


**Weights**

Filters (4D) (convolutions) and Weights (2D)
(fully-connected/matmul) are just few examples of
constants. Normally they are big tensors hat need to be aligned,
and may require additional transformations. For now, we do these
transformations in the host, so the runtime already receives the
tensors in the expected shape.

**Bias**

 Similar to weights, biases are also used in fully-connected
node. However, these tensors have only one dimension and are much
smaller.

**Scale**

 These are very small tensors (cache line size) used to
 quantize/dequantize other tensors. As tensorQuant instruction
 requires the scale to be in memory and not in a register, we
 generate these constants at compile time and pack them before
 loading the network. Normally the scale value is replicated to fit
 the cache line, but in row wise operators that may not be the
 case. Only used in quantized models

Internal
^^^^^^^^^
Bunch of memory used to allocate tensors generated and consumed
internally in the network. Neuralizer uses the internal region as a
single memory block, and reuses the space at free will during the
network execution (optimizing its space to be minimal :-)). From a
Runtime point of view, we allocate all this memory together, and only
deallocate it once the execution of the network finishes. Neuralizer
also uses a special memory region as scratchpad for internal
activations.

Memory Regions Of Each Network & Lifetime
-----------------------------------------

+---------------+---------------+------------------------------------------------+--------------------------------+-----------------------------------+
| Region Name   |  Buffer Name  |      Description                               |  Desired Access Permissions    | Lifetime                          |
+===============+===============+================================================+================================+===================================+
| Code          |  Code         | ELF files that hold the code of the different  |  * PCIe: Write                 | * Allocation when model is loaded |
|               |               | compute kernels that we want to execute        |  * Minion: Read, Execute       | * Deallocation when model is      |
|               |               |                                                |                                |   removed from device             |
+---------------+---------------+------------------------------------------------+--------------------------------+-----------------------------------+
| Constants     | Weights       | Immutable input tensors                        |  * PCIe: Write                 | * Allocation when model is loaded |
|               | Bias          |                                                |  * Minion: Read only           | * Deallocation when model is      |
|               | Scale         |                                                |                                |   removed from device             |
+---------------+---------------+------------------------------------------------+--------------------------------+-----------------------------------+
| Placeholders  | Activations   |  Network inputs                                | PCIe: write, Minion: Read Only | Alloc/Dealloc per inference       |
|               |---------------+------------------------------------------------+--------------------------------+                                   |
|               | Output        |  Network outputs                               | Minion: Write Only             |                                   |
|               |---------------+------------------------------------------------+--------------------------------+                                   |
|               | InOut         |  Network in/out                                | Minion: Read / Write           |                                   |
+---------------+---------------+------------------------------------------------+--------------------------------+                                   |
| Internal      | Internal      | Neuralizer spill area                          | Minion : Read / Write          |                                   |
+---------------+---------------+------------------------------------------------+--------------------------------+-----------------------------------+


Implementation Details
----------------------

Memory Regions
^^^^^^^^^^^^^^


.. figure :: _static/MemoryRegions.svg
  :scale: 100%

  The proposed memory regions


**Memory Regions**

The DRAM is going to be divided with two main memory regions

Code
  The code data-region will hold the compute-kernel ELF files
  of each network, could be one if we have an uber-kernel. This
  region will allow only read and execute permissions.

Data
  The data region holds both the constant and placeholder
  tensors of a network. The two types of tensor types have been
  separate because they have different access permissions
  requirements and lifecycles: Constants are read-only data that are
  allocated once along side with the code, and we should not execute
  from. Placeholder tensors change on each inference run that we do,
  we need to write to them. Also multiple inferences might be
  pipe-lined to execute back-to-back and multiple copies of
  placeholder tensors could be allocated.

Logging
  The logging region is a carve-out of memory that the
  runtime allocated to store the logs that Master and Worker Minions
  will generate during execution.

The code is in a separate region so that we can apply uniform memory
protection to all code in the system. If we had virtual memory we
could use a single 1GB big-page to store all code in the system.


Memory Allocators
^^^^^^^^^^^^^^^^^

To simplify the implementation we are going to have 1 memory allocator per device.

As it has been eluded, the memory allocator logic will actually
execute inside the host-runtime and not inside the
device-minion-runtime. This is done to keep the minion-runtime simple
at this point. The functionality could move inside the minion-runtime
in the future, once we have silicon back and FW development is easier.

The device memory-allocator will employ internally different
allocators per memory region:

Linear Memory Allocator
  The linear memory allocator, is a simple allocator that does a
  greedy search trying to find the first free memory region, big
  enough to hold the requested memory.

Pool Memory Allocator:
  The pool memory allocator will hold a list of memory regions that
  can be re-used to allocate place-holder buffers of each network. The
  Pool allocator will create a separate pool of buffers per network we
  load on the device and will try to re-use the same memory regions
  per network inference as possible.

Code-Region
  The code region will use a single linear allocator. Its size will be
  fixed at device boot/initialization time : e..g 1 GB

Data-Region:
  The data-region will use both a linear and a pool memory
  allocator. The memory available to each allocator will grow over
  time until they “collide”

The exact sizes of each memory region and allocator sizes will have to
be tuned as we start running more networks on the device and can
develop potential heuristics.

On Device Memory Meta-Data
^^^^^^^^^^^^^^^^^^^^^^^^^^

To facilitate debugging of memory different allocated buffers/tensors
will have the following meta-data to allow us to associate the buffer
back to the original execution and system execution

Common Buffer Types are defined in :ref:`file_src_MemoryManagement_BufferInfo.h`

+-------------------------------------------------------------------+--------------------+
| Struct                                                            | Description        |
+===================================================================+====================+
| :class:`et_runtime::device::memory_management::BufferCommonMD`    | Common meta-data   |
+-------------------------------------------------------------------+--------------------+
| :class:`et_runtime::device::memory_management::CodeBuffer`        | Code region buffer |
+-------------------------------------------------------------------+--------------------+
| :class:`et_runtime::device::memory_management::ConstantBuffer`    | Constant Buffer    |
+-------------------------------------------------------------------+--------------------+
| :class:`et_runtime::device::memory_management::PlaceholderBuffer` | Place holder       |
+-------------------------------------------------------------------+--------------------+
| :class:`et_runtime::device::memory_management::LoggingBuffer`     | Logging Buffer     |
+-------------------------------------------------------------------+--------------------+


Memory Allocator Implementations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Abstract Memory Allocator
"""""""""""""""""""""""""

The top-level abstract allocator class is implemented
under :class:`et_runtime::device::memory_management::BaseMemoryAllocator`

Linear Memory Allocator
"""""""""""""""""""""""

The Linear Memory Allocator is implemented under class
:class:`et_runtime::device::memory_management::LinearAllocator`


Bidirectional Memory Allocator
""""""""""""""""""""""""""""""

The Bidirectional Memory Allocator is implemented under class
:class:`et_runtime::device::memory_management::BidirectionalAllocator`

Combined Memory Allocator
"""""""""""""""""""""""""

The class :class:`et_runtime::device::memory_management::MemoryManagerInternals`
implements the combined memory allocator that separates the code and data regions.
