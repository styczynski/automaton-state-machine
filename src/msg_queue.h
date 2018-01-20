#ifndef __MSG_QUEUE_H__
#define __MSG_QUEUE_H__

#ifndef DEBUG_MSG_QUEUE
#define DEBUG_MSG_QUEUE 0
#endif // DEBUG_MSG_QUEUE

#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <stdarg.h>
#include "memalloc.h"
#include "syslog.h"

#define MAX_MSG_QUEUE_NAME_SIZE 50


typedef struct MsgQueue MsgQueue;

struct MsgQueue {
    struct mq_attr mq_a;
    mqd_t desc;
    char* name;
    char* buff;
    int buff_size;
    int is_blocking;
};

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

MsgQueue msgQueueOpen(const char* q_name, const int msg_size, const int max_msg) {
    return msgQueueOpenEx(q_name, msg_size, max_msg, 1);
}

MsgQueue msgQueueOpenNonBlocking(const char* q_name, const int msg_size, const int max_msg) {
    return msgQueueOpenEx(q_name, msg_size, max_msg, 0);
}

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

int msgQueueWritef(MsgQueue msgq, const char* format, ...) {
    if(msgq.name == NULL) return -1;
    
    char buffer[msgq.buff_size + 7];
    
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    
    return msgQueueWrite(msgq, buffer);
}

char* msgQueueSeek(MsgQueue msgq) {
    if(msgq.name == NULL) return NULL;
    
    char* ret = msgQueueRead(msgq);
    msgQueueWrite(msgq, ret);
    
    return ret;
}

int msgQueueAbandon(MsgQueue* msgq) {
    if(msgq->name == NULL) return -1;
    
    FREE(msgq->name);
    FREE(msgq->buff);
    
    msgq->name = NULL;
    msgq->buff = NULL;
    
    return 1;
}

int msgQueueCloseEx(MsgQueue* msgq, int unlink) {
    if(msgq->name == NULL) return -1;
    
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

int msgQueueRemove(MsgQueue* msgq) {
    return msgQueueCloseEx(msgq, 1);
}

int msgQueueClose(MsgQueue* msgq) {
    return msgQueueCloseEx(msgq, 0);
}

#endif // __MSG_QUEUE_H__