#!/usr/bin/env python
from pyPgSQL import PgSQL

class SenslabConnection:
    host = "hercule.inrialpes.fr"
    port = 5432
    
    def __init__(self, user="joneill", passwd="joneill", expNum=3, nodeNum=1, t0=0, duration=-1):
        self.user = user
        self.passwd = passwd
        self.exp = expNum
        self.node = nodeNum
        
        self.t0 = t0
        self.duration = duration
        
    def connect(self):
        # try to connect
        self.conn = PgSQL.connect(None, self.user, self.passwd, self.host, self.user)
        
    def fetch_current(self, node):
        # create cursor
        curs = self.conn.cursor()
        if self.duration == -1:
            query = "SELECT time, value FROM exp"+str(self.exp)+".data_current " \
                +"WHERE node_id="+str(node)+" AND time>="+str(self.t0*1000)+" ORDER BY time ASC"
        else:
            query = "SELECT time, value FROM exp"+str(self.exp)+".data_current " \
                +"WHERE node_id="+str(node)+" AND time>="+str(self.t0*1000)+" " \
                +"AND time<"+str((self.t0+self.duration)*1000)+" " \
                +"ORDER BY time ASC"
        print "query= "+query
        
        max = 0.
        
        curs.execute(query)
        value = curs.fetchone()
        current = open("current_%i.dat"%node, "w")
        while value is not None:
            t = float(value["time"])/1000
            i = float(value["value"])*1000
            current.write("%f\t%f\n" %(t, i) )
            value = curs.fetchone()
            
            if i>max: max = i
        current.close()
        self.i_max = max*1.2
    
    
    def close(self):
        self.conn.close()
    
    def gen_plot(self):
        for i in range(1, 5):
            self.fetch_current(i)
        
        gnu = open("plot.gnu", "w")
        
        gnu.write("""
set term png size 800, 600
set output 'plot.png'

set title 'Power Consumption for token ring app'
set xlabel 'time (s)'
set ylabel 'current (mA)'

set yrange [0:%f]"""%(self.i_max)+"""
set ytics 2
set mytics 2
set ytics textcolor lt 1
set ytics nomirror

plot 'current_1.dat' using 1:2 with lines title 'node1', \\
     'current_2.dat' using 1:2 with lines title 'node2', \\
     'current_3.dat' using 1:2 with lines title 'node3', \\
     'current_4.dat' using 1:2 with lines title 'node4'
""")
        gnu.close()
        
        from subprocess import call
        call(["gnuplot", "plot.gnu"])


def print_usage():
    import sys
    print "usage: "+sys.argv[0]+" <expNum> <nodeNum> [<starttime> <duration>]"


if __name__=='__main__':
    import sys, getpass, os
    
    if len(sys.argv) not in [3, 5]:
        print_usage()
        sys.exit(1)
    try:
        exp = int(sys.argv[1])
        node= int(sys.argv[2])
    except:
        print_usage()
        print "<expNum> and <nodeNum> must be integers"
        sys.exit(1)
    
    if len(sys.argv)==5:
        try:
            t0 = float(sys.argv[3])
            dur = float(sys.argv[4])
        except:
            print_usage()
            print "<starttime> and <duration> must be real numbers"
            sys.exit(1)
    else:
        t0 = 0
        dur = -1
    login = os.getlogin()
    passwd = getpass.getpass("Enter password for user `%s`:"%login)
    
    
    c = SenslabConnection(login, passwd, exp, node, t0, dur)
    c.connect()
    
    c.gen_plot()
    c.close()
