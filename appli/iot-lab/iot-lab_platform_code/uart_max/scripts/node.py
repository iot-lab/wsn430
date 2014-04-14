#!/usr/bin/env python

import threading


class Node (threading.Thread):
    def __init__(self, id):
        threading.Thread.__init__(self)
        self.id = id

        self.rx_ok = 0
        self.rx_missed = 0

    def read(self):
        time.sleep(0.1)
        return ''

    def close(self):
        pass

    def halt(self):
        self.stop = True

    def run(self):
        # Get first char
        latest = ''
        while len(latest) != 1:
            latest = self.read()

        # Convert to int
        latest = ord(latest)

        self.stop = False
        self.errors = []
        while not self.stop:

            c = self.read()
            if c == '':
                continue

            c = ord(c)

            self.rx_ok += 1
            d = (c - latest) % 256
            if d > 1:
                self.rx_missed += 1
                err = (latest,c)
                self.errors.append(err)
                print "Error: ", err

            latest = c
        self.close()
