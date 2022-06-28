

#include "atomic_queue.h"
#include "file_handler.h"
#include "diagnostics_util.h"
#include "fcp_frame_protocol.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <protocol_common.h>
#include "config.h"
#include "charq.h"
#include <string.h>


#define FILE_POOL_SIZE 0x10000
#define READ_FILE_SIZE 0x7FFF
#define MAX_FILE_QUEUE 10
#define FCP_READ_FILE_CODE 0x05
#define FCP_READ_FILE_DONE 0x44


//uint8_t FILE_READ[READ_FILE_SIZE];

uint8_t FILE_WRITE[FILE_POOL_SIZE];


typedef struct  File_Queue
{
  File_Type_t File;
  void * callback;
  struct File_Queue * pNext;
}File_Queue_t;

File_Queue_t file_queue[MAX_FILE_QUEUE];

typedef struct
{
  pthread_cond_t enqueued;
  pthread_cond_t dequeued;
  pthread_mutex_t lock;
  pthread_t threadId;
  uint32_t MsgCnt;
  uint32_t timeout;
  void * pCallback;
  volatile bool run;
  File_Queue_t * pFileQueue;
  Msg_Base_t lastResponse;
  uint8_t fileCnt;
  uint8_t fileRead[READ_FILE_SIZE];;
  CHARQ_STRUCT fileWrite;
}File_Handler_t;

int32_t file_response(uint8_t * buffer, uint32_t size);


File_Handler_t file_handler =
{
  .enqueued = PTHREAD_COND_INITIALIZER,
  .dequeued = PTHREAD_COND_INITIALIZER,
  .lock = PTHREAD_MUTEX_INITIALIZER,
  .timeout = 3,
  .run = false,
  .MsgCnt = 0,
  .pFileQueue = NULL,
  .fileCnt = 0,
  .fileWrite = { FILE_POOL_SIZE,  0,  0,  0, FILE_WRITE }
};

void *file_thread(void *arg);

void *file_thread(void *arg)
{
  static struct timespec time_to_wait = {0, 0};
  //int32_t stat;
  uint32_t i;
  //uint32_t sentBytes, 
  File_Handler_t *pFHandler = (File_Handler_t *)arg;
  File_Queue_t * queue;
  uint8_t recordNumber;
  uint8_t readBuffer[4];
  uint32_t filesize;
  uint8_t receiveCode;
  Atomic_Queue_Msg_t outMsg;
  uint8_t code;
  Msg_Keys_t * sendMsg = (Msg_Keys_t *)pFHandler->fileRead;
  uint8_t * sendData, * tmp;
  int32_t ret;
  int32_t lastSize;

  DiagMsg(DIAG_DEBUG, "File handler started (thread Id %p)", pFHandler->threadId);

  outMsg.callback = (void*)file_response;

  pthread_mutex_lock(&pFHandler->lock);
  while (pFHandler->run)
  {
    if (!pFHandler->pFileQueue)
      pthread_cond_wait(&(pFHandler->enqueued), &(pFHandler->lock));

    queue = pFHandler->pFileQueue;

    while (queue)
    {      
      filesize = 0;
      receiveCode = 0;
      outMsg.code = queue->File.code;
      outMsg.id = queue->File.transaction_id;
      code = (queue->File.nodeId << 5);
      sendMsg->code = queue->File.code;
      sendMsg->id = queue->File.transaction_id;
      sendMsg->size = queue->File.size;
      recordNumber = 0;
      
      DiagMsg(DIAG_DEBUG, "file handler dequeue (id: %d, size: %d)", queue->File.transaction_id, queue->File.size );

      if(queue->File.code == READ_FILE_RECORD_TOPIC)
      {
        code |= FCP_READ_FILE_CODE  & ~(0xE0);
        outMsg.buffer = readBuffer;
        outMsg.size = 5;
        readBuffer[0] = code;
        readBuffer[1] = 2; //size
        readBuffer[2] = queue->File.fileNum;
        sendData = pFHandler->fileRead + sizeof(sendMsg);
      }

      while(!receiveCode)
      {
        if(queue->File.code == READ_FILE_RECORD_TOPIC)
        {
          readBuffer[3] = recordNumber++;
          readBuffer[4] = fcp_calc_crc((FCP_Frame_t*)readBuffer);
          append_send_msg(&outMsg);
        }

        time_to_wait.tv_sec = time(NULL) + pFHandler->timeout;
        if(ETIMEDOUT == pthread_cond_timedwait(&(pFHandler->dequeued), &(pFHandler->lock), &time_to_wait))
        { 
          receiveCode = PROTOCOL_CODE_TIMEOUT;
          sendMsg->err_code = PROTOCOL_CODE_TIMEOUT;
          DiagMsg(DIAG_WARNING, "Msg timeout (ptr: %p id %d)", queue, queue->File.transaction_id);
        }
        else
        {
          if(queue->File.code == READ_FILE_RECORD_TOPIC)
          {
            lastSize = pFHandler->lastResponse.keys.size - (uint8_t)(sizeof(Msg_Keys_t)-1);
            if (lastSize < 0)
            {
              DiagMsg(DIAG_ERROR, "Msg Size error");
              receiveCode = FCP_READ_FILE_DONE;
            }
            else
            {
              filesize += lastSize;
              for(i = 0; i < lastSize; i++)
                *sendData++ = pFHandler->lastResponse.data[i];
              
              if(lastSize < 128)
                receiveCode = FCP_READ_FILE_DONE;
            }
          }
          receiveCode = pFHandler->lastResponse.keys.err_code;
        }
        // Send data
        if(sendMsg->size > 0 && (filesize >= sendMsg->size))
        {
          ret = ((int32_t(*)(uint8_t*, uint32_t))queue->callback)((uint8_t*)sendMsg, sendMsg->size + sizeof( Msg_Keys_t ));
          if(!ret)
          {
            tmp = sendData;
            filesize = filesize - sendMsg->size;
            sendData = pFHandler->fileRead + sizeof(sendMsg);
            for(i=0;i<filesize;i++)
            {
              *sendData++ = *tmp++;
            }
          }
          else
          {
            receiveCode = ret;
            DiagMsg(DIAG_ERROR, "Failed to send file response part");
          }
        }
      }

      if(!ret)
      {
        ret = ((int32_t(*)(uint8_t*, uint32_t))queue->callback)((uint8_t*)sendMsg, filesize + sizeof( Msg_Keys_t ));
        DiagMsg(DIAG_DEBUG, "File sent last or whole (size %d)", filesize + sizeof( Msg_Keys_t ));
      }

      if(ret)
        DiagMsg(DIAG_ERROR, "Failed to send file response");


      // Todo handle send delay
      // Wait for received condition
      

      
      //stat = pAHandler->sendFunc(msg->send.data, msg->send.size, &sentBytes);
      //if(!stat)
      //{
        // queue->_base.err_code = 0;
        // queue->_base.size = (uint8_t)(sizeof(msg->_base.size) 
        //                           + sizeof(msg->_base.err_code) 
        //                           + sizeof(msg->_base.spare) 
        //                           + sizeof(msg->_base.id));
        // if(queue->send.size != sentBytes)
        //   DiagMsg(DIAG_WARNING, "Inconsistend send data %d != %d", msg->send.size, sentBytes);
        if(ETIMEDOUT == pthread_cond_timedwait(&(pFHandler->dequeued), &(pFHandler->lock), &time_to_wait))
        {
        //   msg->stat = TIMEOUT;
        //   msg->_base.err_code = PROTOCOL_CODE_TIMEOUT;
           DiagMsg(DIAG_WARNING, "Msg timeout (ptr: %p id %d)", queue, queue->File.transaction_id);
        }
        // else
        // { 
        //   DiagMsg(DIAG_DEBUG, "Msg received %p id %d", msg, msg->_base.id);
        //   if(!pAHandler->in_code)
        //   {
        //     msg->_base.size += (uint8_t)pAHandler->in_box.size;
            
        //     for(i=0;i<pAHandler->in_box.size;i++)
        //       msg->_base.data[i] = pAHandler->in_box.data[i];
            
        //     msg->stat = PROTOCOL_STATUS_OK;
        //   }
        //   else
        //   {
        //     msg->stat = PROTOCOL_STATUS_NACK;
        //     msg->_base.err_code = pAHandler->in_code;
        //   }
        // }
      // }
      // else
      // {
      //   //msg->stat = SENDING_ERROR;
      //   DiagMsg(DIAG_ERROR, "Failed to send msg (err %d)", stat);
      // }
      // free_msg = msg;
      // msg = msg->next;
      // free_msg->next = NULL;

      //Enqueue receive thread handling
      //pthread_mutex_lock(&(pFHandler->lock));
      // if(pAHandler->receive.queue)
      // {
      //   rec_msg = pAHandler->receive.queue;
      //   while(rec_msg->next)
      //     rec_msg = rec_msg->next;
      //   rec_msg->next = free_msg;
      // }      
      // else
      //   pAHandler->receive.queue = free_msg;

      //pthread_cond_signal( &(pFHandler->enqueued));

      //pthread_mutex_unlock(&(pFHandler->lock));
      queue = queue->pNext;
      file_handler.fileCnt--;
    }
    pFHandler->pFileQueue = NULL;
  }
  pthread_mutex_unlock(&(pFHandler->lock));
  DiagMsg(DIAG_DEBUG, "File thread handler done (Id %p)", pFHandler->threadId);
  return NULL;
}


void start_file_thread(void)
{
  int32_t stat, ret;
  pthread_attr_t attr;
  struct sched_param sp;

  memset((void*)file_queue, 0 , sizeof(file_queue));
  
  stat = pthread_attr_init(&attr);
  if (stat)
    DiagMsg(DIAG_ERROR, "file thread attrib init failed (%d)", stat);

  stat |= pthread_attr_getschedparam(&attr, &sp);
  if (stat)
    DiagMsg(DIAG_ERROR, "file thread attrib prio failed ");
  else
    DiagMsg(DIAG_DEBUG, "file thread priority %d", sp.sched_priority);

 // atomic_queue_handler.sendFunc = sendFunc;
  file_handler.run = true;

  //start Send thread
  if(!stat)
    ret = pthread_create(&(file_handler.threadId), &attr, &file_thread, (void *)&file_handler);
  else
  {
    DiagMsg(DIAG_WARNING,"start file thread without attrib");
    ret = pthread_create(&(file_handler.threadId), NULL, &file_thread, (void *)&file_handler);
  }


  if(ret)
  {
    DiagMsg(DIAG_ERROR, "Failed to start file handler");
    stop_queue_thread();
    return;
  }
  
}

void stop_file_thread(void)
{
  pthread_mutex_lock( &(file_handler.lock));
  file_handler.run = false;
  pthread_cond_signal( &(file_handler.dequeued) );
  pthread_mutex_unlock( &(file_handler.lock) );

}


int32_t append_read_file( void * callback, File_Type_t * file )
{
  //int32_t i = 0;
  //int32_t addCnt = 0;
  pthread_mutex_lock(&(file_handler.lock));
  if (file_handler.fileCnt >= MAX_FILE_QUEUE)
    return 1;

  File_Queue_t * queueItem = file_handler.pFileQueue;
  if(!queueItem)
  {
    file_handler.pFileQueue = file_queue;
    queueItem = file_handler.pFileQueue;
    queueItem->pNext = NULL;
  }

  while (queueItem->pNext)
  {
    queueItem = queueItem->pNext;
  }

  file_handler.fileCnt++;

    
  memcpy((void*)queueItem,  file, sizeof(File_Type_t));
  queueItem->pNext = NULL;
  queueItem->callback = callback;  

  DiagMsg(DIAG_DEBUG, "Enqueue read file (id %d)", file->transaction_id);
  pthread_cond_signal( &(file_handler.enqueued) );

  pthread_mutex_unlock(&(file_handler.lock));
  return 0;
}


int32_t file_response(uint8_t * buffer, uint32_t size)
{
  
  pthread_mutex_lock(&(file_handler.lock));
  memcpy(&file_handler.lastResponse, buffer,size);
  pthread_cond_signal( &(file_handler.dequeued) );
  pthread_mutex_unlock(&(file_handler.lock));
  return 0;
}
