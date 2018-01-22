/** @file
*
*  Message queues (natively: msq) unified interface. (C99 standard)
*
*  @author Piotr Styczyński <piotrsty1@gmail.com>
*  @copyright MIT
*  @date 2018-01-21
*/
#ifndef __MSG_QUEUE_H__
#define __MSG_QUEUE_H__

#ifndef DEBUG_MSG_QUEUE
#define DEBUG_MSG_QUEUE 0
#endif // DEBUG_MSG_QUEUE

#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdint.h>
#include <mqueue.h>
#include <stdarg.h>
#include "memalloc.h"
#include "syslog.h"

/**
 * @def MAX_MSG_QUEUE_NAME_SIZE
 *  Macro defining maximum queue name.
 */
#define MAX_MSG_QUEUE_NAME_SIZE 50

/** Type of message queue */
typedef struct MsgQueue MsgQueue;

/** Type of message queue */
struct MsgQueue {
    struct mq_attr mq_a;
    mqd_t desc;
    char* name;
    char* buff;
    int buff_size;
    int is_blocking;
};

/**
 * GC Destructor for queues
 */
DECL_GC_DESTRUCTOR(_msgq_close_) {
   mqd_t descr = (mqd_t) (intptr_t) ptr;
   if(descr != -1) {
       mq_close(descr);
   }
}

/**
 * Opens new message queue.
 *
 * NOTE:
 *   Each msgQueuOpen msut have corresponding msgQueueRemove(), msgQueueClose() or msgQueueAbandon()
 *
 * @param[in] q_name      : Path to the queue that will be opened (created when not exists)
 * @param[in] msg_size    : Maximum allowed length of single message
 * @param[in] max_msg     : Maximum number of messages
 * @param[in] is_blocking : Is the operations on the queue blocking?
 * @returns Opened MsgQueue
 */
MsgQueue msgQueueOpenEx(const char* q_name, const int msg_size, const int max_msg, const int is_blocking) {
    
    MsgQueue msgq;
    
    if(q_name == NULL) {
        syserrv("msgQueueOpenEx failed due to NULLed queue name");
        msgq.name = NULL;
        msgq.buff = NULL;
        msgq.buff_size = 0;
        return msgq;
    } else if(strlen(q_name) > MAX_MSG_QUEUE_NAME_SIZE) {
        syserrv("msgQueueOpenEx failed due to name size exceeding MAX_MSG_QUEUE_NAME_SIZE length");
        msgq.name = NULL;
        msgq.buff = NULL;
        msgq.buff_size = 0;
        return msgq;
    }
    
    msgq.name = MALLOCATE_ARRAY(char, MAX_MSG_QUEUE_NAME_SIZE);
    msgq.name[0] = '\0';
    strcpy(msgq.name, q_name);
    
    if(max_msg>0 && msg_size>0) {
        msgq.mq_a.mq_flags = 0;
        msgq.mq_a.mq_maxmsg = max_msg;
        msgq.mq_a.mq_msgsize = msg_size;
        msgq.mq_a.mq_curmsgs = 0;
    }
    
    //printf("mq_open(%s)\n", msgq.name);
    if(is_blocking) {
        msgq.is_blocking = 1;
        msgq.desc = mq_open(msgq.name, O_RDWR | O_CREAT, 0664, &msgq.mq_a);
    } else {
        msgq.is_blocking = 0;
        msgq.desc = mq_open(msgq.name, O_RDWR | O_CREAT | O_NONBLOCK, 0664, &msgq.mq_a);
    }
    
    if(msgq.desc == (mqd_t) -1) {
        syserr("msgQueueOpenEx failed due to mq_open(%s) error", msgq.name);
        FREE(msgq.name);
        msgq.name = NULL;
        msgq.buff = NULL;
        msgq.buff_size = 0;
        return msgq;
    } else {
        GC_NEW(100, msgq.desc, _msgq_close_);
    }

    if (mq_getattr(msgq.desc, &msgq.mq_a)) {
        syserr("msgQueueOpenEx failed due to mq_getattr(desc=%d) error", msgq.desc);
        FREE(msgq.name);
        msgq.name = NULL;
        msgq.buff = NULL;
        msgq.buff_size = 0;
        return msgq;
    }
    
    msgq.buff_size = msgq.mq_a.mq_msgsize + 1;
    msgq.buff = MALLOCATE_ARRAY(char, msgq.buff_size);
    msgq.buff[0] = '\0';
    
    return msgq;
}

/**
 * Opens new BLOCKING message queue.
 * 
 * NOTE:
 *   Each msgQueuOpen msut have corresponding msgQueueRemove(), msgQueueClose() or msgQueueAbandon()
 *
 * @param[in] q_name      : Path to the queue that will be opened (created when not exists)
 * @param[in] msg_size    : Maximum allowed length of single message
 * @param[in] max_msg     : Maximum number of messages
 * @returns Opened blocking MsgQueue
 */
MsgQueue msgQueueOpen(const char* q_name, const int msg_size, const int max_msg) {
    return msgQueueOpenEx(q_name, msg_size, max_msg, 1);
}

/**
 * Opens new NONBLOCKING message queue.
 * 
 * NOTE:
 *   Each msgQueuOpen msut have corresponding msgQueueRemove(), msgQueueClose() or msgQueueAbandon()
 *
 * @param[in] q_name      : Path to the queue that will be opened (created when not exists)
 * @param[in] msg_size    : Maximum allowed length of single message
 * @param[in] max_msg     : Maximum number of messages
 * @returns Opened nonblocking MsgQueue
 */
MsgQueue msgQueueOpenNonBlocking(const char* q_name, const int msg_size, const int max_msg) {
    return msgQueueOpenEx(q_name, msg_size, max_msg, 0);
}

/**
 * Read string from the queue.
 *
 * NOTE:
 *   The returned pointer MUST NOT be FREED.
 *   It's valid until next read operation and stored in interal queue strucutres.
 * 
 * @param[in] msgq : Queue to read from
 * @returns Pointer to the internal buffer or NULL on error
 */
char* msgQueueRead(MsgQueue msgq) {
    if(msgq.name == NULL) return NULL;
    
    int ret = mq_receive(msgq.desc, msgq.buff, msgq.buff_size, NULL);
    if(ret >= 0) {
        msgq.buff[ret] = '\0';
    }
    
    log_debug(DEBUG_MSG_QUEUE, MSGQUE, "Read from msg_queue named %s message into buffer of size = %d: {%s}", msgq.name, msgq.buff_size, msgq.buff);
    
    if(ret < 0) {
        if(errno == EAGAIN && !msgq.is_blocking) {
            msgq.buff[0] = '\0';
            return NULL;
        }
        syserr("msgQueueRead failed due to mq_receive(desc=%d, *buffer=%p, len=%d) error", msgq.desc, msgq.buff, msgq.buff_size);
        return NULL;
    }
    
    return msgq.buff;
}

/**
 * Read formatted string from the queue.
 * Operates as scanf do.
 *
 * NOTE:
 *   The returned pointer MUST NOT be FREED.
 *   It's valid until next read operation and stored in interal queue strucutres.
 * 
 * @param[in] msgq   : Queue to read from
 * @param[in] format : Scanf compatible format cstring
 * @param[in] ...    : List of scanf-like pointers to loaded data
 * @returns Return code form executing sscanf
 */
int msgQueueReadf(MsgQueue msgq, const char* format, ...) {
    if(msgq.name == NULL) return -1;
    
    char* rcvMessage = msgQueueRead(msgq);
    if(rcvMessage == NULL) return -1;
    
    va_list args;
    va_start(args, format);
    int resultCode = vsscanf(rcvMessage, format, args);
    va_end(args);
    
    return resultCode;
}

/**
 * Write string to the queue.
 * 
 * NOTE:
 *  Message cannot be greater than maximum size set when creating the queue.
 *
 * @param[in] msgp    : Queue to write to
 * @param[in] message : Message to be written
 * @returns Return -1 on failure; 1 on success
 */
int msgQueueWrite(MsgQueue msgq, char* message) {
    if(msgq.name == NULL) return -1;
    
    log_debug(DEBUG_MSG_QUEUE, MSGQUE, "Write into msg_queue named %s message of size = %d {%s}", msgq.name, strlen(message), message);
    
    int ret = mq_send(msgq.desc, message, strlen(message) + 1, 1);
    if(ret) {
        if(errno == EAGAIN && !msgq.is_blocking) {
            return 1;
        }
        syserr("msgQueueWrite failed due to mq_send(desc=%d, len=%d) error", msgq.desc, strlen(message)+1);
        return -1;
    }
    return ret;
}

/**
 * Write formatted string to the queue.
 * Operates as printf do.
 * 
 * NOTE:
 *  Message cannot be greater than maximum size set when creating the queue.
 *
 * @param[in] msgp    : Queue to write to
 * 
 * @param[in] format : Printf compatible format cstring
 * @param[in] ...    : List of printf-like pointers to loaded data
 * @returns Return -1 on failure; 1 on success
 */
int msgQueueWritef(MsgQueue msgq, const char* format, ...) {
    if(msgq.name == NULL) return -1;
    
    char buffer[msgq.buff_size + 7];
    
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    
    return msgQueueWrite(msgq, buffer);
}

/**
 * Reads form the queue but places the value back in it.
 *
 * Equivalent to execute:
 *
 *    char* ret = msgQueueRead(msgq);
 *    msgQueueWrite(msgq, ret);
 *    return ret;
 * 
 *
 * NOTE:
 *   The returned pointer MUST NOT be FREED.
 *   It's valid until next read operation and stored in interal queue strucutres. 
 *
 * @param[in] msgq : Description for msgq
 * @returns Return description
 */
char* msgQueueSeek(MsgQueue msgq) {
    if(msgq.name == NULL) return NULL;
    
    char* ret = msgQueueRead(msgq);
    msgQueueWrite(msgq, ret);
    
    return ret;
}

/**
 * Abandon queue.
 * It's not closes but all resources are freed.
 * This function does not execute native mq_close(queue) function as msgQueueClose(...) does.
 * 
 * NOTE:
 *   Each msgQueuOpen msut have corresponding msgQueueRemove(), msgQueueClose() or msgQueueAbandon()
 *
 * @param[in] msgq : Queue pointer
 * @returns -1 on error; 1 on success
 */
int msgQueueAbandon(MsgQueue* msgq) {
    if(msgq->name == NULL) return -1;
    
    FREE(msgq->name);
    FREE(msgq->buff);
    
    msgq->name = NULL;
    msgq->buff = NULL;
    
    return 1;
}

/**
 * Closes queue and frees all resoureces.
 * Optionally (if @p unlink is true) unlinks (removes) the queue form filesystem.
 *
 * NOTE:
 *   Each msgQueueOpen must have correspondingcorresponding msgQueueRemove(), msgQueueClose() or msgQueueAbandon()
 *
 * @param[in] msgq   : Queue pointer
 * @param[in] unlink : Should the queue file be removed?
 * @returns -1 on error; 1 on success
 */
int msgQueueCloseEx(MsgQueue* msgq, int unlink) {
    if(msgq->name == NULL) return -1;
    
    GC_DEL(100, msgq->desc);
    
    if(mq_close(msgq->desc)) {
        syserr("msgQueueCloseEx failed due to mq_close(desc=%d) error", msgq->desc);
        return -1;
    }
    
    
    if(unlink) {
        if(mq_unlink(msgq->name)) {
            syserr("msgQueueCloseEx failed due to mq_unlink(%s) error", msgq->name);
            return -1;
        }
    }
    
    msgQueueAbandon(msgq);
    
    return 1;
}

/**
 * Closes queue, frees all resoureces and then unlinks (removes) the queue form filesystem.
 *
 * NOTE:
 *   Each msgQueueOpen must have corresponding msgQueueRemove(), msgQueueClose() or msgQueueAbandon()
 *
 * @param[in] msgq   : Queue pointer
 * @returns -1 on error; 1 on success
 */
int msgQueueRemove(MsgQueue* msgq) {
    return msgQueueCloseEx(msgq, 1);
}

/**
 * Closes queue and frees all resoureces, but leaves queue in the filesystem.
 *
 * NOTE:
 *   Each msgQueueOpen must have corresponding msgQueueRemove(), msgQueueClose() or msgQueueAbandon()
 *
 * @param[in] msgq   : Queue pointer
 * @returns -1 on error; 1 on success
 */
int msgQueueClose(MsgQueue* msgq) {
    return msgQueueCloseEx(msgq, 0);
}

/**
 * Modify blocking behaviour of the queue.
 * 
 * @param[in] msgq : Pointer to the modified queue
 * @param[in] will_block : Will the queue be blocking from now?
 * @returns Return -1 on failure and 1 on success
 */
int msgQueueMakeBlocking(MsgQueue* msgq, int will_block) {
    if(msgq->name == NULL) return -1;
    
    log_warn(IKSDE, "make blocking queue -> %s, %d, %d", msgq->name, msgq->mq_a.mq_maxmsg, msgq->mq_a.mq_msgsize);
    
    char* q_name = MALLOCATE_ARRAY(char, MAX_MSG_QUEUE_NAME_SIZE);
    q_name[0] = '\0';
    strcpy(q_name, msgq->name);
    
    int max_msg = msgq->mq_a.mq_maxmsg;
    int msg_size = msgq->mq_a.mq_msgsize;
    
    msgQueueClose(msgq);
    *msgq = msgQueueOpenEx(q_name, msg_size, max_msg, will_block);
    return 1;
}


#endif // __MSG_QUEUE_H__