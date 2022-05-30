

#include "atomic_queue.h"
#include "diagnostics_util.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <protocol_common.h>

#define MAX_QUEUE 100

typedef struct Msg
{
  uint32_t id;
  uint32_t size; /* data */
  uint8_t *data;
  struct Msg *next;
} Msg_t;

typedef struct
{
  Msg_t *head;
  uint32_t MsgCnt;
  pthread_cond_t enqueued;
  pthread_cond_t dequeued;
  pthread_mutex_t lock;
  pthread_t threadId;
  uint32_t timeout;
  volatile bool run;
  uint32_t transactionCnt;
  uint32_t retry;
  RET_RX_TX_FUNC sendFunc; 
} Atomic_Queue_Handler_t;

Atomic_Queue_Handler_t atomic_queue_handler =
{
  .head = NULL,
  .MsgCnt = 0,
  .enqueued = PTHREAD_COND_INITIALIZER,
  .dequeued = PTHREAD_COND_INITIALIZER,
  .lock = PTHREAD_MUTEX_INITIALIZER,
  .timeout = 3,
  .retry = 0,
  .run = false
};

void *thread_queue(void *arg);

void *thread_queue(void *arg)
{
  static struct timespec time_to_wait = {0, 0};
  int32_t retry;
  int32_t stat;
  uint32_t sentBytes;
  Atomic_Queue_Handler_t *pQHandler = (Atomic_Queue_Handler_t *)arg;
  pthread_mutex_lock(&pQHandler->lock);
  while (pQHandler->run)
  {
    if (!pQHandler->MsgCnt)
      pthread_cond_wait(&(pQHandler->enqueued), &(pQHandler->lock));

    Msg_t *msg = pQHandler->head;
    while (msg)
    {
      retry = pQHandler->retry;
      stat = 1;
      while (retry > -1 && stat)
      {
        stat = 0;
        retry--;
      }

      DiagMsg(DIAG_DEBUG, "Msg thread Id %p dequeue msg %p id %d", pQHandler->threadId, msg, msg->id);
      // Todo handle send delay
      // Wait for received condition
      time_to_wait.tv_sec = time(NULL) + pQHandler->timeout;
      stat = pQHandler->sendFunc(msg->data, msg->size, &sentBytes);
      if(!stat)
      {
        if(msg->size != sentBytes)
          DiagMsg(DIAG_ERROR, "Inconsistend send data %d != %d", msg->size, sentBytes);
        if(ETIMEDOUT == pthread_cond_timedwait(&(pQHandler->dequeued), &(pQHandler->lock), &time_to_wait))
          DiagMsg(DIAG_WARNING, "Msg timeout %p id %d", msg, msg->id);
        else
        {
          DiagMsg(DIAG_DEBUG, "Msg received %p id %d", msg, msg->id);
        }
      }
      else
      {
        DiagMsg(DIAG_ERROR, "Failed to send msg (err %d)", stat);
      }

      msg = msg->next;
    }

    msg = pQHandler->head;
    while (msg)
    {
      Msg_t *msg_free = msg;
      msg = msg->next;

      free(msg_free->data);
      free(msg_free);
    }

    pQHandler->head = NULL;
    DiagMsg(DIAG_DEBUG, "Msg thread Id %p done", pQHandler->threadId);
  }
  pthread_mutex_unlock(&(pQHandler->lock));
  return NULL;
}

void start_queue_thread(RET_RX_TX_FUNC sendFunc)
{
  int32_t stat;
  pthread_attr_t attr;
  struct sched_param sp;
  stat = pthread_attr_init(&attr);
  if (stat)
    DiagMsg(DIAG_ERROR, "send receive queue thread attrib init failed (%d)", stat);

  stat |= pthread_attr_getschedparam(&attr, &sp);
  if (stat)
    DiagMsg(DIAG_ERROR, "send receive queue thread attrib prio failed ");

  DiagMsg(DIAG_DEBUG, "thread priority %d", sp.sched_priority);

  atomic_queue_handler.sendFunc = sendFunc;
  atomic_queue_handler.run = true;
  if(!stat)
    stat = pthread_create(&(atomic_queue_handler.threadId), &attr, &thread_queue, (void *)&atomic_queue_handler);
  else
    stat = pthread_create(&(atomic_queue_handler.threadId), NULL, &thread_queue, (void *)&atomic_queue_handler);

  if(stat)
    DiagMsg(DIAG_ERROR, "Failed to start send receive queue");

  
}

void stop_queue_thread(void)
{
  pthread_mutex_lock( &(atomic_queue_handler.lock));
  atomic_queue_handler.run = false;
  pthread_mutex_unlock( &(atomic_queue_handler.lock) );
}

void signal_received(void)
{
  pthread_mutex_lock( &(atomic_queue_handler.lock));
  pthread_cond_signal( &(atomic_queue_handler.dequeued) );
  pthread_mutex_unlock( &(atomic_queue_handler.lock) );
}

int32_t append_send_queue( uint8_t *buffer, uint32_t size)
{
  int32_t i = 0;
  int32_t addCnt = 0;
  pthread_mutex_lock(&(atomic_queue_handler.lock));
  Msg_t **msg = &(atomic_queue_handler.head);

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
      (*msg)->id = atomic_queue_handler.transactionCnt++;
      (*msg)->next = NULL;
      (*msg)->size = size;
      (*msg)->data = (uint8_t *)malloc(size);
      addCnt++;
    }
  }

  if (addCnt)
  {
    DiagMsg(DIAG_DEBUG, "Msg enqueue msg %p id %d", *msg, (*msg)->id);
    pthread_cond_signal( &(atomic_queue_handler.enqueued) );
  }
  else
  {
    i = 0;
  }

  pthread_mutex_unlock(&(atomic_queue_handler.lock));
  return addCnt;
}
