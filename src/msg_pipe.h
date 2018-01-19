#ifndef __MSG_PIPE_H__
#define __MSG_PIPE_H__

#ifndef DEBUG_MSG_PIPE
#define DEBUG_MSG_PIPE 0
#endif // DEBUG_MSG_PIPE

#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include "syserr.h"
#include "syslog.h"

typedef struct MsgPipe MsgPipe;
typedef struct MsgPipeID MsgPipeID;

struct MsgPipeID {
    int pipe_desc[2];
    int good;
    int buff_size;
};

struct MsgPipe {
    int pipe_desc[2];
    int good;
    char* buff;
    int buff_size;
    int opened_read;
    int opened_write;
};

MsgPipeID msgPipeCreate(const int msg_size) {
    
    MsgPipeID msgpid;
    if (pipe(msgpid.pipe_desc) == -1) {
        syserr("Error in pipe\n");
        msgpid.good = 0;
        return msgpid;
    }
    
    msgpid.buff_size = msg_size + 7;
    msgpid.good = 1;
    
    return msgpid;
}

int msgPipeIDToStr(MsgPipeID msgpid, char* out) {
    if(!msgpid.good) {
        out[0] = '\0';
        return -1;
    }
    sprintf(out, "p%d@%d[%d]", msgpid.pipe_desc[0], msgpid.pipe_desc[1], msgpid.buff_size );
    return 1;
}

MsgPipeID msgPipeIDFromStr(char* in) {
    MsgPipeID msgpid;
    msgpid.good = 1;
    if(!sscanf(in, "p%d@%d[%d]", &msgpid.pipe_desc[0], &msgpid.pipe_desc[1], &msgpid.buff_size )) {
        msgpid.good = 0;
    }
    return msgpid;
}

MsgPipe msgPipeOpen(MsgPipeID msgp_src) {
    MsgPipe msgp;
    
    msgp.good = msgp_src.good;
    if(msgp.good) {
        msgp.pipe_desc[0] = msgp_src.pipe_desc[0];
        msgp.pipe_desc[1] = msgp_src.pipe_desc[1];
        msgp.opened_read = 1;
        msgp.opened_write = 1;
        msgp.buff_size = msgp_src.buff_size;
        msgp.buff = (char*) malloc(msgp.buff_size*sizeof(char));
    }
    
    return msgp;
}

int msgPipeCloseRead(MsgPipe* msgp) {
    if(msgp == NULL) return -1;
    
    if(!msgp->good || !msgp->opened_read) {
        return -1;
    }
    
    if (close(msgp->pipe_desc[0]) == -1) {
        syserr("Error in close(pipe_dsc[0])\n");
        return -1;
    }
    
    msgp->opened_read = 0;
    return 1;
}

int msgPipeCloseWrite(MsgPipe* msgp) {
    if(msgp == NULL) return -1;
    
    if(!msgp->good || !msgp->opened_write) {
        return -1;
    }
    
    if (close(msgp->pipe_desc[1]) == -1) {
        syserr("Error in close(pipe_dsc[1])\n");
        return -1;
    }
    
    msgp->opened_write = 0;
    return 1;
}

int msgPipeAbandon(MsgPipe* msgp) {
    if(msgp == NULL) return -1;
    
    if(!msgp->good) {
        return -1;
    }
    
    free(msgp->buff);
    msgp->buff = NULL;
    msgp->buff_size = 0;
    msgp->good = 0;
    
    return 1;
}

int msgPipeClose(MsgPipe* msgp) {
    if(msgp == NULL) return -1;
    
    if(!msgp->good) {
        return -1;
    }
    
    msgPipeCloseRead(msgp);
    msgPipeCloseWrite(msgp);
    
    return msgPipeAbandon(msgp);
}

char* msgPipeRead(MsgPipe msgp) {
    if(!msgp.good || !msgp.opened_read) return NULL;
    
    log_debug(DEBUG_MSG_PIPE, MSGPIP, "Read from pipe: %d%d", msgp.pipe_desc[0], msgp.pipe_desc[1]);
    
    int read_len = 0;
    if((read_len = read(msgp.pipe_desc[0], msgp.buff, msgp.buff_size - 1)) == -1) {
        syserr("Error in read\n");
        return NULL;
    }
    msgp.buff[read_len < msgp.buff_size - 1? read_len : msgp.buff_size - 1] = '\0';
    
    log_debug(DEBUG_MSG_PIPE, MSGPIP, "Read from pipe: %d%d {%s}", msgp.pipe_desc[0], msgp.pipe_desc[1], msgp.buff);
    
    if(read_len == 0) {
        fatal("Unexpected end-of-file\n");
        return NULL;
    }
    
    return msgp.buff;
}

int msgPipeReadf(MsgPipe msgp, const char* format, ...) {
    if(!msgp.good || !msgp.opened_read) return -1;
    
    char* rcvMessage = msgPipeRead(msgp);
    if(rcvMessage == NULL) return -1;
    
    va_list args;
    va_start(args, format);
    int resultCode = vsscanf(rcvMessage, format, args);
    va_end(args);
    
    return resultCode;
}

int msgPipeWrite(MsgPipe msgp, char* message) {
    if(!msgp.good || !msgp.opened_write) return -1;
    
    const int message_len = strlen(message);
    log_debug(DEBUG_MSG_PIPE, MSGPIP, "Write into pipe: %d%d {%s}", msgp.pipe_desc[0], msgp.pipe_desc[1], message);
    
    if(write(msgp.pipe_desc[1], message, message_len) != message_len) {
        syserr("Error in write\n");
        return -1;
    }
    
    return 1;
}

int msgPipeWritef(MsgPipe msgp, const char* format, ...) {
    if(!msgp.good || !msgp.opened_write) return -1;
    
    char buffer[msgp.buff_size + 7];
    
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    
    return msgPipeWrite(msgp, buffer);
}

#endif // __MSG_PIPE_H__