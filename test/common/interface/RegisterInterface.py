
import logging, os, inspect
import common.logging.LogHandler as LogHandler

log = logging.getLogger('DeviceInterface')
LogHandler.addLogger(log)
__path__ = [str(os.path.dirname(os.path.abspath(__file__)))]
class RegisterInterface(object):
    ExternalInterface = True
    def __init__(self):
        self.ExternalInterface = True

    ##
    # @brief write register value
    #
    def writeRegisterIf(self, nodeId, startReg, dataType, value):
        log.debug("writeRegisterIf not implemented")

    ##
    # @brief read register value
    #
    def readRegisterIf(self, nodeId, startReg, dataType, countReg):
        log.debug("readRegisterIf not implemented")

    ##
    # @brief write file
    #
    def writeFileIf(self, nodeId, filenum, data):
        log.debug("writeFileIf not implemented")

    ##
    # @brief read file
    #
    def readFileIf(self, nodeId, filenum):
        log.debug("readFileIf not implemented")

    ##
    # @brief subscribe data
    # @param kwargs tuplip list of parameters to setup subscription
    # @return subscription id
    def subscribe(self, **kwargs):
        log.debug("subscribe not implemented")

    ##
    # @brief subscribe data
    # @param subscription id to remove
    # @return None
    def unSubscribe(self, id):
        log.debug("unSubscribe not implemented")

    ##
    # @brief Open  client
    #
    def Connect(self):
        log.debug("connect not implemented")

    ##
    # @brief Close  client
    #
    def Close(self):
        log.debug("Close not implemented")
        
    ##
    # @brief is connected
    #
    def isConnect(self):
        log.debug("isconnect not implemented")
        return True

    ##
    # @brief check interface of thread safe handling
    #
    def isThreadSafe(self):
        return False

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
