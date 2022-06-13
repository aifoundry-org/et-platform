# Debug Server

## Usage: ./debug_server [-p port] [-n device_index] [-s shire_id] [-m thread_mask] [-t bp_timeout] [-x file_path] [-h]

    Optional input arguments:
        -p specify the TCP port on which the debug server is to be started, default:51000
        -n ETSoC1 device index on which the debug server is to be started, device_index:0
        -s Shire ID
        -m Thread mask
        -t Breakpoint timeout(ms)
        -x Metadata file path consisting of details of FW data to be dumped
        -h Help, usage instructions