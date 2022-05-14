

#include <stdbool.h>
#include <stdint.h>

#ifndef FTDI_LISTENER
#define FTDI_LISTENER

#ifdef __cplusplus
 extern "C" {
#endif

typedef int32_t (* ftdi_Callback_t )( void * pHandle, uint8_t *Buffer, uint32_t Size );

int start_listener(bool ena);

bool isRunning(void);

void start_diagnostics_print(bool ena);

bool isDiagEna(void);

void print_diagnostics(void);

void setCallBack(ftdi_Callback_t pCallback);

#ifdef __cplusplus
 }
#endif

#endif /* ftdi_listener.h */