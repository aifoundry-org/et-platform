.. _Esperanto_Device:

Esperanto Device
================

The :class:`et_runtime::Device` class provides the basic interface for interacting with the Esperanto
Device.

It exposes interaces to:

* Initialize or reset a Device
* The memory allocator for the Device
* The stream manager for the specific Device
* Copy memory to/from the device

Internally the Device class can target those operations in 2 seaprate different types of
classes:

* The PCIE Device: This type of device talks to an actually "physical" PCIe device on the system

* An RPC Device: This a "fake" type of devices that emulates the behavior of a real PCIe device
  but the underlying target is a simulator

Memory Management
-----------------

Stream Management
-----------------

The :class:`et_runtime::Device` provides two ways to access a :class:`et_runtime::Stream` object:

* One it to access the default stream that each device has by calling
  :func:`et_runtime::Device::defaultStream` , or

* Create a new :class:`et_runtime::Stream` by calling :func:`et_runtime::Device::streamCreate`.


Internal Target Device
----------------------

The internal device interface is implemented by :class:`et_runtime::device::DeviceTarget`. This
class is not expected to be used directly by the user. It implements the underlying operations that
interact directly with the HWdevice: e.g. do an MMIO or a DMA transation or send a Mailbox Message.
The DeviceTarget class is the abstract interface implemented by target classes like :class:`et_runtime::device::PCIeDevice` :class:`et_runtime::device::TargetSysEmu` that perform the actual interactions
with the devices. The classes that are expected to interact with the DeviceTarget classes are the
:class:`et_runtime::device_api::Command`, :class:`et_runtime::device_api::Response`,
:class:`et_runtime::device_api::Event` derived classes

.. _Esperanto_Device_operations:
Device Operations and Interactions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

A device target can support the following types of interactions:

* Basic initialization, teardown and statu reporting
* DRAM reads / writes through MMIO.
* DRAM reads / writes using DMA.
* Mailbox reads and writes.

The DRAM region of the device is used for tranfering "data" on the device. In our case "data" are:

* Kernel and library ELF files.
* Netowrk tensors.
* Log and other tracing information.

The mailbox is the official mechanism for sending and receiving messages to/from the Master-Minion and the
Service Processor. In the mailbox we can have messages of type:

* Commands to the Master Minion.
* Responses from the Master Minion.
* Events, which are asynchronous messages and notifications from the Master-Minion.


.. include:: Devices/PCIEDevice.rst


.. include:: Devices/RPCDevice.rst
