
import numpy as np
class Line:
	ELT_NUM = 6
	def __init__(self, line=None):
		self.time = None
		self.src = None
		self.dst = None
		self.burstid = None
		self.pktid = None
		self.rssi = None

		self.valid = False

		if line is not None:
			self.parse(line)

	def parse(self, line):
		sline = [s.strip() for s in line.split(',')]
		self.valid = False
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

		self.valid = True
	def __str__(self):
		if self.valid:
			return "Line from %i to %i burst %i pkt %i rssi %f" % (self.src, self.dst, self.burstid, self.pktid, self.rssi)
		else:
			return "Line invalid"

class BurstExtract:
	def __init__(self, files, nodeid, burstid, burstlen):
		self.files = files
		if 'node%i.csv' % nodeid in self.files:
			self.files.remove('node%i.csv' % nodeid)

		self.nodenum = len(self.files)
		self.nodes = [s.split('/')[-1] for s in files]
		self.nodes = [n.replace('node', '').replace('.csv','') for n in self.nodes]
		self.nodes = [int(n) for n in self.nodes]

		self.first = np.array(self.nodes).min()

		self.nodeid = nodeid
		self.burstid = burstid

		print "We have ",self.nodenum," nodes"

		self.rssi = np.zeros( (self.nodenum, burstlen)) * np.NaN

	def run(self):
		print "Looking for src=%i, burstid=%i" % (self.nodeid, self.burstid)
		l = Line()
		for f in self.files:
			with open(f, 'r') as fdata:
				for line in fdata:
					l.parse(line)

					if l.valid and l.src == self.nodeid and l.burstid == self.burstid:

						i = l.dst - self.first
						j = l.pktid
						self.rssi[i,j] = l.rssi

	def save(self, filename):
		np.savetxt(filename, self.rssi, delimiter=', ')

if __name__ == '__main__':
	import sys, os

	usage = "usage: %s datapath" % sys.argv[0]

	if len(sys.argv) not in [2]:
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



	for nodeid in range(101, 117):
		for burstid in range(1, 1 + burstnum):
			print "Starting burst export, ", nodeid, burstid
			stat = BurstExtract(files, nodeid, burstid, burstlen)
			stat.run()
			stat.save(path+'/burst%i_%i.dat' % (nodeid, burstid))
