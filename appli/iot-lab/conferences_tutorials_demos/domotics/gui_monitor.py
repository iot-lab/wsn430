#!/usr/bin/env python

import pygtk
pygtk.require('2.0')
import gtk, gobject
import serial, re, math


class Monitor:
    active_node = ''
    sample_rates = {'2Hz':(0<<4), '1Hz':(1<<4),
                    '0.5Hz':(2<<4), '0.25Hz':(3<<4)}

    leds_colors  = {'red':(0<<4), 'green':(1<<4), 'blue':(2<<4), 'all':(3<<4)}
    leds_actions = {'on':(0<<6), 'off':(1<<6), 'toggle':(2<<6)}

    commands = {'refresh':'2', 'sensors':'1', 'leds':'0'}
    sensors = {'light':(1<<6), 'temp':(2<<6)}

    list_ptrn = re.compile(r'''nodelist=(?P<data>[0-9a-f:]*)\r\n''')
    data_ptrn = re.compile(r'''From=(?P<from>[0-9a-f:]+)\tLength=(?P<len>[0-9]+)\t(?P<data>[0-9a-f:]*)\r\n''')

    def refresh_list(self, wg):
        self.serial_port.write(self.commands['refresh'])

    def select_changed(self, wg):
        list, iter = wg.get_selected()
        addr = list.get_value(iter, 0)
        self.active_node = addr

    def send_sens_cmd(self, wg):

        model  = self.rate_cbx.get_model()
        active = self.rate_cbx.get_active()
        if active < 0:
            rate = '1Hz'
        else:
            rate = model[active][0]

        cmd = [self.commands['sensors']]
        rate_cmd = self.sample_rates[rate] + \
                self.sensors['light'] * self.light_chk.get_active() + \
                self.sensors['temp'] * self.temp_chk.get_active()

        cmd.append(chr(rate_cmd))

        try:
            cmd.append( chr( int(self.active_node, 16) ) )
        except:
            cmd.append( chr( 0 ) )

        self.serial_port.write(''.join(cmd))



    def send_led_cmd(self, widget, (action, color) ):
        cmd = [self.commands['leds']]

        cmd.append( chr(self.leds_colors[color] + self.leds_actions[action]) )

        try:
            cmd.append( chr( int(self.active_node, 16) ) )
        except:
            cmd.append( chr( 0 ) )

        self.serial_port.write(''.join(cmd))

    def init_nodes_frame(self):
        vbox = gtk.VBox(False, 0)
        vbox.show()
        self.nodes_f.add(vbox)

        btn = gtk.Button('Refresh Node List')
        btn.show()
        vbox.pack_start(btn, False, False, 0)
        btn.connect('clicked', self.refresh_list)

        # create the list
        self.nodelist = gtk.ListStore(str,str,str)

        # create the treeview
        self.nodeview = gtk.TreeView(model=self.nodelist)
        self.nodeview.show()
        vbox.pack_start(self.nodeview, False, False, 0)

        # create the columns and associate them
        col_addr = gtk.TreeViewColumn('Addr')
        self.nodeview.append_column(col_addr)
        col_temp = gtk.TreeViewColumn('Temp. (C)')
        self.nodeview.append_column(col_temp)
        col_light = gtk.TreeViewColumn('Light (lux)')
        self.nodeview.append_column(col_light)

        # create the cell renderers
        cell_addr  = gtk.CellRendererText()
        cell_temp  = gtk.CellRendererText()
        cell_light = gtk.CellRendererText()

        # pack the cell renderers in the collumns
        col_addr.pack_start(cell_addr)
        col_temp.pack_start(cell_temp)
        col_light.pack_start(cell_light)

        # connect the cell renderers to the data columns
        col_addr.add_attribute(cell_addr, 'text', 0)
        col_temp.add_attribute(cell_temp, 'text', 1)
        col_light.add_attribute(cell_light, 'text', 2)

        # get the selection of the treeview
        sel = self.nodeview.get_selection()
        sel.connect('changed', self.select_changed)


    def init_sensing_frame(self):
        vbox = gtk.VBox(False, 0)
        vbox.show()
        self.sens_f.add(vbox)


        txt = gtk.Label("Select sensing devices:")
        vbox.pack_start(txt, False, False, 5);
        txt.show()

        hbox = gtk.HBox(False, 0)
        vbox.pack_start(hbox, False, False, 5)
        hbox.show()

        self.temp_chk = gtk.CheckButton('temp.')
        self.temp_chk.show()
        hbox.pack_start(self.temp_chk, False, False, 5)

        self.light_chk = gtk.CheckButton('light')
        self.light_chk.show()
        hbox.pack_start(self.light_chk, False, False, 5)


        txt = gtk.Label("Select sample rate:")
        txt.show()
        vbox.pack_start(txt, False, False, 5)

        self.rate_cbx = gtk.combo_box_new_text()
        self.rate_cbx.show()
        vbox.pack_start(self.rate_cbx)

        for i in sorted(self.sample_rates.keys()):
            self.rate_cbx.append_text(i)

        self.rate_cbx.set_active(2)

        btn = gtk.Button('Send Sensing Command')
        btn.show()
        vbox.pack_start(btn, False, False, 0)
        btn.connect('clicked', self.send_sens_cmd)

    def init_leds_frame(self):

        vbox = gtk.VBox(False, 0)
        self.leds_f.add(vbox)
        vbox.show()

        txt = gtk.Label("Click to update the LEDs:")
        txt.show()
        vbox.pack_start(txt, False, False, 5)

        for color in sorted( self.leds_colors.keys() ):

            frame = gtk.Frame(color)
            vbox.pack_start(frame, False, False, 0)
            frame.show()

            bbox = gtk.HButtonBox()
            bbox.set_border_width(2)
            frame.add(bbox)
            bbox.show()

            button = gtk.Button('ON')
            button.connect("clicked", self.send_led_cmd, ('on',color) )
            bbox.add(button)
            button.show()

            button = gtk.Button('OFF')
            button.connect("clicked", self.send_led_cmd, ('off',color) )
            bbox.add(button)
            button.show()

            button = gtk.Button('TOGGLE')
            button.connect("clicked", self.send_led_cmd, ('toggle',color) )
            bbox.add(button)
            button.show()

            bbox.set_layout(gtk.BUTTONBOX_START)

    def compute_lux_from_adc(self, adc0, adc1):
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

    def compute_temp_from_adc(self, msb, lsb):
        t = msb
        if t >= 128: t -= 256
        t += (lsb/256.)
        return t

    def get_iter_from_addr(self, addr):
        iter = self.nodelist.get_iter_first()
        while iter is not None:
            if addr == self.nodelist.get_value(iter, 0):
                break
            iter = self.nodelist.iter_next(iter)
        return iter

    def update_list(self, nodes):
        for node in nodes:
            if node == '':
                pass
            elif self.get_iter_from_addr(node) is None:
                self.nodelist.append([node, '??', '??'])

    def update_data(self, node, data):
        iter = self.get_iter_from_addr(node)
        if iter is None:
            return

        seq = (int(data[1])<<8) + int(data[0])
        type = int(data[2])

        if type == self.sensors['light']:
            a0 = int(data[3])
            a1 = int(data[4])
            l = self.compute_lux_from_adc(a0, a1)
            l = str(l)[:7]
            self.nodelist.set_value(iter, 2, l)
            print l

        elif type == self.sensors['temp']:
            t0 = int(data[3])
            t1 = int(data[4])
            t = self.compute_temp_from_adc(t0, t1)
            t = str(t)[:7]
            self.nodelist.set_value(iter, 1, t)
            print t

        elif type == (self.sensors['light'] + self.sensors['temp']):
            a0 = int(data[3])
            a1 = int(data[4])
            l = self.compute_lux_from_adc(a0, a1)
            l = str(l)[:7]
            self.nodelist.set_value(iter, 2, l)
            t0 = int(data[5])
            t1 = int(data[6])
            t = self.compute_temp_from_adc(t0, t1)
            t = str(t)[:7]
            self.nodelist.set_value(iter, 1, t)
            print l,t


    def poll_data(self):
        self.serial_data += self.serial_port.read(1024)

        if self.serial_data:
            print repr(self.serial_data)
        if "From=" in self.serial_data:
            data_ix = self.serial_data.index("From=")
        else:
            data_ix = None

        if "nodelist=" in self.serial_data:
            list_ix = self.serial_data.index("nodelist=")
        else:
            list_ix = None


        if ((data_ix is not None) and ((not list_ix) or (data_ix < list_ix))):
            # extract data
            self.serial_data = self.serial_data[data_ix:]
            if '\r\n' not in self.serial_data:
                return True

            end_ix = self.serial_data.index('\r\n')+2

            line = self.serial_data[:end_ix]
            self.serial_data = self.serial_data[end_ix:]

            m = self.data_ptrn.match(line)

            if m:
                self.update_data(m.group('from'), m.group('data').rstrip(':').split(':'))


        elif ((list_ix is not None) and ((not data_ix) or (list_ix < data_ix))):
            # extract list
            list_ix = self.serial_data.index("nodelist=")
            self.serial_data = self.serial_data[list_ix:]
            if '\r\n' not in self.serial_data:
                return True

            end_ix = self.serial_data.index('\r\n')+2

            line = self.serial_data[:end_ix]
            self.serial_data = self.serial_data[end_ix:]

            m = self.list_ptrn.match(line)

            if m:
                nodes = m.group('data').rstrip(':').split(':')
                self.update_list(nodes)

        elif data_ix is None and list_ix is None:
            self.serial_data = ''

        return True

    def init_interface(self):
        self.serial_port = serial.Serial('/dev/ttyS0', 115200, timeout=0)
        self.timeout = gobject.timeout_add(100, self.poll_data)
        self.serial_data = ''

    def __init__(self):

        self.init_interface()

        window = gtk.Window(gtk.WINDOW_TOPLEVEL)
        window.set_title("Network Nodes Monitoring")
        window.connect("destroy", lambda x: gtk.main_quit())
        window.set_border_width(10)

        main_hbox = gtk.HBox(False, 0)
        window.add(main_hbox)
        main_hbox.show()

        self.nodes_f = gtk.Frame("Nodes")
        main_hbox.pack_start(self.nodes_f, False, False, 5)
        self.nodes_f.show()
        self.init_nodes_frame()

        self.sens_f = gtk.Frame("Sensing")
        main_hbox.pack_start(self.sens_f, False, False, 5)
        self.sens_f.show()
        self.init_sensing_frame()

        self.leds_f = gtk.Frame("LEDs")
        main_hbox.pack_start(self.leds_f, False, False, 5)
        self.leds_f.show()
        self.init_leds_frame()

        window.show()



if __name__ == '__main__':
    Monitor()
    gtk.main()
