

from gc import callbacks
import logging
import ctypes
import struct

from common.interface.RegisterInterface import RegisterInterface
from common.interface.SocketInterface import SocketInterface, TransAction

import device.stmProtocolTypes as STM

log = logging.getLogger('SocketStm')
import common.logging.LogHandler as LogHandler
LogHandler.addLogger(log)

from common.interface.DeviceInterface import DATA_TYPE

from PySide2.QtCore import QObject, QEventLoop, QTimer


class testStub(QObject):
    def __init__(self, size=10):
        QObject.__init__(self)
        self.size = 10
    
    def setData(self, data):
        print(str(data[:self.size]))


class SubTransObj(TransAction):
    def __init__(self, key, callback, structure, size, endian="<", done = False) -> None:
        TransAction.__init__(self, key)
        self.key = key
        self.err_code = None
        self.data = bytes()
        self.fmt = endian+structure
        self.size = size
        self.done = done
        self.callback = callback
        if hasattr(callback, '__iter__'):
            self.callbackFunc = self.MultiCallbackHandle
        else:
            self.callbackFunc = callback


    def MultiCallbackHandle(self, data):

        for callback in self.callback:
            #todo handle count
            callback[0](data[:callback[1]])
            data = data[callback[1]:]

    def setData(self, data, err_code):
        ldata = self.data + data
        trSize = len(ldata)
        cdata = ()

        if trSize < self.size:
            self.data = data
            return
        try:
            steps = int( trSize/self.size )
            fmtsize = steps * self.size
            self.data = ldata[fmtsize:]
            ldata = ldata[:fmtsize]
            for i in range(steps):
                cdata += struct.unpack(self.fmt,ldata[:self.size])
                ldata = ldata[self.size:]
        except Exception as err:
            log.error(err)
        else:
            self.callbackFunc(cdata)

        self.err_code = err_code

    def isDone(self):
        return self.done

    def setDone(self):
        self.done = True

class SingleTransObj(TransAction):
    def __init__(self, key, datatype=None, count=1, endian="<") -> None:
        TransAction.__init__(self, key)
        self.datatype = datatype
        self.count = count
        self.endian = endian
        self.eventLoop = QEventLoop(self)
    
    def setData(self, data, err_code):
        self.data = data
        self.err_code = err_code
        self.eventLoop.exit()
    
    def timeout(self):
        self.err_code = STM.ErrorCode.CODE_TIMEOUT
        self.eventLoop.exit()

    def getRaw(self, timeoutMs = 5000):
        if(len(self.data) == 0 and self.err_code is None):
            
            if timeoutMs != 0:
                QTimer.singleShot(timeoutMs,self.timeout)
            self.eventLoop.exec_()

        if (self.err_code):
            return b'', self.err_code
        
        return self.data, self.err_code

    def getData(self, timeoutMs = 5000  ):
        if(len(self.data) == 0 and self.err_code is None):
            
            if timeoutMs != 0:
                QTimer.singleShot(timeoutMs,self.timeout)
            self.eventLoop.exec_()

        if (self.err_code):
            return b'', self.err_code
        
        try:
            return struct.unpack(self.endian + "%d%s"  % ((len(self.data) // ctypes.sizeof(self.datatype)), self.datatype._type_), self.data), self.err_code
        except:
            log.error("Failed to unpack datatype:%s" % str(self.datatype))
            return self.data, STM.ERROR_CODE.RES_UNPACK


SOCKET_SINGLE_TOPIC = 100
SOCKET_MULTI_TOPIC =  101
SOCKET_SUBSCRIBE_TOPIC = 110
SOCKET_READ_FILE_TOPIC = 120
SOCKET_WRITE_FILE_TOPIC = 121
SOCKET_KMC_REC_SUBSCRIBE_TOPIC = 150


SOCKET_SINGLE_FRAME_SIZE = 8

SOCKET_SUBSCRIBE_TYPE = 1
SOCKET_UNSUBSCRIBE_TYPE = 2

class SocketStm(SocketInterface, RegisterInterface):
    def __init__(self, **kwargs):
        SocketInterface.__init__(self, **kwargs)

        
        self.LogType=STM.RAM_LOG_TYPE['SINGLE_SHOT']
        self.logvarlist = [0xFFFF]
        self.fw_version = [0,0,0]
        self.endian = kwargs.get('endian','<')
        try:
            self.readType = int(kwargs.get('readType',0))
        except:
            self.readType = 0

        try:
            self.chunkSize = int(kwargs.get('chunkSize',0))
        except:
            self.chunkSize = 0

        try:            
            self.latency = int(kwargs.get('latency',0))
        except:
            self.latency = 0

        #private vars
        self.subList = dict()
        self.subKeyCnt = 0
        self.decimate = 1

    ##
    # @brief write register value
    #
    def writeRegisterIf(self, nodeId, startReg, dataType, value):
        if self.isConnect() is False:
            return
        err_code = 1
        
        try:
            frameid , reg, buffer = self.getWriteMapping(startReg, dataType, value)
            size = len(buffer) + SOCKET_SINGLE_FRAME_SIZE
            data = bytearray([SOCKET_SINGLE_TOPIC, size, nodeId, frameid, reg, 0,0,0,0])
            key = self.getTransactionKey()
            data[5:] = key.to_bytes(4,self.lendian)
            data.extend(buffer)
            #print(str(data))
            callback = SingleTransObj(key,dataType)
            self.sendData(callback, data, key)
            data, err_code = callback.getData()
            if err_code:
                if err_code in STM.ERROR_CODE:
                    log.error("Received nack: %s" % STM.ERROR_CODE[err_code][1])
                else:
                    log.error("Received nack %d" % err_code)
        except:
            log.error("Failed to write data %d" % startReg)
        return err_code

    ##
    # @brief private Get target mapping
    #
    def getWriteMapping(self, address, dataType, value):
        if not hasattr(value, '__iter__'):
            buffer = [value]
        else:
            buffer = value
        if(address < STM.SER_STM_LOGVAR_ADR):
            frameid = STM.FRAME_ID['SET_REG']
            reg = address
            ftm = self.endian + str(len(buffer)) + dataType._type_
        elif(address >= STM.SER_STM_CMD_ADR and  address < STM.SER_STM_CMD_ADR + 1000):
            frameid = STM.FRAME_ID['EXECUTE_CMD']
            reg = address - STM.SER_STM_CMD_ADR
            ftm = self.endian + str(len(buffer)) + dataType._type_
        elif(address >= STM.SER_STM_LOGVAR_ADR):
            ladr = address - STM.SER_STM_LOGVAR_ADR
            if ladr == STM.SER_STM_LOGVAR_SET:
                if len(buffer) > STM.MAX_LOG_VARS:
                    log.error('too many log variables ('+ str(len(buffer)) + ')')  
                    return None, None,None
                reg = len(buffer)
                frameid = STM.FRAME_ID['KE_SET_LOG_VARIABLES']
                ftm = self.endian + str(len(buffer)) + 'B'
            elif ladr == STM.SER_STM_LOGVAR_DECI:
                reg = 1 # Was Activate but use it as Register
                frameid = STM.FRAME_ID['KE_SET_LOG_CONFIG']
                buffer = [self.LogType, value, 0]
                self.decimate = value
                ftm = self.endian + 'BHB'
            elif ladr == STM.SER_STM_LOGVAR_TYPE:
                reg = 1 # Was Activate but use it as Register
                frameid = STM.FRAME_ID['KE_SET_LOG_CONFIG']
                buffer = [value, self.decimate, 0]
                self.LogType = value
                ftm = self.endian + 'BHB'
            elif(ladr == STM.SER_STM_LOGVAR_ENA_SET):
                reg = 1 # Was Activate but use it as Register 
                frameid = STM.FRAME_ID['KE_SET_LOG_CONFIG']
                ENA = 0
                if value:
                    ENA = STM.ENABLE_LOGGING
                buffer = [self.LogType, self.decimate, ENA]
                ftm = self.endian + 'BHB'
            else:
                log.error("Register not supported %d" % address)

        try:
            raw = struct.pack(ftm, *buffer)
        except struct.error as err:
            log.error("Failed to send decoded msg %s " % str(err))
            return None, None, None
        
        return frameid, reg, raw

    ##
    # @brief read register value
    #
    def readRegisterIf(self, nodeId, startReg, dataType, countReg):
        if self.isConnect() is False:
            return
        try:
            frameid , reg = self.getReadMapping(startReg)
            data = bytearray([SOCKET_SINGLE_TOPIC, SOCKET_SINGLE_FRAME_SIZE, nodeId, frameid, reg, 0,0,0,0])
            key = self.getTransactionKey()
            data[5:] = key.to_bytes(4,self.lendian)
            callback = SingleTransObj(key,dataType,countReg)
            self.sendData(callback, data, key)
            data, err_code = callback.getData()
            if err_code:
                if err_code in STM.ERROR_CODE:
                    log.error("Received nack: %s" % STM.ERROR_CODE[err_code][1])
                else:
                    log.error("Received nack %d" % self.err_code)
            else:
                return data
        except Exception as err:
            log.error(err)
        return None

    ##
    # @brief private Get target mapping
    #
    def getReadMapping(self, address):
        if(address < STM.SER_STM_INTERNAL_ADR):
            frameid = STM.FRAME_ID['GET_REG']
            reg = address
        elif((address >= STM.SER_STM_HW_PARAM_ADR) and (address < (STM.SER_STM_HW_PARAM_ADR + 200))):
            frameid = STM.FRAME_ID['KE_GET_HW_CONFIG']
            reg = address - STM.SER_STM_HW_PARAM_ADR
        elif((address >= STM.SER_STM_FW_PARAM_ADR) and (address < (STM.SER_STM_FW_PARAM_ADR + 1000))):
            frameid = STM.FRAME_ID['KE_GET_FW_CONFIG']
            reg = address - STM.SER_STM_HW_PARAM_ADR
        else:
            log.error("Unknown mapping")
            return None, None
        return frameid, reg

    ##
    # @brief write file
    #
    def writeFileIf(self, nodeId, filenum, file):
        if self.isConnect() is False:
            return
        data = b''
        try:
            #code, nodeid, filenum, type
            data = bytearray([SOCKET_WRITE_FILE_TOPIC, nodeId, filenum, self.readType])
            fileSize = len(file)
            data.extend(fileSize.to_bytes(4,self.lendian))
            data.extend(self.latency.to_bytes(4,self.lendian))
            key = self.getTransactionKey()
            data.extend(key.to_bytes(4,self.lendian))
            data.extend(file)
            callback = SingleTransObj(key,None,0)
            self.sendData(callback, data, key)
            data, err_code = callback.getRaw()
            if err_code:
                if err_code in STM.ERROR_CODE:
                    log.error("Write File Nack: %s" % STM.ERROR_CODE[err_code][1])
                else:
                    log.error("Write File Nack %d" % self.err_code)
            
        except Exception as err:
            log.error("failed to retreive file %s" % str(err))

        return data

    ##
    # @brief read file
    #
    def readFileIf(self, nodeId, filenum):
        if self.isConnect() is False:
            return
        data = b''
        try:
            #code, nodeid, filenum, type
            data = bytearray([SOCKET_READ_FILE_TOPIC, nodeId, filenum, self.readType])
            data.extend(self.chunkSize.to_bytes(4,self.lendian))
            data.extend(self.latency.to_bytes(4,self.lendian))
            key = self.getTransactionKey()
            data.extend(key.to_bytes(4,self.lendian))
            callback = SingleTransObj(key,None,0)
            self.sendData(callback, data, key)
            data, err_code = callback.getRaw()
            if err_code:
                if err_code in STM.ERROR_CODE:
                    log.error("Read File Nack: %s" % STM.ERROR_CODE[err_code][1])
                else:
                    log.error("Read File Nack: %d" % self.err_code)
            
        except Exception as err:
            log.error("failed to retreive file %s" % str(err))

        return data

    ##
    # @brief subscribe data
    # @param kwargs tuplip list of parameters to setup subscription
    # @return subscription id
    def subscribe(self, **kwargs):
        if self.isConnect() is False:
            return None

        items = kwargs.get("items", kwargs)
        endian = kwargs.get("endian", self.endian)
        callback = kwargs.get("callback", None)
        code = kwargs.get("code", SOCKET_SUBSCRIBE_TOPIC)
        data = bytearray([code])
        #latency = kwargs.get("latency", 0)
        
        #rec code for kmc rec
        try:
            reg = kwargs.get("address", None)
            if reg is not None:
                if type(reg) != int:
                    reg = int(reg, base=0)
                data.extend(bytes([reg]))
        except Exception as err:
            log.error("Failed decode reg (/rec code) %s" % str(reg))

        structure = ""
        size = 0
        callbacks = []
        try:
            for item in items:
                clbitem = []
                dataType = item.get("dataType")
                if type(dataType) == str:
                    dataType = DATA_TYPE[dataType]
                count = item.get("count","")
                icnt = 1
                if len(count) > 0:
                    icnt = int(count)
                structure += count + dataType._type_
                clbitem.append( item.get("callback", None) )
                clbitem.append( icnt )
                callbacks.append( clbitem )
                size += icnt * ctypes.sizeof( dataType )
                try:
                    reg = item.get("address", None)
                    if reg is not None:
                        if type(reg) != reg:
                            reg = int( reg, base=0 )
                        data.extend( bytes([reg]) )
                except Exception as err:
                    log.error("Failed decode reg (/rec code) %s" % (str(reg)))
            
        except Exception as err:
            size = 0
            log.error("failed to setup subscription %s" % err)

        if callback is not None:
            callbacks = callback

        if(size > 0):
            key = self.getTransactionKey()
            self.subKeyCnt += 1
            data.extend(bytes([size,SOCKET_SUBSCRIBE_TYPE]))
            clb = SubTransObj(key,callback, structure, size, endian)
            self.subList[self.subKeyCnt] = clb

            send_size = int(kwargs.get("send_size",10))*size
            pool_size = int(kwargs.get("pool_size",1000))*size
            data.extend(send_size.to_bytes(4,self.lendian))
            data.extend(pool_size.to_bytes(4,self.lendian))
            data.extend(key.to_bytes(4,self.lendian))
            self.sendData(clb, data, key)
            return self.subKeyCnt
        return 0
        
    ##
    # @brief subscribe data
    # @param subscription id to remove
    # @return None
    def unSubscribe(self, id):

        if id in self.subList:
            self.subList[id].setDone(True)
            #TODO Should be safe to delete here since the obj
            # is also in the transaction worker queue list 
            del self.subList[id]
        return None

        


    ##
    # @brief set client callback for sending binary data
    #
    def setClientIf(self, clientif):
        pass

    ##
    # @brief Update interface arguments
    #
    def UpdateArgs(self, **kwargs):
        return False

    ##
    # @brief Disable interface
    #
    def disable(self):
        return False

    ##
    # @brief Enable interface
    #
    def enable(self):
        return False

    ##
    # @brief read Progress
    #
    def getProgress(self):
        return 0



if __name__ == "__main__":
    from time import sleep
    import sys
    import json
    from PySide2.QtWidgets import QApplication
    from common.graph.SampleData import SampleDataCollection
    formatter = logging.Formatter('%(asctime)-15s %(threadName)-8s %(levelname)-8s %(lineno)-3s:%(module)-15s  %(message)s')
    consolelog = logging.StreamHandler()
    consolelog.setLevel(level=logging.DEBUG)
    consolelog.setFormatter(formatter)
    log.setLevel(level=logging.DEBUG)
    LogHandler.addHandler(consolelog)
    LogHandler.setLevel(logging.DEBUG)

    
    stub = testStub()
    subtrans = SubTransObj(0,stub.setData,"4B",4)
    subtrans.setData(bytes([1,2,3,4,5,6,7]),0)
    subtrans.setData(bytes([8,9,10,11,12,13,14]),0)
    subtrans.setData(bytes([15,16,17,18,19,20]),0)
    subtrans.setData(bytes([21,22,23,24]),0)

    x = input("Press y to exit \n>")
    if  x == 'y':
        exit()

    app = QApplication(sys.argv)
    socket_serial = SocketStm(port="COM28", baud="1000000")
    

    while x != 'q':
        if socket_serial.isConnect():
            print("1  - Close Connection")
            print("2  - Subscribe msg enable maintenance msg")
            print("3  - Subscribe msg enable health msg")
            print("4  - Disable maintenance and health msg")
            print("5  - Read register")
            print("6  - Read UID")
            print("7  - Write register")
            print("8  - Write decimate")
            print("9  - Write logtype")
            print("10 - Write enable")
            print("11 - Write logvar config")
            print("12 - Read logging variable file")
            print("13 - Read log file")
            print("14 - Get interface statistics")
            print("q  - exit")
        else:
            print("1 - Connect")
            print("q  - exit")

        x = input("input cmd \n>")
        if x == '1':            
            if socket_serial.isConnect() is False:
                if socket_serial.Connect() is False:
                    log.error("Failed to connect")
                else:
                    # disable ALL transmitters    
                    log.info("disable transmitters...")
                    socket_serial.writeRegisterIf(2,173, ctypes.c_uint16, 2)
            else:
                socket_serial.Close()



        elif x == '2':
            stub1 = testStub(size=22)
            json_string = \
                u'{ "code": 150, \
                    "address":"0xAA",\
                    "send_size": 400, \
                    "pool_size": 120000, \
                    "items" :[ \
                    {"name":"state_io","dataType":"UNSIGNED8"}, \
                    {"name":"deflection cmd","dataType":"UNSIGNED8"}, \
                    {"name":"theta yaw", "dataType": "REAL32" }, \
                    {"name":"theta pitch", "dataType": "REAL32" }, \
                    {"name":"Q M1", "dataType": "INTEGER16" }, \
                    {"name":"D M1", "dataType": "INTEGER16" }, \
                    {"name":"Q M2", "dataType": "INTEGER16" }, \
                    {"name":"D M2", "dataType": "INTEGER16" } \
                    ]}'

            cnf = json.loads(json_string)
            cnf['callback'] = stub1.setData
            socket_serial.subscribe(**cnf)

            #enable maintenance transmitter
            socket_serial.writeRegisterIf(2,173, ctypes.c_uint16, 3)
            
        elif x == '3':
            stub2 = testStub(size=7)
            json_string = \
                u'{ "code": 150, \
                    "address":"0xBC",\
                    "send_size": 1, \
                    "pool_size": 300, \
                    "items" :[ \
                    {"name":"FaultOccurred M1","dataType":"UNSIGNED16"}, \
                    {"name":"State M1","dataType":"UNSIGNED8"}, \
                    {"name":"FaultOccurred M2", "dataType": "UNSIGNED16" }, \
                    {"name":"State M2", "dataType": "UNSIGNED8" }, \
                    {"name":"io state", "dataType": "UNSIGNED8" } \
                    ]}'
            
            cnf = json.loads(json_string)
            cnf['callback'] = stub2.setData
            socket_serial.subscribe(**cnf)

            #Enable health transmitter
            socket_serial.writeRegisterIf(2,173, ctypes.c_uint16, 4)

        
        elif x == '4':
            socket_serial.writeRegisterIf(2,173, ctypes.c_uint16, 2)


        elif x == '5':
            x = 'y'
            while  x == 'y':
                i = 0
                while i < 3:
                    val = socket_serial.readRegisterIf(2, 187, ctypes.c_int16, 1)

                    log.debug("Received val %s" % str(val))
                    i +=1
                x = input("Press y to read register \n>")
                


        

        elif x == '6':
            val = socket_serial.readRegisterIf(2, 6003, ctypes.c_int32, 3)
            log.debug("Received val %s" % str(val))

        
        elif x == '7':
            val = socket_serial.writeRegisterIf(2,187, ctypes.c_int16, 2)
            val = socket_serial.writeRegisterIf(2,187, ctypes.c_int16, 0)
            log.debug("Received val %s" % str(val))

        
        elif x == '8':
            val = socket_serial.writeRegisterIf(2, 5002, ctypes.c_uint16, 200)
            log.debug("Received val %s" % str(val))

        
        elif x == '9':
            val = socket_serial.writeRegisterIf(2, 5003, ctypes.c_uint16, 0)
            log.debug("Received val %s" % str(val))

        
        elif x == '10':
            val = socket_serial.writeRegisterIf(2, 5006, ctypes.c_uint16, 1)
            log.debug("Received val %s" % str(val))

        
        elif x == '11':
            val = socket_serial.writeRegisterIf(2, 5004, ctypes.c_uint16, [1,2,5])
            log.debug("Received val %s" % str(val))

        
        elif x == '12':
            sampleCollection = SampleDataCollection()
            log.debug("File request")
            val = socket_serial.readFileIf(2,0)
            log.debug("File received")

            sampleCollection.ReceiveSampleBytes(val)
            for logvar in sampleCollection.LogVarList:
                logvar.print()
            
            if len(sampleCollection.LogVarList) > 3:
                x = input("Press y to write file \n>")
                if x == 'y':
                    file = sampleCollection.LogVarList[0].to_bytes()
                    file += sampleCollection.LogVarList[2].to_bytes()
                    log.debug("File request")
                    val = socket_serial.writeFileIf(2,0, file)
                    log.debug("File received")
                    
            #sampleCollection.ReceiveSampleBytes(val)
            #for logvar in sampleCollection.LogVarList:
            #    logvar.print()


        
        elif x == '13':
            log.debug("File request")
            val = socket_serial.readFileIf(2,1)
            log.debug("File received")

        elif x == '14':
            print(socket_serial.getStats())

        if x == 'q':
            socket_serial.Close()
    sys.exit()
