from time import sleep
from turtle import delay
from numpy import byte
import ctypes
import queue
import zmq
import struct
import logging
from PySide2.QtCore import  QThread, QObject, Signal, QMutex



log = logging.getLogger('SocketInterface')
import common.logging.LogHandler as LogHandler
LogHandler.addLogger(log)

TOPICS_LIST = {10:"PortList",15:"ReadRegister",16:"WriteRegister"}

SOCKET_MENU_ACK = "ACK".encode('ascii')
SOCKET_MENU_NACK = "NACK".encode('ascii')

SOCKET_MENU_CKECK = "check".encode('ascii')
SOCKET_MENU_CONNECT = "connect".encode('ascii')
SOCKET_MENU_PORT = "port".encode('ascii')
SOCKET_MENU_SN = "sn".encode('ascii')
SOCKET_MENU_DEV = "dev".encode('ascii')
SOCKET_MENU_BAUD = "baud".encode('ascii')
SOCKET_MENU_CLOSE = "close".encode('ascii')
SOCKET_MENU_DIAG =  "diag".encode('ascii')

class TransAction(QObject):
    def __init__(self, key, cnt = 1) -> None:
        QObject.__init__(self)
        self.key = key
        self.err_code = None
        self.data = bytes()
        self.cnt = cnt

    def key(self):
        return self.key

    def setData(self, data, err_code):
        self.data = data
        self.err_code = err_code

    def isDone(self):
        if self.cnt == 1:
            return True
        if self.cnt == 0:
            return False

        self.cnt -= 1
        return False



class SocketWorker(QThread):
    def __init__(self):
        QThread.__init__(self)
        self.stop = True
        self.closed = True
        self.queue = queue.Queue

    def Connect(self, sstr):
        self.connectstr = sstr
    
    def Stop(self):
        self.stop = True

class ReceiveWorker(SocketWorker):
    #WorkDone = Signal()
    fmt = "<4B2L"
    def __init__(self):
        SocketWorker.__init__(self)
        self.topics = []
        self.queue = dict()
        self.lastSocket = None

    def Subscribe(self, topiclist):
        self.topics = topiclist
    ##
    #@brief Creates a new Hello object.
    #@param key - Trans Action ID
    #@param item - TransAction class (derived)
    def addQueue(self, key, item):
        self.queue[key] = item

    def run(self):
        self.stop = False
        log.debug("Receive socket started")

        try:
            context = zmq.Context()
        except Exception as err:
            log.error("failed to init socket interface")
            return

        try:
            socket = context.socket(zmq.SUB)
        except Exception as err:
            log.error("failed to init socket interface")
            return

        try:
            socket.connect(self.connectstr)
        except Exception as err:
            log.error("failed to init socket interface")
            return

        try:
            #for topic in self.topics:
            #    socket.setsockopt(zmq.SUBSCRIBE, topic)
            socket.setsockopt(zmq.SUBSCRIBE, b'')
            log.debug("subscribe all")
        except:
            log.error("failed to add subscription")
            return

        self.closed = False
        self.lastSocket = socket
        
        i = 0
        while not self.stop:
            try:
                flag = socket.poll(timeout=1000)
                if(flag):
                    #log.debug("receiving")
                    raw = socket.recv()
                    #print(str(raw))
                    (code, range, err_code, type, id, size) = \
                    struct.unpack(self.fmt, raw[:12])
                    
                    #print("msgCnt %d, code %d, size %d, err_code %d, id %d, Data: %s" \
                    #  % (i,code,size,err_code,id, str(raw[12:])))
                    
                    if id in self.queue:
                        #TransAction object
                        item = self.queue[id]
                        item.setData(raw[12:], err_code)
                        if item.isDone() is True:
                            del self.queue[id]
                    else:
                        log.error("elemet not i queue (id: %d)" % id)
                    i += 1


                
            except Exception as error:
                log.error(error)
        
        socket.close()
        self.closed = True

        log.debug("Receiver thread done")

        #self.WorkDone.emit()
    
    def close(self):
        self.Stop()
        i = 0
        while(self.closed is False and i < 10):
            sleep(0.1)
            i += 1

        if self.closed is False:
            try:
                self.terminate()
                self.lastSocket.close()
            except:
                log.warning("Tried to close last socket and failed")
            self.closed = True
        else:
            self.exit()
        


class SendWorker(SocketWorker):
    #WorkDone = Signal()
    def __init__(self, socket):
        SocketWorker.__init__(self, socket)

    def send(self, msg):
        self.running = True
        self.socket.send(msg)
        self.running = False
        
        #self.WorkDone.emit()

class SocketInterface(QObject):
    SocketSignal = Signal(classmethod)
    def __init__(self, **kwargs):
        self.TransactionId = 0
        self.lock = QMutex()
        self.connected = False
        try:
            #TODO multiple context for each? or one for all sockets
            self.context = zmq.Context()
        except Exception as err:
            log.error("failed to init socket interface")


        #recsocket = self.context.socket(zmq.SUB)
        
        

        self.endian = kwargs.get('endian','<')
        if self.endian == '<':
            self.lendian = 'little'
        else:
            self.lendian = 'big'

        try:
            self.port = int(kwargs.get("lport","5555"))
        except:
            log.error("failed to parse lport")
            self.port = 5555

        try:
            self.menuPort = int(kwargs.get("mport",5565))
        except:
            log.error("failed to parse menu port")
            self.menuPort = 5565

        try:
            self.sendPort = int(kwargs.get("sendport", 5575))
        except:
            log.error("failed to parse send port")
            self.sendPort = 5575
        
        try:
            self.recPort = int(kwargs.get("recport", 5585))
        except:
            log.error("failed to parse recieve port")
            self.recPort = 5585

        self.host = kwargs.get("host","tcp://127.0.0.1")

        self.comPort = kwargs.get("port", None)
        
        self.ftdiSn = kwargs.get("ftdi", None)
        self.dev = kwargs.get('dev', None)
        self.baud = kwargs.get("baud", None)

        if self.baud is not None:
            try:
                self.baud = int(self.baud)
            except:
                log.error("failed to set baud")
                self.baud = None

        # self.sendThread = QThread()
        # self.sendworker = SendWorker(sendsocket)
        # self.sendworker.moveToThread( self.sendThread )
        # self.sendThread.finished.connect( self.sendThread.deleteLater )
        

    def getTransactionKey(self):
        ret = 0
        self.lock.lock()
        ret = self.TransactionId
        self.TransactionId +=1
        self.lock.unlock()
        return ret

    ##
    # @brief write register value
    #
    def sendData(self, callback, data, key):
        self.receiveworker.addQueue(key, callback)
        return self.sendSocket.send(data)


    ##
    # @brief Menu socket request
    #
    def menuReq(self,data, timeout=3.0 ):
        data_buffer = bytes()
        self.menuSocket.send(data)
        flag = self.menuSocket.poll(timeout=3000)
        if(flag):
            data_buffer = self.menuSocket.recv()
        else:
            log.error("Connection timeout (%s)" % data.decode('ascii'))
            self.Close()
        return data_buffer, flag

    ##
    # @brief Open  client
    #
    def Connect(self):
        self.connected = False
        try:
            connectionstr = self.host + ":" + str(self.menuPort)
            log.debug("Menu Connection %s" % connectionstr)
            self.menuSocket = self.context.socket(zmq.REQ)
            self.menuSocket.connect(connectionstr)
            #
            connectionstr = self.host + ":" + str(self.sendPort)
            log.debug("Send Connection %s" % connectionstr)
            self.sendSocket = self.context.socket(zmq.PUB)
            self.sendSocket.bind(connectionstr)

            connectionstr = self.host + ":" + str(self.recPort)
            log.debug("Receive Connection %s" % connectionstr)
            self.receiveworker = ReceiveWorker()
            self.receiveworker.finished.connect( self.receiveworker.deleteLater )
            self.receiveworker.Connect(connectionstr)
            self.receiveworker.Subscribe(TOPICS_LIST.keys())
            self.receiveworker.start()

        except Exception as err:
            log.error("failed to create socket connection %s" % err)
            self.Close()
            return False

        try:
            #Check socket menu connection
            data_buffer, flag = self.menuReq(SOCKET_MENU_CKECK)
            if(flag == 0):
                return False
            if(data_buffer[:4] == SOCKET_MENU_NACK):
                log.error("Failed to check line")
                self.Close()
                return False

            #Configure port
            if(self.comPort is not None):
                data_buffer, flag = self.menuReq(SOCKET_MENU_PORT + str(" " + self.comPort).encode("ascii"))
                if(flag == 0):
                    return False
                if(data_buffer[:4] == SOCKET_MENU_NACK):
                    log.error("Failed to configure port %s " % data_buffer[4:].decode("ascii"))
                    self.Close()
                    return False
                log.info("Port Configured %s " % data_buffer[3:].decode("ascii"))

            if(self.ftdiSn is not None):
                data_buffer, flag = self.menuReq(SOCKET_MENU_SN + str(" " + self.ftdiSn).encode("ascii"))
                if(flag == 0):
                    return False
                if(data_buffer[:4] == SOCKET_MENU_NACK):
                    log.error("Failed to configure SN %s " % data_buffer[4:].decode("ascii"))
                    self.Close()
                    return False
                log.info("SN Configured %s " % data_buffer[3:].decode("ascii"))
            if(self.dev is not None):
                data_buffer, flag = self.menuReq(SOCKET_MENU_DEV + str(" " + self.dev).encode("ascii"))
                if(flag == 0):
                    return False
                if(data_buffer[:4] == SOCKET_MENU_NACK):
                    log.error("Failed to configure dev %s " % data_buffer[4:].decode("ascii"))
                    self.Close()
                    return False
                log.info("Dev Configured %s " % data_buffer[3:].decode("ascii"))

            if(self.baud is not None):
                data_buffer, flag = self.menuReq(SOCKET_MENU_BAUD + str(" " + str(self.baud) + "\0").encode("ascii"))
                if(flag == 0):
                    return False
                if(data_buffer[:4] == SOCKET_MENU_NACK):
                    log.error("Failed to configure %s " % data_buffer[4:].decode("ascii"))
                    self.Close()
                    return False
                log.info("Configured %s " % data_buffer[3:].decode("ascii"))

            data_buffer, flag = self.menuReq(SOCKET_MENU_CONNECT)
            if(flag == 0):
                return False
            if(data_buffer[:4] == SOCKET_MENU_NACK):
                    log.error("Failed to Connect %s " % data_buffer[4:].decode("ascii"))
                    self.Close()
                    return False
                    
        except Exception as err:
            self.Close()
            log.error("failed to configure interface %s" % str(err))
            return False
        #socket not ready. driver some time to connect to device... sleep meen while
        sleep(0.5)
        self.connected = True
        return True

    ##
    # @brief Close  client
    #
    def Close(self):
        try:
            if not self.menuSocket.closed:
                data_buffer, flag = self.menuReq(SOCKET_MENU_CLOSE)
                if(flag):
                    if(data_buffer[:4] == SOCKET_MENU_NACK):
                        log.error("failed to close port %s " % data_buffer[4:].decode("ascii"))
                self.menuSocket.close()
                log.debug("Menu socket closed")
        except:
            log.error("failed to close menu socket")

        
        try:
            if not self.sendSocket.closed:
                self.sendSocket.send("close".encode('ascii'))
                
                self.sendSocket.close()
                log.debug("Send socket  closed")
        except:
            log.error("failed to close send socket")

        try:
            self.receiveworker.close()
            while(self.receiveworker.closed is False):
                sleep(0.5)
            log.debug("Receive socket closed")
        except Exception as err:
            log.error("failed to close receive socket (err: %s)" % err)
        
        #Todo set connected false even if we faled?
        self.connected = False
        
        
    ##
    # @brief is connected
    #
    def isConnect(self):
        return self.connected

    ##
    # @brief check interface of thread safe handling
    #
    def isThreadSafe(self):
        return True

    ##
    # @brief get interface statistics
    # @return string interface statistics
    def getStats(self):
        if(self.isConnect() is False):
            log.info("device not connected")
            return

        data_buffer, flag = self.menuReq(SOCKET_MENU_DIAG)

        if(flag == 0):
            log.error("Communication issues - Failed get interface statistics")
            return ""

        if(data_buffer[:4] == SOCKET_MENU_NACK):
            log.error("If Nack - Failed get interface statistics")
            return ""
        else:
            return data_buffer.decode("ascii")

   



if __name__ == "__main__":


    formatter = logging.Formatter('%(asctime)-15s %(threadName)-8s %(levelname)-8s %(lineno)-3s:%(module)-15s  %(message)s')
    consolelog = logging.StreamHandler()
    consolelog.setLevel(level=logging.DEBUG)
    consolelog.setFormatter(formatter)
    log.addHandler(consolelog)
    log.setLevel(level=logging.DEBUG)

    #socket_serial = SocketInterface(port="COM28", ftdi="FT6S9SBG", baud="1000000")
    socket_serial = SocketInterface(ftdi="FT6S9SBG", baud="1000000")
    #socket_serial = SocketInterface(dev="0", baud="1000000")

    socket_serial.Connect()

    sleep(2.0)
    FCP_SINGLE_TOPIC = 10
    MSG_SIZE = 6
    data = bytes([FCP_SINGLE_TOPIC, MSG_SIZE, 99, 11,22,33])
    rr = socket_serial.sendData(data)
    print(str(rr))

    socket_serial.Close()

    # create formatter and add it to the handlers
    
    
    
