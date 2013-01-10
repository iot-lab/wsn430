#!/usr/bin/env python

import sys, re, time

class Parser:
    ptrn = re.compile(''':(?P<len>[0-9A-F]{2})(?P<addr>[0-9A-F]{4})'''+ \
            '''00(?P<data>([0-9A-F]{2})+)''')
        
    def parse_line(self, line):
    
        m = self.ptrn.match(line)
        
        if m is None:
            print "No match"
            return None, None
            
        data_len = int(m.group('len'), 16)
        data_addr = int(m.group('addr'), 16)
        data = m.group('data')
        
        if len(data) != 2*data_len+2:
            print "Length matching error"
            return None, None
        
        data = [int(data[i:i+2], 16) for i in range(0, len(data), 2)][:-1]
        
        return (data_addr, data)

    def __init__(self, filename):
        hex_file = open(filename, 'r')
        
        self.data = []
        line_num = 0
        for l in hex_file:
            addr, data = self.parse_line(l)
            
            if addr is None:
                continue
            if len(self.data) == 0:
                self.data.append([addr, data])
            elif self.data[-1][0]+len(self.data[-1][1]) == addr:
                self.data[-1][1] += data
            else:
                self.data.append([addr, data])
                
        hex_file.close()
    
    def generate_chunks(self, chunk_size = 64):
        
        frames = []
        
        for data_range in self.data:
            range_ix = 0
            range_len = len(data_range[1])
            
            while range_ix < range_len:
                
                if range_ix+chunk_size <= range_len:
                    chunk_len = chunk_size
                else:
                    chunk_len = range_len-range_ix
                    
                frames.append( (data_range[0]+range_ix, data_range[1][range_ix:range_ix+chunk_len]) )
                
                range_ix += chunk_len
        
        return frames
    
    def create_struct(self):
        f = open("ctrlhex.h", 'w')
        
        chunks = self.generate_chunks(240)
        
        i=0
        for chunk in chunks:
            s = ", ".join(["0x%x"%n for n in chunk[1]])
            s= "unsigned char* chunk_%i = {0x%x, 0x%x, 0x%x, 0x%x, %s};"%(i, \
                        (chunk[0]>>8)&0xFF, chunk[0]&0xFF, (len(chunk[1])>>8)&0xFF, len(chunk[1])&0xFF, s)
            f.write(s+"\n\n")
            i+=1
        
        s = ", ".join(["chunk_%i"%a for a in range(i)])
        s = "unsigned char** chunks = {%s};\n\n" % s
        f.write(s)
        f.write("int chunks_len = %i;\n"%i)
        f.close()
        
if __name__ == "__main__":
    p = Parser("ctrlnode_gwtest.hex")
    p.create_struct()
