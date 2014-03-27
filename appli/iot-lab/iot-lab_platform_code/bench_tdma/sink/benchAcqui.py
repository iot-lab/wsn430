#!/usr/bin/env python
import serial, re
import time
import os
import time as t
from svDatas import *


class Sensor():

    def __init__(self,nbchannel):
        self.Node = ''
        self.Count = 0
        self.NbChannel=nbchannel
        self.Data=[0]
        for i in range(1, nbchannel):
            self.Data.append(0)

    def display(self):
        msgdata=''
        for i in range(0, nbchannel-1):
            msgdata=msgdata+' '+str(self.Data[i])
        print self.Node,":",self.Count,msgdata

class acqSensor():
    def __init__(self, nbchannels, period, timerecord, filename, parent=None):
        self.Run=False                     # Thread acqui control
        self.Running=False                 # Indicate if the thread is active
        self.Display=True                  # Print or not data value on GUI
        self.Period=period
        self.TimeRecord=timerecord
        self.FileRecordName=filename
        self.SignalData=[]
        self.SignalIndex=0
        self.Sens=Sensor(nbchannels)

    def gridVerifPress(self):
        if (self.SignalIndex >0):
            print 'NBSCAN=',self.Svdatas.Nbscan
            print 'SCANRATE=',self.Svdatas.Scanrate
            print 'HEADER=',self.Svdatas.Header

            cError=0;
            oldel=self.Svdatas.Datas[0,0]-1;

            index=0
            for el in self.Svdatas.Datas[:,0]:
                if ( ((el-oldel)!=1) and ((el-oldel)!=-65535) ):
                    cError+=1
                    print "Error on index",index," ",el, "skip ",el-oldel-1," stamps"
                oldel=el
                index+=1
            print "Total error= ",cError," ", self.Svdatas.Nbscan," ",cError/self.Svdatas.Nbscan
            print "Rate  error=",(cError/ float (self.Svdatas.Nbscan))*100.0, "%"
#            self.Svdatas.plotAllSignals()
            plt.show()

        else:
            print "Verif","No Signal to plot"

        return

    def saveFile(self,name):
        self.Svdatas.save("./datas/"+name)
        return

    def readData(self,timecount):
        #tpid = os.getpid()
        self.Running=True
        self.SignalData=[]
        self.SignalIndex=0

        ser = serial.Serial('/dev/ttyS0', 115200, timeout=0)

        index=0

        data = ''

        vardata=['d_0']
        analyze='(?P<node_id>[0-9a-f]+):(?P<seq>[\-0-9]+) (?P<d_0>[\-0-9]+)'
        for i in range(1,  self.Sens.NbChannel):
            vardata.append('d_'+str(i))
            analyze=analyze+' (?P<'+vardata[i]+'>[\-0-9]+)'
        analyze=analyze+'\r\n'

#        ptn = re.compile('''(?P<node_id>[0-9a-f]+):(?P<seq>[\-0-9]+) (?P<d_0>[\-0-9]+)\r\n''')

        ptn = re.compile(analyze)

        while self.Run==True:
            data += ser.read(1024)
            if '\r\n' in data:
                end_ix = data.index('\r\n')+2
                line = data[:end_ix]
                data = data[end_ix:]

                m = ptn.match(line)
#               print "RAW LINE=",line
                if m:
                    self.Sens.Node = m.group('node_id')
                    self.Sens.Count = int(m.group('seq'))

                    self.Sens.Data[0]=int(m.group(vardata[0]))
                    tabdat=[self.Sens.Count,self.Sens.Data[0]]
                    for i in range(1,  self.Sens.NbChannel):
                        self.Sens.Data[i]=int(m.group(vardata[i]))
                        tabdat.append(self.Sens.Data[i])

                    if self.SignalIndex==0:
                        self.SignalData=np.array(tabdat)
                    else :
                        self.SignalData=np.vstack((self.SignalData,tabdat))

                    self.SignalIndex+=1

                    if self.Display==True:
                        self.Sens.display()

                if (index>=timecount-1):
                    self.Run=False
                else:
                    index+=1


        self.Running=False

    def stopAcqui(self):
        self.Run=False
        self.Status=0
        if self.SignalIndex > 0 :
            self.Svdatas=svDatas()
            self.Svdatas.addSignal(self.SignalData[:,0],"Count")
            for i in range(1,  self.Sens.NbChannel+1):
                self.Svdatas.addSignal(self.SignalData[:,i],'D'+str(i-1))
            self.Svdatas.Scanrate = self.Period
            self.Svdatas.Nbscan = self.SignalIndex

def test1():
    acq = acqSensor(1,0.08,10.0,"toto")

    acq.Display=False
    acq.Run=True

    if acq.TimeRecord==0.0 :
        print "Start time of acquisition (in sec) ?"
        acq.TimeRecord=input()

    count=int(acq.TimeRecord/acq.Period)
    print "Go..."
    start = t.time()
    acq.readData(count)
    print "Time ",t.time()-start
    acq.stopAcqui()
    acq.gridVerifPress()
    if acq.FileRecordName=="" :
        print "Name of the file to be saved ?"
        acq.FileRecordName=raw_input()

    acq.saveFile(acq.FileRecordName)

    return

if __name__ == '__main__':
    test1()


