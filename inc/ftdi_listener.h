

#include <stdbool.h>
#include <stdint.h>

#ifndef FTDI_LISTENER
#define FTDI_LISTENER

#ifdef __cplusplus
 extern "C" {
#endif


int start_listener(bool ena);

bool isRunning(void);

void start_diagnostics_print(bool ena);

bool isDiagEna(void);

void print_diagnostics(void);


#ifdef __cplusplus
 }
#endif

#endif /* ftdi_listener.h */