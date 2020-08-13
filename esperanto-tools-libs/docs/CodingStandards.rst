Runtime Coding Guidelines
=========================

This Section describes the different coding guidelines of the Runtime Project.


Coding Standards
----------------

In this project we are following the `Google code style guide <https://google.github.io/styleguide/cppguide.html>`_


Code Formatting
---------------

As part of each commit you should run the ``clang-format`` by executing script:

.. code-block:: bash
   :linenos:

   cd <REPO>/host-software/esperanto-tools-libs
   ./scripts/git-clang-format


Code Organization
-----------------

Public Interface
________________
:ref:`namespace_et_runtime`

Internal Interface
__________________


- :ref:`namespace_et_runtime__device`

- :ref:`namespace_et_runtime__device_api`

- :ref:`namespace_et_runtime__device_fw`

- :ref:`namespace_et_runtime__support`

- :ref:`namespace_et_runtime__tracing`



Return Values and Error Reporting
---------------------------------
The list of valid Runtime error codes are defined in  :class:`etrtError`.

It is highly discouraged to return values from functions using output value
arguments: i.e. passing a pointer to an object that gets modified inside a
function.

Instead, users are encouraged to return objects through the return value of a
function. If an object needs to be returned, then that object needs to be
returned through a ``std::unique_ptr`` or a ``std::shared_ptr``.

If we need to return both an error-code or a value from a function, then the user
should make use of the :class:`et_runtime::ErrorOr` class. This class holds both
a :class:`etrtError` or the value of interest. The user should always check that
an error has not been raised as part of a call.

Memory Management
-----------------


Thread Management
-----------------
