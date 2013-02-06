import numpy as np
import matplotlib.pyplot as plt

class PER:
	def __init__(self, path='./'):
		# get the packet delivery from the file
		dat = np.loadtxt(path + 'pkt.csv', delimiter=',')
		data = np.array([])

		# remove links which have no sense (from to same node)
		i = 0
		for line in dat:
			line0 = line[:i]
			line1 = line[i+1:]

			data = np.concatenate((data, line0))
			data = np.concatenate((data, line1))
			i += 1
		self.data = data

	def plot(self):
		plt.subplot(211)
		# the histogram of the data
		#~ pdf, bins, patches = plt.hist(self.data, 100, normed=1, facecolor='green', alpha=0.75, log=True)

		hist, bins = np.histogram(self.data, bins=256, normed=True)
		plt.semilogy(bins[:-1], hist)
		#plt.plot(bins[:-1], hist)

		plt.title('Probability density function of link delivery ratio')
		plt.xlabel('Delivery')
		plt.ylabel('Density')
		plt.axis([0, 1, 0, (np.floor(hist.max()/10)+1)*10])
		plt.grid(True)

		plt.subplot(212)
		# the histogram of the data
		#pdf, bins, patches = plt.hist(self.data, 100, normed=1, facecolor='green', alpha=0.75, cumulative=True)

		cum = np.array(hist)
		for i in range(1, len(cum)):
			cum[i] += cum[i-1]
		cum /= cum[-1]
		plt.plot(bins[:-1], cum)

		plt.title('Cumulative distribution function of link delivery ratio')
		plt.xlabel('Delivery')
		plt.ylabel('Probability')
		#plt.axis([0, 1, 0, 1.1])
		plt.grid(True)

class Rssi:
	def __init__(self, path='./'):
		dat = np.loadtxt(path + 'rssi.csv', delimiter=',')

		dat.shape = (dat.size,)

		data = []
		for i in dat:
			if not np.isnan(i):
				data += [i]
		self.data = np.array(data)

	def plot(self):

		plt.subplot(211)
		# the histogram of the data
		# pdf, bins, patches = plt.hist(self.data, 100, normed=1, facecolor='green', alpha=0.75)
		hist, bins = np.histogram(self.data, bins=128, normed=True)
		plt.plot(bins[:-1], hist)

		plt.xlabel('Rssi')
		plt.ylabel('Density')
		plt.title('Probability density function of link rssi')
		plt.grid(True)

		plt.subplot(212)
		# the histogram of the data
		# pdf, bins, patches = plt.hist(self.data, 100, normed=1, facecolor='green', alpha=0.75, cumulative=True)
		cum = np.array(hist)
		for i in range(1, len(cum)):
			cum[i] += cum[i-1]
		cum /= cum[-1]
		plt.plot(bins[:-1], cum)

		plt.xlabel('Rssi')
		plt.ylabel('Probability')
		plt.title('Cumulative distribution function of link rssi')
		plt.grid(True)


if __name__ == '__main__':
	import sys

	if len(sys.argv) == 2:
		path = sys.argv[1]
	else:
		path = '.'

	if not path.endswith('/'):
		path += '/'

	plt.figure()
	PER(path).plot()

	plt.figure()
	Rssi(path).plot()

	plt.show()
