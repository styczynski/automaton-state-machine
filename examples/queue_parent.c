#include <stdio.h>
#include "msg_queue.h"
#include "fork.h"
#include "syserr.h"


int main(void) {
    
    MsgQueue taskQueue = msgQueueOpen("/FinAutomTskQueue", 30, 10);
    pid_t pid;
    if(processExec(&pid, "./run", "run", NULL)) return 0;
    printf("I am a parent and my pid is %d\n", getpid());
    msgQueueWritef(taskQueue, "Hello: %d", 5);
    processWait();
    msgQueueRemove(taskQueue);
    
    return 0;
}