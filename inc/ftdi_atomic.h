
#include "ftdi_config.h"

#ifndef FTDI_ATOMIC
#define FTDI_ATOMIC

FT_STATUS FT_Open_atomic(
    int deviceNumber,
    FT_HANDLE *pHandle);

FT_STATUS FT_Close_atomic(
    FT_HANDLE ftHandle);

FT_STATUS FT_ResetPort_atomic(
    FT_HANDLE ftHandle);

FT_STATUS FT_SetBaudRate_atomic(
    FT_HANDLE ftHandle,
    uint32_t BaudRate);

FT_STATUS FT_Read_atomic(
    FT_HANDLE ftHandle,
    uint8_t *lpBuffer,
    uint32_t dwBytesToRead,
    uint32_t *lpBytesReturned);



#endif /* ftdi_atomic.h */