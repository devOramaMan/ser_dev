#include "config.h"
#include "ftd2xx.h"
#include <pthread.h>


#ifndef FTDI_CONFIG
#define FTDI_CONFIG

#ifdef _WIN32
typedef struct _EVENT_HANDLE
{
    pthread_cond_t  eCondVar;
    pthread_mutex_t eMutex;
    int             iVar;
} EVENT_HANDLE;
#endif


typedef struct ftdi_config
{
    int devid;
    int baud;
    FT_HANDLE ftHandle;
    FT_DEVICE_LIST_INFO_NODE * pDevInfo;
}ftdi_config_t;

extern ftdi_config_t * pCurrentDev;

extern EVENT_HANDLE * eh;

#endif /* ftdi_config.h */
