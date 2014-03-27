#!/usr/bin/env python

import numpy as np, sys

class NeighStats:

    class Line:
        ELT_NUM = 6
        def __init__(self, line=None):
            self.time = None
            self.src = None
            self.dst = None
            self.burstid = None
            self.pktid = None
            self.rssi = None

            if line is not None:
                self.parse(line)

        def parse(self, line):
            self.valid = False

            sline = [s.strip() for s in line.split(',')]

            # Check if the line has enough elements
            if len(sline) != self.ELT_NUM:
                return

            # Check if the line is not a first line
            if "time" == sline[0]:
                return

            self.valid = True

            self.src = int(sline[1])
            self.dst = int(sline[2])
            self.burstid = int(sline[3])
            self.pktid = int(sline[4])

    def __init__(self, files, bursts, pkts):

        self.burstnum = bursts
        self.pktnum = pkts

        self.files = files
        self.nodenum = len(self.files)
        self.nodes = [s.split('/')[-1] for s in files]
        self.nodes = [n.replace('node', '').replace('.csv','') for n in self.nodes]
        self.nodes = [int(n) for n in self.nodes]

        self.first = np.array(self.nodes).min()

        print "We have ",self.nodenum," nodes"
        self.neighbors = np.zeros((self.nodenum, self.burstnum, self.pktnum))
        print self.neighbors.shape


    def run(self):

        l = self.Line()
        fc = 1
        for filename in self.files:
            with open(filename, 'r') as f:
                print "parsing file: " + filename.split('/')[-1] + ' [%i]' % fc
                fc += 1

                for line in f:
                    l.parse(line)
                    if not l.valid:
                        continue

                    if l.src < self.first or l.src >= (self.first + self.nodenum):
                        print "bad src, skip:", l.src
                    elif l.dst < self.first or l.dst >= (self.first + self.nodenum):
                        print "bad dst, skip:", l.dst
                    elif l.burstid == 0 or l.burstid > self.burstnum:
                        print "bad burst, skip:", l.burstid
                    elif l.pktid >= self.pktnum:
                        print "bad pktid, skip:", l.pktid
                    else:
                        self.neighbors[l.src-self.first, l.burstid-1, l.pktid] += 1

    def export(self, neighfile):
        print neighfile
        np.save(neighfile, self.neighbors)

if __name__ == '__main__':
    import os

    usage = "usage: %s datapath" % sys.argv[0]

    if len(sys.argv) != 2:
        print usage
        sys.exit(0)

    path = sys.argv[1]

    files = [ path + '/' + f for f in os.listdir(path) if f.startswith('node') and f.endswith('.csv') ]

    burstlen = 128
    burstnum = 32
    # check for prop file, and extract data
    if os.path.exists(path+'/run.prop'):
        with open(path+'/run.prop') as prop:
            for p in prop:
                p = [a.strip() for a in p.split(':')]
                if len(p) == 2 and p[0] == 'burstlen':
                    burstlen = int(p[1])
                if len(p) == 2 and p[0] == 'burstnum':
                    burstnum = int(p[1])

    stat = NeighStats(files, burstnum, burstlen)
    stat.run()

    stat.export(path+'/neighbors')
