import pytest
import os, sys, time

class Target:
    def __init__(self, target_type, *args):
        if (target_type == "fifo"):
            #Initialize fifo target specific instance
            #variables
            self.target = target_type
            self.tx_fifo = args[0]
            self.rx_fifo = args[1]
            print("Fifo Target Instance: " + self.tx_fifo+":"+self.rx_fifo)
        if(target_type == "serial"):
            #initialize serial target specific instance
            #variables 
            self.target = target_type
            self.serial_port = args[0]
            self.serial_baud = args[1]
            print("Serial Target Instance: " + self.serial_port+":"+str(self.serial_baud))
        if(target_type == "jtag"):
            #initialize JTAG target specific instance 
            #variables
            self.target = target_type
            print("Jtag target instance creation..")
    def open(self):
        if (self.target == "fifo"):
            #open both tx and rx fifos
            self.tx_fd = os.open(self.tx_fifo, os.O_RDWR|os.O_NONBLOCK) 
            self.rx_fd = os.open(self.rx_fifo, os.O_RDWR|os.O_NONBLOCK)
            #print("Opening TX fifo:" + self.tx_fifo)
            #print("Opening RX fifo:" + self.rx_fifo)
        if (self.target == "serial"):
            #open serial port
            print("Opening Serial port:" + self.serial_port)
        if (self.target == "jtag"):
            #open jtag connection
            print("Opening JTAG:" + self.serial_port)
    def close(self):
        if (self.target == "fifo"):
            #close both tx and rx fifos
            os.close(self.tx_fd) 
            os.close(self.rx_fd)
            #print("Closing TX fifo:" + self.tx_fifo)
            #print("Closing RX fifo:" + self.rx_fifo)
        if (self.target == "serial"):
            #close serial port
            print("Closing Serial port:" + self.serial_port)
        if (self.target == "jtag"):
            #close jtag connection
            print("Closing JTAG")
    def execute_test(self, command):
        if(self.target == "fifo"):
            os.write(self.rx_fd, command)
            time.sleep(1)
            #TODO:this can be improved.
            response = os.read(self.tx_fd, 4096)
            return response
