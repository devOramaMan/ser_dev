

import sys
import ctypes
from enum import Enum
import struct
import time
import logging
from enum import Enum


from ctypes import c_int8, c_uint8, c_byte, c_ubyte, c_int16, c_uint16, \
    c_int32, c_uint32, c_int, c_uint, c_long, c_ulong, c_longlong, c_ulonglong, \
    c_int64, c_uint64, c_float, \
    sizeof

from common.bin.calc import Calc

log = logging.getLogger('SampleData')
import common.logging.LogHandler as LogHandler
LogHandler.addLogger(log)

DEV_CODE = { 0xAAFFAAFF:'MotorController',
            0xBBFFBBFF:'TensionSensor',
            0xCCFFCCFF:'PositionSensor',
            0xAAAAAAAB:'RockStop'}
        
DEV_MODEL = { 0xAAFFAAFF:'KoconMC',
            0xBBFFBBFF:'SG1',
            0xCCFFCCFF:'PositionSensor',
            0xAAAAAAAB:'RockStop' }

DATA_TYPE_STR = { 
     ctypes.c_int8 : 'int8'
    ,ctypes.c_int16: 'int16'
    ,ctypes.c_int32: 'int32'
    ,ctypes.c_uint8: 'uint8'
    ,ctypes.c_uint16:'uint16'
    ,ctypes.c_uint32: 'uint32'
    ,ctypes.c_float: 'float32'  
    }
    
def getValue(dataType, arr, lendian='<'):
    if(len(arr)<dataType.size):
        log.error("getValue Data Size too small")
        return 0

    raw = struct.pack(lendian + "%dB" % len(arr), *arr)
    fmt = dataType.endian + str((len(raw) // dataType.size)) + dataType.fmt

    return struct.unpack(fmt, raw)

def str_to_num(val):
    pass


class DATA_TYPE_ENUM(Enum):
    INTEGER8 = 2
    INTERGER16 = 3
    INTEGER32 = 4
    UNSIGNED8 = 5
    UNSIGNED16 = 6
    UNSIGNED32 = 7
    REAL32 = 8
    STRING = 9
    TU32_INTEGER8 = 12
    TU32_INTERGER16 = 13
    TU32_INTEGER32 = 14
    TU32_UNSIGNED8 = 15
    TU32_UNSIGNED16 = 16
    TU32_UNSIGNED32 = 17
    TU32_REAL32 = 18

DATA_TYPE_STR_MAPPING = { 
    'int8_t': DATA_TYPE_ENUM.INTEGER8,
    'int16_t':DATA_TYPE_ENUM.INTERGER16,
    'int32_t':DATA_TYPE_ENUM.INTEGER32,
    'uint8_t':DATA_TYPE_ENUM.UNSIGNED8, 
    'uint16_t':DATA_TYPE_ENUM.UNSIGNED16,
    'uint32_t':DATA_TYPE_ENUM.UNSIGNED32,
    'float32_t':DATA_TYPE_ENUM.REAL32,
    'float':DATA_TYPE_ENUM.REAL32,
    'int':DATA_TYPE_ENUM.INTEGER32,
    'unsigned int': DATA_TYPE_ENUM.UNSIGNED32,
    'int8': DATA_TYPE_ENUM.INTEGER8,
    'int16':DATA_TYPE_ENUM.INTERGER16,
    'int32':DATA_TYPE_ENUM.INTEGER32,
    'uint8':DATA_TYPE_ENUM.UNSIGNED8, 
    'uint16':DATA_TYPE_ENUM.UNSIGNED16,
    'uint32':DATA_TYPE_ENUM.UNSIGNED32,
    'float32':DATA_TYPE_ENUM.REAL32
    }

def typeStr2Emun(type):
    type = type.replace(" ", "")
    type = type.replace("volatile", "")
    ret = None
    if type in DATA_TYPE_STR_MAPPING:
        ret = DATA_TYPE_STR_MAPPING[type]
    return ret

class LOG_TYPE_ENUM(Enum):
    SINGLE_SHOT = 0
    RING_BUFFER = 1

class LOG_STATUS_ENUM(Enum):
    EMPTY = 0
    LOGGING = 1
    FINISHED = 2
    WRAPPED = 3

class DataType(object):
    LEAGAL_DATA_TYPE_MIN = 2 
    LEAGAL_DATA_TYPE_MAX = 8
    LEAGAL_TIMESTAMP_MIN = 12 
    LEAGAL_TIMESTAMP_MAX = 18
    endian='<'

    def __init__(self, val):
        self.atype = 0
        self.ctype = 0
        self.size = 0
        
        """Dispatch method"""
        if isinstance(val, str):
            lval = typeStr2Emun(val)
            if lval is None:
                log.error("Unknown Str datatype: " + val + ", (Leagalrange: " + str(self.LEAGAL_DATA_TYPE_MIN) + " - " + str(self.LEAGAL_DATA_TYPE_MAX) + ")")
                val = DATA_TYPE_ENUM.UNSIGNED16
            else:                
                val = lval

        if isinstance(val, Enum):
            val = val.value
                
        if ((val < self.LEAGAL_DATA_TYPE_MIN or val > self.LEAGAL_DATA_TYPE_MAX) and 
            (val < self.LEAGAL_TIMESTAMP_MIN or val > self.LEAGAL_TIMESTAMP_MAX )):
            log.error("Unknown datatype: " + str(val) + ", (Leagalrange: " + str(self.LEAGAL_DATA_TYPE_MIN) + " - " + str(self.LEAGAL_DATA_TYPE_MAX) + ")")
        else:
            lval = val
            if(lval >=  self.LEAGAL_TIMESTAMP_MIN and lval <= self.LEAGAL_TIMESTAMP_MAX ):
                lval = lval - 10
                
            method_name = 'type_' + str(lval)
            # Get the method from 'self'. Default to a lambda.
            method = getattr(self, method_name, lambda: "Invalid")
            # Call the method as we return it
            self.type = val
            self.ctype = method()
            

            if(val >=  self.LEAGAL_TIMESTAMP_MIN and val <= self.LEAGAL_TIMESTAMP_MAX ):
                #TODO add nice uni syntax isted of Q and 8
                # Issue: on Raspbarry Pi the uint64 is an unsigned long (L)
                # while on 64x host computer uint64 is unsigned long long
                # Workaround:
                self.fmt = 'Q' + self.ctype._type_
                self.size = 8 + sizeof(self.ctype)
                #This does not work if packing/unpacking on x86 to x64:
                #self.fmt = 'c_uint64._type_' + self.ctype._type_
                #self.size = sizeof(self.ctype) + sizeof(c_uint64)
            else:
                self.fmt = self.ctype._type_
                self.size = sizeof(self.ctype)
            
        
    
    def type_2(self):
        return c_int8
 
    def type_3(self):
        return c_int16

    def type_4(self):
        return c_int32

    def type_5(self):
        return c_uint8

    def type_6(self):
        return c_uint16

    def type_7(self):
        return c_uint32

    def type_8(self):
        return c_float
    
    def name(self):
        if self.ctype in DATA_TYPE_STR:
            return DATA_TYPE_STR[self.ctype]
        else:
            return "unknown"

    def from_str(self, val):
        if self.type == DATA_TYPE_ENUM.TU32_REAL32.value or self.type == DATA_TYPE_ENUM.REAL32.value:
            return float(val)
        else:
            return int(val,0)

    def print(self):
        print("DataType:     " + self.name() + " size: " + str(self.size))

class LogVar(object):
    #Data type 
    fmt = '3I36s12sIfB15x'
    endian = '<'
    bytesSize = 84

    def __init__(self, data_bytes=[], **kwargs):
        #Private
        self.strlist = {
            "logid":self.getLogid,
            "name": self.getName,
            "alias": self.getAlias,
            "dataType": self.getDatatype,
            "unit": self.getUnit,
            "frequency": self.getFreq,
            "scalefactor": self.getScaleStr,
            "address": self.getAddress
            }
        self.invalid = None

        self.valid = True
        self.devName = ""
        # The logVar    
        self.logId = kwargs.get('logid', 0)
        addr = kwargs.get('address', 0)
        if ( isinstance(addr, str)):
            if addr.find('0x') != -1:
                addr = int(addr,0)
            else:
                addr = int(addr) 
        
        self.address = addr
        self.name = kwargs.get('name', 0)
        self.alias = kwargs.get('alias', "")
        self.unit = kwargs.get('unit', "")
        self.frequency = kwargs.get('frequency', 20000)
        scalefactor = kwargs.get('scalefactor', 1.0)
        if isinstance(scalefactor, str):
            self.scaleFactorStr = scalefactor
            try:
                self.scaleFactor = Calc.eval(scalefactor)
            except:
                log.error("Failed to eval scale factor. using 1.0")
                self.scaleFactor = 1.0
                self.scaleFactorStr = "1.0"
        else:
            self.scaleFactorStr = str(scalefactor)
            self.scaleFactor = float(scalefactor)

        self.dataType = DataType(kwargs.get('dataType', DATA_TYPE_ENUM.UNSIGNED16))
        devcodes = list(DEV_CODE.keys())
        self.CODE = kwargs.get('devcode', devcodes[0])
        self.endian = kwargs.get('endian','<')        
        self.from_bytes(data_bytes)

    def from_bytes(self, data_bytes):
        if(len(data_bytes) > 0):
            self.valid = False
        if(len(data_bytes)<self.bytesSize):
            self.valid = False
            return self

        adtval = 0
        code = 0
        name = ""
        unit = ""
        try:
            record = data_bytes[:self.bytesSize] 
            del data_bytes[:self.bytesSize]
            (code, self.logId, self.address, name, unit, self.frequency,
            self.scaleFactor, adtval
            ) = struct.unpack(self.endian + self.fmt, record)   
            self.valid = True
        except struct.error as err:
            log.error(err)
            self.valid = False
            return self
        except:
            log.error("Failed to parse")
            self.valid = False
            return self
        
        #Get DataType
        self.dataType = DataType(adtval)
        self.dataType.endian = self.endian

        self.name = name.decode('utf8').rstrip('\x00')
        self.unit = unit.decode('utf8').rstrip('\x00')

        try:
            self.devName = DEV_CODE[code]
            self.CODE = code
        except IndexError:
            self.valid = False

        #Validate LogVar type
        if (self.dataType.size == 0): 
            self.valid = False
        return self

    def to_bytes(self):
        bytes_data = bytes([])
        #name="{:<36}".format(self.name)
        name = self.resolveName()
        if(len(name)<=36):
            name = bytes(name, "utf-8")
            name += b"\x00" * (36 - len(name))

        if(len(self.unit)<=12):
            unit = bytes(self.unit, "utf-8")
            unit += b"\x00" * (12 - len(unit))
        else:
            unit = self.unit

        try:
            record = (self.CODE, int(self.logId), self.address, name, unit, self.frequency,
            self.scaleFactor, self.dataType.type)
            fmt = self.endian+self.fmt
            bytes_data = struct.pack(fmt, *record)
        except struct.error as err:
            log.error(err)
        except:
            log.error("Failed to parse")

        return bytes_data

    def to_string(self):
        ret = "{"
        for item in self.strlist:
            ret += '"' + item + '"'
            ret += ":"
            var = self.strlist[item]()
            if isinstance(var,str):
                ret += '"' + var + '"'
            else:
                ret += str(var)
            ret += ", "
        ret = ret[:-2] + "}"
        return ret

    def update(self, func, val):
        member = getattr(self, func)
        if member != '':
            member(val)
        else:
            log.error("failed to update attribute %s" % type)
        
    def updateLogVar(self, var):
        self.AliasName(var.getAlias())
        self.DataType(var.getDatatype())
        self.Unit(var.getUnit())
        self.Frequency(var.getFreq())
        self.Scalefactor(var.getScaleStr())
        self.Address(var.getAddress())

    def getLogid(self):
        return self.logId

    def getName(self):
        return self.name
    
    def getAlias(self):
        return self.alias

    def getDatatype(self):
        return self.dataType.name()

    def getFreq(self):
        return self.frequency

    def getUnit(self):
        return self.unit

    def getScaleStr(self):
        return self.scaleFactorStr
    
    def getScale(self):
        return self.scaleFactor

    def getAddress(self,type=0):
        ret = self.address
        if type == 1:
            ret = hex(self.address)
        return ret

    def Name(self, val):
        self.name = val
    def AliasName(self, val):
        self.alias = val
    def DataType(self, val):
        self.dataType = DataType(val)
    def Unit(self, val):
        self.unit = val
    def Frequency(self, val):
        self.frequency = int(val,0)
    def Scalefactor(self, val):
        scalefactor = val
        if isinstance(scalefactor, str):
            self.scaleFactorStr = scalefactor
            try:
                self.scaleFactor = Calc.eval(scalefactor)
            except:
                log.error("Failed to eval scale factor. Using 1.0")
                self.scaleFactor = 1.0
                self.scaleFactorStr = "1.0"
        else:
            self.scaleFactorStr = str(scalefactor)
            self.scaleFactor = float(scalefactor)
    def Address(self,addr):
        laddr=addr
        if ( isinstance(addr, str) ):
            try:
                if laddr.find('0x') != -1:
                    laddr = int(laddr,0)
                else:
                    laddr = int(laddr)
            except:
                log.error("failed to set address %s" % addr)
                return

        if ( isinstance(laddr, int) is False):
            log.error("failed to set address %s" % str(addr))
            return
        self.address = laddr
    def resolveName(self):
        ret = self.name
        if self.alias != '':
            ret = self.alias
        return ret

    def print(self):
        print("*******  LogVar *******")
        print("LogId:        " + str(self.logId))
        print("address:      " + hex(self.address))
        print("name:         " + self.name)
        print("unit:         " + self.unit)
        print("frequency:    " + hex(self.frequency))
        print("scaleFactor:  " + str(self.scaleFactor))
        if(self.dataType != 0):
            self.dataType.print()
        print("valid:        " + str(self.valid))


class SampleData(object):
    #fmt = 'BHB3I'
    fmt = '7I'
    endian='<'
    bytesSize = 28
    CODE=0xAAAAAAAA
    MAX_SAMPLES=0x1000

    def __init__(self, data_bytes=[], **kwargs):
        self.valid = True
        #The Sample Data
        
        self.logVar = 0
        self.logType = kwargs.get('logType', 0)
        self.status = kwargs.get('status', 0)
        self.sAddr = 0
        self.decimate = 0
        self.size = kwargs.get('size', 0x1000)
        self.timeStamp = time.time()
        self.logPos = 0 # The last logged position (if status = wrapped then it is the start else it is the start)
        self.samples = []
        self.timetag = []

        self.check_code(data_bytes)

        self.logVar = LogVar(data_bytes, **kwargs)

        self.from_bytes(data_bytes)

        #Make timestamp for when the logvar is created
        self.createTime = time.time()

    def check_code(self, data_bytes):
        if(len(data_bytes)<4):
            return
        try:
            record = data_bytes[:4]; del data_bytes[:4]
            (code,) = struct.unpack(self.endian + 'I',record)
            if(code != self.CODE):
                self.valid = False

        except struct.error as err:
            log.error(err)
            self.valid = False
            return self


    #
    def from_bytes(self, data_bytes):

        if(len(data_bytes) > 0):
            self.valid = False
        if(len(data_bytes)<self.bytesSize) or (self.logVar.valid == False):
            self.valid = False
            return self

        u8Size = 0
        sampleTime = 0

        try:
            record = data_bytes[:self.bytesSize]; del data_bytes[:self.bytesSize]            
            (self.logType, self.decimate, self.status, self.sAddr, 
            u8Size, sampleTime, self.logPos) = struct.unpack(self.endian + self.fmt,record)
            self.valid=True
        except struct.error as err:
            log.error(err)
            self.valid = False
            return self
        except:
            log.error("Failed to parse")
            self.valid = False
            return self
        
                      
        if (u8Size == 0):
            self.valid = False

        if (sampleTime > 0):
            self.timeStamp = sampleTime

        if(u8Size > len(data_bytes)):
            log.warning("length is greater than trailing payload")
            u8Size = len(data_bytes)

        lencheck = u8Size / self.logVar.dataType.size
        while(lencheck.is_integer() == False):
            u8Size -= 1
            lencheck = u8Size / self.logVar.dataType.size

        #Size 
        self.size = int(u8Size/self.logVar.dataType.size)

        if(self.valid == False or self.logVar.valid == False):
            log.error("Sample Not Valid")

        if(self.logVar.dataType.type >= DataType.LEAGAL_TIMESTAMP_MIN and self.logVar.dataType.type <= DataType.LEAGAL_TIMESTAMP_MAX):
            self.samples = []
            count = 0
            while (count < self.size):
                element = data_bytes[:self.logVar.dataType.size]; del data_bytes[:self.logVar.dataType.size]
                lsample = getValue( self.logVar.dataType, element)
                self.timetag.append(lsample[0])
                self.samples.append(lsample[1])
                count += 1
        else:
            element = data_bytes[:u8Size]; del data_bytes[:u8Size]
            self.samples = getValue(self.logVar.dataType, element)

            #remove last samples if they are equal ZERO
            count = 0
            while(self.samples[-1] == 0 and count < 4): 
                self.samples = self.samples[:-1]
                count += 1



        #count = 0
        #while ((count + self.logVar.dataType.size) <= u8Size) and (len(data_bytes) >= self.logVar.dataType.size):
        #    element = data_bytes[:self.logVar.dataType.size]; del data_bytes[:self.logVar.dataType.size]
        #    lsample = getValue(  self.logVar.dataType, element)
        #    self.samples.append(lsample)
        #    count += self.logVar.dataType.size

        if(len(data_bytes)>0):
                element = data_bytes[0]
        else:
            element = 1
        while (element == 0 and (len(data_bytes) > 0) ):
            del data_bytes[0]
            if(len(data_bytes)>0):
                element = data_bytes[0]
            else:
                element = 1
        
        return self
        
    def to_bytes(self, max=None):
        
        if(len(self.timetag) > 0):
            if(len(self.timetag) != len(self.samples)):
                log.error("Missmatch size time and data")
                return None

        u8Size = int(self.logVar.dataType.size * len(self.samples))
        #print("Pack Bytes %d %d" % (u8Size, len(self.samples)))
        bytes_data = struct.pack(self.endian + 'I', self.CODE)
        bytes_data += self.logVar.to_bytes()
        timestamp = int(self.timeStamp)
        ok = False
        if(len(bytes_data) == 0):
            return bytes_data
        try:
            record = (self.logType, self.decimate, self.status, self.sAddr, 
            u8Size, timestamp, self.logPos)
            bytes_data += struct.pack(self.endian + self.fmt, *record)
            ok = True
        except struct.error as err:
            log.error(err)
        except:
            log.error("Failed to parse")
        
        if(ok == True):
            bsample = bytes([])
            cnt = 0
            
            tlen = len(self.timetag)
            if(max is None):
                max = len(self.samples)


            for sample in self.samples[:max]:
                lSample = 0
                if (cnt < tlen):
                    lSample = (self.timetag[cnt], sample)
                    fmt=self.endian + self.logVar.dataType.fmt
                else:
                    lSample = [sample]
                    fmt=self.endian+self.logVar.dataType.ctype._type_
                try:
                    bsample += struct.pack( fmt , *lSample)
                except struct.error as err:
                    log.error(err)
                    log.error("sample:" + str(lSample) + " fmt:" + fmt)
                cnt += 1
            if(len(bsample) > 0):
                bytes_data += bsample
        return bytes_data

    def to_list(self, scale=False, max=None):
        if(max is None):
            max = len(self.samples)
        if scale is True and self.logVar.scaleFactor != 1.0:
            return [[s*self.logVar.scaleFactor for s in self.samples[:max]]]
        else:
            return [self.samples[:max]]

    def addSampleFromStr(self, value, timetag=None):
        try:
            if self.size is None:
                if timetag is not None:
                    self.timetag.append(float(timetag))
                self.samples.append(self.logVar.dataType.from_str(value))
                self.logPos = len(self.samples)
            else:
                if(len(self.samples) < self.size):
                    if timetag is not None:
                        self.timetag.append(float(timetag))
                    self.samples.append(self.logVar.dataType.from_str(value))
                    self.logPos = len(self.samples) 
                else:
                    self.status = LOG_STATUS_ENUM.FINISHED.value
            return True
        except:
            return False

    def addSample(self, value, timetag=None):
        if self.size is None:
            if timetag is not None:
                self.timetag.append(timetag)
            self.samples.append(value)
            self.logPos = len(self.samples)
        else:
            if(len(self.samples) < self.size):
                if timetag is not None:
                    self.timetag.append(timetag)
                self.samples.append(value)
                self.logPos = len(self.samples)
            else:
                self.status = LOG_STATUS_ENUM.FINISHED.value

    def clear(self):
        self.samples.clear()
        self.timetag.clear()


    def print(self):
        self.logVar.print()
        print("*******  SampleData *******")
        print("LogType:         " + str(self.logType))
        print("status:          " + hex(self.status))
        print("valid:           " + str(self.valid))
        print("Start Addr:      " + hex(self.sAddr))
        print("Size:            " + str(self.size))
        print("End Addr:        " + hex(self.logPos))

        idx = 0
        timesize = len(self.timetag)
        if(idx < timesize):
            print("Time, Sample")
        else:
            print("Sample")

        for sample in self.samples:
            if(idx < timesize):
                strsample = str(self.timetag[idx]) +", " + str(sample)
            else:
                strsample = str(sample)
            
            idx += 1
            print(strsample)

    def __lt__(self, other):
        return self.createTime < other.createTime

class LogVarMap(object):
    def __init__(self,dev, id):
        self.dev = dev
        self.id = id
    def __repr__(self):
        return '({},{})'.format(self.id, self.dev)

class SampleDataCollection(object):
    def __init__(self, **kwargs):
        self.SampleList = kwargs.get('SampleList',[])
        self.LogVarList = kwargs.get('LogVarList',[])
        self.endian = kwargs.get('endian','<')
        self.hasLogVarUpdates = False
        self.hasSampleUpdates = False
    
    def ReceiveSampleBytes(self, data_bytes):
        barr = 0

        if isinstance(data_bytes, bytearray):
            barr = data_bytes
        else:
            barr = bytearray(data_bytes)

        #is the bytes sample data or only logvar?
        self.hasLogVarUpdates = False
        self.hasSampleUpdates = False
        self.lastSampleUpdate = []

        logvarlist = []

        graphvar = self.getType(barr) 
        while(graphvar is not None):
            if(graphvar.CODE == SampleData.CODE):
                self.SampleList.append(graphvar)
                self.lastSampleUpdate.append(LogVarMap(graphvar.logVar.devName,graphvar.logVar.logId))
                self.hasSampleUpdates = True
            else:
                logvarlist.append(graphvar)
                self.hasLogVarUpdates = True
            graphvar = self.getType(barr)
            
        if(self.hasLogVarUpdates):
            self.LogVarList.clear()
            self.LogVarList = logvarlist

        return self
    
    def getType(self, data_bytes):
        if(len(data_bytes)<4):
            return None
        try:
            var = None
            record = data_bytes[:4]
            (code,) = struct.unpack(self.endian + 'I',record)
            if(code == SampleData.CODE):
                var = SampleData(data_bytes)
            elif(code in DEV_CODE):
                var = LogVar(data_bytes)
            
            if var is None:
                return None
            
            #if var.valid == False:
            #    return None
            
            return var

        except struct.error as err:
            log.error(err)
            return None
        except:
            log.error("Failed to parse buffer")
            return None

    def getLogVar(self, id):
        ret = None
        for var in self.LogVarList:
            if var.logId == id:
                ret = var
                break
        return ret

    def nameSampleList(self):
        names = []
        for sample in self.SampleList:
            names.append(sample.logVar.getName())
        return names
    
    def PackSampleData(self):
        data_bytes = bytes([])
        for sample in self.SampleList:
            data_bytes += sample.to_bytes()
        return data_bytes

    def popSample(self):
        return self.SampleList.pop()

    def ReceiveSampleHex(self, arr):
        try:
            plotbarr=bytearray.fromhex(arr)
            self.ReceiveSampleBytes(plotbarr)
        except:
            log.error("failed to unpack hex samples")
    
    def __len__(self):
        return len(self.SampleList) + len(self.LogVarList) 

    def from_csv(filename):
        import csv
        try:
            with open(filename, mode='r') as csvfile:
                csvdict = {}
                reader = csv.DictReader(csvfile)
                logid = 0
                fieldnames = []
                #size = sum(1 for row in reader)
                #lreader = list(reader)
                #size = len(lreader)
                
                row1 = next(reader)  # gets the first line to determin data Format
                        
                for name in reader.fieldnames:
                    if name != "time":
                        dataType = None
                        try:
                            val = int(row1[name],0)
                            dataType = DATA_TYPE_ENUM.INTEGER32
                        except:
                            pass
                        if dataType is None:
                            try:
                                val = float(row1[name])
                                dataType = DATA_TYPE_ENUM.REAL32
                            except:
                                log.error("not real? %s" % row1[name])
                        if dataType is None:
                            log.error("Unable to determine dataType")
                            return

                        csvdict[name] = SampleData(logid=logid, name=name, size=None, dataType=dataType)
                        logid += 1
                        fieldnames.append(name)

                if 'time' in reader.fieldnames:
                    for name in fieldnames:
                            csvdict[name].addSampleFromStr(row1[name],timetag=row1['time'])
                    for row in reader:
                        for name in fieldnames:
                            csvdict[name].addSampleFromStr(row[name],timetag=row['time'])
                else:
                    for name in fieldnames:
                        if csvdict[name].addSampleFromStr(row1[name]) is False:
                            log.error("failed to set data line num %s" % str(reader.line_num))
                            return None
                    for row in reader:
                        for name in fieldnames:
                            if csvdict[name].addSampleFromStr(row[name]) is False:
                                log.error("failed to set data line num %s" % str(reader.line_num))
                                return
                if len(csvdict) > 0:
                    return SampleDataCollection(SampleList=csvdict.values())
        except csv.Error as err:
            log.error("Failed to parse csv (%s)" % err)
        except Exception as err:
            log.error("Failed to parse csv (%s)" % err)


        return None


if __name__ == '__main__':
    import os
    log.setLevel(10)
    # create formatter and add it to the handlers
    formatter = logging.Formatter('%(asctime)-15s %(levelname)-8s %(lineno)-3s:%(module)-15s  %(message)s')

    # reate console handler for logger.
    file='C:/Users/andreas/Documents/record/CollectAll_2022-01-24_12_49_24.csv'
    SampleDataCollection.from_csv(file)

    ch = logging.StreamHandler()
    ch.setLevel(level=logging.DEBUG)
    ch.setFormatter(formatter)
    log.addHandler(ch)
    f = open("./TST", "r")
    TST=str(f.read())
    f.close()
    #sample = SampleData()
    rlsd = SampleDataCollection()
    rlsd.ReceiveSampleHex(TST)
    data_bytes = bytes([])
    data_bytes = rlsd.PackSampleData()

    f = open("TST_conv", "w")
    f.write(data_bytes.hex())
    f.close()

    rlsd2 = SampleDataCollection().ReceiveSampleBytes(data_bytes)

