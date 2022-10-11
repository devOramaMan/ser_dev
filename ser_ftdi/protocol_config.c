/*-----------------------------------------------------------------------------
 * Name:    protocol_config.c
 * Purpose: Configure project interfaces
 *-----------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/

#include "protocol_config.h"
#include "fcp_frame_protocol.h"
#include "atomic_queue.h"
#include "diag_frame_protocol.h"


uint8_t fcp_code[] = { FCP_CODE_ACK, FCP_CODE_NACK };

char fcp_name[] = "FCP";

FCP_Handle_t fcp_handle =
{
  ._super.Code = fcp_code,
  ._super.Size = sizeof(fcp_code),
  ._super.Diag = { 0,0,0,0,0,0,0 },
  ._super.MsgCnt = 0,
  .RxSignal = signal_received
};


uint8_t kmc_rec_codes[KMC_REC_MAX_CODES];

char kmc_rec_name[] = "KMC REC";

KMC_Rec_Handle_t kmc_rec_handle = 
{
  ._super.Code = kmc_rec_codes,
  ._super.Diag = { 0,0,0,0,0,0,0 },
  ._super.Size = 0,
  .codes = NULL,
  .flag = 0,
  .dequeued = PTHREAD_COND_INITIALIZER,
  .lock = PTHREAD_MUTEX_INITIALIZER,
  .run = false,
  .enableCallback = false
};


static uint8_t diag_msg_code = DIAG_FRAME_CODE;

#define DIAG_DEV_TTU (uint8_t)0xBB
#define DIAG_DEV_TVA (uint8_t)0xAA

char diag_TTU_name[] = "TTU";
char diag_TVA_name[] = "TVA";


KMC_Diag_Dev_t diagDevs[] =
{
  {DIAG_DEV_TTU, diag_TTU_name},
  {DIAG_DEV_TVA, diag_TVA_name}
};

char diag_msg_name[] = "DIAG";

KMC_Diag_Handle_t kmc_diag_handle = 
{
  ._super.Code = &diag_msg_code,
  ._super.Size = sizeof(diag_msg_code),
  ._super.Diag = { 0,0,0,0,0,0,0 },
  .DiagDevs = diagDevs,
  .DevSize = (uint16_t) sizeof(diagDevs)/sizeof(KMC_Diag_Dev_t)
};


char listener_name[] = "FTDI Listener";

Ftdi_Listener_t ftdi_listener =
{
  .Diag = { 0,0,0,0,0,0,0 },
  .Stop = false,
  .Running = false,
  .pHandler = (void*)&protocol_handler,
  .callback = (void*)receive_data
};


Protocol_Handle_t protocol_list[] =
{
   { (Protocol_t*)&fcp_handle, fcp_receive },
   { (Protocol_t*)&kmc_rec_handle, kmc_receive},
   { (Protocol_t*)&kmc_diag_handle, diag_receive}
};

static const uint32_t protocol_list_size = sizeof(protocol_list)/sizeof(Protocol_Handle_t);

char protocol_handler_name[] = "Protocol Handler"; 

Protocol_Handler_t protocol_handler =
{
    .ProtocolList = protocol_list,
    .size = protocol_list_size
};

Protocol_Diag_List_t diag_list[] =
{
    { listener_name, (void*)&ftdi_listener.Diag },
    { protocol_handler_name, (void*) &protocol_handler},
    { fcp_name, (void*)&fcp_handle },
    { kmc_rec_name, (void*)&kmc_rec_handle},
    { diag_msg_name, (void*)&kmc_diag_handle}
};

static const uint32_t protocol_diag_size = sizeof(diag_list)/sizeof(Protocol_Diag_List_t);


Protocol_Diag_Handle_t protocol_diag_handle =
{
    .diagList = diag_list,
    .size = protocol_diag_size
};

