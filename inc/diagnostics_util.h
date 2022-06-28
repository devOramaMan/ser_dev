

#ifndef DIAGNOSTICS_UTIL
#define DIAGNOSTICS_UTIL

#include <stdint.h>


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

void diag_set_verbose(uint32_t val);

uint32_t diag_get_verbose() __attribute__((pure));

#endif /* diagnostics_util.h */
