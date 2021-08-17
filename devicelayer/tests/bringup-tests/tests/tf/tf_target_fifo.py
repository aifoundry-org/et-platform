import pytest
import os, sys, time
import socket

from tf.tf_specification import TfSpecification

TF_CMD_CHECKSUM = b'#\xa5\xa5\xa5\xa5%'
class TargetFifo:
    def __init__(self, *args):
        #Initialize fifo target specific instance
        self.tx_fifo = args[0]
        self.rx_fifo = args[1]
        self.tf_spec = args[2]
        print("Fifo Target Instance: " + self.tx_fifo+":"+self.rx_fifo)
    def open(self):
        #open both tx and rx fifos
        self.tx_fd = os.open(self.tx_fifo, os.O_RDWR|os.O_NONBLOCK)
        self.rx_fd = os.open(self.rx_fifo, os.O_RDWR|os.O_NONBLOCK)
    def close(self):
        #close both tx and rx fifos
        os.close(self.tx_fd)
        os.close(self.rx_fd)
    def execute_test(self, command, read_timeout = 2 * 60 ): # 2 min read timeout
        # Write command
        os.write(self.rx_fd, command)

        # Read back the response
        # wait for 1 sec
        time.sleep(1)
        rbytes = b''
        timeout = time.time() + read_timeout
        while True:
            try:
                recv_byte = os.read(self.tx_fd, 4096)
            except OSError:
                if time.time() > timeout:
                    print('TIMEOUT: Waiting for the response')
                    break
                time.sleep(200 / 1000)
                continue

            rbytes += recv_byte

            # Break if tf command checksum is found
            if TF_CMD_CHECKSUM in rbytes:
                break

        response = self.tf_spec.response(rbytes)
        return response