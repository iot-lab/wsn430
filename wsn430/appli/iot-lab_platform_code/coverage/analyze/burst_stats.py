import numpy as np
import matplotlib.pyplot as plt

class BurstStat:
	def __init__(self, path):
		# get the packet delivery from the file
		self.data = np.loadtxt(path, delimiter=',')


		nodburst = path.split('/')[-1].replace('burst', '').replace('.dat', '')
		self.nodeid = int(nodburst.split('_')[0])
		self.burstid = int(nodburst.split('_')[1])

	def compute(self):

		self.stats = []

		for line in self.data:
			# remove nan
			nanix = np.isnan(line)
			nanpos = np.argwhere(nanix).flatten()

			line = np.delete(line, nanpos)

			if line.size == 0:
				min = 0
				max = 0
				avg = 0
				std = 0
			else:
				min = np.amin(line)
				max = np.amax(line)
				avg = np.average(line)
				std = np.std(line)

			self.stats.append((min, max, avg, std))

		return self.stats


if __name__ == '__main__':
	import sys

	if len(sys.argv) == 2:
		path = sys.argv[1]
	else:
		sys.exit(0)

	nodes = 16
	first = 101
	bursts = 4

	for src in range(first, first + nodes):
		for burst in range(1, 1 + bursts):
			b = BurstStat(path+'/burst%i_%i.dat' % (src, burst))

			print b.compute()

