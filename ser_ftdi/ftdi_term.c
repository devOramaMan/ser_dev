#include "config.h"
#include "ftdi_term.h"
#include "ftdi_listener.h"
#include "ftdi_connect.h"
#include "fcp_term.h"
#include "util_common.h"

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




bool get_term_baud_rate(int * local_baud_rate);


void ftdi_menu(void)
{
	int baud_rate = 115200;
	char char_choice[50];
	int int_choice = 0;
	int err;
	uint32_t numDevs;
	FT_DEVICE_LIST_INFO_NODE * devInfo;

	bool got_list = false;
	CLEAR_SCREEN();
	// FTDI Menu
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
		printf("1. Quick Connect (device 0)\n");
		printf("2. Device List\n");
		if (got_list == true && !isRunning()) // Only display option if devices list.
		{
			printf("3. Connect Device\n");
		}
		if (pCurrentDev && !isRunning()) // Only give display if connected.
		{
			printf("4. Close Device\n");
		}
		if (!isRunning()) // Only give display if connected.
		{
			printf("5. Change/set baud-rate\n");
		}
		if (pCurrentDev) // Only give display if connected.
		{
			if (!isRunning())
				printf("6. Start ftdi device listener\n");
			else
				printf("6. Stop ftdi device listener\n");
		}
		if (!isDiagEna())
		{
			printf("7. Enable ftdi device diagnostics\n");
		}
		else
		{
			printf("7. Disable ftdi device diagnostics\n");
		}
		if (pCurrentDev && (isRunning()))
		{
			printf("8. FCP Menu.\n");
		}

		printf("9. Exit\n");

		// Get user choice.
		scanf("%s", char_choice);

		// Convert string to int for switch statement.
		int_choice = atoi(char_choice);
		CLEAR_SCREEN();
		switch (int_choice)
		{
		case 1:
			quick_connect();
		case 2:
			devInfo =  get_device_info(&numDevs);
			if(!numDevs || !devInfo)
			{
				printf("Failed to find ftdi device(s)");
				got_list = false;
			}
			else
			{
				got_list = true;
				for (int i = 0; i < numDevs && i < 8; i++)
				{
					printf("\nDevice: %d:\n", i);
					printf(" 	Flags:         0x%02X\n", devInfo[i].Flags);
					printf(" 	Type:          0x%02X\n", devInfo[i].Type);
					printf(" 	ID:            0x%02X\n", devInfo[i].ID);
					printf(" 	Local ID:      0x%02X\n", devInfo[i].LocId);
					printf(" 	Serial Number: %s\n", devInfo[i].SerialNumber);
					printf(" 	Description:   %s\n", devInfo[i].Description);
					printf(" 	ftHandle =     %p\n", devInfo[i].ftHandle);
				}
			}

			break;
		case 3:
			if (got_list == true) // Only display option if devices listed.
			{
				connect_device(&baud_rate);
			}
			break;
		case 4:
			if (pCurrentDev) // Only give display if connected.
			{
				close_device();
			}
			break;
		case 5:
			if(get_term_baud_rate(&baud_rate))
			{
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
			break;
		case 6:
			if (!isRunning())
			{
				if (pCurrentDev) // Only give display if connected.
					start_listener(true);
			}
			else
			{
				start_listener(false);
				Sleep(1000);
			}
			break;
		case 7:
			if (!isDiagEna())
			{
				start_diagnostics_print(true);
			}
			else
			{
				start_diagnostics_print(false);
			}
			break;
		case 8:
			fcp_menu();
			break;
		case 9:
			// main_menu();
			if (pCurrentDev) // Only give display if connected.
			{
				start_listener(false);
				Sleep(1000);
				close_device();
			}
			int_choice = 99;
			break;
		default:
			printf(""
				   "Bad choice. Hot glue!"
				   "");
			break;
		}
	} while (int_choice != 99);
}

void quick_connect(void)
{
	int local_baud_rate = 115200;
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
	FT_DEVICE_LIST_INFO_NODE * devInfo =  get_device_info(&numDevs);

	CLEAR_SCREEN();
	printf("Which device # (0-8)?\n\n");
	printf("9. Return\n");

	printf("\n\nConnected FTDI:");
	for (int i = 0; i < numDevs && i < 8; i++) {
		printf("\nDevice: %d:\n",i);
		printf(" 	Flags:         0x%02X\n",devInfo[i].Flags);
		printf(" 	Type:          0x%02X\n",devInfo[i].Type);
		printf(" 	ID:            0x%02X\n",devInfo[i].ID);
		printf(" 	Local ID:      0x%02X\n",devInfo[i].LocId);
		printf(" 	Serial Number: %s\n",devInfo[i].SerialNumber);
		printf(" 	Description:   %s\n",devInfo[i].Description);
		printf(" 	ftHandle =     %p\n", devInfo[i].ftHandle);
	}

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
	char char_choice[3];
	int int_choice = 0;

	CLEAR_SCREEN();
	printf("Set baud: \n");	
	printf("1. 9600\n");
	printf("2. 19200\n");
	printf("3. 38400\n");
	printf("4. 57600\n");
	printf("5. 115200\n");
	printf("6. 230400\n");
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
		default:printf("""Bad choice. Hot glue!""");
			return false;
		    break;
	}
	return true;
}



