import pytest
import os, sys, time
import socket

class TargetJtag:
    def __init__(self, *args):
        #initialize JTAG target specific instance
        print("Jtag target instance creation..")
    def open(self):
        #open jtag connection
        print("Opening JTAG:..")
    def close(self):
        #close jtag connection
        print("Closing JTAG")
    def execute_test(self, command):
        print("Execute test over JTAG")