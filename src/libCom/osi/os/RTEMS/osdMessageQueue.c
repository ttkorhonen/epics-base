/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/*
 *      $Id$
 *
 *      Author  W. Eric Norum
 *              norume@aps.anl.gov
 *              630 252 4793
 */

/*
 * We want to access information which is
 * normally hidden from application programs.
 */
#define __RTEMS_VIOLATE_KERNEL_VISIBILITY__ 1

#define epicsExportSharedSymbols
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <rtems.h>
#include <rtems/error.h>
#include "epicsMessageQueue.h"
#include "errlog.h"

epicsShareFunc epicsMessageQueueId epicsShareAPI
epicsMessageQueueCreate(unsigned int capacity, unsigned int maximumMessageSize)
{
    rtems_status_code sc;
    rtems_id qid;
    rtems_interrupt_level level;
    static char c1 = 'a';
    static char c2 = 'a';
    static char c3 = 'a';
    
    sc = rtems_message_queue_create (rtems_build_name ('Q', c3, c2, c1),
        capacity,
        maximumMessageSize,
        RTEMS_FIFO|RTEMS_LOCAL,
        &qid);
    if (sc != RTEMS_SUCCESSFUL) {
        errlogPrintf ("Can't create message queue: %s\n", rtems_status_text (sc));
        return NULL;
    }
    rtems_interrupt_disable (level);
    if (c1 == 'z') {
        if (c2 == 'z') {
            if (c3 == 'z') {
                c3 = 'a';
            }
            else {
                c3++;
            }
            c2 = 'a';
        }
        else {
            c2++;
        }
        c1 = 'a';
    }
    else {
        c1++;
    }
    rtems_interrupt_enable (level);
    return (epicsMessageQueueId)qid;
}

rtems_status_code rtems_message_queue_send_timeout(
    rtems_id id,
    void *buffer,
    rtems_unsigned32 size,
    rtems_interval timeout)
{
  Message_queue_Control    *the_message_queue;
  Objects_Locations         location;
  CORE_message_queue_Status msg_status;
    
  the_message_queue = _Message_queue_Get( id, &location );
  switch ( location )
  {
    case OBJECTS_REMOTE:
      return RTEMS_ILLEGAL_ON_REMOTE_OBJECT;

    case OBJECTS_ERROR:
      return RTEMS_INVALID_ID;

    case OBJECTS_LOCAL:
      msg_status = _CORE_message_queue_Send(
        &the_message_queue->message_queue,
        buffer,
        size,
        id,
        NULL,
        1,
        timeout
      );

      _Thread_Enable_dispatch();

      /*
       *  If we had to block, then this is where the task returns
       *  after it wakes up.  The returned status is correct for
       *  non-blocking operations but if we blocked, then we need 
       *  to look at the status in our TCB.
       */

      if ( msg_status == CORE_MESSAGE_QUEUE_STATUS_UNSATISFIED_WAIT )
        msg_status = _Thread_Executing->Wait.return_code;
      return _Message_queue_Translate_core_message_queue_return_code( msg_status );
  }
  return RTEMS_INTERNAL_ERROR;   /* unreached - only to remove warnings */
}

epicsShareFunc int epicsShareAPI epicsMessageQueueSend(
    epicsMessageQueueId id,
    void *message,
    unsigned int messageSize)
{
    if (rtems_message_queue_send_timeout((rtems_id)id, message, messageSize, RTEMS_NO_TIMEOUT) == RTEMS_SUCCESSFUL)
        return 0;
    else
        return -1;
}

epicsShareFunc int epicsShareAPI epicsMessageQueueSendWithTimeout(
    epicsMessageQueueId id,
    void *message,
    unsigned int messageSize,
    double timeout)
{
    rtems_interval delay;
    rtems_unsigned32 wait;
    extern double rtemsTicksPerSecond_double;
    
    /*
     * Convert time to ticks
     */
    if (timeout <= 0.0)
        return epicsMessageQueueTrySend(id, message, messageSize);
    wait = RTEMS_WAIT;
    delay = (int)(timeout * rtemsTicksPerSecond_double);
    if (delay == 0)
        delay++;
    if (rtems_message_queue_send_timeout((rtems_id)id, message, messageSize, delay) == RTEMS_SUCCESSFUL)
        return 0;
    else
        return -1;
}

epicsShareFunc int epicsShareAPI epicsMessageQueueTryReceive(
    epicsMessageQueueId id,
    void *message)
{
    rtems_unsigned32 size;
    
    if (rtems_message_queue_receive((rtems_id)id, message, &size, RTEMS_NO_WAIT, 0) == RTEMS_SUCCESSFUL)
        return size;
    else
        return -1;
}

epicsShareFunc int epicsShareAPI epicsMessageQueueReceive(
    epicsMessageQueueId id,
    void *message)
{
    rtems_unsigned32 size;
    
    if (rtems_message_queue_receive((rtems_id)id, message, &size, RTEMS_WAIT, RTEMS_NO_TIMEOUT) == RTEMS_SUCCESSFUL)
        return size;
    else
        return -1;
}

epicsShareFunc int epicsShareAPI epicsMessageQueueReceiveWithTimeout(
    epicsMessageQueueId id,
    void *message,
    double timeout)
{
    rtems_interval delay;
    rtems_unsigned32 size;
    rtems_unsigned32 wait;
    extern double rtemsTicksPerSecond_double;
    
    /*
     * Convert time to ticks
     */
    if (timeout <= 0.0) {
        wait = RTEMS_NO_WAIT;
        delay = 0;
    }
    else {
        wait = RTEMS_WAIT;
        delay = (int)(timeout * rtemsTicksPerSecond_double);
        if (delay == 0)
            delay++;
    }
    if (rtems_message_queue_receive((rtems_id)id, message, &size, wait, delay) == RTEMS_SUCCESSFUL)
        return size;
    else
        return -1;
}

epicsShareFunc int epicsShareAPI epicsMessageQueuePending(
            epicsMessageQueueId id)
{
    rtems_unsigned32 count;
    
    if (rtems_message_queue_get_number_pending((rtems_id)id, &count) == RTEMS_SUCCESSFUL)
        return count;
    else
        return -1;
}

epicsShareFunc void epicsShareAPI epicsMessageQueueShow(
            epicsMessageQueueId id,
                int level)
{
}
