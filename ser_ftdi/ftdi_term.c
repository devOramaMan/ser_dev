#include "config.h"
#include "protocol_config.h"
#include "ftdi_term.h"
#include "ftdi_connect.h"
#include "fcp_term.h"
#include "util_common.h"
#include "ftdi_atomic.h"
#include "diagnostics_util.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h> 
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <windef.h>
#include <winnt.h>
#include <winbase.h>
#endif
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include "ftdi_config.h"
#include <sys/time.h>
#include <unistd.h>


void set_loglevel(void);
void set_latency(void);
void quick_connect(int local_baud_rate);
bool get_term_baud_rate(int * local_baud_rate);
FT_DEVICE_LIST_INFO_NODE * list_device(uint32_t *numDevs);


void ftdi_menu(void)
{
	int baud_rate = 115200;
	char char_choice[50];
	int int_choice = 0;
	int err;
	FT_STATUS ftStatus;
	uint8_t tmp = 0;
	int32_t i;
	uint32_t numDevs;
	FT_DEVICE_LIST_INFO_NODE * devInfo = NULL;
	CLEAR_SCREEN();
	// FTDI Menu
	memset(char_choice, 0, sizeof(char_choice));
	do
	{
		// If connected, display the connected device info.
		if (pCurrentDev)
		{
			devInfo = pCurrentDev->pDevInfo;
			printf("\n");
			printf("Connected Device: %d:\n", pCurrentDev->devid);
			if(devInfo)
			{
				printf(" 	Flags:         0x%02X\n", devInfo->Flags);
				printf(" 	Type:          0x%02X\n", devInfo->Type);
				printf(" 	ID:            0x%02X\n", devInfo->ID);
				printf(" 	Local ID:      0x%02X\n", devInfo->LocId);
				printf(" 	Serial Number: %s\n", devInfo->SerialNumber);
				printf(" 	Description:   %s\n", devInfo->Description);
				printf(" 	ftHandle =     %p\n", devInfo->ftHandle);
				ftStatus = FT_GetLatencyTimer(pCurrentDev->ftHandle, (PUCHAR)&tmp);
				if (ftStatus == FT_OK) 
				{
					printf(" 	Latency  =     %d\n", tmp);
				}
				else
				{
					printf(" 	Failed to get Latency\n");
				}
				ftStatus = FT_GetBitMode(pCurrentDev->ftHandle, (PUCHAR)&tmp);
				if (ftStatus == FT_OK) 
				{
					printf(" 	BitMode  =     %d\n", tmp);
				}
				else
				{
					printf(" 	Failed to get BitMode\n");
				}
			}
		}

		printf("FTDI Menu: ");
		if (pCurrentDev)
		{
			printf("       Connected: %d, N, 1     \n\n", pCurrentDev->baud);
		}
		else
		{
			printf("       Not Connected:               \n\n");
		}
		if (!isRunning(&ftdi_listener)) // Only display option if devices list.
		{
			printf("1. Quick Connect (device 0)\n");
		}
		printf("2. Device List\n");
		if (!isRunning(&ftdi_listener)) // Only display option if devices list.
		{
			printf("3. Connect Device\n");
		}
		if (pCurrentDev || isRunning(&ftdi_listener)) // Only give display if connected.
		{
			printf("4. Close Device\n");
		}
		printf("5. Change/set baud-rate\n");
		if (pCurrentDev) // Only give display if connected.
		{
			if (!isRunning(&ftdi_listener))
				printf("6. Start ftdi device listener\n");
			else
				printf("6. Stop ftdi device listener\n");
		}
		
		printf("7. Print protocol diagnostics\n");

		printf("8. Clear protocol diagnostics\n");

		printf("9. Set Log level\n");

		if (pCurrentDev)// && (isRunning()))
		{
			printf("10. FCP Menu.\n");
			printf("11. Set Latency\n");
		}		

		printf("99 Exit\n");

		// Get user choice.
		scanf("%s", char_choice);

		// Convert string to int for switch statement.
		int_choice = atoi(char_choice);
		CLEAR_SCREEN();
		switch (int_choice)
		{
		case 1:
			quick_connect(baud_rate);
		case 2:
			devInfo =  list_device(&numDevs);
			break;
		case 3:
			connect_device(&baud_rate);
			break;
		case 4:
			close_device();
			break;
		case 5:
			if(get_term_baud_rate(&baud_rate))
			{
				printf("baudrate set to %d\n" , baud_rate);
				if(pCurrentDev)
				{
					// set_baud_flag is not used, yet.
					err = set_baud_rate(baud_rate);
					if(err)
						printf("failed to set baudrate (err %d", err );
					else
						printf("Baudrate set to %d", baud_rate);
				}
			}
			else
				printf("Failed to set baudrate (current: %d)\n" , baud_rate);
			break;
		case 6:
			if (!isRunning(&ftdi_listener))
			{
				if (pCurrentDev) // Only give display if connected.
					start_listener(&ftdi_listener,pCurrentDev);
			}
			else
			{
				stop_listener(&ftdi_listener);
				Sleep(1000);
			}
			break;
		case 7:
			printf("press enter to reprint and \"q\" to return\n");
			getchar();
			do
			{
				protocol_diagnostics_print();
				*char_choice = getchar();
			} while (*char_choice != 'q');
			break;
		case 8:
			protocol_diagnostics_clear();
			break;
		case 9:
			set_loglevel();
			break;
		case 10:
			if(pCurrentDev)
				fcp_menu();
			break;
		case 11:
			set_latency();
			break;
		case 99:
			// main_menu();
			if (pCurrentDev) // Only give display if connected.
			{
				stop_listener(&ftdi_listener);
				Sleep(1000);
				close_device();
			}
			int_choice = 99;
			break;
		default:
			printf("Bad choice (%d) ", (int32_t)strlen(char_choice));
			for(i = 0; i < strlen(char_choice) && i < sizeof(char_choice); i++)
			{
				printf("0x%X ", char_choice[i] & 0xff);
				char_choice[i] = 0;
			}
			printf("\n");
			break;
		}
	} while (int_choice != 99);
}

FT_DEVICE_LIST_INFO_NODE * list_device(uint32_t *numDevs)
{
	FT_DEVICE_LIST_INFO_NODE * devInfo = NULL;
	devInfo =  get_device_info(numDevs);
	int32_t lComPortNumber;
	if(!numDevs || !devInfo)
	{
		printf("Failed to find ftdi device(s)");
	}
	else
	{
		for (int i = 0; i < *numDevs && i < 8; i++)
		{
			
			printf("\nDevice: %d:\n", i);
			printf(" 	Flags:         0x%02X\n", devInfo[i].Flags);
			printf(" 	Type:          0x%02X\n", devInfo[i].Type);
			printf(" 	ID:            0x%02X\n", devInfo[i].ID);
			printf(" 	Local ID:      0x%02X\n", devInfo[i].LocId);
			printf(" 	Serial Number: %s\n", devInfo[i].SerialNumber);
			printf(" 	Description:   %s\n", devInfo[i].Description);
			printf(" 	ftHandle =     %p\n", devInfo[i].ftHandle);
			if(!FT_GetComPortNumber_Atomic(i, devInfo[i].ftHandle, &lComPortNumber) && lComPortNumber>-1)
		    	printf(" 	Port:          COM%d\n", lComPortNumber);
			else
				printf(" 	Port:        UNKNOWN\n");

		}
	}
	return devInfo;
}

void set_loglevel(void)
{
	char char_choice[50];
	printf("\n\nSet level 0-x:");
	scanf("%s", char_choice);

		// Convert string to int for switch statement.
	diag_set_verbose(atoi(char_choice));
}

void set_latency(void)
{
	if(!pCurrentDev)
		return;
	FT_STATUS ftStatus;
	int32_t latency;
	char char_choice[50];
	printf("\n\nSet latency 2-x:");
	scanf("%s", char_choice);

	if(!pCurrentDev)
	{
		printf("connection closed\n");
		return;
	}

	latency = atoi(char_choice);
	if( latency > 1 && latency < 255)
	{
		ftStatus = FT_SetLatencyTimer(pCurrentDev->ftHandle, (UCHAR)latency);
		if(ftStatus != FT_OK)
			printf("Failed to set Latency (err:%d)\n", ftStatus);
	}
	else
	{
		printf("Illegal latency: %d (2-254)\n", latency);
	}
}

void quick_connect(int local_baud_rate)
{
	uint32_t numDevs;

	// Create and get the device information list
	FT_DEVICE_LIST_INFO_NODE * devInfo = get_device_info(&numDevs);


	if (numDevs > 0 && devInfo) 
	{
		// Open user's selection.
		// Allocate storage for list based on numDevs.
		connect(0, local_baud_rate);
	}
}

bool connect_device(int * local_baud_rate)
{
	char char_choice[3];
	int int_choice = 0;
	int err;
	uint32_t numDevs;

	CLEAR_SCREEN();	
	printf("\n\nConnected FTDI:");
	if(!list_device(&numDevs))
	{
		printf("\n\nNo FTDI device found:");
		sleep(1);
		return false;		
	}

	printf("Which device # (0-x)?\n\n");
	printf("9. Return\n");
	
	scanf("%s", char_choice);
	int_choice = atoi(char_choice);

	// Limit list to 9 devices.  Really, who has more connected at once?
	if (int_choice == 9)
	{
		return false;
	}
	else if (int_choice > -1 && int_choice < 9 && int_choice <= numDevs)
	{
		// Open user's selection.
		*local_baud_rate = 115200;
		if((err = connect(int_choice, *local_baud_rate)))
		{
			printf("Could not open FTDI device #%i.\n", int_choice);
			Sleep(3000);
			return false;
		}
	}
	else
	{
		printf("FTDI device #%i dosn't exist.\n", int_choice);
		return false;
	}
	return false;
}

bool get_term_baud_rate(int * local_baud_rate)
{	 
	char char_choice[50];
	int int_choice = 0;

	CLEAR_SCREEN();
	printf("Set baud: \n");	
	printf("1. 9600\n");
	printf("2. 19200\n");
	printf("3. 38400\n");
	printf("4. 57600\n");
	printf("5. 115200\n");
	printf("6. 230400\n");
	printf("Custom. > 9600\n");
	printf("9. Exit\n");

	scanf("%s", char_choice);
	int_choice = atoi(char_choice);

	switch (int_choice)
	{
		case 1:
			*local_baud_rate = 9600;
			break;
		case 2:
			*local_baud_rate = 19200;
			break;
		case 3:
			*local_baud_rate = 38400;
			break;
		case 4:
			*local_baud_rate = 57600;
			break;
		case 5:
			*local_baud_rate = 115200;
			break;
		case 6:
			*local_baud_rate = 230400;
			break;
		case 9:
			return false;
			break;
		default:
			if(int_choice < 9600)
			{
				printf("""Bad choice. Hot glue!\n""");
				return false;
			}
			else
			{
				*local_baud_rate = int_choice;
			}			
		    break;
	}
	return true;
}



