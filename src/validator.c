#include <stdio.h>
#include "getline.h"
#include "automaton.h"
#include "msg_queue.h"
#include "msg_pipe.h"
#include "fork.h"
#include "syserr.h"


int main(void) {
    
    TransitionGraph tg = newTransitionGraph();
    loadTransitionGraph(stdin, tg);
    printTransitionGraph(tg);
    
    /*MsgQueue taskQueue = msgQueueOpen("/FinAutomTskQueue", 30, 10);
    pid_t pid;
    if(processExec(&pid, "./run", "run", NULL)) return 0;
    printf("I am a parent and my pid is %d\n", getpid());
    msgQueueWritef(taskQueue, "Hello: %d", 5);
    processWait();
    msgQueueRemove(taskQueue);*/
    
    MsgPipeID taskPipeID = msgPipeCreate(100);
    
    pid_t pid;
    if(processFork(&pid)) {
        
        MsgPipe taskPipe = msgPipeOpen(taskPipeID);
        
        printf("? Child\n");
        msgPipeCloseWrite(&taskPipe);
        
        for(int t=0;t<1;++t) {
            int v = 0;
            printf("--> %s\n", msgPipeRead(taskPipe));
        }
        
        msgPipeClose(&taskPipe);
    } else {
        MsgPipe taskPipe = msgPipeOpen(taskPipeID);
        
        printf("? Parent\n");
        msgPipeCloseRead(&taskPipe);
        
        msgPipeWritef(taskPipe, "1.2.3.4.5.6.7.8.9.10.11.12.13.14.15.16.17.18:%d", 42);
        processWait();
        printf("? Parent finish\n");
        
        msgPipeClose(&taskPipe);
    }
    
    return 0;
}