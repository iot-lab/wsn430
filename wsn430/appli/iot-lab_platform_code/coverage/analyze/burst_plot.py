import numpy as np
import matplotlib.pyplot as plt

class Burst:
	def __init__(self, path, nodeid):
		# get the packet delivery from the file
		self.bursts = [np.loadtxt(f, delimiter=',') \
			for f in [path+'/burst%i_%i.dat'%(nodeid, i) \
				for i in range(1, 5)]]

		self.nodeid = nodeid


	def plot_bursts(self):
		i = 101
		for line in self.data:
			plt.plot(line, label='node%i'%i)
			i+=1

		plt.grid(True)
		plt.title("RSSI received from node %i burst %i" % (self.nodeid, self.burstid))
		plt.xlabel("pktid")
		plt.ylabel("rssi [dBm]")
		plt.legend()

	def plot_corr(self, node=1):

		fig = plt.gcf()
		fig.suptitle("Autocorrelation of RSSI signals\nreceived from node%i during 4 bursts of 16384 packets, sent every 10ms" % self.nodeid)

		select = range(109,113)

		for burst in range(1, 5):
			data = self.bursts[burst-1]

			plt.subplot('42%i'%(2*burst-1))

			i = 101
			for node in data:

				burst_len = len(node)
				# remove nan
				nanix = np.isnan(node)
				nanpos = np.argwhere(nanix).flatten()
				ok_len = len(np.delete(node, nanpos))

				per = (1.0 * burst_len - ok_len) / burst_len

				node = np.nan_to_num(node)

				if ok_len == 0:
					i += 1
					continue

				if i not in select:
					i += 1
					continue

				node -= np.average(node)

				# compute the correlation
				xcorr = np.correlate(node, node, mode='full')
				# We are interested only in the second half (symetry)
				xcorr = xcorr[len(node)-1:]

				# Normalize
				xcorr /= xcorr[0]

				# plot
				plt.plot(xcorr, label="node%i [PER=%.2e]"%(i,per))
				plt.title("burst %i" % burst, fontsize='small')
				if burst == 4:
					plt.xlabel('n')
				i += 1

			plt.legend(prop={'size':'small'})
			plt.axis([-1,100, -0.1,0.5])

			plt.axis([-50,3000, -0.5,1.1])

			plt.subplot('42%i'%(2*burst))
			i = 101
			for node in data:
				# remove nan
				# remove nan
				nanix = np.isnan(node)
				nanpos = np.argwhere(nanix).flatten()
				ok_len = len(np.delete(node, nanpos))

				node = np.nan_to_num(node)

				if ok_len == 0:
					i += 1
					continue

				if i not in select:
					i += 1
					continue

				node -= np.average(node)

				# compute the correlation
				xcorr = np.correlate(node, node, mode='full')
				# We are interested only in the second half (symetry)
				xcorr = xcorr[len(node)-1:]

				# Normalize
				xcorr /= xcorr[0]

				# plot
				plt.plot(xcorr, label="node%i"%i)
				plt.title("burst %i (zoom)" % burst, fontsize='small')
				if burst == 4:
					plt.xlabel('n')
				i += 1

			plt.legend(prop={'size':'small'})
			plt.axis([-1,100, -0.1,0.5])


	def continuous_stat(self):

		pktok = np.logical_not(np.isnan(self.data))

		stats = []
		ratio = []
		per = []

		for line in pktok:
			stats += [[]]

			c = 0
			for ok in line:
				#~ print c, ok

				if ok:
					if c == 0:
						c = 1
					elif c > 0:
						c += 1
					else:
						stats[-1] += [c]
						c = 1

				else:
					if c == 0:
						c = -1
					elif c < 0:
						c -= 1
					else:
						stats[-1] += [c]
						c = -1
			stats[-1] += [c]

			stat = np.array(stats[-1])
			ratio += [-1.0 * ( stat * (stat<0) ).sum() / ( stat * (stat>0) ).sum()]
			per += [1.0 - 1.0 * line.sum() / line.size]


		data = zip(range(101, 117), per, ratio)
		print "result: "
		for i, per, r in data:
			print "%i\t%.2e\t%.2e" % (i, per, r)

		return data



if __name__ == '__main__':
	import sys

	if len(sys.argv) == 3:
		path = sys.argv[1]
		nodeid = int(sys.argv[2])
	else:
		print "usage: %s folder_path nodeid" % sys.argv[0]
		sys.exit(0)

	plt.figure()
	b = Burst(path, nodeid)

	#~ b.plot_bursts()
	#~ b.continuous_stat()
	b.plot_corr()
	plt.show()
