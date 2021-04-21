import pytest
import os, sys, stat, time

sys.path.append(".")

from tf_target import *

#from tf_targets import target

tf_defs = {
    "base_path":"cd ../../../../build/host-software/deviceLayer/tests/",
    "create_tx_fifo_cmd":"mkfifo -m=0666 ",
    "create_rx_fifo_cmd":"mkfifo -m=0666 ",
    "path_to_tx_fifo":"sp_uart0_tx",
    "path_to_rx_fifo":"sp_uart0_rx",
    "launch_sysemu_cmd":"ctest -R device-layer:TestLaunchSysemu.LaunchSuccess &",
    "launch_sysemu": "true"
}

class tf:
    def __init__(self):
        if tf_defs["launch_sysemu"] == "true":
            print("Launching sysemu..")
            #Create names UNIX fifos
            os.system(tf_defs["create_tx_fifo_cmd"]+tf_defs["path_to_tx_fifo"])
            os.system(tf_defs["create_rx_fifo_cmd"]+tf_defs["path_to_rx_fifo"])
            #Launch sysemu
            os.system(tf_defs["launch_sysemu_cmd"])        
            time.sleep(1) # some time for sysemu to launch

@pytest.fixture()
def env_setup():
    env = tf()
    return env 


