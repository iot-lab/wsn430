#!/usr/bin/env python

import pygtk
pygtk.require('2.0')
import gtk, gobject
import serial, re, math

def compute_lux_from_adc(adc0, adc1):
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
    t = msb
    if t >= 128: t -= 256
    t += (lsb/256.)
    return t

class Interface:
    def __init__(self):
        self.serialport = serial.Serial('/dev/ttyS0', 115200, timeout=0)

    def send_cmd(self, cmd):
        for i in cmd:
            self.serialport.write(chr(i))

    def read_data(self):
        return self.serialport.read(1024)


class LedsFrame:
    colors = ['red', 'green', 'blue', 'all']

    LED_CMD = 0x30

    CMD_LED_ON =    (0x0 << 6)
    CMD_LED_OFF =   (0x1 << 6)
    CMD_LED_TOGGLE = (0x2 << 6)
    CMD_LED_BLINK = (0x3 << 6)

    def set_on(self, widget, data=None):
        led = self.colors.index(data)
        cmd = self.CMD_LED_ON + (led<<4)
        print "LED %s ON, cmd=%d" % (data, cmd)
        self.itf.send_cmd([self.LED_CMD, cmd])

    def set_off(self, widget, data=None):
        led = self.colors.index(data)
        cmd = self.CMD_LED_OFF + (led<<4)
        print "LED %s OFF, cmd=%d" % (data, cmd)
        self.itf.send_cmd([self.LED_CMD, cmd])

    def toggle(self, widget, data=None):
        led = self.colors.index(data)
        cmd = self.CMD_LED_TOGGLE + (led<<4)
        print "LED %s TOGGLE, cmd=%d" % (data, cmd)
        self.itf.send_cmd([self.LED_CMD, cmd])

    def __init__(self, masterframe, itf):

        self.itf = itf

        vbox = gtk.VBox(False, 0)
        masterframe.add(vbox)
        vbox.show()

        for color in self.colors:

            frame = gtk.Frame(color)
            vbox.pack_start(frame, False, False, 0)
            frame.show()

            bbox = gtk.HButtonBox()
            bbox.set_border_width(2)
            frame.add(bbox)
            bbox.show()

            button = gtk.Button('ON')
            button.connect("clicked", self.set_on, color)
            bbox.add(button)
            button.show()

            button = gtk.Button('OFF')
            button.connect("clicked", self.set_off, color)
            bbox.add(button)
            button.show()

            button = gtk.Button('TOGGLE')
            button.connect("clicked", self.toggle, color)
            bbox.add(button)
            button.show()

            bbox.set_layout(gtk.BUTTONBOX_START)


class MonitorFrame:
    read = ''
    pattern = re.compile(r'''Length=(?P<len>[0-9]+)\t(?P<data>[0-9a-f:]+)\r\n''')

    SENSOR_MASK  = (0x3 << 6)
    SENSOR_LIGHT = (0x1 << 6)
    SENSOR_TEMP  = (0x2 << 6)
    SENSOR_BOTH  = (0x3 << 6)

    def handle_data(self):
        self.read += itf.read_data()

        if 'Length=' not in self.read:
            self.read = ''
        else:
            self.read = self.read[self.read.index('Length'):]
            if '\r\n' in self.read:
                end_index = self.read.index('\r\n')+2
                line = self.read[:end_index]
                self.read = self.read[end_index:]

                match = self.pattern.match(line)
                if match:
                    data_len = int(match.group('len'))
                    data = match.group('data').rstrip(':').split(':')
                    self.parse_data(data)

        return True

    def parse_data(self, data):
        print 'raw data =',data
        seqnum = int(data[0]) + (int(data[1])<<8)
        print 'seqnum', seqnum
        data = data[2:]

        data_type = int(data[0])
        data = data[1:]

        print 'data_type', data_type

        if data_type & self.SENSOR_MASK == self.SENSOR_LIGHT:
            light_lev = compute_lux_from_adc( int(data[-2]), int(data[-1]) )
            print 'light_lev', light_lev
            self.lightbar.set_fraction(max(light_lev/1000,0) )
        elif data_type & self.SENSOR_MASK == self.SENSOR_TEMP:
            temp = compute_temp_from_adc( int(data[-2]), int(data[-1]) )
            print 'temp', temp
            self.tempbar.set_fraction((temp-23.)/5)
        else:
            light_lev = compute_lux_from_adc( int(data[-4]), int(data[-3]) )
            print 'light_lev', light_lev
            self.lightbar.set_fraction(max(light_lev/1000,0) )
            temp = compute_temp_from_adc( int(data[-2]), int(data[-1]) )
            print 'temp', temp
            self.tempbar.set_fraction((temp-23.)/5)


    def __init__(self, masterframe, itf):
        self.itf = itf


        hbox = gtk.HBox(True, 0)
        masterframe.add(hbox)
        hbox.show()

        self.tempbar = gtk.ProgressBar(adjustment=None)
        hbox.pack_start(self.tempbar, False, False, 5)
        self.tempbar.show()
        self.tempbar.set_fraction(0.3)
        self.tempbar.set_orientation(gtk.PROGRESS_BOTTOM_TO_TOP)
        self.tempbar.set_text('temp.')

        self.lightbar = gtk.ProgressBar(adjustment=None)
        hbox.pack_start(self.lightbar, False, False, 5)
        self.lightbar.show()
        self.lightbar.set_fraction(0.7)
        self.lightbar.set_orientation(gtk.PROGRESS_BOTTOM_TO_TOP)
        self.lightbar.set_text('light')

        self.timeout = gobject.timeout_add(100, self.handle_data)

class SensorConfigFrame:
    SENSOR_CMD = 0x31

    SENSOR_NONE  = (0x0 << 6)
    SENSOR_LIGHT = (0x1 << 6)
    SENSOR_TEMP  = (0x2 << 6)
    SENSOR_BOTH  = (0x3 << 6)

    RATE_4HZ   = (0x0 << 4)
    RATE_8HZ   = (0x1 << 4)
    RATE_16HZ  = (0x2 << 4)
    RATE_32HZ  = (0x3 << 4)
    RATES     = ['4Hz','8Hz','16Hz','32Hz']
    RATES_VAL = [RATE_4HZ,RATE_8HZ,RATE_16HZ,RATE_32HZ]

    SEND_EVERY_MASK = (0x0F)

    def compute_cmd(self):
        cmd = self.sensor + self.rate + self.samplenum
        return cmd

    def get_active_text(self, combobox):
        model = combobox.get_model()
        active = combobox.get_active()
        if active < 0:
            return None
        return model[active][0]

    def sensor_changed(self, widget, data=None):
        on = widget.get_active()
        if data=='temp':
            if on:
                self.sensor += self.SENSOR_TEMP
            else:
                self.sensor -= self.SENSOR_TEMP
        else:
            if on:
                self.sensor += self.SENSOR_LIGHT
            else:
                self.sensor -= self.SENSOR_LIGHT

        cmd = self.compute_cmd()
        itf.send_cmd([self.SENSOR_CMD, cmd])
        print "%s sensor changed to %s" %(data, ("in"*(1-widget.get_active())+"active"))

    def sample_rate_changed(self, widget, data=None):
        i = self.RATES.index(self.get_active_text(widget))
        self.rate = self.RATES_VAL[i]
        cmd = self.compute_cmd()
        itf.send_cmd([self.SENSOR_CMD, cmd])
        print "sample rate changed to ", self.get_active_text(widget)

    def sample_per_pkt_changed(self, widget, data=None):
        self.samplenum = int(self.get_active_text(widget))-1
        cmd = self.compute_cmd()
        itf.send_cmd([self.SENSOR_CMD, cmd])
        print "sample per packet changed to", self.get_active_text(widget)

    def __init__(self, masterframe, itf):
        self.itf = itf

        self.sensor = self.SENSOR_NONE
        self.rate = self.RATE_16HZ
        self.samplenum = 15

        vbox = gtk.VBox(False, 0)
        masterframe.add(vbox)
        vbox.show()

        hbox = gtk.HBox(False, 0)
        vbox.pack_start(hbox, False, False, 5)
        hbox.show()

        txt = gtk.Label("Sensors:")
        hbox.pack_start(txt, False, False, 5);
        txt.show()

        temp_btn = gtk.CheckButton('temperature')
        hbox.pack_start(temp_btn, False, False, 5)
        temp_btn.show()
        temp_btn.connect("toggled", self.sensor_changed, "temp")

        light_btn = gtk.CheckButton('light')
        hbox.pack_start(light_btn, False, False, 5)
        light_btn.show()
        light_btn.connect("toggled", self.sensor_changed, "light")

        hbox = gtk.HBox(False, 0)
        vbox.pack_start(hbox, False, False, 5)
        hbox.show()

        txt = gtk.Label("Sample Rate:")
        hbox.pack_start(txt, False, False, 5);
        txt.show()

        cbx = gtk.combo_box_new_text()
        hbox.pack_start(cbx)
        cbx.show()

        cbx.append_text('4Hz')
        cbx.append_text('8Hz')
        cbx.append_text('16Hz')
        cbx.append_text('32Hz')
        cbx.set_active(2)
        cbx.connect("changed", self.sample_rate_changed)

        hbox = gtk.HBox(False, 0)
        vbox.pack_start(hbox, False, False, 5)
        hbox.show()

        txt = gtk.Label("Sample per Packet:")
        hbox.pack_start(txt, False, False, 5);
        txt.show()

        cbx = gtk.combo_box_new_text()
        hbox.pack_start(cbx)
        cbx.show()

        for i in range(1,17):cbx.append_text(str(i))
        cbx.set_active(5)
        cbx.connect("changed", self.sample_per_pkt_changed)



class MainWindow:
    def __init__(self, title="Example Control"):
        self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
        self.window.set_title(title)
        self.window.connect("destroy", lambda x: gtk.main_quit())
        self.window.set_border_width(10)

        main_hbox = gtk.HBox(False, 0)
        self.window.add(main_hbox)
        main_hbox.show()

        self.ledframe = gtk.Frame("LEDs Control")
        main_hbox.pack_start(self.ledframe, False, False, 5)
        self.ledframe.show()

        sensors_vbox = gtk.VBox(False, 0)
        main_hbox.pack_start(sensors_vbox, False, False, 5)
        sensors_vbox.show()

        self.monitorframe = gtk.Frame("Sensors Monitoring")
        sensors_vbox.pack_start(self.monitorframe, False, False, 5)
        self.monitorframe.show()

        self.sensorframe = gtk.Frame('Sensors Configuration')
        sensors_vbox.pack_start(self.sensorframe, False, False, 5)
        self.sensorframe.show()

        self.window.show()

def main():
    # Enter the event loop
    gtk.main()
    return 0

if __name__ == "__main__":

    itf = Interface()

    win = MainWindow()
    LedsFrame(win.ledframe, itf)
    MonitorFrame(win.monitorframe, itf)
    SensorConfigFrame(win.sensorframe, itf)
    main()
