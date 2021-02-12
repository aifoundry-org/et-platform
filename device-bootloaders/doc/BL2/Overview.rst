Device BootLoader->BL2 Overview
===============================


BL2 Device Management (DM) Support
----------------------------------
The BL2 provides support for managing DM services over PCIe interface aka InBand DM:

   - **Asset Tracking and Management** : The asset tracking service provides information about hardware and software assets present on the chip such as manufactures name, form factor, firmware version, etc.
   - **Thermal and Power Management** : This service deals with the thermal and power management of the chip such as providing the chip temperature, voltage, power, setting the temperature thresholds etc.
   - **Error Monitoring and Control** :	This services allows the host to configure threshold for various bus and memory errors.
   - **Link Management** : This service handles link management requests such as PCIE rest, get link width etc.   
   - **Performance Management** : Provides performance metric such as chip utilization, memory BW, clocks frequencies etc
   - **Master Minion (MM) State** : Provides execution state of Master Minion
   - **Historical Extreme Values** : Provides the historical maximum value for various chip parameters such as temperature, bus and memory error count, etc.
   - **Firmware Update** : Provides the support to update the SP and MM firmware.

Architecture
------------

The figure below depicts the architecture of DM services in the BL2.

.. image:: dm_bl2_arch.png
  :width: 400

MSI ISR
^^^^^^^

The ISR function handles the interrupt triggered by the host PCIE driver. Host triggers the interrupt after copying the DM request message to Submission Queue (SQ). The ISR notifies the availability of DM command to command dispatcher task.

DM Command Dispatcher Task
^^^^^^^^^^^^^^^^^^^^^^^^^^

The DM command dispatcher task handles all the incoming DM services requests from the host. This task waits on the notification from the ISR to commence the request processing. The DM task performs the following operations:

   - Pops the DM request from the SQ
   - Parses the request and dispatches the request to appropriate DM service driver
   - Pushes the response to Completion Queue(CQ)


DM Service Driver
^^^^^^^^^^^^^^^^^

This component handles low-level details to get/set hardware parameters. The DM service driver has a function call interface with the dispatcher task and all the calls are initiated by it. The DM service driver  returns or configures the required parameters from the following sources:

  - **Global structures** : Populated by the DM sampling task
  - **Hardware registers** : Parameters which are obtained or configured directly in registers
  - **PMIC** : Parameters which are obtained or configured via PMIC
  - **MM** : Parameters obtained from the MM

DM Sampling Task
^^^^^^^^^^^^^^^^

This thread periodically samples the hardware resources (hardware registers and PMIC) and saves the values in the global data structure. These values are fetched by the appropriate service driver in response to command dispatcher request. The cached values are used to minimize the request processing latency.


Processing Model
----------------
  - Host pushes the DM request command in the Submission Queue
  - Host inform the Service Processor using a PCIe interrupt
  - The SP PCIe ISR wakes up the Command dispatcher task
  - Command dispatcher task pops the message from the SQ
  - Command dispatcher task parses the command header 
  - Command is dispatched to the appropriate DM service handler
  - The Service handler will execute the command and respond with the appropriate status
  - The response in the form of a packet is pushed into the CQ
  - Then a PCIe interrupt is sent to the host to inform the availability of the response
  - Host DM library will according process the response and update the User application
