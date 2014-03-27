import numpy as np
import matplotlib.pyplot as plt

class Neigh:
	def __init__(self, per):
		# get the packet delivery from the file
		self.data = np.loadtxt(per, delimiter=',')

	def plot(self):

		for per in 0.1 * 2 ** np.arange(-4, 3, 1.0):
			neigh = self.data>(1-per)
			neigh = neigh.sum(axis=1)

			# the histogram of the data
			hist, bins = np.histogram(neigh, bins=np.arange(0, 251, 20))
			#~ plt.semilogy(bins[:-1], hist)
			plt.plot(bins[:-1], hist, label='per <= %.2e'%per)

		plt.title('Distribution of the number of neighbors with different PER min')
		plt.xlabel('Neighbors number')
		plt.ylabel('Count')
		plt.legend()
		plt.grid(True)


if __name__ == '__main__':
	import sys

	if len(sys.argv) == 2:
		perpath = sys.argv[1]
	else:
		print 'usage: %s perpath' % sys.argv[0]
		sys.exit()

	Neigh(perpath).plot()

	plt.show()
