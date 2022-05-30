
#include "config.h"
#include "ftdi_listener.h"
#include "ftdi_atomic.h"
#include <pthread.h>
#include <stdio.h>
#include "ftdi_config.h"
#include <sys/time.h>
#include "diagnostics_util.h"
#include "protocol_common.h"

#ifdef _WIN32
    #include "windows.h"
#endif


#define DIAG_OUT_TIME 2000
#define LISTENER_MAX 10



pthread_t threadId;
void * threadlistener(void * arg);

typedef struct 
{
    Protocol_Handle_t ProtocolList[LISTENER_MAX];
    uint16_t size;
    bool Stop;
    bool Diag_Ena;
    bool Running;
    Protocol_Diag_t Diag;
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
            ret = pthread_create(&threadId, NULL, &threadlistener, NULL);
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
    DWORD RxBytes;
    DWORD slen,alen;
    uint64_t timestamp_now, timestamp_last;
    uint8_t RxBuffer[256];
    uint8_t *pBuffer;
    uint8_t i, code;
    HANDLE hEvent;
    PROTOCOL_CALLBACK callback;
    //EVENT_HANDLE eh;

    timestamp_last = time(NULL);
    if (pCurrentDev)
        printf("ftdi listener :: Start (id: %p, l:%d, dev:%d)\n", threadId, ftdi_listener_handle.Stop, pCurrentDev->devid);
    else
        printf("ftdi listener :: Start (id: %p, l:%d)\n", threadId, ftdi_listener_handle.Stop);

    while ((!ftdi_listener_handle.Stop) && (pCurrentDev))
    {
        timestamp_now = time(NULL);
        pthread_mutex_lock(&ftdi_read_mutex);
        // Check inside the mutex
        if (pCurrentDev)
        {
            hEvent = CreateEvent(NULL,
                                 false, // auto-reset event
                                 false, // non-signalled state
                                 "");
            EventDWord = FT_EVENT_RXCHAR | FT_EVENT_MODEM_STATUS;   
            ftStatus = FT_SetEventNotification(pCurrentDev->ftHandle,EventDWord,hEvent);
            if(ftStatus)
            {
                DiagMsg(DIAG_ERROR, "listener failed to create event");
                pthread_mutex_unlock(&ftdi_read_mutex);
                break;
            }
            pthread_mutex_unlock(&ftdi_read_mutex);
            WaitForSingleObject(hEvent,INFINITE);
            pthread_mutex_lock(&ftdi_read_mutex);
            FT_GetStatus(pCurrentDev->ftHandle, &RxBytes, &TxBytes, &EventDWord);
            if (RxBytes > 0)
            {
                ftStatus = FT_Read(pCurrentDev->ftHandle, RxBuffer, RxBytes, &BytesReceived);
                pthread_mutex_unlock(&ftdi_read_mutex);
                if (ftStatus == FT_OK)
                {
                    // send to pool
                    pBuffer = RxBuffer;
                    len = BytesReceived;
                    while(len > 0)
                    {

                        for (i = 0; i < ftdi_listener_handle.size; i++)
                        {

                             = ftdi_listener_handle.ProtocolList[i].pCallback;
                            if (callback)
                                callback(ftdi_listener_handle.ProtocolList[i].pProtocol, pBuffer, &len);

                            if (len > 0 && pBuffer)
                            {
                                ftdi_listener_handle.Diag.UNKNOWN_MSG++;
                            }
                        }
                    }
                }
                else
                {
                    // FT_Read Failed
                    ftdi_listener_handle.Diag.Flag = true;
                    ftdi_listener_handle.Diag.RX_ERROR++;
                }
            }
            else
            {
                pthread_mutex_unlock(&ftdi_read_mutex);
            }
        }
        else
        {
            pthread_mutex_unlock(&ftdi_read_mutex);
        }

        // TODO macro for sleep
        if (ftdi_listener_handle.Diag_Ena)
        {
            if ((timestamp_now - timestamp_last) > DIAG_OUT_TIME)
            {
                print_diagnostics();
                timestamp_last = time(NULL);
            }
        }

        Sleep(100);
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
