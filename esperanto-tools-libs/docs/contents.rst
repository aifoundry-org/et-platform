Welcome to Esperanto Runtime's documentation!
=============================================
.. toctree::
   :maxdepth: 2

   about
   api/runtime_root

Esperanto Runtime Purpose
-------------------------

The primary purpose that the Esperanto runtime serves is to abstract to the
user the interactions with the real hardware device and the Esperanto PCIe driver.

Runtime API Flavors
__________________

The Esperanto Runtime has both a C and a C++ flavor. The implementation is done in
C++ and is the primarily supported API. The C-API just wrapps the C++ implementation

* C-API is described in :ref:`file_C-API_etrt.h`
* C++ API, the top-level class is :class:`et_runtime::DeviceManager`

C++ Runtime API
----------------

The major classes the user needs to be aware of are:

* :class:`et_runtime::DeviceManager`
* :class:`et_runtime::Device`
* :class:`et_runtime::Stream`
* :class:`et_runtime::Event`
* :class:`et_runtime::Kernel`

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
