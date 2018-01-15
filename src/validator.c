#include <stdio.h>
#include "getline.h"
#include "automaton.h"
#include "msg_queue.h"
#include "msg_pipe.h"
#include "fork.h"
#include "syserr.h"
#include "syslog.h"

int main(void) {
    
    TransitionGraph tg = newTransitionGraph();
    loadTransitionGraph(stdin, tg);
    
    MsgQueue taskQueue = msgQueueOpen("/FinAutomTskQueue", 30, 10);
    
    char buffer[LINE_BUF_SIZE];
    long long buffer_pid;
    
    log_ok("SERVER", "Server is up.");
    
    while(1) {
        char* msg = msgQueueRead(taskQueue);
        
        log("SERVER", "Command := [%s]", msg);
        if(sscanf(msg, "parse: %[^NULL]", buffer)) {
            log("SERVER", "Received word {%s}", buffer);
            pid_t pid;
            if(processExec(&pid, "./run", "run", NULL)) return 0;
        } else if(sscanf(msg, "run-terminate: %lld", &buffer_pid)) {
            log("SERVER", "Run terminated: %lld", buffer_pid);
        }
    }
    
    msgQueueRemove(&taskQueue);
    
    return 0;
}