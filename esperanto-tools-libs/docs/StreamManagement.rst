.. _Stream_Management:

Stream Management And Device Threading
======================================

Code Streams
------------

As described in :class:`et_runtime::Stream`, a ``Stream`` is a serial queue of commands executing
in order one after the other. A ``Stream`` also provides the infrastructure to issue and
block after each individual command executes, or until a synchronization event has been met.

Device Command Management And Internal Threading
------------------------------------------------

All ``Command`` s submitted to a ``Stream`` have to eventually be serialized and execute in a
:class:`et_runtime::Device`. The ``Device`` provides a mechanism to be asynchronously notified
when a ``Response`` for a specfic ``Command`` has completed.

Data Transfer Async Support
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. note:

   Add description how we support async data


Mailbox Message Async Support
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The Runtime provides support for issueing DeviceAPI Commands and getting notified
asynchronously when a Response is ready.

The ``Device``, internally has 2 threads:

* The :member:`et_runtime::Device::command_executor_` thread, executes all commands
  serialized in the ``Device`` 's :member:`et_runtime::Device::cmd_queue_` queue, as the
  queue fills up. Per ``Command`` the executor calls its :func:`et_runtime::Command::execute` function.
  The ``execute`` function is supposed to interact with the device: e.g. submit the MailBox command
  to be executed by the Master-Minion. It is the resposibility of the ``Command`` to register a CallBack using
  :func:`et_runtime::Device::registerMBReadCallBack` function. The Callback will be executed when
  the ``Device`` recognizes that a Mailbox response has been received for the respective ``Command``.

* The :member:`et_runtime::Device::device_reader_` thread on the other hand, blocks on the Mailbox Device
  until a new ``Response`` or ``Event`` is available from MasterMinion. The DeviceAPI Response does
  contain the ``CommandID`` of the issuing ``Command`` and using that information, the ``Device`` can
  execute the callback of the ``Command``, set its ``Response`` information and unblock the issuing
  thread.

 The different thread interactions look like follows:

+------------------------------+----------------------------------+------------------------+
|  Main Thread / Stream        |   Command Executor Thread        |  Device Reader Thread  |
+==============================+==================================+========================+
|::                            |::                                |::                      |
|                              |                                  |                        |
|  auto cmd = new Command()    |  ..                              |   ..                   |
|  stream->addCommand(cmd)     |  ..                              |   ..                   |
|                              |  ..                              |   ..                   |
|                              |  cmd->execute()                  |   ..                   |
|                              |     dev->registerMBReadCallBack  |   rcv_mailbox          |
|                              |  ..                              |   CmdCallback()        |
|                              |  ..                              |     cmd->setResponse() |
|  ftr = cmd->getFuture()      |  ..                              |   ..                   |
|  // Block until the response |  ..                              |   ..                   |
|  // is set                   |  ..                              |   ..                   |
|  ftr.get()                   |  ..                              |   ..                   |
+------------------------------+----------------------------------+------------------------+
