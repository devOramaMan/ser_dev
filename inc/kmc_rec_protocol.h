


#ifndef KMC_REC_PROTOCOL
#define KMC_REC_PROTOCOL

#include <stdint.h>
#include "protocol_common.h"
#include <charq.h>
#include <pthread.h>

#define KMC_REC_MAX_CODES         20
typedef struct KMC_Transmit_Frame_s
{
	uint8_t Code;         /**< Identifier of the Frame. States the nature of the Frame. */
	const uint8_t Size;   /**< Size of the Payload of the frame in bytes. */
  uint8_t cnt;          /**< cnt to verify all is received. */
}KMC_Rec_Frame_t;



typedef struct KMC_Rec_Subscribe
{
  uint8_t code;
  uint8_t rec_code;  
  uint8_t size;
  uint8_t type;
  uint32_t send_size;
  uint32_t pool_size; 
  uint32_t transaction_id;
}KMC_Rec_Subscribe_t;

typedef struct KMC_Code_List
{
  KMC_Rec_Subscribe_t subscriber;
  CHARQ_STRUCT pool;
  volatile int32_t tmpHead;
  int16_t lastCnt;
  Msg_Keys_t * outmsg;
  void * outBuffer;
  void * callback;
  struct KMC_Code_List * pNext;
}KMC_Code_List_t;

typedef struct KMC_Rec_Handle
{
  Protocol_t _super;
  KMC_Code_List_t * codes;
  uint8_t flag; //use in receive only lock when signaling
  pthread_cond_t dequeued;
  pthread_mutex_t lock;
  pthread_t threadId;
  volatile bool enableCallback;
  volatile bool run;
}KMC_Rec_Handle_t;

void init_kmc_rec(KMC_Rec_Handle_t * pHandle);

int32_t kmc_receive( Protocol_t * pHandle, uint8_t ** buffer, uint32_t * size );

uint32_t kmc_rec_subscribe(KMC_Rec_Handle_t * pHandle, void* pCallback, KMC_Rec_Subscribe_t * msg );

void kmc_rec_enable(KMC_Rec_Handle_t * pHandle, bool ena);

void kmc_rec_unsubscribe(KMC_Rec_Handle_t * pHandle);

void stop_rec_thread(KMC_Rec_Handle_t * pHandle);

//void * getHandler(void);

#endif /* KMC_REC_PROTOCOL.h */
