Synchronization Primitives and DeviceAPI
========================================

This section tries to describe the solution to two problems that the Runtime needs to provide:

* How can we perform operations on the Device, :ref:`see <Esperanto_Device_operations>` for the types of
  operations a Device can support.
* How we can synchronize across an operation and its side-effects and enable us to operate
  both an in blocking or an asynchronous fashion.

Type of Operations
------------------

A device can effectively do two different types of operations:

* Data transfers: Effectively read/write to the Device DRAM.
* Mailbox transfers: The mechanism to exchange information with the Master-Minion and the Service-Processor.

For either type of operation we have 3 types of primitives:

* A Command: a command is responsible to initiating an operation on the Device.
* A Response: a response carries the result of a command and it is associated 1-1 with a specific command.
* An Event: an event is any asynchronous message that the device generates that has not been initiated, and it
  is not expected by the Runtime.

The Data related operations are contained in namespaces:

* :ref:`namespace_et_runtime__device_api__pcie_responses`
* :ref:`namespace_et_runtime__device_api__pcie_commands`

The Mailbox related operations are contained in:

* :ref:`namespace_et_runtime__device_api__devfw_commands`
* :ref:`namespace_et_runtime__device_api__devfw_responses`
* :ref:`namespace_et_runtime__device_api__devfw_events`

Primitive Types
---------------

The type primitives that are the basic blocks are:

Command Class
^^^^^^^^^^^^^

The :class:`et_runtime::device_api::Command` template class provides the basic interface
that all commands should implement:

* Takes as a template argument the Response class that is associated with the specific Command
* Another template argument is the information that the command is expected to send to the Device.
  This is a "helper" argument that enables auto-generation of the commands that have similar behavior.
* A synchronization mechanism that will enable the calling thread to wait until the actions and Response
  for this Command have finished.

Response Class
^^^^^^^^^^^^^^
The :class:`et_runtime::device_api::Response` template class provides the basic interface
that all responses should implement, and allows us to retrieve the reply information that the Device
generated.

Event Class
^^^^^^^^^^^
The :class:`et_runtime::device_api::Event` class holds the payload from and event that the Device reported.

Synchronization Support
-----------------------

One of the problems that we need to solve is that for a given ``Command`` we need to wait until its
side-effects have taken effect and there is a result ready. The solution to problem is :

* A ``Command`` internally holds a `std::promise <https://en.cppreference.com/w/cpp/thread/promise>`_ . The
  ``std::promise`` will serve as the synchronization mechanism we will use to block execution until the
  results of a command are ready.

* The :func:`et_runtime::device_api::Command::setResponse` call sets the data of the ``Response`` for this
  ``Command``. The function internally sets the value of the ``std::promise`` that the ``Command`` holds,
  and unblocks access to the ``std::future`` that we can return from the ``std::promise``.

* A ``Command`` returns a `std::future <https://en.cppreference.com/w/cpp/thread/future>`_ in
  :func:`et_runtime::device_api::Command::getFuture`. The ``std::future`` holds the data of the
  Response that is associated with the Command. This is a blocking funciton that will not return until in
  another thread or previously we have called ``setResponse`` and update the value of the ``std::promise``.

We are going to provide more detail about the internal threads Runtime creates in :ref:`Stream_Management`,
but a simple example use of the synchronization mechanism is the following:

+-----------------------------+------------------------+
|   Thread A                  |  Thread B              |
+=============================+========================+
|::                           |                        |
|                             |                        |
|    auto cmd = new Command() |                        |
|    cmd->execute()           |                        |
|                             | ::                     |
|                             |                        |
|                             |     cmd->setResponse() |
|::                           |                        |
|                             |                        |
|     cmd->getFuture()        |                        |
+-----------------------------+------------------------+

In the above example `Thread A` will block until we set the response in `Thread B`.
The execution of the command and the update of the response could be in separate threads because we could
have a thread that blocks on a ``read`` of the Mailbox, waiting for a reply to get generated from the
Master Minion.
