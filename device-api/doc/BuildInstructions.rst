.. _BuildInstructions:

******************
Build Instructions
******************


How To Build This Repo
======================

This repo is built as a submodule of the sw-platform repo. We can also
build it stand-alone as it does not have external Esperanto build dependencies.

What Artifacts Are Generated ?
==============================

Inside the build folder a number of C/C++ headers will generated under
``<BUILD_FOLDER>/src/device-api``

The same set of headers are generated per segment of the Device-API, consists of:


* ``device_api_spec_*.h``: API specification for the specific API-segment. This file
  includes the API SemVersion and enumeration of API messages. It does not include any
  other API-segment specific types. All files of this prefix are intended to be included
  by a single source C file, and should have not name and type conflicts.

* ``device_api_rpc_types_*.h``: This header includes all C/C++ types that describe the different
  RPC messages that we can exchange in the different API-segments. These headers are not expected
  to be included together in the same source file, as they do not provide guarantees that the C/C++
  type names will not be conflicting. This header is a `pure-C` header and it is intended to be
  copied inside a `C-only` code-base if need be: e.g. PCIe driver.

* ``device_api_cxx_*.h``: C++ headers. This set of headers, are to be included by C++ code. They
  wrap the Device-API-segment types specified in the previous headers, in C++ namespaces (thus
  solving any name conflicts) and also implement additional functionality specific to C++.
  We have intentionally separated the C++ specific code per reasons described previously.

How Are the Artifacts Consumed
==============================

This repo has a default ``make install`` target that installs:

* The auto-generated headers under ``include/esperanto/device-api``

* The API specification YAML files, and JSON-Schema file  under ``lib//esperanto/device-api/``

The appropriate `CMAKE` package is being created and installed as well. Consumers of this
repo should use ``find_package(esperanto-device-api)`` to find this package, where it is installed
in the system.
