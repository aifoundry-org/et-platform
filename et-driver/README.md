## ET-SOC1 PCIe Linux Kernel Driver

The driver registers device files named `/dev/et%d_mgmt` and `/dev/et%d_ops`, one for each enumerated device.

### To install from source:

* You can simply run `make` and `insmod et-soc1.ko`...
* ...or if you make sure to install dkms:
    * `apt install dkms` (Debian, Ubuntu)
    * `dnf install dkms` (Fedora)
    * `apk install akms` (Alpine)
    * `dnf install epel-release && dnf install dkms` (Enterprise Linux based)

and then:
```
make dkms
```
* For Alpine linux
```
make akms
```

### To uninstall:
```
make dkms-remove
```
* For Alpine linux
```
make akms-remove
```
