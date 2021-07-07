import pytest
import os, sys, time
import socket
import serial

class TargetSerial:
    def __init__(self, *args):
        #initialize serial target specific instance
        self.port_type = args[0]
        self.serial_host = args[1]
        self.serial_attr = args[2]
        print("Serial Target Instance: " + self.serial_host+":"+str(self.serial_attr))
    def open(self):
        #open serial port
        print("Opening Serial port:" + self.serial_host)
        if self.port_type in 'TCP':
            self.conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.conn.connect((self.serial_host, self.serial_attr))
            print (f'"{self.port_type}" connection establised on {self.serial_host}:{self.serial_attr}')
        elif self.port_type in 'COM':
            self.conn = serial.Serial(port = self.serial_host, baudrate=self.serial_attr, bytesize=8, timeout=2, stopbits=serial.STOPBITS_ONE)
            print (f'"{self.port_type}" port connection establised on {host} with baud rate {attr}')
    def close(self):
        #close serial port
        print("Closing Serial port:" + self.serial_port)
    def uart_send(self, command):
        print(f'"{self.port_type}" Sending command {command}')
        if self.port_type in 'TCP':
            self.conn.sendall(command)
        elif self.port_type in 'COM':
            self.conn.write(b'{command}')
    def uart_receive(self):
        print (f'"{self.port_type}" Waiting for the response')
        if self.port_type in 'TCP':
            response = self.conn.recv(1024)
        elif self.port_type in 'COM':
            while (self.conn.in_waiting == 0):
                #pass
                pass
            response = self.conn.readline()
            response = response.decode('Ascii')
        return response
