import numpy as np
import matplotlib.pyplot as plt

class Link:
	"""A link is defined by an ID, it will look for a file named
	link<id>.csv for continuous rssi measures, and for a file named
	burst<id>.csv for rssi and PER averaged on each burst"""

	def __init__(self, id):
		self.id = id

		self.rssi = np.loadtxt('data/link%i.csv' % self.id, delimiter=',')

		tmp = np.loadtxt('data/burst%i.csv' % self.id, delimiter=',')
		self.burst_rssi = tmp[0]
		self.burst_per = tmp[1]


class LinkPlot:
	"""A link plot plots the rssi distribution, the burst rssi evolution,
	and the burst PER evolution in time for several links"""

	def __init__(self, links=[]):
		self.links = [Link(l) for l in links]

	def plot(self):
		# First Subplot: RSSI distribution
		plt.subplot(311)
		plt.title('RSSI distribution per link')
		plt.xlabel('RSSI [dBm]')
		plt.ylabel('density')
		for link in self.links:
			plt.hist(link.rssi, np.arange(-85, -39, 0.5), \
				normed=1, alpha=0.5, label="Link 1-%i" % link.id)
		plt.legend()

		# Second Subplot : Burst RSSI evolution
		plt.subplot(312)
		plt.title('RSSI average evolution per link')
		plt.xlabel('Burst')
		plt.ylabel('RSSI [dBm]')
		for link in self.links:
			plt.plot(range(1, 33), link.burst_rssi, label="Link 1-%i" % link.id)
		plt.legend()

		# Third Subplot : Burst PER evolution
		plt.subplot(313)
		plt.title('PER evolution per link')
		plt.xlabel('Burst')
		plt.ylabel('PER')
		for link in self.links:
			plt.plot(range(1, 33), link.burst_per, label="Link 1-%i" % link.id)
		plt.legend()

if __name__=='__main__':
	import os
	nodes = [int(i.replace('burst', '').replace('.csv', '')) for i in os.listdir('data/') if i.startswith('burst') and i.endswith('.csv')]
	print nodes

	plot = LinkPlot(nodes)
	plot.plot()
	plt.show()
