

#include "kmc_rec_protocol.h"
#include <stddef.h>
#include "protocol_common.h"
#include "diagnostics_util.h"
#include <stdbool.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>





/* Defines ----------------------------------------------------------------*/


/** @brief Index of the kmc frame payload lenght*/
#define KMC_REC_PAYLOAD_LEN_IDX   1
/** @brief Index of the msg counter*/
#define KMC_REC_CNT_IDX           2
/** @brief Index of the payload */
#define KMC_REC_PAYLOAD_IDX       3
/** @brief Offset of the CRC from payload length*/
#define KMC_REC_CRC_OFFSET        2
/** @brief Size of the frame size (total msg size = payload lenght + this define)*/
#define KMC_TX_FRAME_SIZE         3
/** @brief Size of the Header of an FCP frame */
#define KMC_REX_HEADER_SIZE       3

#define KMC_REC_BUSY              1

/** @brief Subscribe */
#define KMC_SUBSCRIBE_TYPE        1
/** @brief Unsubscribe */
#define KMC_UNSUBSCRIBE_TYPE      2

/**
 * @brief KMC Transmitter frame Header (base object)
 */


/* Local func -------------------------------------------------------------*/
KMC_Code_List_t * kmc_frame_has_code(KMC_Rec_Handle_t * pHandle, uint8_t code );
uint8_t kmc_frame_calc_CRC( KMC_Rec_Frame_t * pFrame );
int32_t kmc_frame_parse(KMC_Rec_Handle_t * pHandle, uint8_t **Buffer, uint32_t * size);

/* Local var --------------------------------------------------------------*/



int32_t kmc_register_code(uint8_t val, uint8_t size);

void signal_dequeue(KMC_Rec_Handle_t * pHandle);


void *kmc_rec_thread(void *arg);

void *kmc_rec_thread(void *arg)
{
  KMC_Code_List_t * pCode;
  int32_t i = 0, ret;
  uint8_t * sendData;
  Msg_Keys_t * pOutMsg;
  KMC_Rec_Handle_t *pHandler = (KMC_Rec_Handle_t *)arg;

  DiagMsg(DIAG_DEBUG, "kmc rec thread started (id: 0x%x)", pHandler->threadId);
  
  while (pHandler->run)
  {

    pthread_mutex_lock(&pHandler->lock);
    pthread_cond_wait(&(pHandler->dequeued), &(pHandler->lock));
    DiagMsg(DIAG_DEBUG, "kmc rec thread dequeued");
    pCode = pHandler->codes;
    //Todo handle busy flag
    while(pCode)
    {
      pOutMsg = pCode->outmsg;
      if(_CHARQ_SIZE(&pCode->pool) >= pCode->subscriber.send_size)
      {
        sendData = (uint8_t*)(pCode->outBuffer + sizeof(Msg_Keys_t));
        for(i = 0; i < pCode->subscriber.send_size; i++ )
        {
          _CHARQ_DEQUEUE(&(pCode->pool),sendData[i]);
        }
        DiagMsg(DIAG_DEBUG, "KMC REC dequeue (msg 0x%X)",pCode->subscriber.rec_code);

        if(pHandler->enableCallback)
        {
          ret = ((int32_t(*)(uint8_t*, uint32_t))pCode->callback)((uint8_t*)pOutMsg,pOutMsg->size);
        
          if(ret)
            DiagMsg(DIAG_ERROR, "Failed to send kmc rec msg");
        }
      }
      pCode = pCode->pNext;
    }
    pthread_mutex_unlock(&(pHandler->lock));

  }

  DiagMsg(DIAG_DEBUG, "kmc rec thread Done (id: 0x%x)", pHandler->threadId);


  return NULL;
}

void signal_dequeue(KMC_Rec_Handle_t * pHandle)
{
  if(!pthread_mutex_trylock(&pHandle->lock))
  {
    pthread_cond_signal( &(pHandle->dequeued) );
    pthread_mutex_unlock(&(pHandle->lock));
  }
}

void init_kmc_rec(KMC_Rec_Handle_t * pHandle)
{
  int32_t stat;
  pthread_attr_t attr;
  struct sched_param sp;

  pHandle->run = true;

  stat = pthread_attr_init(&attr);
  if (stat)
    DiagMsg(DIAG_ERROR, "Kmc rec thread attrib init failed (%d)", stat);

  stat |= pthread_attr_getschedparam(&attr, &sp);
  if (stat)
    DiagMsg(DIAG_ERROR, "Kmc rec queue thread attrib prio failed ");
  else
    DiagMsg(DIAG_DEBUG, "Kmc rec thread priority %d", sp.sched_priority);
  //start Send thread
  if(!stat)
    stat = pthread_create(&(pHandle->threadId), &attr, &kmc_rec_thread, (void *)pHandle);
  else
  {
    DiagMsg(DIAG_WARNING,"start send thread without attrib");
    stat = pthread_create(&(pHandle->threadId), NULL, &kmc_rec_thread, (void *)pHandle);
  }
  if(stat)
  {
    DiagMsg(DIAG_ERROR, "Failed to start kmc rec thread");
  }
}

/**
 * @brief This structure contains and formats a Frame Communication Protocol's frame
 */
KMC_Code_List_t * kmc_frame_has_code(KMC_Rec_Handle_t * pHandle, uint8_t rec_code )
{
  KMC_Code_List_t * pCode = pHandle->codes;
  while(pCode)
  {
    if(pCode->subscriber.rec_code == rec_code )
      break;
    pCode = pCode->pNext;
  }
  return pCode;
}

/**
  * @brief Calculate UINT8 CRC.
  * @details
  * @retval uint8_t, CRC result
  * @param[in] KMC_Rec_Frame_t, the input data for crc caculation
  */
inline uint8_t kmc_frame_calc_CRC( KMC_Rec_Frame_t * pFrame )
{
  uint8_t nCRC = 0;
  uint16_t nSum = 0;
  uint8_t i;
  uint8_t size = pFrame->Size + KMC_TX_FRAME_SIZE;
  uint8_t * pBuffer = (uint8_t*)pFrame;   
  for ( i = 0; i < size; i++ )
    nSum += *pBuffer++;

  nCRC = (uint8_t)(nSum & 0xFF) ; // Low Byte of nSum
  nCRC += (uint8_t) (nSum >> 8) ; // High Byte of nSum

  return nCRC;
}


inline int32_t kmc_frame_parse(KMC_Rec_Handle_t * pHandle, uint8_t **Buffer, uint32_t * size)
{ 
  uint8_t crc;
  KMC_Code_List_t * pCode;
  uint32_t lsize = *size;
  uint8_t i;
  uint8_t * pBuffer = *Buffer;
  uint8_t * payload = (uint8_t*) (pBuffer + sizeof(KMC_Rec_Frame_t));
  KMC_Rec_Frame_t * pFrame = (KMC_Rec_Frame_t*)pBuffer;

  if (lsize < KMC_TX_FRAME_SIZE)
  {
    return PROTOCOL_STATUS_UNDERFLOW;
  }
  else
  {
    pthread_mutex_lock(&(pHandle->lock));
    pCode = kmc_frame_has_code(pHandle, pFrame->Code);
    if (!pCode)
    {
      pthread_mutex_unlock(&(pHandle->lock));
      return PROTOCOL_STATUS_INVALID_CODE;
    }

    if(lsize < (pFrame->Size + KMC_TX_FRAME_SIZE + 1))
    {
      pthread_mutex_unlock(&(pHandle->lock));
      return PROTOCOL_STATUS_UNDERFLOW;
    }
    
    //TODO Handle cnt
    // pFrame->cnt;
    crc = pBuffer[pFrame->Size + KMC_TX_FRAME_SIZE];

    if( crc != kmc_frame_calc_CRC(pFrame) )
    {
      pthread_mutex_unlock(&(pHandle->lock));
      return PROTOCOL_CODE_CRC_ERROR;
    }

    if(pCode->lastCnt >= 0 && (pCode->lastCnt != (uint8_t)(pFrame->cnt - 1)) )
    {
      DiagMsg(DIAG_WARNING, "Loss of rec msg %d (last %d, current %d)", pFrame->Code,pCode->lastCnt, pFrame->cnt-1);
    }
    pCode->lastCnt = pFrame->cnt;

    if(_CHARQ_FULL(&pCode->pool))
    {
      if( (pFrame->Size + KMC_TX_FRAME_SIZE + 1 ) < lsize)
        *Buffer = &pBuffer[pFrame->Size + KMC_TX_FRAME_SIZE + 1];
      else
        *Buffer = NULL;
      DiagMsg(DIAG_WARNING, "KMC rec full (code %d)", pCode->subscriber.rec_code);
      pthread_mutex_unlock(&(pHandle->lock));
      return PROTOCOL_STATUS_OK;
    }
    else
    {
      for( i = 0; i < pFrame->Size; i++ )
      {
        _CHARQ_ENQUEUE(&pCode->pool, payload[i]);
      }
      if(_CHARQ_SIZE(&pCode->pool) >= pCode->subscriber.send_size)
      {
        //signal_dequeue(pHandle);
        pthread_cond_signal( &(pHandle->dequeued) );
      }
        
    }
    pthread_mutex_unlock(&(pHandle->lock));

    if( (pFrame->Size + KMC_TX_FRAME_SIZE + 1 ) < lsize)
      *Buffer = &pBuffer[pFrame->Size + KMC_TX_FRAME_SIZE + 1];
    else
      *Buffer = NULL;
      
    *size -= pFrame->Size + KMC_TX_FRAME_SIZE + 1;
    
    return PROTOCOL_STATUS_OK;
  }
}

int32_t kmc_receive( Protocol_t * pHandle, uint8_t ** buffer, uint32_t * size )
{
  KMC_Rec_Handle_t * pKmcRecHandle = (KMC_Rec_Handle_t *) pHandle;
  //if(pKmcRecHandle->flag)
  //  return PROTOCOL_STATUS_BUSY;
  int32_t  ret = PROTOCOL_STATUS_OK;
  if (*buffer && (*size > 0))
  {
    ret = kmc_frame_parse(pKmcRecHandle, buffer, size);

    if(!ret)
    {
      pHandle->Diag.MSG_OK++;
    }
    else
    {
      if(ret == PROTOCOL_CODE_CRC_ERROR)
      {
        pHandle->Diag.CRC_ERROR++;
      }
      else if(ret == PROTOCOL_STATUS_INVALID_CODE)
        pHandle->Diag.UNKNOWN_MSG++;
      
      if( ret == PROTOCOL_STATUS_UNDERFLOW)
        pHandle->Diag.UNDERFLOW++;
      else
        pHandle->Diag.RX_ERROR++;
    }
  }
  return ret;
}

uint32_t kmc_rec_subscribe( KMC_Rec_Handle_t * pHandle, void* pCallback, KMC_Rec_Subscribe_t * msg )
{  
  int32_t i;
  uint8_t * queue;
  KMC_Code_List_t * last, * code;
  Msg_Keys_t * pOutMsg;
  Protocol_t * pProt = (Protocol_t *)pHandle;
  pHandle->flag = KMC_REC_BUSY;

  pthread_mutex_lock(&pHandle->lock);
  last = pHandle->codes;
  code = pHandle->codes;

  while(code)
  {
    if(code->subscriber.rec_code == msg->rec_code)
    { 
      if(msg->type == KMC_UNSUBSCRIBE_TYPE)
      {
        DiagMsg(DIAG_INFO, "Unsubscribing code %d", msg->rec_code);
        if(last)
          last->pNext = code->pNext;
        free(code->pool.QUEUE);
        free(code);
        for(i = 0; i< pProt->Size; i++)
        {
          if(pProt->Code[i] == msg->rec_code)
          {
            pProt->Code[i] = 0;
            for(i = i; i+1 < pProt->Size; i++)
              pProt->Code[i] = pProt->Code[i+1];
            pProt->Size--;
            break;
          }
        }
        if(!pProt->Size)
          pHandle->codes = NULL;
      }
      else
      {
        DiagMsg(DIAG_INFO, "Update subscription %d", msg->rec_code);
        code->subscriber.size = msg->size;
        code->subscriber.send_size = msg->send_size;
        code->subscriber.transaction_id = msg->transaction_id;
        pOutMsg = code->outmsg;
          pOutMsg->code = msg->code;
        pOutMsg->err_code = 0;
        pOutMsg->id = msg->transaction_id;
        pOutMsg->range = 0;
        pOutMsg->type = 0;
        pOutMsg->size = msg->send_size + sizeof(Msg_Keys_t);
        _CHARQ_CLEAR(&code->pool);
      }
      pHandle->flag = 0;
      pthread_mutex_unlock(&(pHandle->lock));
      return 0;
    }
    last = code;
    code = code->pNext;
  }

  if(msg->type != KMC_SUBSCRIBE_TYPE)
  {
    DiagMsg(DIAG_WARNING, "Not Subscribing for 0x%x", msg->rec_code);
    pHandle->flag = 0;
    pthread_mutex_unlock(&(pHandle->lock));
    return 0;
  }

  if(pProt->Size >= KMC_REC_MAX_CODES)
  {
    DiagMsg(DIAG_ERROR, "Kmc recorder subscription is full (size 0x%x)", pProt->Size);
    pHandle->flag = 0;
    pthread_mutex_unlock(&(pHandle->lock));
    return 0;
  }
  
  code = (KMC_Code_List_t*)malloc(sizeof(KMC_Code_List_t));
  if(code)
  {
    code->callback = pCallback;
    memcpy(&code->subscriber, msg, sizeof(KMC_Rec_Subscribe_t));
    
    code->lastCnt = -1;
    
    queue = (uint8_t*)malloc(msg->pool_size);   
    if(!queue)
    {
      DiagMsg(DIAG_ERROR, "Failed to cread kmc rec pool - Memory not available");
      pHandle->flag = 0;
      // TODO Null ptr pNext
      pthread_mutex_unlock(&(pHandle->lock));
      return 1;
    }
    _CHARQ_INIT(&code->pool, msg->pool_size, queue);

    code->outBuffer = (uint8_t*)malloc(msg->send_size) + sizeof(Msg_Keys_t);
    
    if(!code->outBuffer)
    {
      DiagMsg(DIAG_ERROR, "Failed to cread kmc rec outmsg - Memory not available");
      pHandle->flag = 0;
      // TODO Null ptr pNext
      pthread_mutex_unlock(&(pHandle->lock));
      return 1;
    }

    pOutMsg = (Msg_Keys_t*) code->outBuffer;
    code->outmsg = pOutMsg;
    pOutMsg->code = msg->code;
    pOutMsg->err_code = 0;
    pOutMsg->id = msg->transaction_id;
    pOutMsg->range = 0;
    pOutMsg->type = 0;
    pOutMsg->size = msg->send_size + sizeof(Msg_Keys_t); 
    code->pNext = NULL;
    if(last)
      last->pNext = code;
    else
      pHandle->codes = code;
    
    pProt->Code[pProt->Size++] = msg->rec_code;
    DiagMsg(DIAG_INFO, "KMC recorder started subscripiton on code 0x%X", msg->rec_code);
  }
  
  pHandle->flag = 0;
  
  pthread_mutex_unlock(&(pHandle->lock));
  return 0;
}

void kmc_rec_enable(KMC_Rec_Handle_t * pHandle, bool ena)
{
  KMC_Code_List_t * code = pHandle->codes;
  pthread_mutex_lock(&pHandle->lock);
  pHandle->enableCallback = ena;
  if(ena)
  {
    while(code)
    {
      code->lastCnt = -1;
      _CHARQ_CLEAR(&code->pool);
      code = code->pNext;        
    }
  }
  pthread_mutex_unlock(&pHandle->lock);
}

//todo handle unsubscribe
void kmc_rec_unsubscribe(KMC_Rec_Handle_t * pHandle)
{
  KMC_Code_List_t * tmp;
  KMC_Code_List_t * pCode = pHandle->codes; 
  pthread_mutex_lock(&pHandle->lock);
  pHandle->flag = KMC_REC_BUSY;
  pHandle->_super.Size = 0;
  while(pCode)
  {
    tmp = pCode;
    pCode = pCode->pNext;
    free(tmp->pool.QUEUE);
    free(tmp->outBuffer);
    free(tmp);
  }
  pHandle->codes = NULL;
  pHandle->flag = 0;
  pthread_mutex_unlock(&pHandle->lock);
}


void stop_rec_thread(KMC_Rec_Handle_t * pHandle)
{
  pthread_mutex_lock(&pHandle->lock);  
  pHandle->run = false;
  pthread_cond_signal(&pHandle->dequeued);
  pthread_mutex_unlock(&pHandle->lock);
  kmc_rec_unsubscribe(pHandle);
}


//void * getHandler(void)
// {
//   return (void *)&kmc_rec_handle;
// }
