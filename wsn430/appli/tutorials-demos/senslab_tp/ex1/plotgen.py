#!/usr/bin/env python

class SenslabPlot:

    def __init__(self, nodeNum=1, t0=0, duration=-1):
        self.node = nodeNum

        self.t0 = t0
        self.duration = duration

    def check_time (self, t):
    	"""Return True if t is within the range defined by self.t0 and self.duration"""

	return t >= self.t0 and (self.duration == -1 or t <= self.duration + t0)

    def fetch_current(self):
        fileDesc = open("current.csv")
        max = 0.

        values = fileDesc.readline()
        current = open("current.dat", "w")
        while values != "":
            try:
                node, time, value = values.split(',')
                if int(node) == self.node:
                    t = float(time)
                    i = float(value)*1000
                    if self.check_time(t):
                        current.write("%f\t%f\n" %(t, i) )
                        if i>max: max = i
                values = fileDesc.readline()
            except: pass


        fileDesc.close()
        self.i_max = max*1.2

    def fetch_voltage(self):
        fileDesc = open("voltage.csv")
        max = 0.

        values = fileDesc.readline()
        voltage = open("voltage.dat", "w")
        while values != "":
            try:
                node, time, value = values.split(',')
                if int(node) == self.node:
                    t = float(time)
                    v = float(value)
                    if self.check_time(t):
                        voltage.write("%f\t%f\n" %(t, v) )
                        if v>max: max = v
                values = fileDesc.readline()
            except: pass


        fileDesc.close()
        self.v_max = max*1.1


    def gen_cv_plot(self):

        self.fetch_current()
        self.fetch_voltage()

        gnu = open("plot.gnu", "w")

        gnu.write("""
set term png size 800, 600
set output 'cv_plot.png'

set title 'Power Consumption for blinking leds'
set xlabel 'time (s)'
set ylabel 'current (mA)'

set yrange [0:%f]"""%(self.i_max)+"""
set ytics 2
set mytics 2
set ytics textcolor lt 1
set ytics nomirror

set y2label 'voltage (V)'
set y2range [0:%f]"""%(self.v_max)+"""
set y2tics 0.5
set my2tics 5
set y2tics textcolor lt 2

plot 'current.dat' using 1:2 with lines title 'current' axis x1y1, \\
     'voltage.dat' using 1:2 with lines title 'voltage' axis x1y2
""")
        gnu.close()

        from subprocess import call
        call(["gnuplot", "plot.gnu"])


def print_usage():
    import sys
    print "usage: "+sys.argv[0]+" <nodeNum> [<starttime> <duration>]"


if __name__=='__main__':
    import sys, getpass, os

    if len(sys.argv) not in [2, 4]:
        print_usage()
        sys.exit(1)
    try:
        node= int(sys.argv[1])
    except:
        print_usage()
        print "<nodeNum> must be integer"
        sys.exit(1)

    if len(sys.argv)==4:
        try:
            t0 = float(sys.argv[2])
            dur = float(sys.argv[3])
        except:
            print_usage()
            print "<starttime> and <duration> must be real numbers"
            sys.exit(1)
    else:
        t0 = 0
        dur = -1

    c = SenslabPlot(node, t0, dur)
    c.gen_cv_plot()
