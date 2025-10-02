Device API  {#device_api}
==========

This repo contains:

## The Device API Specification

This is the list of messages that are being exchanged between the host
SW-stack (run-time) and the Device-Firmware over PCIe. The API is versioned
using Sem-Versioning.

The messages are currently divided in two categories:

* Commands: That are the commands that the host sends to the device
* Responses: Are the replies that the device sends back for a specific command
  The plan is that there is a one-to-one match between a command and its respective
  response type.
* Events: Are messages that originate from the device and are not triggered by a respective
  command.


## The Device API Infrastructure

We need to be able to serialize and deserialize the messages of the deviceApi from both
the host and the device. The host run-time SW stack is written in C++ while the device-firmware
is written in C. Also we need to have support for python tools that will be able to parse
or generate traces of messages to support testing, debugging, and performance analysis tools.

For this reason we have selected that we are going to describe the schema of the deviceApi
messages using a high level description language and auto-generate tools and other collateral
from there.

The details are described in \subpage device_api_infra_ref


## Simulator API Infrastructure

In our infrastructure we have a number of simulators that communicate over a socket with the
host runtime or PCIe device: e.g. CVS, SysEMU, Minionsim. For debugging and testing reasons
we need to be able to replace the direct socket connection with a trace-feeder or consumer.
This way we can decouple the host and the target simulator and drive them independently and
enable component testing.

The details are under \subpage sim_api
