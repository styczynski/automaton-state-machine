/** @file
*
*  Message pipes unified interface. (C99 standard)
*
*  @author Piotr Styczyński <piotrsty1@gmail.com>
*  @copyright MIT
*  @date 2018-01-21
*/
#ifndef __MSG_PIPE_H__
#define __MSG_PIPE_H__

#ifndef DEBUG_MSG_PIPE

/**
 * @def DEBUG_MSG_PIPE
 *  If DEBUG_MSG_PIPE is equal to 1 then every message sent by pipe is logged
 *  via syslog.h functions
 */
#define DEBUG_MSG_PIPE 0

#endif // DEBUG_MSG_PIPE

#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <stdint.h>
#include "memalloc.h"
#include "syslog.h"

/** Type of message pipe */
typedef struct MsgPipe MsgPipe;

/** Type of message pipe identificator */
typedef struct MsgPipeID MsgPipeID;

/** Type of message pipe identificator */
struct MsgPipeID {
    int pipe_desc[2]; ///< pipe descriptors (0 - read; 1 - write)
    int good;         ///< no error indicator
    int buff_size;    ///< recommented size of the buffer for pipe messages
};

/** Type of message pipe */
struct MsgPipe {
    int pipe_desc[2]; ///< pipe descriptors (0 - read; 1 - write)
    int good;         ///< no error indicator
    char* buff;       ///< read buffer
    int buff_size;    ///< read buffer size
    int opened_read;  ///< is read descriptor open?
    int opened_write; ///< is write descriptor open? 
};

/**
 * GC Destructor for pipes
 */
DECL_GC_DESTRUCTOR(_pipe_close_) {
   int descr = (int) (intptr_t) ptr;
   if(descr != -1) {
       close(descr);
   }
}

/**
 * Create new pipe with given message size limit.
 * 
 * @param[in] msg_size : Limit for message length
 * @returns Returns pipe identificator
 */
MsgPipeID msgPipeCreate(const int msg_size) {
    
    MsgPipeID msgpid;
    if (pipe(msgpid.pipe_desc) == -1) {
        syserr("msgPipeCreate failed due to pipe(...) error");
        msgpid.good = 0;
        return msgpid;
    } else {
        GC_NEW(200, msgpid.pipe_desc[0], _pipe_close_);
        GC_NEW(200, msgpid.pipe_desc[1], _pipe_close_);
    }
    
    msgpid.buff_size = msg_size + 7;
    msgpid.good = 1;
 
    return msgpid;
}

/**
 * Converts message pipe identificator to the string representation.
 * Stringified pipe indentificator can be passed as program arguments or sent
 * into remote places to be converted back and used to open pipes.
 * 
 * @param[in] msgpid : Pipe indentificator
 * @param[in] out    : Output buffer 
 * @returns -1 for failure; 1 for success
 */
int msgPipeIDToStr(MsgPipeID msgpid, char* out) {
    if(!msgpid.good) {
        out[0] = '\0';
        return -1;
    }
    sprintf(out, "p%d@%d[%d]", msgpid.pipe_desc[0], msgpid.pipe_desc[1], msgpid.buff_size);
    return 1;
}

/**
 * Checks if the indicator is valid and the pipe has not crashed.
 * 
 * @param[in] msgpid : Pipe indentificator
 * @returns If the identificator and pipe itself is not broken?
 */
int msgPipeIsGoodID(MsgPipeID msgpid) {
    return msgpid.good;
}

/**
 * Converts message pipe stringified identificator back to the object.
 * Stringified pipe indentificator can be passed as program arguments or sent
 * into remote places to be converted back and used to open pipes.
 * 
 * @param[in] out    : Input buffer 
 * @returns  Pipe indentificator 
 */
MsgPipeID msgPipeIDFromStr(char* in) {
    if(in == NULL) {
        syserrv("msgPipeIDFromStr() failed string is NULL");
    }
    
    MsgPipeID msgpid;
    msgpid.good = 1;
    msgpid.pipe_desc[0] = -1;
    msgpid.pipe_desc[1] = -1;
    msgpid.buff_size = -1;
    
    if(!sscanf(in, "p%d@%d[%d]", &msgpid.pipe_desc[0], &msgpid.pipe_desc[1], &msgpid.buff_size )) {
        syserrv("msgPipeIDFromStr() failed string {%s} does not match valid pipe schema!", in);
        msgpid.good = 0;
    }
    return msgpid;
}

/**
 * Opens blocking pipe.
 * 
 * NOTE:
 *   Each msgPipeOpen msut have corresponding
 *   msgPipeClose() or msgPipeAbandon()
 *
 * @param[in] msgp_src :  Pipe indentificator
 * @returns Returns pipe object
 */
MsgPipe msgPipeOpen(MsgPipeID msgp_src) {
    MsgPipe msgp;
    
    msgp.good = msgp_src.good;
    if(msgp.good) {
        msgp.pipe_desc[0] = msgp_src.pipe_desc[0];
        msgp.pipe_desc[1] = msgp_src.pipe_desc[1];
        msgp.opened_read = 1;
        msgp.opened_write = 1;
        msgp.buff_size = msgp_src.buff_size;
        msgp.buff = MALLOCATE_ARRAY(char, msgp.buff_size);
    }
    
    return msgp;
}

/**
 * Closes pipe (but only in read direction).
 *
 * NOTE:
 *  This function only modifies pipe operating mode.
 *  Each msgPipeOpen msut have corresponding msgPipeClose() or msgPipeAbandon()
 *  msgPipeCloseRead/-Write do not act as msgPipeClose
 * 
 * @param[in] msgp_src :  Pipe pointer
 * @returns -1 on error (e.g. multiple read-closes); 1 on success
 */
int msgPipeCloseRead(MsgPipe* msgp) {
    if(msgp == NULL) return -1;
    
    if(!msgp->good || !msgp->opened_read) {
        return -1;
    }
    
    GC_DEL(200, msgp->pipe_desc[0]);
    if (close(msgp->pipe_desc[0]) == -1) {
        syserr("msgPipeCloseRead failed due to close(desc=%d) error", msgp->pipe_desc[0]);
        return -1;
    }
    
    msgp->opened_read = 0;
    return 1;
}

/**
 * Closes pipe (but only in write direction).
 *
 * NOTE:
 *  This function only modifies pipe operating mode.
 *  Each msgPipeOpen msut have corresponding msgPipeClose() or msgPipeAbandon()
 *  msgPipeCloseRead/-Write do not act as msgPipeClose
 * 
 * @param[in] msgp_src :  Pipe pointer
 * @returns -1 on error (e.g. multiple write-closes); 1 on success
 */
int msgPipeCloseWrite(MsgPipe* msgp) {
    if(msgp == NULL) return -1;
    
    if(!msgp->good || !msgp->opened_write) {
        return -1;
    }
    
    GC_DEL(200, msgp->pipe_desc[1]);
    if (close(msgp->pipe_desc[1]) == -1) {
        syserr("msgPipeCloseWrite failed due to close(desc=%d) error", msgp->pipe_desc[1]);
        return -1;
    }
    
    msgp->opened_write = 0;
    return 1;
}

/**
 * Abandon pipe.
 * It's not closes but all resources are freed.
 * This function does not execute native close(pipe) function as msgPipeClose(...) does.
 * 
 * NOTE:
 *   Each msgPipeOpen msut have corresponding msgPipeClose() or msgPipeAbandon()
 *
 * @param[in] msgp : Pipe pointer
 * @returns -1 on error; 1 on success
 */
int msgPipeAbandon(MsgPipe* msgp) {
    if(msgp == NULL) return -1;
    
    log_debug(DEBUG_MSG_PIPE, MSGPIP, "Abandon pipe: %d%d", msgp->pipe_desc[0], msgp->pipe_desc[1]);
    
    if(!msgp->good) {
        return -1;
    }
    
    FREE(msgp->buff);
    msgp->buff = NULL;
    msgp->buff_size = 0;
    msgp->good = 0;
    
    return 1;
}

/**
 * Closes pipe and frees all resoureces.
 *
 * NOTE:
 *   Each msgPipeOpen msut have corresponding msgPipeClose() or msgPipeAbandon()
 *
 * @param[in] msgp : Pipe pointer
 * @returns -1 on error; 1 on success
 */
int msgPipeClose(MsgPipe* msgp) {
    if(msgp == NULL) return -1;
    
    if(!msgp->good) {
        return -1;
    }
    
    msgPipeCloseRead(msgp);
    msgPipeCloseWrite(msgp);
    
    return msgPipeAbandon(msgp);
}

/**
 * Read string from the pipe.
 *
 * NOTE:
 *   The returned pointer MUST NOT be FREED.
 *   It's valid until next read operation and stored in interal pipe strucutres.
 * 
 * @param[in] msgp : Pipe to read from
 * @returns Pointer to the internal buffer or NULL on error
 */
char* msgPipeRead(MsgPipe msgp) {
    if(!msgp.good || !msgp.opened_read) return NULL;
    
    log_debug(DEBUG_MSG_PIPE, MSGPIP, "Read from pipe: %d%d", msgp.pipe_desc[0], msgp.pipe_desc[1]);
    
    int read_len = 0;
    if((read_len = read(msgp.pipe_desc[0], msgp.buff, msgp.buff_size - 1)) == -1) {
        syserr("msgPipeRead failed due to read(desc=%d, *buff=%p, size=%d) error", msgp.pipe_desc[0], &(msgp.buff), msgp.buff_size-1);
        return NULL;
    }
    msgp.buff[read_len < msgp.buff_size - 1? read_len : msgp.buff_size - 1] = '\0';
    
    log_debug(DEBUG_MSG_PIPE, MSGPIP, "Read from pipe: %d%d {%s}", msgp.pipe_desc[0], msgp.pipe_desc[1], msgp.buff);
    
    if(read_len == 0) {
        syserr("msgPipeRead failed due to number of read bytes = 0");
        return NULL;
    }
    
    return msgp.buff;
}

/**
 * Read formatted string from the pipe.
 * Operates as scanf do.
 *
 * NOTE:
 *   The returned pointer MUST NOT be FREED.
 *   It's valid until next read operation and stored in interal pipe strucutres.
 * 
 * @param[in] msgp   : Pipe to read from
 * @param[in] format : Scanf compatible format cstring
 * @param[in] ...    : List of scanf-like pointers to loaded data
 * @returns Return code form executing sscanf
 */
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

/**
 * Write string to the pipe.
 * 
 * NOTE:
 *  Message cannot be greater than maximum size set when creating the pipe.
 *
 * @param[in] msgp    : Pipe to write to
 * @param[in] message : Message to be written
 * @returns Return -1 on failure; 1 on success
 */
int msgPipeWrite(MsgPipe msgp, char* message) {
    if(!msgp.good || !msgp.opened_write) return -1;
    
    const int message_len = strlen(message);
    log_debug(DEBUG_MSG_PIPE, MSGPIP, "Write into pipe: %d%d {%s}", msgp.pipe_desc[0], msgp.pipe_desc[1], message);
    
    if(write(msgp.pipe_desc[1], message, message_len) != message_len) {
        syserr("msgPipeWrite failed due to write(desc=%d, length=%d) error", msgp.pipe_desc[1], message_len);
        return -1;
    }
    
    return 1;
}

/**
 * Write formatted string to the pipe.
 * Operates as printf do.
 * 
 * NOTE:
 *  Message cannot be greater than maximum size set when creating the pipe.
 *
 * @param[in] msgp    : Pipe to write to
 * 
 * @param[in] format : Printf compatible format cstring
 * @param[in] ...    : List of printf-like pointers to loaded data
 * @returns Return -1 on failure; 1 on success
 */
int msgPipeWritef(MsgPipe msgp, const char* format, ...) {
    if(!msgp.good || !msgp.opened_write) return -1;
    
    char buffer[msgp.buff_size + 7];
    
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    
    return msgPipeWrite(msgp, buffer);
}

/**
 * GC Destructor for pipes
 */
DECL_GC_DESTRUCTOR(MsgPipeDestructor) {
    msgPipeClose((MsgPipe*) ptr);
}

#endif // __MSG_PIPE_H__