

#ifndef DIAGNOSTICS_UTIL
#define DIAGNOSTICS_UTIL

#include <stdint.h>


typedef enum
{
	DIAG_ERROR = 0x01,
	DIAG_WARNING = 0x02,	
	DIAG_INFO = 0x04,
	DIAG_DEBUG = 0x08,
	DIAG_RXMSG = 0x10,
	DIAG_TXMSG = 0x20,
	DIAG_ASCII = 0x40,
    DIAG_UNKNOWN = 0
}DiagnosticType;

typedef void(*FS_CALLBACK)(DiagnosticType type, const char* str, ...);

extern volatile FS_CALLBACK pFSCallback;

#define DiagMsg(A,C,...) pFSCallback(A,C,##__VA_ARGS__)

void diag_set_verbose(uint32_t val);

uint32_t diag_get_verbose() __attribute__((pure));

#endif /* diagnostics_util.h */
