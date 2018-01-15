#include <stdio.h>
#include "getline.h"
#include "automaton.h"
#include "msg_queue.h"
#include "syserr.h"

int main(void) {
    
    printf("Run hello!\n");
    
    MsgQueue taskQueue = msgQueueOpen("/fin_autom_tsk_queue", 30, 10);
    
    char* msg = msgQueueRcv(taskQueue);
    if(msg) printf("Received queue message: {%s}\n", msg);
    
    msgQueueClose(taskQueue);
    
    return 0;
}