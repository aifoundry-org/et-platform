import pytest
import os, sys, time
import socket

from tf.tf_specification import TfSpecification

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
    def execute_test(self, command, wait_for_response_secs = 1):
        os.write(self.rx_fd, command)
        time.sleep(wait_for_response_secs)
        #TODO:this can be improved.
        raw_response = os.read(self.tx_fd, 4096)
        response = self.tf_spec.response(raw_response)
        return response