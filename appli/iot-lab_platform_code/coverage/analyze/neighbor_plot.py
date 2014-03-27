#!/usr/bin/env python

import numpy as np, sys
import matplotlib.pyplot as plt

class NeighPlot:

    def __init__(self, filepath):
        self.neighbors = np.load(filepath)
        print self.neighbors.shape

        self.nodenum, self.burstnum, self.pktnum = self.neighbors.shape

    def compute(self):
        self.neigh_flat = None

        for nodedata in self.neighbors:
            if self.neigh_flat is None:
                self.neigh_flat = np.array([nodedata.flatten()])
            else:
                self.neigh_flat = np.concatenate((self.neigh_flat, [nodedata.flatten()]))

        self.min = np.amin(self.neigh_flat, 1)
        self.max = np.amax(self.neigh_flat, 1)
        self.avg = np.average(self.neigh_flat, 1)
        self.std = np.std(self.neigh_flat, 1)

    def plot(self):
        #~ plt.plot(self.avg)
        #~ lines = plt.errorbar(range(1, self.nodenum+1), self.avg, \
        	#~ yerr=[self.avg-self.min, self.max-self.avg], \
        	#~ color='red', ecolor='red', fmt=None)
        #~ lines[1][0].set_label('min/max')

        lines = plt.errorbar(range(1, self.nodenum+1), self.avg, \
        	yerr=self.std, \
        	color='green', ecolor='green', elinewidth=0, fmt=None)
        lines[1][0].set_label("average +/- std")

        plt.errorbar(range(1, self.nodenum+1), self.avg, \
        	color='blue', fmt='b.', label="average")

        plt.legend()
        plt.title("Number of neighbors for each node")
        plt.xlabel("Node Number")
        plt.ylabel("Neighbors number")
        plt.axis([0, 17, 0, None])


if __name__ == '__main__':
    import os

    usage = "usage: %s datapath" % sys.argv[0]

    if len(sys.argv) != 2:
        print usage
        sys.exit(0)

    path = sys.argv[1]

    neigh = NeighPlot(path)
    neigh.compute()
    neigh.plot()

    plt.show()

