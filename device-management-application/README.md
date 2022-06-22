# Device Management Application

## Sysemu

### Usage: dev_mngt_service -o ncode | -m command [-n node] [-u nmsecs] [-h][-c ncount | -p active_pwr_m | -r nreset | -s nspeed | -w nwidth | -t nlevel | -e nswtemp]

        -o, --code=ncode
                Command by ID (see below)

                Ex. dev_mngt_service -o 0

        -m, --command=command
                Command by name:

                        55: DM_CMD_GET_MM_ERROR_COUNT
                        54: DM_CMD_GET_ASIC_LATENCY
                        44: DM_CMD_SET_PCIE_MAX_LINK_SPEED
                        39: DM_CMD_SET_PCIE_RESET
                        50: DM_CMD_GET_DRAM_CAPACITY_UTILIZATION
                        38: DM_CMD_SET_SRAM_ECC_COUNT
                        35: DM_CMD_GET_MAX_MEMORY_ERROR
                        34: DM_CMD_GET_MODULE_MAX_DDR_BW
                        43: DM_CMD_GET_MODULE_SRAM_ECC_UECC
                        33: DM_CMD_GET_MODULE_RESIDENCY_POWER_STATES
                        53: DM_CMD_GET_ASIC_STALLS
                        32: DM_CMD_GET_MODULE_MAX_TEMPERATURE
                        49: DM_CMD_GET_DRAM_BANDWIDTH
                        31: DM_CMD_GET_MODULE_POWER
                        37: DM_CMD_SET_PCIE_ECC_COUNT
                        30: DM_CMD_GET_MODULE_VOLTAGE
                        28: DM_CMD_GET_MODULE_RESIDENCY_THROTTLE_STATES
                        25: DM_CMD_SET_MODULE_STATIC_TDP_LEVEL
                        24: DM_CMD_GET_MODULE_STATIC_TDP_LEVEL
                        23: DM_CMD_SET_MODULE_ACTIVE_POWER_MANAGEMENT
                        21: DM_CMD_SET_MODULE_TEMPERATURE_THRESHOLDS
                        20: DM_CMD_GET_MODULE_TEMPERATURE_THRESHOLDS
                        19: DM_CMD_SET_FIRMWARE_VALID
                        41: DM_CMD_GET_MODULE_DDR_BW_COUNTER
                        18: DM_CMD_SET_FIRMWARE_VERSION_COUNTER
                        51: DM_CMD_GET_ASIC_PER_CORE_DATAPATH_UTILIZATION
                        42: DM_CMD_GET_MODULE_DDR_ECC_UECC
                        47: DM_CMD_RESET_ETSOC
                        29: DM_CMD_GET_MODULE_UPTIME
                        16: DM_CMD_SET_SP_BOOT_ROOT_CERT
                        40: DM_CMD_GET_MODULE_PCIE_ECC_UECC
                        15: DM_CMD_GET_FIRMWARE_BOOT_STATUS
                        46: DM_CMD_SET_PCIE_RETRAIN_PHY
                        14: DM_CMD_SET_FIRMWARE_UPDATE
                        13: DM_CMD_GET_MODULE_FIRMWARE_REVISIONS
                        12: DM_CMD_GET_FUSED_PUBLIC_KEYS
                        11: DM_CMD_GET_MODULE_MEMORY_TYPE
                        8: DM_CMD_GET_MODULE_REVISION
                        22: DM_CMD_GET_MODULE_POWER_STATE
                        10: DM_CMD_GET_MODULE_MEMORY_VENDOR_PART_NUMBER
                        6: DM_CMD_GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED
                        7: DM_CMD_GET_MODULE_MEMORY_SIZE_MB
                        5: DM_CMD_GET_MODULE_PCIE_ADDR
                        52: DM_CMD_GET_ASIC_UTILIZATION
                        45: DM_CMD_SET_PCIE_LANE_WIDTH
                        17: DM_CMD_SET_SW_BOOT_ROOT_CERT
                        3: DM_CMD_GET_ASIC_CHIP_REVISION
                        2: DM_CMD_GET_MODULE_SERIAL_NUMBER
                        1: DM_CMD_GET_MODULE_PART_NUMBER
                        48: DM_CMD_GET_ASIC_FREQUENCIES
                        36: DM_CMD_SET_DDR_ECC_COUNT
                        26: DM_CMD_GET_MODULE_CURRENT_TEMPERATURE
                        9: DM_CMD_GET_MODULE_FORM_FACTOR
                        0: DM_CMD_GET_MODULE_MANUFACTURE_NAME

                Ex. dev_mngt_service -m DM_CMD_GET_MODULE_MANUFACTURE_NAME

        -n, --node=node
                Device node by index

                Ex. dev_mngt_service -o 0 -n 0

        -u, --timeout=nmsecs
                timeout in miliseconds

                Ex. dev_mngt_service -o 0 -u 70000

        -h, --help
                Print usage; this output

                Ex. dev_mngt_service -h

        -c, --memcount=ncount
                Set memory ECC count for DDR, SRAM, or PCIE (ex. 0)

                Ex. dev_mngt_service -o 36 -c 0
                Ex. dev_mngt_service -m DM_CMD_SET_DDR_ECC_COUNT -c 0

                Ex. dev_mngt_service -o 37 -c 0
                Ex. dev_mngt_service -m DM_CMD_SET_PCIE_ECC_COUNT -c 0

                Ex. dev_mngt_service -o 38 -c 0
                Ex. dev_mngt_service -m DM_CMD_SET_SRAM_ECC_COUNT -c 0

        -p, --active_pwr_mgmt=active_pwr_m
                Set active power management:

                        1: ACTIVE_POWER_MANAGEMENT_TURN_ON
                        0: ACTIVE_POWER_MANAGEMENT_TURN_OFF

                Ex. dev_mngt_service -o 23 -p 0
                Ex. dev_mngt_service -m DM_CMD_SET_MODULE_ACTIVE_POWER_MANAGEMENT -p 0

        -r, --pciereset=nreset
                Set reset type:

                        2: PCIE_RESET_WARM
                        0: PCIE_RESET_FLR
                        1: PCIE_RESET_HOT

                Ex. dev_mngt_service -o 39 -r 0
                Ex. dev_mngt_service -m DM_CMD_SET_PCIE_RESET -r 0

        -s, --pciespeed=nspeed
                Set PCIE link speed:

                        1: PCIE_LINK_SPEED_GEN4
                        0: PCIE_LINK_SPEED_GEN3

                Ex. dev_mngt_service -o 44 -s 0
                Ex. dev_mngt_service -m DM_CMD_SET_PCIE_MAX_LINK_SPEED -s 0

        -w, --pciewidth=nwidth
                Set PCIE lane width:

                        1: PCIE_LANE_W_SPLIT_x8
                        0: PCIE_LANE_W_SPLIT_x4

                Ex. dev_mngt_service -o 45 -w 0
                Ex. dev_mngt_service -m DM_CMD_SET_PCIE_LANE_WIDTH -w 0

        -t, --tdplevel=nlevel
                Set TDP level in Watts:

                        0 < tdp level < 40

                Ex. dev_mngt_service -o 25 -t 0
                Ex. dev_mngt_service -m DM_CMD_SET_MODULE_STATIC_TDP_LEVEL -t 25

        -e, --thresholds=nswtemp
                Set temperature thresholds (software threshold)

                Ex. dev_mngt_service -o 21 -e 80
                Ex. dev_mngt_service -m DM_CMD_SET_MODULE_TEMPERATURE_THRESHOLDS -e 80

## Zebu

### Usage: dev_mngt_service -o ncode | -m command [-n node] [-u nmsecs] [-h][-c ncount | -p active_pwr_m | -r nreset | -s nspeed | -w nwidth | -t nlevel | -e nswtemp]

# et-top application

## Usage: et-top DEVICE (where DEVICE is 0-63)

Current et-top sample output:

        ET device 0 stats Fri Jun 17 20:52:23 2022

        Contiguous Mem Alloc:      0 MB      0 MB/sec
        Uncorrectable Errors:      0
        Correctable Errors:        0
        PCI AER Errors:            0
        Queues:
                SQ0: msgs: 0          msgs/sec: 0          util%: 0
                SQ1: msgs: 0          msgs/sec: 0          util%: 0
                CQ0: msgs: 0          msgs/sec: 0          util%: 0

        Watts:                                            Temp(C):
                CARD        avg: 105  min: 40   max: 212
                - ETSOC     avg: 0    min: 0    max: 0    ETSOC     avg: 0    min: 0    max: 0
                  - MINION  avg: 0    min: 0    max: 0    - MINION  avg: 51   min: 51   max: 51
                  - SRAM    avg: 0    min: 0    max: 0
                  - NOC     avg: 0    min: 0    max: 0
        Compute:
                Thru put    Kernel/sec    avg: 0      min: 0      max: 0
                Util        Minion(%)     avg: 0      min: 46975  max: 0
                            DMA Chan(%)   avg: 0      min: 0      max: 0
                DDR BW      Read  (MB/s)  avg: 0      min: 46975  max: 0
                            Write (MB/s)  avg: 0      min: 46975  max: 0
                L3 BW       Read  (MB/s)  avg: 0      min: 46975  max: 0
                            Write (MB/s)  avg: 0      min: 46975  max: 0
                PCI DMA BW  Read  (GB/s)  avg: 0      min: 46975  max: 0
                            Write (GB/s)  avg: 0      min: 46975  max: 0
                            Write (GB/s)  avg: 0      min: 46975  max: 0
