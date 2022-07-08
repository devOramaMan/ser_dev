/*-----------------------------------------------------------------------------
 * Name:    protocol_diag.h
 * Purpose: Protocol diagnostics 
 *-----------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/

#ifndef PROTOCOL_DIAG
#define PROTOCOL_DIAG

#include <stdint.h>

typedef struct Protocol_Diag_List
{
    char * name;
    void * diag;
}Protocol_Diag_List_t;

typedef struct Protocol_Diag_Handle
{
    Protocol_Diag_List_t * diagList;
    uint32_t size;
}Protocol_Diag_Handle_t;

void protocol_diagnostics_print(void);

void protocol_diagnostics_clear(void);

#endif

