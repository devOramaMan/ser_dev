#include "config.h"
#include "ftd2xx.h"
#include <pthread.h>

#ifndef FTDI_CONFIG
#define FTDI_CONFIG



typedef struct ftdi_config
{
    int devid;
    int baud;
    FT_HANDLE ftHandle;
    FT_DEVICE_LIST_INFO_NODE * pDevInfo;
}ftdi_config_t;

extern ftdi_config_t * pCurrentDev;

extern pthread_mutex_t ftdi_read_mutex; 

#endif /* ftdi_config.h */
