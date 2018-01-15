#include <stdio.h>
#include "getline.h"
#include "automaton.h"
#include "msg_queue.h"
#include "msg_pipe.h"
#include "fork.h"
#include "syserr.h"
#include "syslog.h"
#include "serialize.h"


int main(void) {
    
    //TransitionGraph tg = newTransitionGraph();
    //loadTransitionGraph(stdin, tg);

    char* transitionGraphDesc = loadTransitionGraphDesc(stdin);
    
    MsgQueue dataQueue = msgQueueOpen("/FinAutomDataQueue", 1000, 2);
    MsgQueue reportQueue = msgQueueOpen("/FinAutomReportQueue", 30, 10);
    MsgQueue taskQueue = msgQueueOpen("/FinAutomTaskQueue", 30, 10);
    
    msgQueueWrite(dataQueue, transitionGraphDesc);
    
    char buffer[LINE_BUF_SIZE];
    long long buffer_pid;
    
    log_ok("SERVER", "Server is up.");
    
    while(1) {
        char* msg = msgQueueRead(reportQueue);
        
        log("SERVER", "Command := [%s]", msg);
        if(sscanf(msg, "parse: %[^NULL]", buffer)) {
            log("SERVER", "Received word {%s}", buffer);
            pid_t pid;
            
            msgQueueWritef(taskQueue, "parse: %s", buffer);
            if(processExec(&pid, "./run", "run", NULL)) return 0;
        } else if(sscanf(msg, "run-terminate: %lld", &buffer_pid)) {
            log("SERVER", "Run terminated: %lld", buffer_pid);
        }
    }
    
    msgQueueRemove(&reportQueue);
    msgQueueRemove(&dataQueue);
    msgQueueRemove(&taskQueue);
    
    free(transitionGraphDesc);
    
    return 0;
}