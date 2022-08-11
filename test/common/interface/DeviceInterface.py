
import logging

import ctypes
from os import startfile
import time
import struct



from common.graph.SampleData import SampleDataCollection
from common.graph.SampleData import LogVarMap
from PySide2.QtCore import  QObject, QThread, Signal, QTimer, QMutex
import copy
import gc
import serial

log = logging.getLogger('DeviceInterface')
import common.logging.LogHandler as LogHandler
LogHandler.addLogger(log)

class stat_worker():
    CNT = 0
    ERR = 0
    OK = 0


DATA_TYPE = { 'INTEGER8': ctypes.c_int8
    ,'INTEGER16': ctypes.c_int16
    ,'INTEGER32': ctypes.c_int32
    ,'UNSIGNED8': ctypes.c_uint8
    ,'UNSIGNED16': ctypes.c_uint16
    ,'UNSIGNED32': ctypes.c_uint32
    ,'STRING': ctypes.c_uint8
    ,'REAL32': ctypes.c_float }

def num(s):
    try:
        return int(s)
    except ValueError:
        return float(s)

class DeviceWorker(QThread):
    runDone = Signal(int, QObject)  
    receivedData = Signal(list, dict)
    receivedByte = Signal(bytes, dict)
    receivedNone = Signal()

    def __init__(self, argslist, devif):
        QThread.__init__(self)
        self.workerno = stat_worker.CNT
        stat_worker.CNT += 1
        self.argslist = copy.copy(argslist)
        self.devif = devif


    def run(self):
        errcnt = 0
        for args in self.argslist:
            if(len(args) > 0):
                result = self.devif.read(**args)
                if(result is None):
                    if(stat_worker.ERR > 4):
                        log.warning("Worker received None. Supending worker... (%d)" % (self.workerno))
                        break
                    elif(errcnt == 0 and len(self.argslist) > 1):
                        stat_worker.OK = 0
                        self.receivedNone.emit()
                    stat_worker.ERR += 1
                    errcnt = +1

                if isinstance(result,tuple) or isinstance(result, list):
                    self.receivedData.emit(result, args)
                if isinstance(result,bytes):
                    self.receivedByte.emit(result, args)
                
                self.sleep(0.1)
        self.runDone.emit(self.workerno, self)


class DeviceInterface(QObject):
    updateLogVar = Signal()
    updateSampleView = Signal(list)
    connectionClosed = Signal()
    openSignal = Signal()
    preCloseSignal = Signal()

    Types = ['name','type', 'Id', 'host', 'port','baud']
    TypesExt = ['parity', 'stopbits','bytesize', 'timeout','endian']
    ArgsMapping = {'nodeid': 'Id', 'method': 'None','port': 'port', 'host':'host', 'baud':'baudrate', 'parity': 'parity', 'stopbits': 'stopbit','bytesize':'bytesize','timeout':'timeout','endian':'endian'}
    ArgsIntMap = ['baud','baudrate', 'Id', 'timeout']

    def __init__(self, constr=None, config=None, devname="Unknown", **kwargs):
        super(DeviceInterface, self).__init__()
        self.ena = True
        self.workList = []
        self.devWorkList = []
        self.module = None
        self.kwargs = dict()
        self.instance = None
        self.logvarlist = []
        self.IfLock = False
        self.sampleCollection = SampleDataCollection()
        if config is not None:
            for stype in self.Types:
                if stype in config:
                    if stype in self.ArgsMapping:
                        if( stype in self.ArgsIntMap ):
                            self.kwargs.update({self.ArgsMapping[stype]: int(config[stype])})
                        else:
                            self.kwargs.update({self.ArgsMapping[stype]: config[stype]})
                    else:
                        if( stype in self.ArgsIntMap ):
                            self.kwargs.update({stype: int(config[stype])})
                        else:
                            self.kwargs.update({stype: config[stype]})
                    #setattr(self, stype, constr[stype])
                #else:
                    #var = None
                    #setattr(self, stype, var)
        else:
            self.kwargs = kwargs

        if constr is not None:
            self.kwargs.update(constr)

        self.kwargs.update({"LogVarFile":None})
        self.constr = constr
        self.changed = False
        #self.kwargs = {"nodeid": self.id, method='rtu', port=args.Port, parity='N', stopbits=1, bytesize=8, timeout=3, baudrate=args.Baud
        self.type = self.kwargs.get('type', None)
        self.name = self.kwargs.get('name', None)
        self.devname = devname
        self.closed = True

        self.rMutex = None
        self.qMutex = QMutex()



        if(self.type is not None):
            try:
                self.module = __import__(self.type, fromlist=[''])
            except:
                log.error(self.name + " Interface not found (" + self.type + ")")

        self.connection = constr

    def isRemote(self):
        host = self.kwargs.get('host', None)
        if host is None:
            return False
        else:
            return True

    def updateArgs(self, dtype, value):
        stype = dtype
        aval = value
        if stype in self.ArgsMapping:
            stype = self.ArgsMapping[stype]
        if stype in self.ArgsIntMap:
            aval = int(aval)

        if stype in self.kwargs:
            self.kwargs[stype] = aval

        if dtype in self.kwargs:
            self.kwargs[dtype] = aval

        if dtype in self.constr:
            self.constr[dtype] = aval

        if stype in self.constr:
            self.constr[stype] = aval

    def getArgs(self):
        return self.constr

    def isConnect(self):
        if(self.instance is not None):
            return self.instance.isConnect()
        #self.workerTimeout.stop()
        return False

    def createInstance(self):
        name = self.type.split('.')[-1].strip()
        #name = self.type
        class_ = getattr(self.module, name)
        self.instance = class_(**self.kwargs)
        self.instance.setClientIf(self)

        if self.instance.isThreadSafe() is False and self.rMutex is None:
            self.rMutex = QMutex()

    def open(self):

        if self.module is not None:
            try:
                if self.instance is None:
                    self.createInstance()
                else:
                    #update interface arguments.
                    self.instance.Close()
                    time.sleep(0.5)
                    if self.instance.UpdateArgs(**self.kwargs) is False:                        
                        self.createInstance()
                        
            except:
                log.error(self.name + " interface not found (" + self.devname + ")")

            try:
                self.instance.Connect()
                if self.instance.isConnect() is True:
                    log.info("Connected to " + self.devname)
                    self.openSignal.emit()
                else:
                    log.warning("Unable to verify connection status (" + self.devname + ")")
                self.closed = False
            except serial.SerialException as msg:
                log.error("%s Failed to connect: %s (%s)" % (self.name , msg, self.devname))
            except Exception as msg:
                log.error("%s Failed to connect: %s (%s)" % (self.name , msg, self.devname))
            except:
                log.error(self.name + " Failed to connect (" + self.devname + ")")
        else:
            log.info("Device Interface doesn't exist (" + self.type + ")")
        #for stype in self.Types:
            #print (str(getattr(self, stype)))

    def close(self):
        self.closed = True
        if self.instance is None:
            return
        try:
            if self.isConnect():
                self.preCloseSignal.emit()
            self.instance.Close()
            log.info(self.devname + " closed")
            self.connectionClosed.emit()
        except:
            log.error(" Failed to close (" + self.devname + ")")

    def recNone(self):
        QTimer.singleShot(3000, self.clearErr)

    def clearErr(self):
        #If error count is greater than 5 within 3 secounds then close the interface
        if( ((stat_worker.ERR > 20)) and (self.closed == False) ):
            log.warning("Excided error ratio. Closing device" )
            self.close()
        stat_worker.ERR = 0

    def send(self, bytes_data):
        if(bytes_data is not None):
            self.ReceiveSample(bytes_data)

    def ReceiveSample(self, samples):
        if isinstance(samples,str):
            self.sampleCollection.ReceiveSampleHex(samples)
        if isinstance(samples,bytes):
            self.sampleCollection.ReceiveSampleBytes(samples)
        if(self.sampleCollection.hasLogVarUpdates):
            self.updateLogVar.emit()
        if(self.sampleCollection.hasSampleUpdates):
            self.updateSampleView.emit(self.sampleCollection.lastSampleUpdate)

    def write(self, **kwargs):
        access = str(kwargs.get('access', 'W')).lower()
        if access.find('w') == -1:
            return None

        if self.IfLock is True:
            log.info("Interface Lock. %s" % str(self.lockTxt))
            return

        if(self.instance is None):
            self.connectionClosed.emit()
            log.error("Interface " + self.devname + " not connected.")
            return None
        if(self.instance.isConnect() == False ):
            log.error("Interface " + self.devname + " not connected.")
            self.connectionClosed.emit()
            return None

        if 'writefile' in kwargs:
            return self.writeFile(int(kwargs.get('writefile')), kwargs.get('value'), **kwargs)

        self.rwLock()
        try:
            value = num(kwargs.get('value'))
            min = kwargs.get('min',0)
            max = kwargs.get('max',0)
            address = kwargs.get('address')
            datatype = kwargs.get('dataType')
            nodeid = kwargs.get('id', 0)
            if nodeid == 0:
                nodeid = self.kwargs.get('id', 1)
            ok = False
            if(min == 0 and max == 0):
                ok = True
            else:
                if(value >= min and value <= max):
                    ok = True

            if(ok == False):
                log.error("Value out of bounds (min: " + str(min) + ", max: " + str(max) + ", value: " + str(value))
                self.rwUnlock()
                return None

            if(datatype not in DATA_TYPE):
                log.error("Data type not supported " + datatype)
                self.rwUnlock()
                return None

            dtype = DATA_TYPE[datatype]
            log.info("write id %d, addr %d, %s, val % s" % (nodeid, address, datatype, str(value)) )
            self.instance.writeRegisterIf(nodeid,address,dtype,value)
            self.rwUnlock()
        except Exception as err:
            log.error("Error occured %s (tulip : %s)" % (err, str(kwargs)))
            self.rwUnlock()

    def read(self, **kwargs):
        access = str(kwargs.get('access', 'r')).lower()
        if access.find('r') == -1:
            #lest not suspend workers
            return 0


        if self.IfLock is True:
            log.info("Interface Lock. %s" % str(self.lockTxt))
            return

        if(self.instance is None):
            self.connectionClosed.emit()
            log.error("Interface " + self.devname + " not connected.")
            return None

        if(self.instance.isConnect() == False ):
            self.connectionClosed.emit()
            log.error("Interface " + self.devname + " not connected.")  
            return None

        if 'readfile' in kwargs:
            filenum = int(kwargs.get('readfile'))
            return self.readFile(filenum, **kwargs)

        self.rwLock()
        try:
            address = kwargs.get('address')
            datatype = kwargs.get('dataType')
            nodeid = kwargs.get('id', 1)
            
            count = int(kwargs.get('count', 1)) 

            if(datatype not in DATA_TYPE):
                log.error("Data type not supported " + datatype)
                self.rwUnlock()
                return None

            dtype = DATA_TYPE[datatype]
            #log.info("read - id %d, addr %d, cnt %d, dtype %s, datatype %s" % (nodeid, address, count, str(dtype), datatype) )
            ret = self.instance.readRegisterIf(nodeid,address,dtype,count)
            self.rwUnlock()
            if datatype == "STRING":
                try:
                    raw = bytearray(ret)
                    if kwargs.get('reverse', False) is True:
                        raw.reverse()
                    
                    ret = [str(raw.decode('utf-8')).replace('\x00','')]
                except:
                    log.error( "Filed to parse to string utf-8 (%s)" % str(ret) )
                    ret = str(ret)

            #log.info("read - id %d, addr %d,dt %s,val %s" % (nodeid, address, datatype, str(ret)) )
            if(ret is not None):
                stat_worker.OK += 1
            return ret
        except:
            self.rwUnlock()
            return None

    def readFile(self,filenum, **kwargs):
        if self.IfLock is True:
            log.info("Interface Lock. %s" % str(self.lockTxt))
            return

        if(self.instance is None):
            self.connectionClosed.emit()
            log.error("Interface " + self.devname + " not connected.")
            return None

        if(self.instance.isConnect() == False ):
            self.connectionClosed.emit()
            log.error("Interface " + self.devname + " not connected.")
            return None

        self.rwLock()
        try:
            nodeid = kwargs.get('id', None)
            if nodeid is None:
                nodeid = self.kwargs.get('id', 1)

            ret = self.instance.readFileIf(nodeid,filenum)
            self.rwUnlock()
            if(ret is not None):
                stat_worker.OK += 1
            return ret
        except:
            self.rwUnlock()
            return None


    def writeFile(self,filenum, data, **kwargs):
        if self.IfLock is True:
            log.info("Interface Lock. %s" % str(self.lockTxt))
            return

        if(self.instance is None):
            self.connectionClosed.emit()
            log.error("Interface " + self.devname + " not connected.")
            return None

        if(self.instance.isConnect() == False ):
            self.connectionClosed.emit()
            log.error("Interface " + self.devname + " not connected.")
            return None

        self.rwLock()
        try:
            nodeid = kwargs.get('id', None)
            if nodeid is None:
                nodeid = self.kwargs.get('id', 1)

            ret = self.instance.writeFileIf(nodeid,filenum,data)
            self.rwUnlock()
            if(ret is not None):
                stat_worker.OK += 1
            return ret
        except:
            self.rwUnlock()
            return None

    def addWork(self, **work, ):
        if self.IfLock is True:
            log.info("Interface Lock. %s" % str(self.lockTxt))
            return
        self.workList.append(work)
        

    def readWorker(self, callback, priority=0):
        if self.IfLock is True:
            log.info("Interface Lock. %s" % str(self.lockTxt))
            return
        if(self.instance is None):
            self.connectionClosed.emit()
            log.error("Interface " + self.devname + " not connected.")
            return False

        if(self.instance.isConnect() == False ):
            self.connectionClosed.emit()
            log.error("Interface " + self.devname + " not connected.")
            return False


        obj = DeviceWorker(self.workList, self)
        #obj.finished.connect(self.removeWorker)
        obj.runDone.connect(self.workerHandle)
        obj.receivedData.connect(callback)
        obj.receivedByte.connect(callback)
        obj.receivedNone.connect(self.recNone)
        #obj.setTerminationEnabled(True)
        if(priority == 0):
            obj.start(QThread.IdlePriority)
        else:
            obj.start(QThread.HighPriority)

        self.workList.clear()
        #self.workList = []

        self.qMutex.lock()
        self.devWorkList.append(obj)
        self.qMutex.unlock()
        #if(self.isRunning() == False):
        #    self.start()


    def clearLogVar(self):
        self.logvarlist = [0xFFFF]

    ##
    # @brief Add loggin variables is
    #  for instruments that has logging variables
    #  defined on the target
    #
    def addLogVar(self, id):
        self.logvarlist.extend([id])

    ##
    # @brief send variables to log on the target
    #  If filenum is defined in project.json or import from bin
    #  functionality is supported then write file (bytes) else
    #  write the id(s).
    #
    def sendLogVar(self, logvarlist=[]):
        if self.IfLock is True:
            log.info("Interface Lock. %s" % str(self.lockTxt))
            return False
        if ( self.instance is None ):
            return False
        if(len(self.logvarlist) == 0):
            return False
        if(len(self.logvarlist) > 1):
            self.logvarlist = self.logvarlist[1:]

        filenum = self.kwargs.get("LogVarFile", None)
        address = self.kwargs.get('logvar', 5004)
        nodeid = self.kwargs.get('id', 1)
        ret = True
        if(filenum is None):
            self.rwLock()
            try:
                #todo get return statement from writeRegisterIf
                self.instance.writeRegisterIf( nodeid, address, ctypes.c_uint16, self.logvarlist )
                self.rwUnlock()
            except:
                ret = False
                self.rwUnlock()

        else:
            try:
                arr = bytes()
                for var in self.logvarlist:
                    logvar = self.sampleCollection.getLogVar(var)
                    if logvar is not None:
                        arr += logvar.to_bytes()
                for logvar in logvarlist:
                    arr += logvar.to_bytes()
            except Exception as err:
                log.error("Failed to make logvar buffer %s" % err)
                ret = False
            else:
                if len(arr) > 0:
                    self.writeFile(filenum, arr, **self.kwargs)
                else:
                    ret = False
        return ret
            
    
    def workerHandle(self, cnt, wclass):
        #log.info("worker done %d" % cnt)
        self.qMutex.lock()
        

        if wclass.isFinished():
            if (wclass in self.devWorkList):
                self.devWorkList.remove(wclass)

            
        else:
            log.debug("Not Finished")
            #wclass.quit()
            #wclass.wait()
            #if wclass.isFinished():
            #    log.debug("Still Not Finished")

        for inst in self.devWorkList:
            if(inst.isFinished()):
                self.devWorkList.remove(inst)

            
        
        #wclass.quit()
        #wclass.wait()

        #del wclass
        #gc.collect()
        self.qMutex.unlock()

    def rwLock(self):
        if self.rMutex is not None:
            self.rMutex.lock()

    def rwUnlock(self):
        if self.rMutex is not None:
            self.rMutex.unlock()


    ##
    # @brief Disable interface
    #
    def disable(self):
        if self.isConnect() is False:
            return False
        if self.instance is not None and self.ena is True:
            self.ena = False
            self.instance.disable()

    ##
    # @brief Enable interface
    #
    def enable(self):
        if self.isConnect() is False:
            return False
        if self.instance is not None and self.ena is False:
            self.ena = True
            self.instance.enable()

    
    ##
    # @brief Lock interface
    #
    def setInterfaceLock(self, locktxt=None):
        self.IfLock = True
        self.lockTxt = locktxt
    
    ##
    # @brief UnLock interface
    #
    def setInterfaceUnLock(self):
        self.IfLock = False
        self.lockTxt = None



    #     running = False:
    #     while worker in self.workList:
    #         worker.isActive()

    #     pass



#if __name__ == '__main__':

