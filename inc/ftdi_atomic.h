#include "config.h"
#include "ftdi_config.h"

#ifndef FTDI_ATOMIC
#define FTDI_ATOMIC

FT_STATUS FT_Open_Atomic(
    int deviceNumber,
    FT_HANDLE *pHandle);

FT_STATUS FT_Close_Atomic(
    FT_HANDLE ftHandle);

FT_STATUS FT_ResetPort_Atomic(
    FT_HANDLE ftHandle);

FT_STATUS FT_SetBaudRate_Atomic(
    FT_HANDLE ftHandle,
    uint32_t BaudRate);

int32_t Read_Atomic(
    uint8_t *lpBuffer,
    uint32_t dwBytesToRead,
    uint32_t *lpBytesReturned);

int32_t Write_Atomic(
    uint8_t *lpBuffer,
    uint32_t dwBytesToRead,
    uint32_t *lpBytesReturned);



#endif /* ftdi_atomic.h */