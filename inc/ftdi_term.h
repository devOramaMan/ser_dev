
#ifndef FTDI_TERM
#define FTDI_TERM

#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
 extern "C" {
#endif

// Lists FTDI commands.
void ftdi_menu(void);
bool connect_device(int * local_baud_rate);


#ifdef __cplusplus
 }
#endif

#endif /* ftdi_term.h */
