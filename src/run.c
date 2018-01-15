#include <stdio.h>
#include "getline.h"
#include "automaton.h"
#include "msg_queue.h"
#include "msg_pipe.h"
#include "syserr.h"

int main(void) {
    
    printf("Run hello!\n");
    
    MsgQueue taskQueue = msgQueueOpen("/FinAutomTskQueue", 30, 10);
    int v = 0;
    msgQueueReadf(taskQueue, "Hello: %d", &v);
    printf("Got value = %d\n", v);
    msgQueueClose(taskQueue);
    
    
    return 0;
}