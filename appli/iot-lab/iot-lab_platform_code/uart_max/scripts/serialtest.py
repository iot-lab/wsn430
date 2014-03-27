#!/usr/bin/env python

import serial
import node
import time

class Sernode (node.Node):
    def __init__(self, id, dev="/dev/ttyS0"):
        node.Node.__init__(self, id)
        self.ser = serial.Serial(dev, 115200, timeout=0.1)

    def start(self):
        self.ser.flushInput()
        node.Node.start(self)

    def write(self, data):
        self.ser.write(data)

    def read(self):
        return self.ser.read(1)

    def close(self):
        return self.ser.close()

print "Starting UART test program"
n1 = Sernode(1)
n1.start()

DURATION = 60
t = DURATION
while t != 0:
    print t
    if t >= 1:
        time.sleep(1)
        t -= 1
    else:
        time.sleep(t)
        t = 0

n1.halt()
n1.join()

fail = 100. * n1.rx_missed / n1.rx_ok

print "Test Results"
print " %f%% error" % fail
print " %i errors, %i characters" % (n1.rx_missed, n1.rx_ok)