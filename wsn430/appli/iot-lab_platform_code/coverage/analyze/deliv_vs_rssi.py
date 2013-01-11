import numpy as np
import matplotlib.pyplot as plt

class DelivRssi:
	def __init__(self, path):
		rssi  = np.loadtxt(path+'rssi.csv', delimiter=',')
		deliv = np.loadtxt(path+'pkt.csv', delimiter=',')

		plt.title("Delivery Ratio versus average Rssi")
		plt.xlabel("average Rssi (dBm)")
		plt.ylabel("delivery Ratio")
		plt.scatter(rssi, deliv, s=1)
		plt.grid(True)

if __name__ == '__main__':
	import sys
	
	usage = "%s folderpath" % sys.argv[0]
	if len(sys.argv) != 2:
		print usage
		sys.exit()
	else:
		path = sys.argv[1]
	
	if not path.endswith('/'):
		path += '/'
	d = DelivRssi(path)
	
	plt.show()
