#!/usr/bin/env python

import numpy as np

class NodeStat:

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
            sline = [s.strip() for s in line.split(',')]

            # Check if the line has enough elements
            if len(sline) != self.ELT_NUM:
                return

            # Check if the line is not a first line
            if "time" in sline[0]:
                return

            self.time = float(sline[0])
            self.src = int(sline[1])
            self.dst = int(sline[2])
            self.burstid = int(sline[3])
            self.pktid = int(sline[4])
            self.rssi = float(sline[5])

    def __init__(self, files, pktcount):

        self.files = files
        self.nodenum = len(self.files)
        self.nodes = [s.split('/')[-1] for s in files]
        self.nodes = [n.replace('node', '').replace('.csv','') for n in self.nodes]
        self.nodes = [int(n) for n in self.nodes]

        self.first = np.array(self.nodes).min()

        self.pktcount = pktcount

        print "We have ",self.nodenum," nodes"

        self.pkt = np.zeros( (self.nodenum, self.nodenum) )
        self.rssi = np.zeros( (self.nodenum, self.nodenum) )


    def run(self):

        for filename in self.files:
            with open(filename, 'r') as f:
				print "parsing file: " + filename.split('/')[-1]

				for line in f:
					l = self.Line(line)

					if l.time is None:
						continue

					if l.src not in self.nodes or l.dst not in self.nodes:
						print "skipping line ", l.src,":", l.dst
						continue

					self.pkt [l.src-self.first][l.dst-self.first] += 1
					self.rssi[l.src-self.first][l.dst-self.first] += l.rssi

        self.rssi /= self.pkt
        self.per = (self.pktcount - self.pkt) / self.pktcount
        self.pkt /= self.pktcount


    def export(self, pktfile, rssifile, perfile):
        np.savetxt(pktfile, self.pkt, delimiter=', ')
        np.savetxt(perfile, self.per, delimiter=', ')
        np.savetxt(rssifile, self.rssi, delimiter=', ')

if __name__ == '__main__':
    import sys, os

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


    stat = NodeStat(files, burstnum * burstlen)
    stat.run()

    stat.export(path+'/pkt.csv', path+'/rssi.csv', path+'/per.csv')
