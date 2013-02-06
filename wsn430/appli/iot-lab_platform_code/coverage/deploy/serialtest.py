#!/usr/bin/env python

import serial
import node

class Sernode (node.Node):
    def __init__(self, id, dev="/dev/ttyS0"):
        node.Node.__init__(self, id)
        self.ser = serial.Serial(dev, 115200, timeout=0.1)

    def write(self, data):
        self.ser.write(data)

    def read(self):
        return self.ser.read(1)

    def close(self):
        return self.ser.close()

n1 = Sernode(7)
n1.start()
n1.setid()
n2 = Sernode(9, "/dev/ttyUSB0")
n2.start()
n2.setid()


import time
time.sleep(1)
n1.send()
time.sleep(.1)

n2.send()
time.sleep(.1)
n2.send()

time.sleep(.1)
n1.send()
n1.send()
time.sleep(.1)
n1.send()


n1.halt()
n2.halt()
n1.join()
n2.join()