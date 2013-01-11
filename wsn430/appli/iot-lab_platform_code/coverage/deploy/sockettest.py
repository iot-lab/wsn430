#!/usr/bin/env python

import socket, time, threading, traceback, sys
import node

class Logger:
    errorf = None
    sendf = None
    outf = None
    
    @staticmethod
    def initcheck():
        if Logger.errorf is None:
            Logger.errorf = open("errors.log", "a")
        
        if Logger.sendf is None:
            Logger.sendf = open("sends.log", "a")
            Logger.sendf.write("time, src, burstid, burstsize, extralen\n")
        
        if Logger.outf is None:
            Logger.outf = open("output.log", "a")
            Logger.outf.write("Start\n")
            
    @staticmethod
    def log_send(src, burst, num, len):
        Logger.initcheck()
        
        msg = "%f, %i, %i, %i, %i\n" % (time.time(), src, burst, num, len)
        Logger.sendf.write(msg)
        Logger.sendf.flush()

    @staticmethod
    def log_error(msg):
        Logger.initcheck()
        
        Logger.errorf.write(msg)
        Logger.errorf.flush()
    
    @staticmethod
    def log_print(*args):
        Logger.initcheck()
        
        Logger.outf.write(' '.join([str(a) for a in args]) + '\n')
        Logger.outf.flush()
        
    @staticmethod
    def close():
        if Logger.errorf is not None:
            Logger.errorf.close()
        if Logger.sendf is not None:
            Logger.sendf.close()
        if Logger.outf is not None:
            Logger.outf.close()

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

class ParallelCommand:
    
    class MyThread(threading.Thread):
        def __init__(self, node, cmd, args):
            threading.Thread.__init__(self)
            self.node = node
            self.cmd = cmd
            self.args = args
        
        def run(self):
            ret = self.node.__getattribute__(self.cmd)(*self.args)
            if ret and not ret[0]:
                msg = "t:%f, Error executing " % time.time() + self.cmd + " on node " + str(self.node.id) + ": " + ret[1]
                Logger.log_print(msg)
                Logger.log_error(msg + '\n')
    
    def __init__(self, nodes):
        self.nodes = nodes
    
    def parallelexec(self, cmd, *args):
        threads = []
        for n in self.nodes:
            threads .append(self.MyThread(n, cmd, args))
        
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

class CoverageTest:
    TRIES = 2
    TEMPO = 0.2
    
    POWER = (-30, -20, -10, 0, 10)
    POWER_REG = {10:0xC2, 0:0x50, -10:0x27, -20:0x0F, -30:0x03}
    
    def __init__(self, node_num=256, first_node=1, burst_num=32, burst_size=128):
        self.NODES = node_num
        self.N_START = first_node
        self.BURSTS = burst_num
        self.PKTS = burst_size
        
        self.nodes = ParallelCommand([Socknode(i) for i in range(self.N_START, self.N_START + self.NODES)])
    
    def nodesinit(self):
        # Start each node's thread
        self.nodes.parallelexec("start")
        time.sleep(self.TEMPO)
        
        # associate ID to each node, find inactive nodes
        
        try:
            # Add a little robustness to setid
            for n in self.nodes:
                Logger.log_print("Setting node ", n.id) 
                for i in range(self.TRIES):
                    ret = n.setid()
                    if ret[0]:
                        break
                    else:
                        Logger.log_print("Node %i warning: [%s]" % (n.id, ret[1]))
                        if i + 1 == self.TRIES:
                            Logger.log_print("Node %i error, too many attemps, removing" % n.id)
                            Logger.log_error("T:%f, N%i" % (time.time(), n.id) + " init error, removing (%s)\n" % ret[1])
                            ix = self.nodes.nodes.index(n)
                            self.nodes.nodes = self.nodes.nodes[:ix] + self.nodes.nodes[ix + 1:]
                            n.halt()
                            n.join()
            
            Logger.log_print("List of valid nodes: ", [n.id for n in self.nodes])
            time.sleep(self.TEMPO)
        
        except Exception, e:
            Logger.log_print(e)
            traceback.print_exc(file=sys.stdout)
            raise e
    
    def run(self):
        
        try:
            self.nodes.parallelexec('resetrx')
            time.sleep(self.TEMPO)
            
            for burst in range(1, self.BURSTS + 1):
                
                # Set power
                Logger.log_print("Setting power to %i dBm" % self.POWER[burst-1])
                self.nodes.parallelexec('settxpow', self.POWER_REG[self.POWER[burst-1]])
                time.sleep(self.TEMPO)
                
                for node in self.nodes:
                    Logger.log_print("Sending burst %i from node %i" % (burst, node.id))
                    # Check that the node has its ID really set
                    ret = node.cmpid()
                    if not ret[0]:
                        msg = "T:%f, B%i, N%i" % (time.time(), burst, node.id) + " error: ID not set [%s], fixing" % ret[1]    
                        Logger.log_print( msg )
                        Logger.log_error(msg + "\n")
                        node.setid()
                        node.settxpow(self.POWER_REG[self.POWER[burst-1]])
                    
                    Logger.log_send(node.id, burst, self.PKTS, 0)
                    ret = node.send(burst, self.PKTS)
                    if not ret[0]:
                        Logger.log_print( "Node %i error: " % node.id + ret[1] )
                        Logger.log_error("T:%f, B%i, N%i" % (time.time(), burst, node.id) + "error: " + ret[1] + "\n")
                    time.sleep(self.TEMPO)
                    
                    self.nodes.parallelexec("resetrx")
            
        except Exception, e:
            Logger.log_print( e)
            traceback.print_exc(file=sys.stdout)
            raise e
    
    def stop(self):
        self.nodes.parallelexec("halt")
        self.nodes.parallelexec("join")
        Logger.close()


if __name__ == '__main__':
    print "Senslab Radio Coverage Test Program"
    run1 = "allnodes"
    run2 = "longburst"
    
    if len(sys.argv) != 2:
        print "usage: %s ( %s | %s )" % (sys.argv[0], run1, run2)
        sys.exit(0)
    elif sys.argv[1] not in [run1, run2]:
        print "usage: %s ( %s | %s )" % (sys.argv[0], run1, run2)
        sys.exit(0)
    
    if sys.argv[1] == run1:
        test = CoverageTest(node_num=256, first_node=1, burst_num=32, burst_size=256)
    else:
        test = CoverageTest(node_num=16, first_node=101, burst_num=5, burst_size=16384)
    
    try:
        test.nodesinit()
        test.run()
    except:
        pass
    finally:
        test.stop()
        
    
