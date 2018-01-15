#include <stdio.h>
#include "getline.h"
#include "automaton.h"
#include "msg_queue.h"
#include "msg_pipe.h"
#include "syserr.h"
#include "syslog.h"

int main(void) {
    
    log("RUN", "Hello!");
    
    MsgQueue taskQueue = msgQueueOpen("/FinAutomTskQueue", 30, 10);
    
    msgQueueWritef(taskQueue, "run-terminate: %lld", (long long)getpid());
    
    log("RUN", "Terminate.");
    msgQueueClose(&taskQueue);
    
    
    return 0;
}