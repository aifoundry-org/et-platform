.. _Runtime:

Esperanto Runtime Overview
==========================

The Esperanto runtime library provides the means to interface with the device
and the PCIe driver.

It maintains and keeps track of the state of the Esperanto accelerator.

More specifically the r provides the following functionality:

* Converts the functionality to provides through its API to specific
  Device-API commands. Per command, it keeps track if its status and
  waits for the respective Response to arrive from the Device.
  It communicates any Device-API Events reported from the device back to
  the caller of the Runtime library.

* It is responsible for loading the compiled code binaries on the device.

* It is responsible for transferring data (tensors to/from) the device.

* It is responsible for the memory management of the device and partitioning
  the DRAM across the different networks that are loaded on the device.

* It tracks the different user-space streams that execute on the host and result in different
  neural-network "launches" on the device, as well as their state and status.

* It enables system-wide logging, aggregating information across the Runtime library
  itself and the device.


Current Design Assumptions/Patterns
------------------------------------

A single process will be talking to the device.
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Currently we are assuming that a single Linux process/thread will be talking to the device
and launching kernels.

This design decision should be viewed as an intermediate step to enable our deliverables, that
helps us to simplify our SW stack currently and not as the desired design point.

A single process can register with multiple devices, but no multiple processes can register with a
single device.

This current design decision allows us to simplify our runtime library in the following ways:

* No need for thread safety. We need to be enforcing that no multiple threads try to execute
  calls in our runtime library. This should not be considered that in the future we should not need
  to re-write our library to be thread save. In general we should be avoiding global state that will
  hinder a future effort to parallelize our implementation.

.. todo::

   We need to describe how we make the Runtime library thread save


* The runtime can "easily" keep track of the memory state of the device, as we know that there is
  only one Linux process that can allocate device memory and create streams of execution.

* The kernel can assume that there is only one process registered with the device and if that process
  dies then we need to reset the device to bring it back to a clean state.

* Different instances of the library can be loaded from different Linux-processes

* The version of the runtime needs to be using the same/compatible version of the Device-API with the M&M Firmware.
  Having a 1-1 match between the runtime and the loaded M&M Firmware removes that problem.


Enabling multiple processes to talk to a single device
_______________________________________________________

To enable multiple threads/processes to run and talk to a single device the following design
changes will have to be made.

* Device memory management will have to move from the runtime to either the kernel or the device firmware.
* The runtime library needs to be thread safe
* We need to maintain an association of the memory+streams created per Linux process and what is running
  and is allocated on the device.

  * If a process dies then the runtime or the Linux-kernel need to reset and discard the partial
    state on the device while maintain what is alive.

  * Implementing global static state in a single library that is shared across multiple processes is not
    "wise" (?). So this functionality will have to move inside our kernel driver.

* We need to have a mature DeviceAPI that will allow even different version of the runtime to talk to the same
  device.

The runtime keeps track of the device memory status
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The runtime library keeps track of how the device memory is partitioned across the different kernels and keeps
track how memory is being used.

.. todo::

   We need to define the different memory regions on the device

The PCIe Linux Kernel Driver Only Moves Data
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Currently we are trying to limit the complexity of the PCIe Linux kernel driver. We want the kernel driver to maintain
as little state about the system and just be responsible for the data movement.

Device Management
^^^^^^^^^^^^^^^^^^

The purpose of the Device Management API is to:

* Enumerate the list of available devices in the system.
* Register the process with a device and "claim" ownership of the device for a specific Linux-thread.
* Programatically query and extract information about the static configuration of the device and its current status.
* Extract information about the loaded device driver.
* Other ?


Memory Management
------------------

The Runtime library needs to provide the following functionality to the caller:

* Enable the device memory management:

  * Reserve memory on the device.

  * Free memory on the device.

* Enable the data transfer between the host and the device:

  * Allocate DMA-able memory on the host.

  * Copy host memory to the device.

  * Copy device memory back to the host.

* Provide synchronous and asynchronous versions of the data-movement operations between the host and the device.

For more details see: :ref:`MemoryManagement`

Stream Management
------------------

To manage the different execution-contexts on the device we are organizing their "work" in streams.
A stream is a sequence of API calls that effectively send commands to the device and "sync" statements
where we try to inspect the state of the device based on the responses issues by the device.

For more details see: :ref:`StreamManagement`

Event Managemnt
----------------

.. todo::

   Populate the event description

For more details see: \ref ETCRT_EVENT_MGMT

Kernel Launch
-------------

For more details see: :ref:`KernelLaunch`

Device Profiling
-----------------

.. todo::

   Populate the related information and section
