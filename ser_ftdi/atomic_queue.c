

#include "atomic_queue.h"
#include "diagnostics_util.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <protocol_common.h>
#include "config.h"

#define MAX_QUEUE 100

typedef enum
{
  SINGLE_MSG,
  RECUR_MSG
}MSG_TYPE;

typedef enum
{
  OK_MSG,
  SENDING,
  TIMEOUT,
  SENDING_ERROR,
  DEV_ERROR  
}MSG_STAT;

typedef struct Payload
{
  uint32_t size; /* data */
  uint8_t *data;
} Payload_t;



typedef struct Msg
{
  Msg_Base_t _base;
  uint8_t type;
  uint8_t stat;
  uint32_t delay;
  Payload_t send;
  void * callback;
  struct Msg *next;
} Msg_t;

typedef struct
{
  Msg_t *queue;
  pthread_cond_t enqueued;
  pthread_cond_t dequeued;
  pthread_mutex_t lock;
  pthread_t threadId;
  volatile bool run;
}Queue_Handler_t;

typedef struct
{
  Queue_Handler_t send;
  Queue_Handler_t receive;
  uint8_t in_code;
  Payload_t in_box;
  uint32_t MsgCnt;
  uint32_t timeout;
  uint32_t transactionCnt;
  uint32_t retry;
  RET_RX_TX_FUNC sendFunc; 
} Atomic_Queue_Handler_t;

Atomic_Queue_Handler_t atomic_queue_handler =
{
  .send.queue = NULL,
  .send.enqueued = PTHREAD_COND_INITIALIZER,
  .send.dequeued = PTHREAD_COND_INITIALIZER,
  .send.lock = PTHREAD_MUTEX_INITIALIZER,
  .send.run = false,
  .receive.queue = NULL,
  .receive.enqueued = PTHREAD_COND_INITIALIZER,
  .receive.dequeued = PTHREAD_COND_INITIALIZER,
  .receive.lock = PTHREAD_MUTEX_INITIALIZER,
  .receive.run = false,
  .in_code = 0,
  .in_box = {0, NULL},
  .MsgCnt = 0,
  .timeout = 3,
  .retry = 0  
};

void *send_thread(void *arg);
void *receive_thread(void *arg);

void *send_thread(void *arg)
{
  static struct timespec time_to_wait = {0, 0};
  int32_t retry;
  int32_t stat;
  uint32_t sentBytes, i;
  Atomic_Queue_Handler_t *pAHandler = (Atomic_Queue_Handler_t *)arg;
  Queue_Handler_t *pQHandler = (Queue_Handler_t *)&pAHandler->send;
  Msg_t * msg, * rec_msg, *free_msg;

  DiagMsg(DIAG_DEBUG, "Send Queue handler started (thread Id %p)", pQHandler->threadId);

  pthread_mutex_lock(&pQHandler->lock);
  while (pQHandler->run)
  {
    if (!pQHandler->queue)
      pthread_cond_wait(&(pQHandler->enqueued), &(pQHandler->lock));

    msg = pQHandler->queue;

    while (msg)
    {
      retry = pAHandler->retry;
      stat = 1;
      while (retry > -1 && stat)
      {
        stat = 0;
        retry--;
      }

      DiagMsg(DIAG_DEBUG, "Msg thread Id %p dequeue msg %p id %d", pQHandler->threadId, msg, msg->_base.id);
      // Todo handle send delay
      // Wait for received condition
      time_to_wait.tv_sec = time(NULL) + pAHandler->timeout;
      stat = pAHandler->sendFunc(msg->send.data, msg->send.size, &sentBytes);
      if(!stat)
      {
        msg->_base.err_code = 0;
        msg->_base.size = (uint8_t)(sizeof(msg->_base.size) \
                                  + sizeof(msg->_base.err_code) \
                                  + sizeof(msg->_base.spare) \
                                  + sizeof(msg->_base.id));
        if(msg->send.size != sentBytes)
          DiagMsg(DIAG_WARNING, "Inconsistend send data %d != %d", msg->send.size, sentBytes);
        if(ETIMEDOUT == pthread_cond_timedwait(&(pQHandler->dequeued), &(pQHandler->lock), &time_to_wait))
        {
          msg->stat = TIMEOUT;
          msg->_base.err_code = PROTOCOL_CODE_TIMEOUT;
          DiagMsg(DIAG_WARNING, "Msg timeout %p id %d", msg, msg->_base.id);
        }
        else
        { 
          DiagMsg(DIAG_DEBUG, "Msg received %p id %d", msg, msg->_base.id);
          if(!pAHandler->in_code)
          {
            msg->_base.size += (uint8_t)pAHandler->in_box.size;
            
            for(i=0;i<pAHandler->in_box.size;i++)
              msg->_base.data[i] = pAHandler->in_box.data[i];
            
            msg->stat = PROTOCOL_STATUS_OK;
          }
          else
          {
            msg->stat = PROTOCOL_STATUS_NACK;
            msg->_base.err_code = pAHandler->in_code;
          }
        }
      }
      else
      {
        msg->stat = SENDING_ERROR;
        DiagMsg(DIAG_ERROR, "Failed to send msg (err %d)", stat);
      }
      free_msg = msg;
      msg = msg->next;
      free_msg->next = NULL;

      //Enqueue receive thread handling
      pthread_mutex_lock(&(pAHandler->receive.lock));
      if(pAHandler->receive.queue)
      {
        rec_msg = pAHandler->receive.queue;
        while(rec_msg->next)
          rec_msg = rec_msg->next;
        rec_msg->next = free_msg;
      }      
      else
        pAHandler->receive.queue = free_msg;

      pthread_cond_signal( &(pAHandler->receive.enqueued));

      pthread_mutex_unlock(&(pAHandler->receive.lock));
    }
    pQHandler->queue = NULL;
  }
  pthread_mutex_unlock(&(pQHandler->lock));
  DiagMsg(DIAG_DEBUG, "Send thread handler done (Id %p)", pQHandler->threadId);
  return NULL;
}

void *receive_thread(void *arg)
{
  int32_t ret;
  Atomic_Queue_Handler_t *pAHandler = (Atomic_Queue_Handler_t *)arg;
  Queue_Handler_t *pQHandler = (Queue_Handler_t *)&pAHandler->receive;

  DiagMsg(DIAG_DEBUG, "Receive Queue handler started (thread Id %p)", pQHandler->threadId);

  while (pQHandler->run)
  {
    pthread_mutex_lock(&pQHandler->lock);
    if (!pQHandler->queue)
      pthread_cond_wait(&(pQHandler->enqueued), &(pQHandler->lock));
    
    DiagMsg(DIAG_DEBUG, "Handle receive thread");
    
    Msg_t * msg = pQHandler->queue;
    if (msg)
    {
      pQHandler->queue = msg->next;
    }
    pthread_mutex_unlock(&(pQHandler->lock));
    if (msg)
    {
      //TODO send msg to API
      
      if(msg->callback)
      {
        if(msg->stat == PROTOCOL_STATUS_OK)
        {
          ret = ((int32_t(*)(uint8_t*, uint8_t))msg->callback)((uint8_t*)msg, msg->_base.size + 1 );
        }
        else
        {
          if(!msg->_base.err_code)
          {
            //internal processing error
            msg->_base.err_code = msg->stat;
          }
          ret = ((int32_t(*)(uint8_t*, uint8_t))msg->callback)((uint8_t*)msg, msg->_base.size + 1);
        }
          
        
        if(ret)
        {
          DiagMsg(DIAG_ERROR, "Failed to forward msg from dev to API (id %d, err code %d)", msg->_base.id, msg->_base.err_code);
        }
        else
          DiagMsg(DIAG_DEBUG, "Forward msg to API from dev (id %d, err code %d)", msg->_base.id, msg->_base.err_code);
      }
      if(msg->send.data)
        free(msg->send.data);
      free(msg);
    }
  }
  DiagMsg(DIAG_DEBUG, "Receive thread handler done (Id %p)", pQHandler->threadId);
  return NULL;
}

void start_queue_thread(RET_RX_TX_FUNC sendFunc)
{
  int32_t stat, ret;
  pthread_attr_t attr;
  struct sched_param sp;
  stat = pthread_attr_init(&attr);
  if (stat)
    DiagMsg(DIAG_ERROR, "send receive queue thread attrib init failed (%d)", stat);

  stat |= pthread_attr_getschedparam(&attr, &sp);
  if (stat)
    DiagMsg(DIAG_ERROR, "send receive queue thread attrib prio failed ");
  else
    DiagMsg(DIAG_DEBUG, "Queue thread priority %d", sp.sched_priority);

  atomic_queue_handler.sendFunc = sendFunc;
  atomic_queue_handler.send.run = true;

  //start Send thread
  if(!stat)
    ret = pthread_create(&(atomic_queue_handler.send.threadId), &attr, &send_thread, (void *)&atomic_queue_handler);
  else
  {
    DiagMsg(DIAG_WARNING,"start send thread without attrib");
    ret = pthread_create(&(atomic_queue_handler.send.threadId), NULL, &send_thread, (void *)&atomic_queue_handler);
  }

  if(ret)
  {
    DiagMsg(DIAG_ERROR, "Failed to start send queue");
    return;
  }
  atomic_queue_handler.receive.run = true;
  //start Receive thread    
  if(!stat)
    ret = pthread_create(&(atomic_queue_handler.receive.threadId), &attr, &receive_thread, (void *)&atomic_queue_handler);
  else
  {
    DiagMsg(DIAG_WARNING,"start receive thread without attrib");
    ret = pthread_create(&(atomic_queue_handler.receive.threadId), NULL, &receive_thread, (void *)&atomic_queue_handler);
  }


  if(ret)
  {
    DiagMsg(DIAG_ERROR, "Failed to start receive queue");
    stop_queue_thread();
    return;
  }
  
}

void stop_queue_thread(void)
{
  //stop send queue thread
  pthread_mutex_lock( &(atomic_queue_handler.send.lock));
  atomic_queue_handler.send.run = false;
  pthread_cond_signal( &(atomic_queue_handler.send.dequeued) );
  pthread_mutex_unlock( &(atomic_queue_handler.send.lock) );

  //stop receive queue thread
  pthread_mutex_lock( &(atomic_queue_handler.receive.lock));
  atomic_queue_handler.receive.run = false;  
  pthread_cond_signal( &(atomic_queue_handler.receive.dequeued) );
  pthread_mutex_unlock( &(atomic_queue_handler.receive.lock) );
}

int32_t signal_received(uint8_t * buffer, uint32_t size, uint32_t err_code)
{
  pthread_mutex_lock( &(atomic_queue_handler.send.lock));
  atomic_queue_handler.in_box.data = buffer;
  atomic_queue_handler.in_box.size = size;
  atomic_queue_handler.in_code = (uint8_t) err_code;
  pthread_cond_signal( &(atomic_queue_handler.send.dequeued) );
  pthread_mutex_unlock( &(atomic_queue_handler.send.lock) );
  return 0;
}

int32_t append_send_queue( uint8_t *buffer, uint32_t size )
{
  int32_t i = 0;
  int32_t addCnt = 0;
  pthread_mutex_lock(&(atomic_queue_handler.send.lock));
  Msg_t **msg = &(atomic_queue_handler.send.queue);

  while ((*msg) && (i++ < MAX_QUEUE))
  {
    msg = &(*msg)->next;
    i++;
  }

  if (i < MAX_QUEUE && !(*msg))
  {
    *msg = (Msg_t *)malloc(sizeof(Msg_t));
    if (*msg)
    {
      (*msg)->_base.id = atomic_queue_handler.transactionCnt++;
      (*msg)->next = NULL;
      (*msg)->callback = NULL;
      (*msg)->type = (uint8_t) SINGLE_MSG;
      (*msg)->stat = (uint8_t) SENDING;
      (*msg)->_base.code = 0;
      (*msg)->_base.err_code = 0;
      (*msg)->send.size = size;
      if(size)
      {
        (*msg)->send.data = (uint8_t *)malloc(size);
        for(i=0; i<size;i++)
          (*msg)->send.data[i] = buffer[i];
      }
      else
        (*msg)->send.data = NULL;
      (*msg)->_base.size = 0;
      addCnt++;
    }
  }

  if (addCnt)
  {
    DiagMsg(DIAG_DEBUG, "Msg enqueue msg %p id %d", *msg, (*msg)->_base.id);
    pthread_cond_signal( &(atomic_queue_handler.send.enqueued) );
  }
  else
  {
    i = 0;
  }

  pthread_mutex_unlock(&(atomic_queue_handler.send.lock));
  return addCnt;
}

int32_t append_send_msg( const Atomic_Queue_Msg_t * amsg )
{
   int32_t i = 0;
  int32_t addCnt = 0;
  pthread_mutex_lock(&(atomic_queue_handler.send.lock));
  Msg_t **msg = &(atomic_queue_handler.send.queue);

  while ((*msg) && (i++ < MAX_QUEUE))
  {
    msg = &(*msg)->next;
    i++;
  }

  if (i < MAX_QUEUE && !(*msg))
  {
    *msg = (Msg_t *)malloc(sizeof(Msg_t));
    if (*msg)
    {
      (*msg)->_base.id = amsg->id;
      (*msg)->callback = amsg->callback;
      (*msg)->next = NULL;
      
      (*msg)->type = (uint8_t) SINGLE_MSG;
      (*msg)->stat = (uint8_t) SENDING;
      (*msg)->_base.code = amsg->code;
      (*msg)->_base.err_code = 0;
      (*msg)->send.size = amsg->size;
      if(amsg->size)
      {
        (*msg)->send.data = (uint8_t *)malloc(amsg->size);
        for(i=0; i<amsg->size;i++)
          (*msg)->send.data[i] = amsg->buffer[i];
      }
      else
        (*msg)->send.data = NULL;
      (*msg)->_base.size = 0;
      addCnt++;
    }
  }

  if (addCnt)
  {
    DiagMsg(DIAG_DEBUG, "Msg enqueue msg %p id %d", *msg, (*msg)->_base.id);
    pthread_cond_signal( &(atomic_queue_handler.send.enqueued) );
  }
  else
  {
    i = 0;
  }

  pthread_mutex_unlock(&(atomic_queue_handler.send.lock));
  return addCnt;
}
