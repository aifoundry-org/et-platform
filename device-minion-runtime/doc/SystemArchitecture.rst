Compute Firmware System Architecture
===================================

There are 2 different type of minions, all having same privilege modes but running different services/software on each layer depending on its functionality:

* Master Minion: minion 0-15 of the master shire, control the overall execution of compute workloads
* Compute Minions: there are 1024 of such minions, distributed in 32 different shires (shire Ids 0 to 31) and minions 16 to 31 of the master shire (shire Id 32)

The software that runs on the different type of minions is organized in 3 different
components:

* @subpage Mastet minion_firmware : Includes the software stack that runs on the Master-Minion.

* @subpage Worker minion firmware : Includes the software stack that runs on the Compute minions.

* @subpage Machine minion firmware : Describes the software stack running on the Master and Compute minions.

