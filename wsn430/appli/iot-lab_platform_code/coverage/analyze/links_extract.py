import numpy as np


class Packet:
	def __init__(self, line):
		if not self.parse_line(line):
			self.valid = False
		else:
			self.valid = True

	def parse_line(self, line):
		if 'time' in line:
			return False

		time, src, dst, burstid, pktid, rssi = line.split(', ')

		self.time = float(time)
		self.src = int(src)
		self.dst = int(dst)
		self.burstid = int(burstid)
		self.pktid = int(pktid)
		self.rssi = float(rssi)

		return True

	def __str__(self):
		return "time=%.2f, from=%i, burstid=%i, pktid=%i, rssi=%.2f" % \
			(self.time, self.src, self.burstid, self.pktid, self.rssi)


class Burst:
	def __init__(self, src, id, len):
		self.src = src
		self.id = id
		self.len = len

		self.pkts = 0
		self.rssi = 0

	def add_packet(self, pkt):
		if pkt.src == self.src and pkt.burstid == self.id:
			self.pkts += 1
			self.rssi += pkt.rssi

	def get_results(self):
		if self.pkts == 0:
			return (0, np.NaN, 1.0)

		rssi = self.rssi / self.pkts
		per = 1 - float(self.pkts) / self.len

		if per < 0:
			per = 0.0

		return (self.pkts, rssi, per)


	def print_stats(self):
		pkt, rssi, per = self.get_results()
		print "pkts=%i, rssi=%.2f, per=%.2e" % (pkt, rssi, per)

class Link:
	BURST_LEN = 128
	BURST_NUM = 32

	def __init__(self, id):
		self.id = id

		self.bursts = {}

		for i in range(1, 1+self.BURST_NUM):
			self.bursts[i] = Burst(id, i, self.BURST_LEN)

		self.file = open("data/link%i.csv" % self.id, 'w')
		self.first = True

	def parse_packet(self, pkt):
		if pkt.burstid in self.bursts and pkt.src == self.id:
			self.bursts[pkt.burstid].add_packet(pkt)
			if self.first:
				self.first = False
				self.file.write('%e' % pkt.rssi)
			else:
				self.file.write(', %e' % pkt.rssi)

	def print_stats(self):
		for b in self.bursts:
			print " Burst %i" % b
			self.bursts[b].print_stats()

	def write_stats(self):
		b_rssi = []
		b_per  = []
		for i in range(1, 1+self.BURST_NUM):
			pkts, rssi, per = self.bursts[i].get_results()
			b_rssi.append(rssi)
			b_per.append(per)

		burst = np.array([b_rssi, b_per])
		np.savetxt('data/burst%i.csv' % self.id, burst, delimiter=', ')


	def close(self):
		self.file.close()


class Experience:
	def __init__(self, links=[], filename="node1.csv"):
		self.links = {}
		self.filename = filename

		for l in links:
			self.links[l] = Link(l)

	def parse_line(self, line):
		pkt = Packet(line)

		if not pkt.valid:
			return

		if pkt.src not in self.links:
			return

		self.links[pkt.src].parse_packet(pkt)

	def compute(self):
		f = open(self.filename, "r")
		for line in f:
			self.parse_line(line)
		f.close()

	def print_stats(self):
		for l in self.links:
			print "LINK %i" % l
			self.links[l].print_stats()

	def save(self):
		for l in self.links:
			self.links[l].write_stats()



if __name__ == '__main__':
	import sys
	usage = "%s filepath node1 node2 ..." % sys.argv[0]

	if len(sys.argv) < 3:
		print usage
		sys.exit()

	filepath = sys.argv[1]
	nodes = [int(i) for i in sys.argv[2:]]


	import os
	os.system('rm data/*')

	exp = Experience(nodes, filepath)

	exp.compute()
	exp.save()

	print "Nodes "+str(nodes)+" extracted"
