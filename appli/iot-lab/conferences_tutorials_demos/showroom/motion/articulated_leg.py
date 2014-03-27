#!/usr/bin/env python

import visual, time, math, threading, re, serial

class Axes:
    def __init__(self, frame=None):
        if frame is None:
            self.frame = visual.frame()
        else:
            self.frame = frame

        self.ax = visual.arrow(frame=self.frame, pos=(0,0,0), axis=(1,0,0), color=(1,0,0), shaftwidth=0.02)
        self.ay = visual.arrow(frame=self.frame, pos=(0,0,0), axis=(0,1,0), color=(0,1,0), shaftwidth=0.02)
        self.az = visual.arrow(frame=self.frame, pos=(0,0,0), axis=(0,0,1), color=(0,0,1), shaftwidth=0.02)

        self.tx = visual.label(frame=self.frame, pos=(1.1,0,0), text='x', color=(1,0,0))
        self.ty = visual.label(frame=self.frame, pos=(0,1.1,0), text='y', color=(0,1,0))
        self.tz = visual.label(frame=self.frame, pos=(0,0,1.1), text='z', color=(0,0,1))


class Leg:
    TIBIA_LENGTH = 1
    TIBIA_RADIUS = 0.1
    THIGH_LENGTH = 1
    THIGH_RADIUS = 0.2
    FOOT_LENGTH = 0.5

    def __init__(self):
        self.frame = visual.frame()
        # create
        #~ self.axes = Axes(frame=self.frame)

        self.thigh = visual.cylinder(frame=self.frame, \
                                    axis=(0,-self.THIGH_LENGTH,0), \
                                    radius=self.THIGH_RADIUS, \
                                    color=(0,1,0))
        self.hip = visual.sphere(frame=self.frame, \
                                    radius=self.THIGH_RADIUS, \
                                    color=(0,1,0))

        self.tibia = visual.cylinder(frame=self.frame, \
                                    pos=self.thigh.axis, \
                                    axis=(0,-self.TIBIA_LENGTH,0), \
                                    radius=self.TIBIA_RADIUS, \
                                    color=(0,0,1))
        self.knee = visual.sphere(frame=self.frame, \
                                    radius=self.THIGH_RADIUS, \
                                    pos=self.thigh.axis, \
                                    color=(0.2,0.3,0.4))
        self.foot = visual.box(frame=self.frame, \
                                    axis = (self.FOOT_LENGTH, 0, 0), \
                                    height=self.TIBIA_RADIUS, \
                                    width=2*self.TIBIA_RADIUS, \
                                    color=(0.89,0.12,0.77))
        self.foot.pos = self.tibia.pos+self.tibia.axis+self.foot.axis/2

    def set_thigh_angle(self, angle):
        y = visual.sin(angle)
        x = visual.cos(angle)
        self.frame.axis = (x, y, 0)
        if x<0:
            self.frame.up = (0,-1,0)
        else:
            self.frame.up = (0,1,0)

    def set_thigh_axis(self, axis):
        axis = visual.vector(axis)
        axis.mag = 1
        self.frame.axis = axis

    def set_tibia_angle(self,angle):
        x = visual.cos(angle)
        y = visual.sin(angle)

        self.tibia.pos = self.thigh.axis
        self.tibia.axis = (-y, -x, 0)

        x = visual.cos(angle+visual.pi/2)
        y = visual.sin(angle+visual.pi/2)
        self.foot.axis = (y, x, 0)
        self.foot.axis.mag = self.FOOT_LENGTH
        self.foot.pos = self.tibia.pos+self.tibia.axis+self.foot.axis/2


class Body:

    def __init__(self):
        len = 1.2
        self.trunk = visual.box(pos=(0,len/2.,0), axis=(0,len,0), height=0.4, width=0.8, color=(0.2, 0.4, 0.6))
        self.leg_right = Leg()
        self.leg_right.frame.pos = (0,0,-0.2)

        self.leg_left = Leg()
        self.leg_left.frame.pos = (0,0,0.2)

class Receiver(threading.Thread):
    dev = None

    thigh_addr = None
    tibia_addr = None
    foot_addr = None

    foot = [0,0,0]
    tibia = [0,0,0]
    thigh = [0,0,0]

    thigh_a = 0
    tibia_a = 0

    stop = False

    ptrn = re.compile("^from (?P<from>[0-9a-f]{4}) : (?P<x>-?[0-9]+) (?P<y>-?[0-9]+) (?P<z>-?[0-9]+)")

    def __init__(self, port="/dev/ttyUSB0"):
        threading.Thread.__init__(self)
        self.dev = serial.Serial(port, 115200, timeout=0.5)

    def is_addr_known(self, addr):
        if self.thigh_addr==addr:
            return True
        if self.tibia_addr==addr:
            return True
        if self.foot_addr==addr:
            return True
        return False

    def run(self):
        data = ""
        while self.stop is not True:
            data += self.dev.read(1)

            while '\n' in data:
                line = data.split('\n')[0]
                data = '\n'.join(data.split('\n')[1:])

                #~ print line
                m = self.ptrn.match(line)
                if m is not None:
                    src = m.group('from')
                    x = int(m.group('x'))
                    y = int(m.group('y'))
                    z = int(m.group('z'))
                    measure = [x, y, z]
                    mag = math.sqrt((x**2)+(y**2)+(z**2))
                    if mag < 1e-3:
                        mag = 1e-3
                    #~ print "from=%s (%i,%i,%i)"%(src, x, y, z)

                    if not self.is_addr_known(src) and self.thigh_addr is None:
                        self.thigh_addr = src
                    elif not self.is_addr_known(src) and self.tibia_addr is None:
                        self.tibia_addr = src
                    elif not self.is_addr_known(src) and self.foot_addr is None:
                        self.foot_addr = src

                    if src == self.foot_addr:
                        self.foot = measure
                        #~ print 'foot=',self.foot
                    elif src == self.tibia_addr:
                        self.tibia = measure
                        self.tibia_a = math.acos(y/mag)
                        #~ self.tibia_a *= -1

                        if z<0:
                            self.tibia_a *= -1
                        #~ print 'tibia angle = ', 180/visual.pi*self.tibia_a
                    elif src == self.thigh_addr:
                        self.thigh = measure
                        self.thigh_a = math.acos(y/mag)
                        if z<0:
                            self.thigh_a *= -1
                        #~ print 'thigh angle = ', 180/visual.pi*self.thigh_a


if __name__ == '__main__':
    a = Axes()
    b = Body()
    r = Receiver()
    r.start()

    count = 20
    div = 1.
    while True:
        visual.rate(10)

        b.leg_left.set_thigh_angle(r.thigh_a)
        b.leg_left.set_tibia_angle(r.thigh_a-r.tibia_a)
        v = visual.vector(r.foot)/4096
        visual.scene.background = v

