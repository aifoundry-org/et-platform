# Debug Server

## Sysemu

### Usage: ./debug_server [-p port] [-n device_index] [-s shire_id] [-m thread_mask] [-t bp_timeout] [-h]

    Optional input arguments:
        -p specify the TCP port on which the debug server is to be started, default:51000
        -n ETSoC1 device index on which the debug server is to be started, device_index:0
	-s Shire ID
	-m Thread mask
	-t Breakpoint timeout(ms)
        -h Help, usage instructions