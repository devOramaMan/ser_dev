

#include "ftdi_config.h"

#ifndef FTDI_CONNECT
#define FTDI_CONNECT

#include <stdbool.h>
#include <stdint.h>

// FTDI support functions

int connect(int dev , int baudrate);
int close_device(void);

FT_DEVICE_LIST_INFO_NODE * get_device_info(uint32_t * numDevs);

bool get_device_sn(const char * sn, int * device);
bool get_device_port(uint32_t port, int * device);
int reset_device(int baud_rate);
int set_baud_rate(int baud_rate);

#endif /* ftdi_connect.h */