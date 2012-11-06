#!/usr/bin/env python

import numpy as np, sys
import matplotlib.pyplot as plt

class NeighBurstPlot:
    
    def __init__(self, filepath):
        self.neighbors = np.load(filepath)
        print self.neighbors.shape
        
        self.nodenum, self.burstnum, self.pktnum = self.neighbors.shape
    
    def compute(self):
        self.neighburst = np.zeros((self.nodenum, self.burstnum))
        
        n = 0
        for nodedata in self.neighbors:
            self.neighburst[n] = np.average(nodedata, axis=1)
            n += 1
        
    def plot(self):
        
        n = 101
        for line in self.neighburst:
            plt.plot(range(1, 1+self.burstnum), line, '-o', label='node%i' % (n))
            n += 1
        
        plt.title('Average number of neighbors for different TX power values')
        plt.legend(loc='lower right')
        plt.xlabel("TX Power [dBm]")
        plt.xticks(np.arange(1, 6, 1), ('-30', '-20', '-10', '0', '+10'))
        plt.ylabel("Number of neighbors")
        plt.grid()
        
        plt.axis([0.5, 5.5, -0.5, 15.5])
        

if __name__ == '__main__':
    import os
    
    usage = "usage: %s datapath" % sys.argv[0]
    
    if len(sys.argv) != 2:
        print usage
        sys.exit(0)
    
    path = sys.argv[1]
    
    neigh = NeighBurstPlot(path)
    neigh.compute()
    neigh.plot()
    
    plt.show()
    
