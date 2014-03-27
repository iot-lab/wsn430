#!/usr/bin/env python

import threading, time, Queue

class Command:
    cmds = {
        'setid' : (0xC0, 2),
        'cmpid' : (0xC1, 2),
        'resetrx' : (0xC2, 0),
        'setfreq' : (0xC3, 3),
        'setchanbw' : (0xC4, 2),
        'setdrate' : (0xC5, 2),
        'setmod' : (0xC6, 1),
        'settxpow' : (0xC7, 1),
        'send' : (0xC8, 7) }

    cmd_bytes = [cmds[k][0] for k in cmds]

    def __init__(self, cmd, args=[]):
        if cmd not in self.cmds:
            raise Exception("Error, %s no a valid command" % cmd)

        if len(args) != self.cmds[cmd][1]:
            raise Exception("Error, parameters count does not match")

        self.txt = cmd
        self.cmd = self.cmds[cmd][0]

        self.args = args

    @staticmethod
    def computeCRC(data):
        def crc8_byte( crc, byte ):

            crc ^= byte;
            for i in range(8):

                if crc & 1:
                    crc = (crc >> 1) ^ 0x8c;
                else:
                    crc >>= 1;

            return crc

        crc = 0;
        for d in data:
            crc = crc8_byte( crc, ord(d) )

        return crc

    def string(self):
        s = chr(0x80)
        s += chr(self.cmd)
        s += chr(len(self.args))
        s += ''.join([chr(i) for i in self.args])
        s += chr(self.computeCRC(s[1:]))
        return s

    def __str__(self):
        return '[' + self.txt + "] " + ':'.join(['%.2x' % ord(i) for i in list(self.string())])

    def __repr__(self):
        return self.__str__()


class Node (threading.Thread):
    def __init__(self, id):

        threading.Thread.__init__(self)
        self.id = id

        self.queue = Queue.Queue(1)

        filename = "node%i.csv" % self.id
        import os.path
        if not os.path.exists(filename):
            self.file = open(filename, "w")
            self.file.write("time, src, dst, burstid, pktid, rssi\n")
            self.file.flush()
        else:
            self.file = open(filename, "a")

    def read(self):
        time.sleep(0.1)
        return ''

    def write(self, data):
        pass

    def close(self):
        pass

    def unpack(self, pkt):
        sync = pkt[0]
        type = pkt[1]
        arglen = pkt[2]
        src = (pkt[3] << 8) + pkt[4]
        burst = (pkt[5] << 8) + pkt[6]
        pktid = (pkt[7] << 8) + pkt[8]
        rssi = pkt[9]
        if rssi >= 128:
            rssi = (rssi - 256) / 2. - 70
        else:
            rssi = rssi / 2. - 70

        t = time.time()

        return {'sync':sync, 'type':type, 'src':src, 'burst':burst, 'pktid':pktid, 'rssi':rssi, 'time':t}

    def handle_frame(self, data):
        # Check CRC
        crc = Command.computeCRC(''.join([chr(i) for i in data[1:-1]]))
        if crc != data[-1]:
            print "Discarding frame from Node %i [Bad CRC received]" % self.id
            return


        if data[1] in Command.cmd_bytes and data[2] == 1:
            r = (data[1], data[3])
            try:
                self.queue.put_nowait(r)
            except:
                try:
                    self.queue.get_nowait()
                except:
                    pass
                try:
                    self.queue.put_nowait(r)
                except:
                    pass
        elif data[1] == 0xF0:
            print "Node %i BOOTED" % self.id
        elif data[1] == 0xFF:
            pkt = self.unpack(data)
            pkt['dst'] = self.id

            #print "From %i to %i burst %i packet %i rssi %f" % (pkt['src'], pkt['dst'], pkt['burst'],pkt['pktid'], pkt['rssi'])
            line = "%f, %i, %i, %i, %i, %f\n" % (pkt['time'], pkt['src'], pkt['dst'], pkt['burst'], pkt['pktid'], pkt['rssi'])
            self.file.write(line)
            self.file.flush()

    def run(self):
        self.stop = False
        data = []
        while not self.stop:

            c = self.read()
            if c == '':
                continue

            c = ord(c)

            # Check length
            if len(data) == 0:
                # empty, look for a sync
                if c == 0x80:
                    data = [c]
            elif len(data) == 1:
                # got sync, look for a type
                if c in Command.cmd_bytes + [0xFF, 0xF0]:
                    # good type, store
                    data += [c]
                else:
                    # bad type, reset
                    data = []
            else:
                # store
                data += [c]

                # check if the arglen is not too big
                if data[2] > 10:
                    data = []
                    continue

                if len(data) == data[2] + 4:
                    self.handle_frame(data)
                    data = []
                    continue

    def halt(self):
        self.stop = True
        self.close()
        self.file.close()

    def setid(self):
        i0 = self.id >> 8
        i1 = self.id % 256

        self.cmd = Command('setid', [i0, i1])
        return self.execute()

    def cmpid(self):
        i0 = self.id >> 8
        i1 = self.id % 256

        self.cmd = Command('cmpid', [i0, i1])
        return self.execute()

    def setfreq(self, freq):
        """freq unit is 412Hz"""
        f2 = (freq >> 16) % 256
        f1 = (freq >> 8) % 256
        f0 = freq % 256

        self.cmd = Command('setfreq', [f2, f1, f0])
        return self.execute()

    def setchanbw(self, chan_e, chan_m):

        self.cmd = Command('setchanbw', [chan_e, chan_m])
        return self.execute()

    def setdrate(self, drate_e, drate_m):

        self.cmd = Command('setdrate', [drate_e, drate_m])
        return self.execute()

    def setmod(self, mod):

        if mod == "2FSK":
            m = 0
        elif mod == "GFSK":
            m = 1
        elif mod == "ASK":
            m = 2
        elif mod == "MSK":
            m = 3

        self.cmd = Command('setmod', [m])
        return self.execute()

    def settxpow(self, pow):
        self.cmd = Command('settxpow', [pow])
        return self.execute()

    def resetrx(self):
        self.cmd = Command('resetrx')
        return self.execute()

    def send(self, burstid, burstlen=128, period=10, data=0):
        self.burstlen = burstlen

        bid = [burstid >> 8, burstid % 256]
        blen = [burstlen >> 8, burstlen % 256]
        per = [period >> 8, period % 256]
        args = bid + blen + per + [data]
        self.cmd = Command('send', args)
        return self.execute()

    def execute(self):
        # Flush the queue
        try:
            self.queue.get_nowait()
        except:
            pass

        #Write the commant
        self.write(self.cmd.string())

        timeout = 0.5
        if self.cmd.txt == "send":
            timeout = 0.5 + 0.01 * self.burstlen

        # Get the response, wait max 0.5 s
        try:
            resp = self.queue.get(True, timeout)
        except:
            resp = ''


        if len(resp) != 2:
            return (False, 'Timeout')
        elif chr(resp[0]) != self.cmd.string()[1]:
            return (False, "resp byte does not match command: %x" % resp[0] )
        elif resp[1] != 0x00:
            if resp[1] == 0x3:
                return (False, "BAD CRC")
            return (False, "NACK: %x" % resp[1])
        else:
            return (True, 'OK')

