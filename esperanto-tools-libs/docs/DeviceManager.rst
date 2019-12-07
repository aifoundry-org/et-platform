.. _DeviceManager:

Device Manager
==============

The entrypoint class to the Esperanto Runtiem is the :class:`et_runtime::DeviceManager`.

This class provides the necessary functionality for:

* Enumerate / list the available devices on the system
* Register and take ownership of an Esperanto accelerator.

Currently we support that only one process can have acccess to an Esperanto accelerator.
This restriction is enforced at the level of the PCIE driver. The Runtime, and in our case the
:class:`et_runtime::DeviceManager` will, return an error if registraction fails.

Device Enumeration
------------------

To enumerate the available accelerators in the system you can make use of the
:func:`et_runtime::DeviceManager::enumerateDevices` function.


Register With A Device
----------------------

To register with a device you have to take ownership of a :class:`et_runtime::Device` object,
by calling the :func:`et_runtime::DeviceManager::registerDevice`. The :class:`et_runtime::Device` class
will be the "gateway" for you to access other resources on the device: e.g. to allocate memory or create
streams.
