import os, sys, stat, time
import socket
sys.path.append(".")

tf_defs = {
    "tx_fifo_path":"run/sp_uart1_tx",
    "rx_fifo_path":"run/sp_uart1_rx",
    "create_tx_fifo":"mkdir -p run; mkfifo -m=0666 ",
    "create_rx_fifo":"mkdir -p run; mkfifo -m=0666 ",
    "launch_tf_manager":"cd ../;./launchSim 9999 &",
    "start_sim_cmd":"1",
    "stop_simcmd":"2"
}

HOST = '127.0.0.1'  # tf_manager IP
PORT = 9999         # tf_manager port

class TfEnv:
    def __init__(self, env_type):
        if(env_type == "sim"):
            #Create names UNIX fifos to communicate with simulator
            os.system(tf_defs["create_tx_fifo"]+tf_defs["tx_fifo_path"])
            os.system(tf_defs["create_rx_fifo"]+tf_defs["rx_fifo_path"])
            #Launch TF Manager application that manages lifecycle of simulator
            print("Launch TF Manager")
            os.system(tf_defs["launch_tf_manager"])
            time.sleep(1) # some time for tf manager to launch
            self.conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.conn.connect((HOST, PORT))
            self.conn.send(tf_defs["start_sim_cmd"].encode())
            time.sleep(1) # some time for sim to launch


    def finalize(self, env_type):
        if(env_type == "sim"):
            print("Tear down environment")
            self.conn.send(tf_defs["stop_simcmd"].encode())
            time.sleep(1)  # some time for sim stop
            self.conn.close()
            # Forcefully close sim
            pNames = os.popen("ps -Af").read()
            if pNames.count('launchSim') > 0:
                os.system("pkill launchSim")