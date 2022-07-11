
#include "config.h"
#include "ftdi_listener.h"
#include "ftdi_atomic.h"
#include <pthread.h>
#include <stdio.h>
#include "ftdi_config.h"
#include <sys/time.h>
#include "diagnostics_util.h"
#include "protocol_common.h"
#include "util_common.h"

#ifdef _WIN32
#include "windows.h"
#endif

#define DIAG_OUT_TIME 2000
#define LISTENER_MAX 10


void *threadlistener(void *arg);

int32_t start_listener(Ftdi_Listener_t * listener, void * dev)
{
  int ret = 0;
  if (isRunning(listener))
  {
    DiagMsg(DIAG_INFO, "Ftdi Listener already Running\n");
    ret = 1;
  }
  else
  {
    listener->pDev = dev;
    listener->Stop = false;
    listener->event = eh;
    ret = pthread_create(&listener->threadId, NULL, &threadlistener, (void *)listener);
    if (!ret)
    {
      listener->Running = true;
    }
    else
    {
      DiagMsg(DIAG_ERROR, "start_listener - Thread creation failed (err Id: %d)\n", ret);
    }
  }
  
  return ret;
}

void stop_listener(Ftdi_Listener_t *listener)
{
  listener->Stop = true;
#ifdef _WIN32
  SetEvent((HANDLE)listener->hEvent);
#else
  pthread_cond_signal(&(listener->event->eCondVar));
#endif
}

bool isRunning(Ftdi_Listener_t * listener)
{
  return listener->Running;
}

void *threadlistener(void *arg)
{
  FT_STATUS ftStatus;
  DWORD EventDWord;
  DWORD TxBytes;
  DWORD RxBytes = 0;
  DWORD slen, alen, tmpLen;
  uint8_t RxBuffer[512];
  int32_t ret;  
  bool unlock;
  uint8_t *pRxBuffer = RxBuffer;
  pthread_mutex_t *rLock;
  Ftdi_Listener_t *pFtdiListener = (Ftdi_Listener_t *)arg;
  EVENT_HANDLE *event = pFtdiListener->event;
  ftdi_config_t * pDev = (ftdi_config_t*) pFtdiListener->pDev;

  rLock = &(event->eMutex);


  // timestamp_last = time(NULL);
  if (pDev)
    DiagMsg(DIAG_DEBUG, "ftdi listener :: Start (id: %p, l:%d, dev:%d)\n", pFtdiListener->Stop, pDev->devid);
  else
    DiagMsg(DIAG_DEBUG, "ftdi listener :: Start (id: %p, l:%d)\n", pFtdiListener->threadId, pFtdiListener->Stop);
  EventDWord = FT_EVENT_RXCHAR | FT_EVENT_MODEM_STATUS;
#ifdef _WIN32
  pFtdiListener->hEvent = CreateEvent(NULL,
                                      true,  // manual-reset event
                                      false, // non-signalled state
                                      "");
  ftStatus = FT_SetEventNotification(pDev->ftHandle, EventDWord, pFtdiListener->hEvent);

#else
  ftStatus = FT_SetEventNotification(pDev->ftHandle, EventDWord, (PVOID)pFtdiListener->event);
#endif
  if (ftStatus)
  {
    DiagMsg(DIAG_ERROR, "listener failed to create event");
    pFtdiListener->Running = false;
    return NULL;
  }

  while ((!pFtdiListener->Stop) && (pDev))
  {
    // timestamp_now = time(NULL);
    pthread_mutex_lock(rLock);
    unlock = true;
    // Check inside the mutex
    if (pDev)
    {
      FT_GetStatus(pDev->ftHandle, &RxBytes, &TxBytes, &EventDWord);
      if (!RxBytes)
      {
        //DiagMsg(DIAG_RXMSG, "wait for rx");
#ifndef _WIN32
        pthread_cond_wait(&(q->eCondVar), &(eh->eMutex));
#else
        pthread_mutex_unlock(rLock);
        DiagMsg(DIAG_DEBUG, "Wait for serial");
        WaitForSingleObject(pFtdiListener->hEvent, INFINITE);
        pthread_mutex_lock(rLock);
        ResetEvent(pFtdiListener->hEvent);
#endif
        //DiagMsg(DIAG_RXMSG, "listener wakeup");
      }

      if (pDev)
      {
        FT_GetStatus(pDev->ftHandle, &RxBytes, &TxBytes, &EventDWord);
        if (RxBytes > 0)
        {
          if(!pRxBuffer)
            pRxBuffer = RxBuffer;
          tmpLen = (pRxBuffer - RxBuffer);
          DiagMsg(DIAG_RXMSG, "(RxBytes %d, tmpLen %d, pRxBuffer %p): ", RxBytes, tmpLen, pRxBuffer);
          if ((tmpLen + RxBytes) >= sizeof(RxBuffer))
            RxBytes = sizeof(RxBuffer) - tmpLen;

          ftStatus = FT_Read(pDev->ftHandle, pRxBuffer, RxBytes, &alen);
          unlock = false;
          pthread_mutex_unlock(rLock);
          if (ftStatus == FT_OK)
          {
            slen = alen + tmpLen;
            DiagMsg(DIAG_RXMSG, "RX (len %d, totLen %d ): ", alen, slen );
            pRxBuffer = RxBuffer; 
            ret = ((int32_t(*)(void*, uint8_t**, uint32_t))pFtdiListener->callback)(pFtdiListener->pHandler,&pRxBuffer, slen);
            if(ret && ret != PROTOCOL_STATUS_UNDERFLOW)
              pFtdiListener->Diag.UNKNOWN_MSG++;
          }
          else
          {
            // FT_Read Failed
            pFtdiListener->Diag.RX_ERROR++;
          }
        }
      }
    }
    if (unlock)
      pthread_mutex_unlock(rLock);
  }
  if (pDev)
    DiagMsg(DIAG_DEBUG, "ftdi listener :: End (id: %p, l:%d, dev:%d)\n", pFtdiListener->threadId, pFtdiListener->Stop, pDev->devid);
  else
    DiagMsg(DIAG_DEBUG, "ftdi listener :: End (id: %p, l:%d)\n", pFtdiListener->threadId, pFtdiListener->Stop);

  pFtdiListener->Running = false;
  return NULL;
}


