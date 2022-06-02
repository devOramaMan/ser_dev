

#include "config.h"
#include "ftdi_connect.h"
#include "ftdi_atomic.h"
#include "diagnostics_util.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

#include <stdint.h>

ftdi_config_t  CurrentDev;
ftdi_config_t * pCurrentDev = NULL;
FT_DEVICE_LIST_INFO_NODE * pdevInfo = NULL;


int strcicmp(char const *a, char const *b)
{
    for (;; a++, b++) 
	{
        int d = tolower((unsigned char)*a) - tolower((unsigned char)*b);
        if (d != 0 || !*a)
            return d;
    }
	return -1;
}

void setCurrentDev(int devid, int baud, FT_DEVICE_LIST_INFO_NODE * devInfo )
{
    if ( !devInfo )
	{
        return;
 	}
	CurrentDev.devid = devid;
	CurrentDev.baud = baud;
    CurrentDev.ftHandle = devInfo->ftHandle;
	CurrentDev.pDevInfo = devInfo;
	pCurrentDev = &CurrentDev;
}



int connect(int dev , int baudrate)
{
	int32_t ret;
	uint32_t lComPortNumber;

    if(!pdevInfo)
	{
        return 1;
	}

	if(pCurrentDev)
	{
		close_device();	
	}

	if( (ret = FT_Open_Atomic(dev, &pdevInfo[dev].ftHandle)) )
        return ret;

	ret = (int32_t) FT_GetComPortNumber(pdevInfo[dev].ftHandle,(LPLONG)&lComPortNumber) ;

	if( ret )
		DiagMsg(DIAG_ERROR,"Failed to get com port number (err %d)", ret);
	else
		DiagMsg(DIAG_INFO, "COM%d Connected!", lComPortNumber);

	DiagMsg(DIAG_INFO, "purge %d", (int)FT_Purge(pdevInfo[dev].ftHandle, FT_PURGE_RX | FT_PURGE_TX)); // Purge both Rx and Tx buffers

	DiagMsg(DIAG_INFO,"sentstat %d",(int)FT_Write(pdevInfo[dev].ftHandle, (uint8_t*)"tull", 4, (LPDWORD)&ret));
	// Set default baud rate.
	if( !(ret = FT_SetBaudRate_Atomic(pdevInfo[dev].ftHandle, baudrate)) )
        setCurrentDev(dev, baudrate, &pdevInfo[dev]);



	return ret;
}

int close_device(void)
{
    int ret = 0;
	if(!pCurrentDev)
		return ret;
	
	return FT_Close_Atomic(pCurrentDev->ftHandle);
}

FT_DEVICE_LIST_INFO_NODE * get_device_info(uint32_t * numDevs)
{
	// Create the device information list.
	if(FT_CreateDeviceInfoList(numDevs))
        return NULL;

    if (pdevInfo)
		free(pdevInfo);
	// Allocate storage for list based on numDevs.
	pdevInfo =
		(FT_DEVICE_LIST_INFO_NODE *)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE) * (*numDevs));

	// Get the device information list.
	if(FT_GetDeviceInfoList(pdevInfo, numDevs))
        return NULL;

    return pdevInfo;
}

bool get_device(const char * sn, int * device)
{
	bool ret = false;
    uint32_t numDevs;
	
	// Create and Get the device information list.
	FT_DEVICE_LIST_INFO_NODE * devInfo = get_device_info(&numDevs);
	
	if(devInfo)
	{
		for (int i = 0; i < numDevs; i++)
		{
			if(!strcicmp(sn,devInfo[i].SerialNumber))
			{
				ret = true;
				*device = i;
				break;
			}
		}
	}
	return ret;
}

int reset_device(int local_baud_rate)
{
    int ret;
	if(!pCurrentDev)
		return false;
	if ( (ret = (int) FT_ResetPort_Atomic(pCurrentDev->ftHandle)) )
        return ret;
	usleep(50);
	return FT_SetBaudRate_Atomic(pCurrentDev->ftHandle, local_baud_rate);
}

int set_baud_rate(int baud_rate)
{
    int ret;
    if (!pCurrentDev)
        return 1;

    if( !(ret = FT_SetBaudRate_Atomic(pCurrentDev->ftHandle, baud_rate)) )
        pCurrentDev->baud = baud_rate;
    
    return ret;
}
