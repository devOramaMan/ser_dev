
import logging

Handlers = []
Loggers = []
Loglevel = logging.INFO
PlainText = None

KmcRecord = None


def addLogger(logger):    
    global Loggers
    global Handlers
    if logger in Loggers:
        return
    logger.setLevel(Loglevel)
    Loggers.append(logger)
    if(len(Handlers) > 0):        
        for handle in Handlers:
            logger.addHandler(handle)

def addHandler(handler):
    global Loggers
    global Handlers
    if handler in Handlers:
        return
    Handlers.append(handler)
    if(len(Loggers) > 0):        
        for log in Loggers:
            log.addHandler(handler)

def setLevel(level):
    global Loggers
    global Handlers
    global Loglevel
    Loglevel = level
    for logger in Loggers:
        logger.setLevel(level)
    
    for handler in Handlers:
        handler.setLevel(level)

def getStream():
    global Loggers
    if(len(Loggers) > 0):  
        return Loggers[0].steam

def LogPlainText(txt):
    global PlainText
    if(PlainText is not None):
        PlainText.externPlainTxt(txt)

def LogStrText(txt):
    global PlainText
    if(PlainText is not None):
        PlainText.externStrTxt(txt)

def kmcRecAddMaintSample(collection):
    global KmcRecord
    if(KmcRecord is not None):
        KmcRecord.addMaintSamples(collection)


def kmcRecAddHelthSample(collection):
    global KmcRecord
    if(KmcRecord is not None):
        KmcRecord.addHelthSamples(collection)

