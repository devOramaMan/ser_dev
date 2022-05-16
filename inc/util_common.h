
#include <stdint.h>

#define LISTENER_STATUS_OK                0x00

#define LISTENER_STATUS_TRANSFER_ONGOING  0x01
#define LISTENER_STATUS_WAITING_TRANSFER  0x02
#define LISTENER_STATUS_INVALID_PARAMETER 0x03
#define LISTENER_STATUS_TIME_OUT          0x04
#define LISTENER_STATUS_INVALID_FRAME     0x05

#define LISTENER_STATUS_UNDERFLOW         0x06
#define LISTENER_STATUS_INVALID_CODE      0x07
#define LISTENER_STATUS_CRC_ERROR         0x08

#define MAX_PRINT_SIZE 122

typedef enum
{
	DIAG_ERROR,
	DIAG_WARNING,	
	DIAG_INFO,
	DIAG_DEBUG,
	DIAG_RXMSG,
	DIAG_TXMSG,
    DIAG_UNKNOWN
}DiagnosticType;

typedef void(*FS_CALLBACK)(DiagnosticType type, const char* str, ...);

extern volatile FS_CALLBACK pFSCallback;

#define DiagMsg(A,C,...) pFSCallback(A,C,##__VA_ARGS__)

