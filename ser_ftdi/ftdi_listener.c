
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



pthread_t threadId;

bool hasCode(Protocol_t * prot, uint8_t code);
void * threadlistener(void * arg);
int32_t handleAscii(uint8_t * buffer, uint32_t * size);

typedef struct 
{
  Protocol_Handle_t ProtocolList[LISTENER_MAX];
  uint16_t size;
  bool Stop;
  bool Diag_Ena;
  bool Running;
  Protocol_Diag_t Diag;
  #ifdef _WIN32
  HANDLE hEvent;
  #endif
  EVENT_HANDLE * event;
}Ftdi_Listener_t;

Ftdi_Listener_t ftdi_listener_handle =
{
  .ProtocolList = {{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
  .size       = 0,
  .Stop       = false,
  .Diag_Ena   = false,
  .Running    = false,
  .Diag       = { 0, 0 , true}
};




__weak int32_t handleAscii(uint8_t * buffer, uint32_t * size)
{
  //int32_t i;
  //uint8_t * ch = buffer;
  if(!checkAscii(buffer, *size))
  {
    if(diag_get_verbose() >= DIAG_RXMSG)
    {
      while(*size > 0) 
      {
          putchar(*buffer++);
          *size -=1;
      }
    }
  }
  return 0;
}

bool hasCode(Protocol_t *prot, uint8_t code)
{
  uint32_t i;
  bool ret = false;

  if (!prot)
    return ret;

  for (i = 0; i < prot->Size; i++)
  {
    if (prot->Code[i] == code)
    {
      ret = true;
      break;
    }
  }

  return ret;
}

int start_listener(bool ena)
{
    int ret = 0; 
    if(ena)
    {
        if (isRunning())
        {
            printf("Ftdi Listener already Running\n");
            ret = 1;
        }
        else
        {
            ftdi_listener_handle.Stop = false;
            ftdi_listener_handle.event = eh;
            ret = pthread_create(&threadId, NULL, &threadlistener, (void*)&ftdi_listener_handle);
            if(!ret)
            {
                ftdi_listener_handle.Running = true;
            }
            else
            {
                printf("start_listener - Thread creation failed (err Id: %d)\n", ret);
            }
        }
    }
    else
    {
        ftdi_listener_handle.Stop = true;
        #ifdef _WIN32
        SetEvent(ftdi_listener_handle.hEvent);
        #else
        pthread_cond_signal( &(ftdi_listener_handle.event->eCondVar) );
        #endif

    }
    return ret;
}

bool isRunning(void)
{
    return ftdi_listener_handle.Running; 
}

void *threadlistener(void *arg)
{
  FT_STATUS ftStatus;
  DWORD EventDWord;
  DWORD TxBytes;
  DWORD RxBytes = 0;
  DWORD slen, alen, tmpLen;
  uint64_t timestamp_now, timestamp_last;
  uint8_t RxBuffer[256];
  uint8_t *pBuffer, *pRxBuffer = RxBuffer;
  uint8_t i;
  Ftdi_Listener_t *pFtdiListener = (Ftdi_Listener_t *)arg;
  PROTOCOL_CALLBACK callback;
  int32_t ret;
  pthread_mutex_t *rLock = &(pFtdiListener->event->eMutex);
  bool unlock;

  timestamp_last = time(NULL);
  if (pCurrentDev)
    DiagMsg(DIAG_DEBUG,"ftdi listener :: Start (id: %p, l:%d, dev:%d)\n", threadId, ftdi_listener_handle.Stop, pCurrentDev->devid);
  else
    DiagMsg(DIAG_DEBUG,"ftdi listener :: Start (id: %p, l:%d)\n", threadId, ftdi_listener_handle.Stop);
  EventDWord = FT_EVENT_RXCHAR | FT_EVENT_MODEM_STATUS;
#ifdef _WIN32
  pFtdiListener->hEvent = CreateEvent(NULL,
                                      true,  // manual-reset event
                                      false, // non-signalled state
                                      "");
  ftStatus = FT_SetEventNotification(pCurrentDev->ftHandle, EventDWord, pFtdiListener->hEvent);

#else
  ftStatus = FT_SetEventNotification(pCurrentDev->ftHandle, EventDWord, (PVOID)pFtdiListener->event);
#endif
  if (ftStatus)
  {
    DiagMsg(DIAG_ERROR, "listener failed to create event");
    ftdi_listener_handle.Running = false;
    return NULL;
  }

  while ((!ftdi_listener_handle.Stop) && (pCurrentDev))
  {
    timestamp_now = time(NULL);
    pthread_mutex_lock(rLock);
    unlock = true;
    // Check inside the mutex
    if (pCurrentDev)
    {
      FT_GetStatus(pCurrentDev->ftHandle, &RxBytes, &TxBytes, &EventDWord);
      if (!RxBytes)
      {
        DiagMsg(DIAG_DEBUG, "wait for rx");
#ifndef _WIN32
        pthread_cond_wait(&(q->eCondVar), &(eh->eMutex));
#else
        pthread_mutex_unlock(rLock);
        WaitForSingleObject(pFtdiListener->hEvent, INFINITE);
        pthread_mutex_lock(rLock);
        ResetEvent(pFtdiListener->hEvent);
#endif
        DiagMsg(DIAG_DEBUG, "listener wakeup");
      }
      
      if (pCurrentDev)
      {
        FT_GetStatus(pCurrentDev->ftHandle, &RxBytes, &TxBytes, &EventDWord);
        if (RxBytes > 0)
        {
          tmpLen = (pRxBuffer - RxBuffer);
          if((tmpLen + RxBytes) > sizeof(RxBuffer))
            RxBytes = sizeof(RxBuffer) - tmpLen;

          ftStatus = FT_Read(pCurrentDev->ftHandle, pRxBuffer, RxBytes, &alen);
          unlock = false;
          pthread_mutex_unlock(rLock);
          if (ftStatus == FT_OK)
          {
            slen = alen + tmpLen;
            DiagMsg(DIAG_RXMSG, "RX (len %d, totLen %d, pRxBuffer %p): ", alen, slen, pRxBuffer);
            ret = 0;
            // send to pool
            pBuffer = RxBuffer;
            
            while (slen > 0)
            {
              callback = NULL;
              for (i = 0; i < ftdi_listener_handle.size; i++)
              {
                if (hasCode(ftdi_listener_handle.ProtocolList[i].pProtocol, *pBuffer))
                {
                  callback = ftdi_listener_handle.ProtocolList[i].pCallback;
                  break;
                }
              }
              if (callback)
              {
                ret = callback(ftdi_listener_handle.ProtocolList[i].pProtocol, pBuffer, &slen);
                if (!ret)
                {
                  ftdi_listener_handle.Diag.MSG_OK++;
                }
                else
                {
                  if (ret == PROTOCOL_STATUS_UNDERFLOW)
                  {
                    DiagMsg(DIAG_RXMSG, "Underflow (code %d, len %d)", *pBuffer, slen);
                    pRxBuffer = pBuffer + slen;
                    slen = 0;
                  }
                  else
                  {
                    if(handleAscii(pBuffer, &slen))
                    {
                      DiagMsg(DIAG_RXMSG, "Invalid msg");
                      if (ret == PROTOCOL_CODE_CRC_ERROR)
                        ftdi_listener_handle.Diag.CRC_ERROR++;
                      else if (ret == PROTOCOL_STATUS_INVALID_CODE)
                        ftdi_listener_handle.Diag.UNKNOWN_MSG++;

                      ftdi_listener_handle.Diag.RX_ERROR++;
                    }
                  }
                }
              }
              else
              {
                if(handleAscii(pBuffer, &slen))
                {
                  ret = PROTOCOL_STATUS_INVALID_CODE;
                  DiagMsg(DIAG_RXMSG, "Unknown code:  %d: ", *pBuffer);
                }
              }

              if(ret && ret != PROTOCOL_STATUS_UNDERFLOW)
              {
                if (slen > 0 && pBuffer)
                {
                  ftdi_listener_handle.Diag.UNKNOWN_MSG++;
                  ftdi_listener_handle.Diag.RX_ERROR++;
                  pBuffer++;
                  slen--;
                }
              }
            }
            if(ret != PROTOCOL_STATUS_UNDERFLOW)
              pRxBuffer = RxBuffer;
          }
          else
          {
            // FT_Read Failed
            ftdi_listener_handle.Diag.Flag = true;
            ftdi_listener_handle.Diag.RX_ERROR++;
          }
        }
      }
    }
    if(unlock)
      pthread_mutex_unlock(rLock);

    // TODO macro for sleep
    if (ftdi_listener_handle.Diag_Ena)
    {
      if ((timestamp_now - timestamp_last) > DIAG_OUT_TIME)
      {
        print_diagnostics();
        timestamp_last = time(NULL);
      }
    }
  }
  if (pCurrentDev)
    DiagMsg(DIAG_DEBUG, "ftdi listener :: End (id: %p, l:%d, dev:%d)\n", threadId, ftdi_listener_handle.Stop, pCurrentDev->devid);
  else
    DiagMsg(DIAG_DEBUG, "ftdi listener :: End (id: %p, l:%d)\n", threadId, ftdi_listener_handle.Stop);

  ftdi_listener_handle.Running = false;
  return NULL;
}

void register_protocol(void *pHandler, PROTOCOL_CALLBACK prot)
{
    uint8_t size = ftdi_listener_handle.size;
    if( ftdi_listener_handle.size >= LISTENER_MAX )
    {
        DiagMsg(DIAG_ERROR, "ftdi listener full");
    }
    ftdi_listener_handle.ProtocolList[size].pProtocol = pHandler;
    ftdi_listener_handle.ProtocolList[size].pCallback = prot;
    ftdi_listener_handle.size++;
}

void start_diagnostics_print(bool ena)
{
    ftdi_listener_handle.Diag_Ena = ena;
}

bool isDiagEna(void)
{
    return ftdi_listener_handle.Diag_Ena;
}

void print_diagnostics(void)
{
    if(ftdi_listener_handle.Diag.Flag)
    {
        printf("  diag { RX Error %d }\r ", ftdi_listener_handle.Diag.RX_ERROR );
        ftdi_listener_handle.Diag.Flag = false;
    }
}
