
#include "ftdi_atomic.h"

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

uint64_t lock_count = 0;
pthread_mutex_t ftdi_read_mutex;
pthread_mutex_t ftdi_write_mutex;

inline void set_lock(pthread_mutex_t *lock)
{
  lock_count++;
  pthread_mutex_lock(lock);
}

inline int32_t try_lock(pthread_mutex_t *lock)
{
  int ret;
  if (!(ret = pthread_mutex_trylock(lock)))
    lock_count++;
  return ret;
}

inline void un_lock(pthread_mutex_t *lock)
{
  lock_count--;
  pthread_mutex_unlock(lock);
}

inline FT_STATUS FT_Open_Atomic(
    int deviceNumber,
    FT_HANDLE *pHandle)
{
  FT_STATUS ftStatus;
  set_lock(&ftdi_read_mutex);
  set_lock(&ftdi_write_mutex);
  ftStatus = FT_Open(deviceNumber, pHandle);
  un_lock(&ftdi_read_mutex);
  un_lock(&ftdi_write_mutex);
  return ftStatus;
}

inline FT_STATUS FT_Close_Atomic(
    FT_HANDLE ftHandle)
{
  FT_STATUS ftStatus;
  set_lock(&ftdi_read_mutex);
  set_lock(&ftdi_write_mutex);
  ftStatus = FT_Close(ftHandle);
  pCurrentDev = NULL;
  un_lock(&ftdi_read_mutex);
  un_lock(&ftdi_write_mutex);
  return ftStatus;
}

inline FT_STATUS FT_ResetPort_Atomic(
    FT_HANDLE ftHandle)
{
  FT_STATUS ftStatus;
  set_lock(&ftdi_read_mutex);
  set_lock(&ftdi_write_mutex);
  ftStatus = FT_ResetPort(ftHandle);
  pCurrentDev = NULL;
  un_lock(&ftdi_read_mutex);
  un_lock(&ftdi_write_mutex);
  return ftStatus;
}

inline FT_STATUS FT_SetBaudRate_Atomic(
    FT_HANDLE ftHandle,
    uint32_t BaudRate)
{
  FT_STATUS ftStatus;
  set_lock(&ftdi_read_mutex);
  set_lock(&ftdi_write_mutex);
  ftStatus = FT_SetBaudRate(ftHandle, BaudRate);
  un_lock(&ftdi_read_mutex);
  un_lock(&ftdi_write_mutex);
  return ftStatus;
}

inline FT_STATUS FT_Read_Atomic(
    FT_HANDLE ftHandle,
    uint8_t *lpBuffer,
    uint32_t dwBytesToRead,
    uint32_t *lpBytesReturned)
{
  FT_STATUS ftStatus;
  set_lock(&ftdi_read_mutex);
  ftStatus = FT_Read(ftHandle, lpBuffer, dwBytesToRead, lpBytesReturned);
  un_lock(&ftdi_read_mutex);
  return ftStatus;
}

inline FT_STATUS FT_Write_Atomic(
    FT_HANDLE ftHandle,
    uint8_t *lpBuffer,
    uint32_t dwBytesToWrite,
    uint32_t *lpdwBytesWritten)
{
  FT_STATUS ftStatus;
  set_lock(&ftdi_write_mutex);
  ftStatus = FT_Write(ftHandle, lpBuffer, dwBytesToWrite, lpdwBytesWritten);
  un_lock(&ftdi_write_mutex);
  return ftStatus;
}

int32_t Read_Atomic(
    uint8_t *lpBuffer,
    uint32_t dwBytesToRead,
    uint32_t *lpBytesReturned)
{
  FT_STATUS ftStatus;
  set_lock(&ftdi_read_mutex);
  if (pCurrentDev)
  {
    ftStatus = FT_Read_Atomic(pCurrentDev, lpBuffer, dwBytesToRead, lpBytesReturned );
  }
  un_lock(&ftdi_read_mutex);
  return (int32_t) ftStatus;
}

int32_t Write_Atomic(
    uint8_t *lpBuffer,
    uint32_t dwBytesToRead,
    uint32_t *lpBytesReturned)
{
  FT_STATUS ftStatus;
  set_lock(&ftdi_write_mutex);
  if (pCurrentDev)
  {
    ftStatus = FT_Write_Atomic( pCurrentDev, lpBuffer, dwBytesToRead, lpBytesReturned );
  }
  un_lock(&ftdi_write_mutex);
  return (int32_t) ftStatus;
}
