
#include "config.h"
#include "ftdi_listener.h"
#include "ftdi_atomic.h"
#include <pthread.h>
#include <stdio.h>
#include "ftdi_config.h"
#include <sys/time.h>

#ifdef _WIN32
    #include "windows.h"
#endif


#define DIAG_OUT_TIME 2000
#define LISTENER_MAX 10

typedef struct ftdi_listener_diag
{
    uint32_t RX_ERROR;
    uint32_t TX_ERROR;
    uint32_t UNKNOWN_MSG;
    bool flag;
}ftdi_listener_diag_t;


pthread_t threadId;
void * threadlistener(void * arg);

 

typedef int32_t (*PROTOCOL_CALLBACK)( void * pHandle, uint8_t * buffer, uint32_t * size );

typedef struct 
{
    void * pHandle;
    PROTOCOL_CALLBACK pProtocol;
}Protocol_t;

typedef struct 
{
    Protocol_t ProtocolList[LISTENER_MAX];
    Protocol_t Ascii;
    uint16_t size;
    bool stop;
    bool diag_ena;
    bool running;
    ftdi_listener_diag_t diag;
}ftdi_listener_t;

ftdi_listener_t ftdi_listener_handle =
{
    .ProtocolList = {{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
    .Ascii      = {0,0},
    .size       = 0,
    .stop       = false,
    .diag_ena   = false,
    .running    = false,
    .diag       = { 0, 0 , true}
};


int start_listener(bool ena)
{
    int ret = 0; 
    if(ena)
    {
        if (isRunning())
        {
            printf("Ftdi Listener already running\n");
            ret = 1;
        }
        else
        {
            ftdi_listener_handle.stop = false;
            ret = pthread_create(&threadId, NULL, &threadlistener, NULL);
            if(!ret)
            {
                ftdi_listener_handle.running = true;
            }
            else
            {
                printf("start_listener - Thread creation failed (err Id: %d)\n", ret);
            }
        }
    }
    else
    {
        ftdi_listener_handle.stop = true;
    }
    return ret;
}

bool isRunning(void)
{
    return ftdi_listener_handle.running; 
}


void * threadlistener(void * arg)
{
    FT_STATUS ftStatus;
    DWORD EventDWord;
    DWORD TxBytes;
    DWORD RxBytes;
    DWORD BytesReceived, len;
    uint64_t timestamp_now, timestamp_last;
    uint8_t RxBuffer[256];
    uint8_t * pBuffer;
    uint8_t i;
    
    
        timestamp_last = time(NULL);
        if(pCurrentDev)
            printf("ftdi listener :: Start (id: %p, l:%d, dev:%d)\n", threadId, ftdi_listener_handle.stop, pCurrentDev->devid);
        else
            printf("ftdi listener :: Start (id: %p, l:%d)\n", threadId, ftdi_listener_handle.stop);

        while( (!ftdi_listener_handle.stop) && (pCurrentDev) )
        {
            timestamp_now = time(NULL);
            pthread_mutex_lock(&ftdi_mutex);
            // Check inside the mutex
            if(pCurrentDev)
            {
                FT_GetStatus(pCurrentDev->ftHandle,&RxBytes,&TxBytes,&EventDWord);
                if (RxBytes > 0)
                {
                    ftStatus = FT_Read_atomic(pCurrentDev->ftHandle, RxBuffer, RxBytes, &BytesReceived);
                    if (ftStatus == FT_OK)
                    {
                        // send to pool
                        pBuffer = RxBuffer;
                        len = BytesReceived;
                        for(i = 0; i < ftdi_listener_handle.size; i++)
                        {
                            PROTOCOL_CALLBACK callback = ftdi_listener_handle.ProtocolList[i].pProtocol;
                            if(callback)
                                callback(ftdi_listener_handle.ProtocolList[i].pHandle, pBuffer, &len);

                            if(len>0 && pBuffer)
                            {
                                if( ftdi_listener_handle.Ascii.pProtocol)
                                {
                                    if(ftdi_listener_handle.Ascii.pProtocol(ftdi_listener_handle.Ascii.pHandle, pBuffer, &len))
                                    {
                                        ftdi_listener_handle.diag.UNKNOWN_MSG++;
                                    }
                                }
                                else
                                {
                                    ftdi_listener_handle.diag.UNKNOWN_MSG++;
                                }
                            }
                        }
                    }
                    else
                    {
                        // FT_Read Failed
                        ftdi_listener_handle.diag.flag = true;
                        ftdi_listener_handle.diag.RX_ERROR++;
                    }
                }
            }
            pthread_mutex_unlock(&ftdi_mutex);
            // TODO macro for sleep
            if( ftdi_listener_handle.diag_ena )
            {
                if((timestamp_now - timestamp_last) > DIAG_OUT_TIME)
                {
                    print_diagnostics();
                    timestamp_last = time(NULL);
                }
            }
                
            Sleep(100);
        }
    if(pCurrentDev)
        printf("ftdi listener :: End (id: %p, l:%d, dev:%d)\n", threadId, ftdi_listener_handle.stop, pCurrentDev->devid);
    else
        printf("ftdi listener :: End (id: %p, l:%d)\n", threadId, ftdi_listener_handle.stop);

    ftdi_listener_handle.running = false;
    return NULL;
}

void register_protocol(void *pHandler, PROTOCOL_CALLBACK prot)
{
    uint8_t size = ftdi_listener_handle.size;
    if( ftdi_listener_handle.size < LISTENER_MAX )
    {
        ftdi_listener_handle.ProtocolList[size].pHandle = pHandler;
        ftdi_listener_handle.ProtocolList[size].pProtocol = prot;
        ftdi_listener_handle.size++;
    }
}

void start_diagnostics_print(bool ena)
{
    ftdi_listener_handle.diag_ena = ena;
}

bool isDiagEna(void)
{
    return ftdi_listener_handle.diag_ena;
}

void print_diagnostics(void)
{
    if(ftdi_listener_handle.diag.flag)
    {
        printf("  diag { RX Error %d }\r ", ftdi_listener_handle.diag.RX_ERROR );
        ftdi_listener_handle.diag.flag = false;
    }
}
