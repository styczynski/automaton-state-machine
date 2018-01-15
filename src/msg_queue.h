#ifndef __MSG_QUEUE_H__
#define __MSG_QUEUE_H__

#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <stdarg.h>
#include "syserr.h"

#define MAX_MSG_QUEUE_NAME_SIZE 30


typedef struct MsgQueue MsgQueue;

struct MsgQueue {
    struct mq_attr mq_a;
    mqd_t desc;
    char* name;
    char* buff;
    int buff_size;
};

MsgQueue msgQueueOpen(const char* q_name, const int msg_size, const int max_msg) {
    
    MsgQueue msgq;
    
    if(q_name == NULL) {
        syserr("Error queue name is NULL");
        msgq.name = NULL;
        msgq.buff = NULL;
        msgq.buff_size = 0;
        return msgq;
    } else if(strlen(q_name) > MAX_MSG_QUEUE_NAME_SIZE) {
        syserr("Error queue name exceeds MAX_MSG_QUEUE_NAME_SIZE");
        msgq.name = NULL;
        msgq.buff = NULL;
        msgq.buff_size = 0;
        return msgq;
    }
    
    msgq.name = (char*) malloc(sizeof(char) * MAX_MSG_QUEUE_NAME_SIZE);
    msgq.name[0] = '\0';
    strcpy(msgq.name, q_name);
    
    if(max_msg>0 && msg_size>0) {
        msgq.mq_a.mq_flags = 0;
        msgq.mq_a.mq_maxmsg = max_msg;
        msgq.mq_a.mq_msgsize = msg_size;
        msgq.mq_a.mq_curmsgs = 0;
    }
    
    //printf("mq_open(%s)\n", msgq.name);
    msgq.desc = mq_open(msgq.name, O_RDWR | O_CREAT, 0664, &msgq.mq_a);
    if(msgq.desc == (mqd_t) -1) {
        syserr("Error in mq_open");
        free(msgq.name);
        msgq.name = NULL;
        msgq.buff = NULL;
        msgq.buff_size = 0;
        return msgq;
    }

    if (mq_getattr(msgq.desc, &msgq.mq_a)) {
        syserr("Error in getattr");
        free(msgq.name);
        msgq.name = NULL;
        msgq.buff = NULL;
        msgq.buff_size = 0;
        return msgq;
    }
    
    msgq.buff_size = msgq.mq_a.mq_msgsize + 1;
    msgq.buff = (char*) malloc(msgq.buff_size*sizeof(char));
    msgq.buff[0] = '\0';
    
    return msgq;
}

char* msgQueueRead(MsgQueue msgq) {
    if(msgq.name == NULL) return NULL;
    
    int ret = mq_receive(msgq.desc, msgq.buff, msgq.buff_size, NULL);
    if(ret < 0) {
        syserr("Error in mq_receive");
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
    int ret = mq_send(msgq.desc, message, strlen(message) + 1, 1);
    if(ret) {
        syserr("Error in mq_send");
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

int msgQueueCloseEx(MsgQueue* msgq, int unlink) {
    if(msgq->name == NULL) return -1;
    
    if(mq_close(msgq->desc)) {
        syserr("Error in close:");
        return -1;
    }
    
    if(unlink) {
        if(mq_unlink(msgq->name)) {
            syserr("Error in unlink:");
            return -1;
        }
    }
    
    free(msgq->name);
    free(msgq->buff);
    
    msgq->name = NULL;
    msgq->buff = NULL;
    
    return 1;
}

int msgQueueRemove(MsgQueue* msgq) {
    return msgQueueCloseEx(msgq, 1);
}

int msgQueueClose(MsgQueue* msgq) {
    return msgQueueCloseEx(msgq, 0);
}

#endif // __MSG_QUEUE_H__