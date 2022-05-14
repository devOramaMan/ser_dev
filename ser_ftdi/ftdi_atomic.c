
#include "ftdi_atomic.h"

uint64_t lock_count = 0;

pthread_mutex_t ftdi_mutex;

void set_lock(void)
{
    lock_count++;
    pthread_mutex_lock(&ftdi_mutex);
}

int32_t try_lock(void)
{
    int ret;
    if ( !(ret = pthread_mutex_trylock(&ftdi_mutex)) )
        lock_count++;
    return ret;
}

void un_lock(void)
{
    lock_count--;
    pthread_mutex_unlock(&ftdi_mutex);
}

inline FT_STATUS FT_Open_atomic(
    int deviceNumber,
    FT_HANDLE *pHandle)
{
    FT_STATUS ftStatus;
    set_lock();
    ftStatus = FT_Open(deviceNumber, pHandle);
    un_lock();
    return ftStatus;
}

inline FT_STATUS FT_Close_atomic(
    FT_HANDLE ftHandle)
{
    FT_STATUS ftStatus;
    set_lock();
    ftStatus = FT_Close(ftHandle);
    pCurrentDev = NULL;
    un_lock();
    return ftStatus;
}

inline FT_STATUS FT_ResetPort_atomic(
    FT_HANDLE ftHandle)
{
    FT_STATUS ftStatus;
    set_lock();
    ftStatus = FT_ResetPort(ftHandle);
    pCurrentDev = NULL;
    un_lock();
    return ftStatus;
}

inline FT_STATUS FT_SetBaudRate_atomic(
    FT_HANDLE ftHandle,
    uint32_t BaudRate)
{
    FT_STATUS ftStatus;
    set_lock();
    ftStatus = FT_SetBaudRate(ftHandle, BaudRate);
    un_lock();
    return ftStatus;
}

inline FT_STATUS FT_Read_atomic(
    FT_HANDLE ftHandle,
    uint8_t * lpBuffer,
    uint32_t dwBytesToRead,
    uint32_t *lpBytesReturned)
{
    FT_STATUS ftStatus;
    set_lock();
    ftStatus = FT_Read(ftHandle, lpBuffer, dwBytesToRead, lpBytesReturned);
    un_lock();
    return ftStatus;
}
