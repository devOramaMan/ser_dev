

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


#define FILE_POOL_SIZE (uint32_t)0x10000
#define READ_FILE_SIZE (int16_t)0x7FFF
#define MAX_FILE_QUEUE 10

#define FCP_WRITE_FILE_CODE 0x04
#define FCP_READ_FILE_CODE  0x05



#define FCP_FILE_DONE 0x44


//uint8_t FILE_READ[READ_FILE_SIZE];

uint8_t FILE_WRITE[FILE_POOL_SIZE];

#define ERROR_ARGUMENTS 0x0E
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


//TODO Move to project configuration
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
  FCP_Frame_t fcp_msg;
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
  outMsg.buffer = (uint8_t*)&fcp_msg;

  pthread_mutex_lock(&pFHandler->lock);
  while (pFHandler->run)
  {
    if (!pFHandler->pFileQueue)
      pthread_cond_wait(&(pFHandler->enqueued), &(pFHandler->lock));

    queue = pFHandler->pFileQueue;
    sendData = pFHandler->fileRead + MSG_KEY_SIZE;

    while (queue)
    {      
      filesize = 0;
      receiveCode = 0;
      outMsg.code = queue->File.code;
      outMsg.id = queue->File.transaction_id;
      
      sendMsg->code = queue->File.code;
      sendMsg->id = queue->File.transaction_id;
      sendMsg->size = queue->File.size;
      recordNumber = 0;

      code = (queue->File.nodeId << 0x05);
      
      DiagMsg(DIAG_DEBUG, "file handler dequeue (id: %d, size: %d)", queue->File.transaction_id, queue->File.size );

      if(queue->File.code == READ_FILE_RECORD_TOPIC)
      {
        code |= FCP_READ_FILE_CODE  & ~(0xE0);
        
        outMsg.size = 5;
        fcp_msg.Size = 2; //size
        sendData = pFHandler->fileRead + MSG_KEY_SIZE;
      }
      else if (queue->File.code == WRITE_FILE_RECORD_TOPIC)
      {
        code |= FCP_WRITE_FILE_CODE  & ~(0xE0);
      }
      else
      {
        receiveCode = 1;
      }

      fcp_msg.Buffer[0] = queue->File.fileNum;

      fcp_msg.Code = code;

      while(!receiveCode)
      {
        fcp_msg.Buffer[1] = recordNumber++;
        if(queue->File.code == READ_FILE_RECORD_TOPIC)
        {
          fcp_msg.Buffer[2] = fcp_calc_crc(&fcp_msg);

          // Send data
          if(sendMsg->size > 0 && (filesize >= sendMsg->size))
          {
            ret = ((int32_t(*)(uint8_t*, uint32_t))queue->callback)((uint8_t*)sendMsg, sendMsg->size + sizeof( Msg_Keys_t ));
            if(!ret)
            {
              tmp = sendData;
              filesize = filesize - sendMsg->size;
              sendData = pFHandler->fileRead + MSG_KEY_SIZE;
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
        else if( queue->File.code == WRITE_FILE_RECORD_TOPIC )
        {
          tmp = &fcp_msg.Buffer[2];
          for(i = 2; (i < 128) && (filesize < queue->File.size); i++)
          {
             _CHARQ_DEQUEUE( &pFHandler->fileWrite, *tmp++ );
            filesize++;
          }
          fcp_msg.Size = i;
          outMsg.size = fcp_msg.Size + 3;
          *tmp = fcp_calc_crc(&fcp_msg);
        }
        append_send_msg(&outMsg);

        time_to_wait.tv_sec = time(NULL) + pFHandler->timeout;
        if(ETIMEDOUT == pthread_cond_timedwait(&(pFHandler->dequeued), &(pFHandler->lock), &time_to_wait))
        {
          receiveCode = PROTOCOL_CODE_TIMEOUT;
          sendMsg->err_code = PROTOCOL_CODE_TIMEOUT;
          DiagMsg(DIAG_WARNING, "Msg timeout (ptr: %p id %d)", queue, queue->File.transaction_id);
        }
        else
        {
          receiveCode = pFHandler->lastResponse.keys.err_code;
          if(queue->File.code == READ_FILE_RECORD_TOPIC)
          {
            lastSize = pFHandler->lastResponse.keys.size - MSG_KEY_SIZE;
            if ( lastSize < 0 )
            {
              DiagMsg(DIAG_ERROR, "Msg Size error");
              receiveCode = FCP_FILE_DONE;
            }
            else
            {
              filesize += lastSize;
              for( i = 0; i < lastSize; i++ )
                *sendData++ = pFHandler->lastResponse.data[i];
              
              if( lastSize < 128 )
                receiveCode = FCP_FILE_DONE;
            }
          }
          else if (queue->File.code == WRITE_FILE_RECORD_TOPIC)
          {
            if( filesize >= queue->File.size )
            {
              receiveCode = FCP_FILE_DONE;
            }
          }          
          
          sendMsg->err_code = pFHandler->lastResponse.keys.err_code;
        }
      }

      if( queue->File.code == WRITE_FILE_RECORD_TOPIC )
      {
        filesize = sizeof( Msg_Keys_t );
        ret = ((int32_t(*)(uint8_t*, uint32_t))queue->callback)((uint8_t*)sendMsg, filesize );
        DiagMsg(DIAG_DEBUG, "File Write response (size %d, err_code %d, ret %d)", filesize , sendMsg->err_code, ret);
      }
      else if ( queue->File.code == READ_FILE_RECORD_TOPIC )
      {
        //TODO bad file complete nack code For now handle here
        if(filesize > 0 && (sendMsg->err_code == ERROR_ARGUMENTS) )
          sendMsg->err_code = 0;
        if( filesize > 0 || sendMsg->err_code )
          filesize = filesize + sizeof( Msg_Keys_t );
        
        if(filesize)
        {
          ret = ((int32_t(*)(uint8_t*, uint32_t))queue->callback)((uint8_t*)sendMsg, filesize );
          DiagMsg(DIAG_DEBUG, "File sent last or whole (size %d, err_code %d, ret %d)", filesize, sendMsg->err_code, ret);
        }
      }
      else
      {
        sendMsg->err_code  = PROTOCOL_STATUS_INVALID_CODE;
      }
      

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
  pthread_mutex_lock(&(file_handler.lock));
  if (file_handler.fileCnt >= MAX_FILE_QUEUE)
  {
    pthread_mutex_unlock(&(file_handler.lock));
    return 1;
  }    

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


int32_t append_write_file( void * callback, File_Type_t * file )
{
  int32_t i;
  uint8_t * buffer;
  File_Queue_t * queueItem;
  pthread_mutex_lock(&(file_handler.lock));
  
  if ( (file_handler.fileCnt >= MAX_FILE_QUEUE) || (_CHARQ_AVAILABLE_SIZE(&file_handler.fileWrite) < file->size) )
  {
    DiagMsg(DIAG_WARNING, "Failed to append Write file request (filecnt %d, writefile size %d)", file_handler.fileCnt, _CHARQ_SIZE(&file_handler.fileWrite) );
    pthread_mutex_unlock(&(file_handler.lock));
    return 1;
  }

  queueItem = file_handler.pFileQueue;

  buffer = (uint8_t*) file + sizeof(File_Type_t);

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

  for( i = 0; i < queueItem->File.size; i++ )
  {
    _CHARQ_ENQUEUE(&file_handler.fileWrite, *buffer++);
  }

  DiagMsg(DIAG_DEBUG, "Enqueue write file (id %d, size %d)", file->transaction_id, file->size);

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
