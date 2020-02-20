Device Firmware System Architecture
===================================

There are 5 different type of minions, all having same privilege modes but running different services/software on each layer depending on its functionality:

* Service Processor: there’s a single instance of such minion in the IO shire
* Compute Minions: there are 1024 of such minions, distributed in 32 different shires (shire Ids 0 to 31).
* Sync Minions: minions 16 to 31 of the master shire (shire Id 32)
* Master Minion: minion 0 of the master shire
* Runtime Minions: minion 1 to 15 of the master shire

The software that runs on the different type of minions is organized in 3 different
components:

* @subpage master_minion_firmware : Includes the software stack that runs on the Master-Minion and the Runtime-Minions.

* @subpage slave_minion_firmware : Includes the software stack that runs on the Compute and Sync minions.

* @subpage service_processor : Describes the software stack running on the Service Processor.


High-level Execution Flows
---------------------------

This section provides a list of the different high-level execution flows that are supported
in the firmware and

* @subpage kernel_execution


---------------------------

@subpage sw-hw-interactions
