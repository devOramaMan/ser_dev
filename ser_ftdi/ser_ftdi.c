#include "config.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h> 
// #if defined(_WIN32) || defined(_WIN64)
//     #include <windows.h>
//     #include <windef.h>
//     #include <winnt.h>
//     #include <winbase.h>
// #endif
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>
#include <getopt.h>

#include "protocol_common.h"
#include "ftdi_term.h"
#include "ftdi_connect.h"
#include "diagnostics_util.h"
#include "atomic_queue.h"
#include "file_handler.h"
#include "ftdi_atomic.h"
#include "ftdi_listener.h"
#include "util_common.h"

#include "socket_com.h"
#include "socket_term.h"
#include "msg_handler.h"


void print_help(void);


#define HELP_ARG 100
#define VERBOSE_ARG 101
#define VERSION_ARG 102
#define FTDI_TERMINAL_ARG 103
#define CONNECT_ARG 104
#define BAUDRATE_ARG 105
#define SOCKET_SERVER_ARG 106
#define SOCKET_CONNECT_STR_ARG 107

static const char version[] = "0.1.0";

extern Protocol_t * init_fcp(RX_TX_CODE_FUNC RxSignal, RX_TX_FUNC TxQueue);
extern int32_t fcp_receive( Protocol_t * pHandle, uint8_t *Buffer, uint32_t * Size );
extern void register_protocol(void *pHandler, PROTOCOL_CALLBACK prot);


const struct option long_opts[] = {{"help", no_argument, NULL, HELP_ARG},
                                   {"h", no_argument, NULL, HELP_ARG},
                                   {"v", required_argument, NULL, VERBOSE_ARG},
                                   {"verbose", required_argument, NULL, VERBOSE_ARG},
                                   {"ver", no_argument, NULL, VERSION_ARG},
                                   {"term",    no_argument ,NULL, FTDI_TERMINAL_ARG},
                                   {"t",    no_argument ,NULL, FTDI_TERMINAL_ARG},
                                   {"c",    required_argument ,NULL, CONNECT_ARG},
                                   {"b",    required_argument ,NULL, BAUDRATE_ARG},
                                   {"s",    no_argument ,NULL, SOCKET_SERVER_ARG},
                                   {"sc",   required_argument ,NULL, SOCKET_SERVER_ARG},
                                   {NULL,   0,           NULL, 0}};

void print_help(void)
{
    printf("\n");
    printf("ser_ftdi.exe - serial ftdi interface (version: %s)\n", version);
    printf("\n");
    printf("-v [0-x] -verbose [0-x] debug print out level\n");
    printf("-ver print version\n");
    printf("-t / -term - run ftdi terminal\n");
    printf("-c [sn/com] - connect to device");
    printf("-b [x] - set baudrate");
    printf("-s - start socket server");
    printf("-h -help print help\n\n");
}

#define TOPIC_SIZE (uint32_t)3

Topic_Type_t msg_types[TOPIC_SIZE] =
{
    { FCP_SINGLE_TOPIC, (void*) msg_fcp_single },
    { READ_FILE_RECORD_TOPIC, (void*) append_read_file },
    { WRITE_FILE_RECORD_TOPIC, (void*) append_write_file  }
};

extern int testSocket(void);

int main(int argc, char *argv[])
{
    int opt;
    bool terminal = false;
    bool socket_server = false;
    bool connected = false;
    int debug_level = 0;
    int device_num = -1;
    int baud = 1000000;
    int tmp;
    char ser_sn[30];
    char connectstr[100] = "tcp://127.0.0.1:5565";

    setvbuf(stdout, NULL, _IONBF, 0); 

    if (argc > 1)
    {
        while ((opt = getopt_long_only(argc, argv, "", long_opts, NULL)) != -1)
        {
            switch (opt)
            {
            case HELP_ARG:
                print_help();
                return 0;
                break;
            case VERBOSE_ARG:
                debug_level = atoi(optarg);                
                diag_set_verbose(debug_level);
                DiagMsg( DIAG_INFO,"Verbose level set: %d", debug_level);
                break;
            case FTDI_TERMINAL_ARG:
                terminal = true;
                break;
            case VERSION_ARG:
                printf("version: %s\n", version);
                return 0;
            case CONNECT_ARG:
                snprintf(ser_sn,sizeof(ser_sn),optarg);
                if(!get_device_sn(ser_sn, &device_num))
                    DiagMsg( DIAG_ERROR, "Failed to find: %s", ser_sn);
                break;
            case BAUDRATE_ARG:
                tmp = atoi(optarg);
                if (!tmp)
                    DiagMsg( DIAG_ERROR,"Failed to get baud argument (using default %d)", baud);
                break;
            case SOCKET_SERVER_ARG:
                socket_server = true;
                break;
            case SOCKET_CONNECT_STR_ARG:
                socket_server = true;
                tmp = strlen(optarg);
                if(tmp < sizeof(connectstr))
                    memcpy((void*)connectstr, optarg, tmp);
                else
                    DiagMsg(DIAG_ERROR, "Connection string too long");
                break;
            case '?':
            default:
                DiagMsg(DIAG_ERROR,"Unknown argument\n");
                print_help();
                return 1;
            }
        }
    }
    else
    {
        DiagMsg(DIAG_ERROR,"Missing Argument\n");
        print_help();
    }

    // optind is for the extra arguments
    // which are not parsed
    for(; optind < argc; optind++)
    {
        DiagMsg(DIAG_ERROR,"unknown extra option: %s\n", argv[optind]);
    }

    if(device_num != -1)
    {
        if((tmp = connect(device_num, baud)))
        {
            DiagMsg(DIAG_ERROR, "Failed to connect to device num %d (sn %s)", device_num, ser_sn);
        }
        else
        {
            DiagMsg(DIAG_INFO,"Connected to %s (dev %d, baud %d)", ser_sn, device_num, baud);
            connected = true;
        }
    }

    testSocket();

    if( socket_server )
    {
        register_topics(msg_types, TOPIC_SIZE);
        init_socket_menu(connectstr);
    }


    if ( terminal )
    {
        Protocol_t * fcp = init_fcp(signal_received, append_send_queue);
        register_protocol((void*)fcp, fcp_receive);
        start_queue_thread(Write_Atomic);
        start_file_thread();
        // if(connected)
        //     start_listener(true);  
        printf("\ntype any char to open menu..\n");
        getchar();
        ftdi_menu();

        if(socket_server)
            close_socket_menu();
        start_listener(false);
        stop_queue_thread();
        close_device();
    }
    else
    {
        if(connected)
        {
            Protocol_t * fcp = init_fcp(signal_received, append_send_queue);
            register_protocol((void*)fcp, fcp_receive);
            start_listener(true);
            start_queue_thread(Write_Atomic);
            start_file_thread();
            printf("\ntype any char to exit..\n");
            getchar();
            
            if(socket_server)
                close_socket_menu();
            stop_queue_thread();
            start_listener(false);
            close_device();
        }
    }        

    return 0;
}
