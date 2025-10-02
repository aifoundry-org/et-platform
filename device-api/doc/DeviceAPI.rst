.. _DeviceAPI:

.. toctree::
   :maxdepth: 2


************************
Device API Specification
************************

What is the Device-API
======================

The Device-API repo provides:

* The specification of two APIs that the Device (Device-Minion-Runtime) implements,
  and the host calls (We will also use the term "Device-API-segment" to describe them.) .
  The Device-API is comprised of a number of messages that can be exchanged between the host
  and the device.

* The infrastructure to auto-generate C and C++ code from an abstract API specification
  that describes the contents of the different messages.


The two APIs implemented are the :ref:`PrivilegedAPI` and the :ref:`NonPrivilegedAPI`.
The code generation infrastructure is described in :ref:`DeviceAPICodeGen`

The APIs follow the versioning rules described in :ref:`DeviceAPIVersioning`.

Build instructions can be found under :ref:`BuildInstructions`.


.. _PrivilegedAPI:

Privileged API
==============

The Privileged API describes the set of RPC commands/responses and events a "privileged"-task
on the host, in our case the PCIe driver, can send to the device. Those commands
are "sensitive", as in they can perturb the state of the device, or cause non recoverable
side-effects (e.g.  start a DMA when a DMA has not been programmed yet), and should not be
issued by any host user-space application.

The Privileged segment of the Device-API is described under folder
``<REPO>/src/device-api/privileged-api``

The generated files that hold the API are:

* :ref:`file_src_device-api_device_api_spec_privileged.h`

* :ref:`file_src_device-api_device_api_rpc_types_privileged.h`

* :ref:`file_src_device-api_device_api_cxx_privileged.h`

.. _NonPrivilegedAPI:

Non-Privileged API
==================

The Non-Privileged segment of the Device-API holds all commands/responses/event that a
"non-privileged"-task on the host can send to our device. The primary caller of this
API is the Host-Runtime library.

The Non-Privileged segment is described under ``<REPO>/src/device-api/non-privileged-api``

The generated files that hold the API are:

* :ref:`file_src_device-api_device_api_spec_non_privileged.h`

* :ref:`file_src_device-api_device_api_rpc_types_non_privileged.h`

* :ref:`file_src_device-api_device_api_cxx_non_privileged.h`
