#!/usr/bin/env python

import pygtk
pygtk.require('2.0')
import gtk, gobject
import serial, re, math, time, os

def compute_lux_from_adc(adc0, adc1):
    adc0 = int(adc0)
    adc1 = int(adc1)
    
    chord0_number = (adc0>>4)&0x7
    step0_number  =  adc0&0x0F
    step0_value  = 2**chord0_number
    chord0_value = int(16.5*(step0_value-1))
    
    adc0_count = chord0_value + step0_value*step0_number
    
    chord1_number = (adc1>>4)&0x7
    step1_number  =  adc1&0x1F
    step1_value  = 2**chord1_number
    chord1_value = int(16.5*(step1_value-1))
    
    adc1_count = chord1_value + step1_value*step1_number
    
    if (adc0_count - adc1_count) == 0:
        R = 1e6
    else:
        R = 1.0*adc1_count / (adc0_count - adc1_count)
    
    return (adc0_count - adc1_count) * 0.39 * math.exp(-0.181*(R**2))
    

def compute_temp_from_adc(msb, lsb):
    msb = int(msb)
    lsb=int(lsb)
    t = msb
    if t >= 128: t -= 256
    t += (lsb/256.)
    return t
    

class Interface:
    def __init__(self):
        self.serialport = serial.Serial('/dev/ttyUSB1', 115200, timeout=0)
    
    def send_cmd(self, cmd):
        for i in cmd:
            self.serialport.write(chr(i))
    
    def read_data(self):
        return self.serialport.read(1024)
    

class MainWindow:
    pattern = re.compile(r'''node=(?P<node>[0-9a-f]+)&rssi=(?P<rssi>[0-9]+)&''' + \
                         r'''temp=(?P<temp0>[0-9]+):(?P<temp1>[0-9]+)&''' + \
                         r'''light=(?P<light0>[0-9]+):(?P<light1>[0-9]+)&num=(?P<neigh_num>[0-9]+)''' + \
                         r'''(?P<neighbors>[0-9a-z&]*)\r\n''')
    
    files = []
    nodes = []
    names = []
    labels = []
    labels_time = []
    TABLE_SIZE = 16
    
    count     = [0]*(TABLE_SIZE-1)
    count_max = [0]*(TABLE_SIZE-1)
    
    record = False
    
    def add_node(self, node):
        if node not in self.nodes:
            self.nodes.append(node)
            node_ix = self.nodes.index(node)
            self.labels[node_ix+1][0].set_text(node)
            self.labels[0][node_ix+1].set_text(node)
            self.files.append(open(node+'.dat', 'w'))
            self.names[node_ix].set_text(node)
            
        node_ix = self.nodes.index(node)
        return node_ix
    
    def update_node(self, node, T, L):
        if node not in self.nodes:
            self.nodes.append(node)
            node_ix = self.nodes.index(node)
            
            self.labels[0][node_ix+1].set_text(node)
        node_ix = self.nodes.index(node)
        self.labels[node_ix+1][0].set_text("%s\nT=%sC\nL=%slux"%(node, str(T)[:4], str(L)[:4]))
        return node_ix
    
    def clear_line(self, node):
        node_ix = self.nodes.index(node)
        for i in range(1,self.TABLE_SIZE):
            self.labels[node_ix+1][i].set_text('.')
            self.labels_time[node_ix+1][i] = time.time()
            
    
    def handle_data(self):
        self.read += self.itf.read_data()
        
        cont = True
        while cont:
            if 'node=' in self.read:
                self.read = self.read[self.read.index('node='):]
                
                if '\r\n' in self.read:
                    
                    end_index = self.read.index('\r\n')+2
                    line = self.read[:end_index].replace('|','&')
                    self.read = self.read[end_index:]
                    
                    match = self.pattern.match(line)
                    if match:
                        node = match.group('node')
                        neigh_num = match.group('neigh_num')
                        neighb = match.group('neighbors')
                        node_ix = self.add_node(node)
                        
                        self.clear_line(node)    
                        self.labels[node_ix+1][node_ix+1].set_text('o')
                        
                        T = compute_temp_from_adc(match.group('temp0'),match.group('temp1'))
                        L = compute_lux_from_adc(match.group('light0'),match.group('light1'))
                        
                        self.update_node(node, T, L)
                        
                        data = [0]*self.TABLE_SIZE
                        data[node_ix] = 1
                        
                        if len(neighb):
                            neighb=neighb.strip('&').split('&')
                            
                            for neigh in neighb:
                                neigh_ix = self.add_node(neigh)
                                self.labels[node_ix+1][neigh_ix+1].set_text('x')
                                data[neigh_ix] = 1
                            
                        if self.nodes[0] in neighb:
                            self.count[node_ix]+=1
                            if self.count[node_ix] > self.count_max[node_ix]:
                                self.count_max[node_ix] = self.count[node_ix]
                        else:
                            if self.count[node_ix] > self.count_max[node_ix]:
                                self.count_max[node_ix] = self.count[node_ix]
                            self.count[node_ix] = 0
                            
                        t=time.time()-self.start_time
                        t = str(t)[:str(t).index('.')+2]
                        #~ self.files[node_ix].write(t)
                        i = 1
                        for link in data:
                            if link and self.record:
                                self.files[node_ix].write('%s\t%s\n'%(t,str(link*i)))
                            i+=1
                        #~ self.files[node_ix].write('\n')
                        self.files[node_ix].flush()
                else:
                    cont = False
            else:
                cont = False
                
        # remove dead nodes
        now = time.time()
        for i in range(1, self.TABLE_SIZE):
            for j in range(1, self.TABLE_SIZE):
                if self.labels_time[i][j] + 3 < now:
                    self.labels[i][j].set_text('.')
        
        return True
    
    def quit_all(self, x):
        plot = open("plot.gnu","w")
        plot.write("set term png size 1280, 1024 \n")
        plot.write("set output 'plot.png' \n")

        plot.write("set nokey \n")
        plot.write("set size 1,1 \n")
        plot.write("set origin 0,0 \n")
        plot.write("set multiplot layout 5,2 \n")
        
        for f in self.files:
            n = f.name
            ix = self.files.index(f)
            f.close()
            
            os.system("mv %s %s"%(n, self.names[ix].get_text()+".dat"))
            
            plot.write("set title '%s'\n"%self.names[ix].get_text())
            plot.write("set yrange [0:%i]\n", self.TABLE_SIZE)
            plot.write("set ytics (")
            
            i=0
            for name in self.names:
                if i != 0:
                    plot.write(',')
                plot.write('\" '+name.get_text()+'\"'+str(1+i))
                i+=1
            plot.write(')\n')
            plot.write("plot \'"+self.names[ix].get_text()+".dat\' using 1:2 with points\n")
        plot.write('unset multiplot\n')
        plot.close()
        os.system("gnuplot plot.gnu")
        
        
        gtk.main_quit()
    
    def record_start_cb(self, widget, data=None):
        self.record = True
        self.start_time = time.time()
        self.rec_lb.set_text("recording started")
        self.count = [0]*10
        self.count_max = [0]*10
        
    def record_stop_cb(self, widget, data=None):
        self.record = False
        self.rec_lb.set_text("recording stopped")
        print "Le malade etait "+self.names[0].get_text()
        print "Les contacts les plus longs des autres personnes avec le malade ont ete :"
        for i in range(len(self.names)):
            print self.names[i].get_text(), " : ", self.count_max[i]
        
        print len(self.count_max), len(self.names)
        max_l = [i for i in self.count_max]
        names_l = [i.get_text() for i in self.names]
        top_list = []
        for i in range(len(max_l)):
            ix = max_l.index(max(max_l))
            top_list.append((names_l[ix],max_l[ix]))
            max_l = max_l[:ix]+max_l[ix+1:]
            names_l = names_l[:ix]+names_l[ix+1:]
        
        print "top liste"
        for (n, m) in top_list:
            print n, m
    
    def __init__(self, itf):
        self.itf = itf
        
        self.start_time = time.time()
        
        self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
        self.window.set_title('Neighborhood Discovery')
        self.window.connect("destroy", self.quit_all)
        self.window.set_border_width(10)
        
        vbox = gtk.VBox(False, 0)
        vbox.show()
        self.window.add(vbox)
        
        b_hbox = gtk.HBox(False, 10)
        b_hbox.show()
        vbox.pack_start(b_hbox, False, False, 0)
        bt1 = gtk.Button("Start")
        bt1.show()
        b_hbox.pack_start(bt1, False, False, 0)
        bt1.connect("clicked", self.record_start_cb)
        
        bt2 = gtk.Button("Stop")
        bt2.show()
        b_hbox.pack_start(bt2, False, False, 0)
        bt2.connect("clicked", self.record_stop_cb)
        
        fr = gtk.Frame()
        fr.show()
        b_hbox.pack_start(fr, False, False, 0)
        
        self.rec_lb = gtk.Label("recording stopped")
        self.rec_lb.show()
        fr.add(self.rec_lb)
        
        hbox = gtk.HBox(False, 0)
        hbox.show()
        vbox.pack_start(hbox, False, False, 0)
        name_table = gtk.Table(rows=self.TABLE_SIZE, columns=1, homogeneous=True)
        name_table.show()
        hbox.pack_start(name_table, False, False, 0)
        for i in range(1,self.TABLE_SIZE):
            f = gtk.Frame()
            f.show()
            entry = gtk.Entry(max=0)
            entry.show()
            self.names.append(entry)
            f.add(entry)
            name_table.attach(f, 0,1,i,i+1)
            
        
        table = gtk.Table(rows=self.TABLE_SIZE, columns=self.TABLE_SIZE, homogeneous=True)
        table.show()
        #~ self.window.add(table)
        hbox.pack_start(table)
        
        for i in range(self.TABLE_SIZE):
            self.labels.append([])
            self.labels_time.append([])
            
            for j in range(self.TABLE_SIZE):
                frame = gtk.Frame()
                frame.show()
                table.attach(frame, j, j+1, i, i+1)
                label = gtk.Label('.')
                label.set_justify(gtk.JUSTIFY_CENTER)
                label.show()
                frame.add(label)
                self.labels[-1].append(label)
                self.labels_time[-1].append(time.time())
        
        
        self.timeout = gobject.timeout_add(100, self.handle_data)
        
        self.window.show()
        
        self.read = ''

def main():
    # Enter the event loop
    gtk.main()
    return 0

if __name__ == "__main__":
    
    itf = Interface()
    win = MainWindow(itf)
    
    main()
