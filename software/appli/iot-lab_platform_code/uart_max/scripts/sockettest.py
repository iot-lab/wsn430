#!/usr/bin/env python

import socket, time, threading, traceback, sys
import node

class Socknode (node.Node):
    HOST = "experiment"
    PORTBASE = 30000
    def __init__(self, id):
        node.Node.__init__(self, id)
        
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((self.HOST, self.PORTBASE + id))
        self.sock.settimeout(0.1)
        
    
    def write(self, data):
        self.sock.send(data)
    
    def read(self):
        try:
            data = self.sock.recv(1)
        except socket.timeout:
            return ''
        return data
    
    def close(self):
        return self.sock.close()
    
    def start(self):
        # Flush the RX buffer
        try:
            while True:
                self.sock.recv(4096)
        except:
            pass
        node.Node.start(self)

class ParallelNodes:
    
    class MyThread(threading.Thread):
        def __init__(self, node, cmd):
            threading.Thread.__init__(self)
            self.node = node
            self.cmd = cmd
        
        def run(self):
            ret = self.node.__getattribute__(self.cmd)()
    
    def __init__(self, nodes):
        self.nodes = nodes
    
    def parallelexec(self, cmd):
        threads = []
        for n in self.nodes:
            threads .append(self.MyThread(n, cmd))
        
        for t in threads:
            t.start()
        
        for t in threads:
            t.join()
    
    def __getitem__(self, key):
        if not isinstance(key, int):
            raise TypeError
        
        return self.nodes[key]
    
    def __iter__(self):
        self.index = 0
        return self
    
    def next(self):
        if self.index < len(self.nodes):
            self.index += 1
            return self.nodes[self.index - 1]
        else:
            raise StopIteration

class UartTest:
    DURATION = 240
    
    def __init__(self, node_num=256, first_node=1):
        self.NODES = node_num
        self.N_START = first_node
        
        self.nodes = ParallelCommand([Socknode(i) for i in range(self.N_START, self.N_START + self.NODES)])
    
    def start(self):
        # Start each node's thread
        self.nodes.parallelexec("start")
        
    
    def stop(self):
        # Stop each node's thread
        self.nodes.parallelexec("halt")
        self.nodes.parallelexec("join")
        
        min_err = -1
        max_err = -1
        total_err = -1
        
        min_pkt = -1
        max_pkt = -1
        total_pkt = -1
        total_nodes = -1
        
        for n in nodes:
            ok = n.rx_ok
            ko = n.rx_missed
            
            if ok == 0:
                continue
            
            total_nodes += 1
            
            if ok > max_pkt:
                max_pkt = ok
            if ok < min_pkt or min_pkt == -1:
                min_pkt = ok
            total_pkt += ok
            
            if ko > max_err:
                max_err = ko
            if ko < min_err or min_err == -1:
                min_err = ko
            total_err += ko
            
        avg_err = 1. * total_err / total_nodes
        avg_pkt = 1. * total_pkt / total_nodes
        
        print "Char Errors (min/max/avg/total) : %i/%i/%f/%i" % (min_err, max_err, avg_err, total_err)
        

if __name__ == '__main__':
    print "Senslab Radio Coverage Test Program"
    
    test = UartTest()
    try:
        test.start()
    except:
        pass
    finally:
        test.stop()
