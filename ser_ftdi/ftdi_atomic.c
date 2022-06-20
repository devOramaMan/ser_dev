
#include "ftdi_atomic.h"
//#include "WinTypes.h"

FT_STATUS FT_Read_Atomic(
    FT_HANDLE ftHandle,
    uint8_t *lpBuffer,
    uint32_t dwBytesToRead,
    uint32_t *lpBytesReturned);

FT_STATUS FT_Write_Atomic(
    FT_HANDLE ftHandle,
    uint8_t *lpBuffer,
    uint32_t dwBytesToWrite,
    uint32_t *lpdwBytesWritten);

void set_lock(pthread_mutex_t *lock);
int32_t try_lock(pthread_mutex_t *lock);
void un_lock(pthread_mutex_t *lock);

typedef struct Atomic_Ftdi_Handler
{
  uint64_t lock_count;
  EVENT_HANDLE ftdi_read_mutex;
  pthread_mutex_t ftdi_write_mutex;
}Atomic_Ftdi_Handler_t; 

Atomic_Ftdi_Handler_t atomic_ftdi_handler =
{
  .lock_count = 0,
  .ftdi_read_mutex.eCondVar = PTHREAD_COND_INITIALIZER,
  .ftdi_read_mutex.eMutex = PTHREAD_MUTEX_INITIALIZER,
  .ftdi_read_mutex.iVar = 0,
  .ftdi_write_mutex = PTHREAD_MUTEX_INITIALIZER
};

EVENT_HANDLE * eh = &(atomic_ftdi_handler.ftdi_read_mutex);

inline void set_lock(pthread_mutex_t *lock)
{
  atomic_ftdi_handler.lock_count++;
  pthread_mutex_lock(lock);
}

inline int32_t try_lock(pthread_mutex_t *lock)
{
  int ret;
  if (!(ret = pthread_mutex_trylock(lock)))
    atomic_ftdi_handler.lock_count++;
  return ret;
}

inline void un_lock(pthread_mutex_t *lock)
{
  atomic_ftdi_handler.lock_count--;
  pthread_mutex_unlock(lock);
}

inline FT_STATUS FT_Open_Atomic(
    int deviceNumber,
    FT_HANDLE *pHandle)
{
  FT_STATUS ftStatus;
  set_lock(&atomic_ftdi_handler.ftdi_read_mutex.eMutex);
  set_lock(&atomic_ftdi_handler.ftdi_write_mutex);
  ftStatus = FT_Open(deviceNumber, pHandle);
  un_lock(&atomic_ftdi_handler.ftdi_read_mutex.eMutex);
  un_lock(&atomic_ftdi_handler.ftdi_write_mutex);
  return ftStatus;
}


inline FT_STATUS FT_Close_Atomic(
    FT_HANDLE ftHandle)
{
  FT_STATUS ftStatus;
  set_lock(&atomic_ftdi_handler.ftdi_read_mutex.eMutex);
  set_lock(&atomic_ftdi_handler.ftdi_write_mutex);
  ftStatus = FT_Close(ftHandle);
  pCurrentDev = NULL;
  un_lock(&atomic_ftdi_handler.ftdi_read_mutex.eMutex);
  un_lock(&atomic_ftdi_handler.ftdi_write_mutex);
  return ftStatus;
}

inline FT_STATUS FT_GetComPortNumber_Atomic(
    int32_t deviceNumber,
    FT_HANDLE pHandle,
    int32_t * ComPort)
{
  FT_STATUS ftStatus;
  set_lock(&atomic_ftdi_handler.ftdi_read_mutex.eMutex);
  set_lock(&atomic_ftdi_handler.ftdi_write_mutex);
  if (pCurrentDev && pCurrentDev->devid == deviceNumber)
  {
    ftStatus = FT_GetComPortNumber(pHandle,(LPLONG)ComPort);
  }
  else
  {
    ftStatus = FT_Open(deviceNumber, &pHandle);
    ftStatus |= FT_GetComPortNumber(pHandle,(LPLONG)ComPort);
    FT_Close(pHandle);
  }
  un_lock(&atomic_ftdi_handler.ftdi_read_mutex.eMutex);
  un_lock(&atomic_ftdi_handler.ftdi_write_mutex);
  return ftStatus;
}

inline FT_STATUS FT_ResetPort_Atomic(
    FT_HANDLE ftHandle)
{
  FT_STATUS ftStatus;
  set_lock(&atomic_ftdi_handler.ftdi_read_mutex.eMutex);
  set_lock(&atomic_ftdi_handler.ftdi_write_mutex);
  ftStatus = FT_ResetPort(ftHandle);
  pCurrentDev = NULL;
  un_lock(&atomic_ftdi_handler.ftdi_read_mutex.eMutex);
  un_lock(&atomic_ftdi_handler.ftdi_write_mutex);
  return ftStatus;
}

inline FT_STATUS FT_SetBaudRate_Atomic(
    FT_HANDLE ftHandle,
    uint32_t BaudRate)
{
  FT_STATUS ftStatus;
  set_lock(&atomic_ftdi_handler.ftdi_read_mutex.eMutex);
  set_lock(&atomic_ftdi_handler.ftdi_write_mutex);
  ftStatus = FT_SetBaudRate(ftHandle, BaudRate);
  un_lock(&atomic_ftdi_handler.ftdi_read_mutex.eMutex);
  un_lock(&atomic_ftdi_handler.ftdi_write_mutex);
  return ftStatus;
}

inline FT_STATUS FT_Read_Atomic(
    FT_HANDLE ftHandle,
    uint8_t *lpBuffer,
    uint32_t dwBytesToRead,
    uint32_t *lpBytesReturned)
{
  FT_STATUS ftStatus;
  set_lock(&atomic_ftdi_handler.ftdi_read_mutex.eMutex);
  ftStatus = FT_Read(ftHandle, lpBuffer, dwBytesToRead, lpBytesReturned);
  un_lock(&atomic_ftdi_handler.ftdi_read_mutex.eMutex);
  return ftStatus;
}

inline FT_STATUS FT_Write_Atomic(
    FT_HANDLE ftHandle,
    uint8_t *lpBuffer,
    uint32_t dwBytesToWrite,
    uint32_t *lpdwBytesWritten)
{
  FT_STATUS ftStatus;
  set_lock(&atomic_ftdi_handler.ftdi_write_mutex);
  ftStatus = FT_Write(ftHandle, lpBuffer, dwBytesToWrite, lpdwBytesWritten);
  un_lock(&atomic_ftdi_handler.ftdi_write_mutex);
  return ftStatus;
}

int32_t Read_Atomic(
    uint8_t *lpBuffer,
    uint32_t dwBytesToRead,
    uint32_t *lpBytesReturned)
{
  FT_STATUS ftStatus = 1;
  set_lock(&atomic_ftdi_handler.ftdi_read_mutex.eMutex);
  if (pCurrentDev)
  {
    ftStatus = FT_Read(pCurrentDev->ftHandle, lpBuffer, dwBytesToRead, lpBytesReturned );
  }
  un_lock(&atomic_ftdi_handler.ftdi_read_mutex.eMutex);
  return (int32_t) ftStatus;
}

int32_t Write_Atomic(
    uint8_t *lpBuffer,
    uint32_t dwBytesToWrite,
    uint32_t *lpdwBytesWritten)
{
  FT_STATUS ftStatus = 1;
  set_lock(&atomic_ftdi_handler.ftdi_write_mutex);
  if (pCurrentDev)
  {
    ftStatus = FT_Write( pCurrentDev->ftHandle, lpBuffer, dwBytesToWrite, lpdwBytesWritten );
  }
  un_lock(&atomic_ftdi_handler.ftdi_write_mutex);
  return (int32_t) ftStatus;
}
