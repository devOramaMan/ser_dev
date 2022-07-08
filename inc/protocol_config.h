/*-----------------------------------------------------------------------------
 * Name:    protocol_config.h
 * Purpose: Configure project interfaces
 *-----------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/

#include "ftdi_listener.h"
#include "protocol_handler.h"
#include "kmc_rec_protocol.h"
#include "protocol_diag.h"

#ifndef PROTOCOL_CONFIG
#define PROTOCOL_CONFIG

extern Ftdi_Listener_t ftdi_listener;
extern Protocol_Handler_t protocol_handler;

extern KMC_Rec_Handle_t kmc_rec_handle;

extern Protocol_Diag_Handle_t protocol_diag_handle;

//TODO maby remove if not used extern
//extern FCP_Handle_t fcp_handle;

#endif
