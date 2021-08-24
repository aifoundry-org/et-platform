# Device Management Tester

## Sysemu

### Usage: dm-tester -o ncode | -m command [-n node] [-u nmsecs] [-h][-c ncount | -p active_pwr_m | -r nreset | -s nspeed | -w nwidth | -t nlevel | -e nlotemp,nhitemp]

        -o, --code=ncode
                Command by ID (see below)

                Ex. dm-tester -o 0

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

                Ex. dm-tester -m DM_CMD_GET_MODULE_MANUFACTURE_NAME

        -n, --node=node
                Device node by index

                Ex. dm-tester -o 0 -n 0

        -u, --timeout=nmsecs
                timeout in miliseconds

                Ex. dm-tester -o 0 -u 70000

        -h, --help
                Print usage; this output

                Ex. dm-tester -h

        -c, --memcount=ncount
                Set memory ECC count for DDR, SRAM, or PCIE (ex. 0)

                Ex. dm-tester -o 36 -c 0
                Ex. dm-tester -m DM_CMD_SET_DDR_ECC_COUNT -c 0

                Ex. dm-tester -o 37 -c 0
                Ex. dm-tester -m DM_CMD_SET_PCIE_ECC_COUNT -c 0

                Ex. dm-tester -o 38 -c 0
                Ex. dm-tester -m DM_CMD_SET_SRAM_ECC_COUNT -c 0

        -p, --active_pwr_mgmt=active_pwr_m
                Set active power management:

                        1: ACTIVE_POWER_MANAGEMENT_TURN_ON
                        0: ACTIVE_POWER_MANAGEMENT_TURN_OFF

                Ex. dm-tester -o 23 -p 0
                Ex. dm-tester -m DM_CMD_SET_MODULE_ACTIVE_POWER_MANAGEMENT -p 0

        -r, --pciereset=nreset
                Set reset type:

                        2: PCIE_RESET_WARM
                        0: PCIE_RESET_FLR
                        1: PCIE_RESET_HOT

                Ex. dm-tester -o 39 -r 0
                Ex. dm-tester -m DM_CMD_SET_PCIE_RESET -r 0

        -s, --pciespeed=nspeed
                Set PCIE link speed:

                        1: PCIE_LINK_SPEED_GEN4
                        0: PCIE_LINK_SPEED_GEN3

                Ex. dm-tester -o 44 -s 0
                Ex. dm-tester -m DM_CMD_SET_PCIE_MAX_LINK_SPEED -s 0

        -w, --pciewidth=nwidth
                Set PCIE lane width:

                        1: PCIE_LANE_W_SPLIT_x8
                        0: PCIE_LANE_W_SPLIT_x4

                Ex. dm-tester -o 45 -w 0
                Ex. dm-tester -m DM_CMD_SET_PCIE_LANE_WIDTH -w 0

        -t, --tdplevel=nlevel
                Set TDP level:

                        4: TDP_LEVEL_INVALID
                        0: TDP_LEVEL_ONE
                        1: TDP_LEVEL_TWO
                        2: TDP_LEVEL_THREE
                        3: TDP_LEVEL_FOUR

                Ex. dm-tester -o 25 -t 0
                Ex. dm-tester -m DM_CMD_SET_MODULE_STATIC_TDP_LEVEL -t 0

        -e, --thresholds=nlotemp,nhitemp
                Set temperature thresholds (low,high)

                Ex. dm-tester -o 21 -e 80,100
                Ex. dm-tester -m DM_CMD_SET_MODULE_TEMPERATURE_THRESHOLDS -e 80,100

## Zebu

### Usage: dm-tester -o ncode | -m command [-n node] [-u nmsecs] [-h][-c ncount | -p active_pwr_m | -r nreset | -s nspeed | -w nwidth | -t nlevel | -e nlotemp,nhitemp]
