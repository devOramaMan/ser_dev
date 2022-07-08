/*-----------------------------------------------------------------------------
 * Name:    ftri_listener.h
 * Purpose: handle ftdi read worker
 *-----------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/


#ifndef FTDI_LISTENER
#define FTDI_LISTENER

#include "protocol_common.h"
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#ifdef __cplusplus
 extern "C" {
#endif

typedef struct Ftdi_Listener
{
  Protocol_Diag_t Diag;
  bool Stop;
  bool Diag_Ena;
  bool Running;
  void* hEvent;
  void *event;
  void* pHandler;
  void* callback;
  void * pDev;
  pthread_t threadId;
} Ftdi_Listener_t;


int32_t start_listener(Ftdi_Listener_t * listener, void * dev);

void stop_listener(Ftdi_Listener_t *listener);

bool isRunning(Ftdi_Listener_t * listener);

#ifdef __cplusplus
 }
#endif

#endif /* ftdi_listener.h */