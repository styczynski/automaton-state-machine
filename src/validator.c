#include <stdio.h>
#include <string.h>
#include "getline.h"
#include "automaton.h"
#include "msg_queue.h"
#include "msg_pipe.h"
#include "fork.h"
#include "syserr.h"
#include "syslog.h"
#include "serialize.h"
#include "dynamic_lists.h"

typedef struct ClientSlot ClientSlot;

struct ClientSlot {
    MsgPipeID graphDataPipeID;
    MsgPipe graphDataPipe;
};

int main(void) {
    
    char* transitionGraphDesc = loadTransitionGraphDesc(stdin);

    MsgQueue reportQueue = msgQueueOpen("/FinAutomReportQueue", 30, 10);
    MsgQueue taskQueue = msgQueueOpen("/FinAutomTaskQueue", 30, 10);
    
    char buffer[LINE_BUF_SIZE];
    long long buffer_pid;
    
    log_ok(SERVER, "Server is up.");
    
    int activeTasksCount = 0;
    int shouldTerminate = 0;
    
    while(1) {
        char* msg = msgQueueRead(reportQueue);
        
        if(strcmp(msg, "exit") == 0) {
            log_warn(SERVER, "Server received termination command and will close. Be aware.");
            shouldTerminate = 1;
        } else if(sscanf(msg, "parse: %[^NULL]", buffer)) {
            log(SERVER, "Received word {%s}", buffer);
            pid_t pid;
     
            MsgPipeID graphDataPipeID = msgPipeCreate(100);
            MsgPipe graphDataPipe = msgPipeOpen(graphDataPipeID);
            //++current_free_slot;
            
            msgPipeWrite(graphDataPipe, transitionGraphDesc);

            char graphDataPipeIDStr[100];
            msgPipeIDToStr(graphDataPipeID, graphDataPipeIDStr);
            
            msgQueueWritef(taskQueue, "parse: %s", buffer);
            
            ++activeTasksCount;
            if(processExec(&pid, "./run", "run", graphDataPipeIDStr, NULL)) return 0;
            
        } else if(sscanf(msg, "run-terminate: %lld", &buffer_pid)) {
            --activeTasksCount;
            log(SERVER, "Run terminated: %lld", buffer_pid);
            if(activeTasksCount <= 0 && shouldTerminate) {
                break;
            }
            //msgPipeClose(&);
        }
    }
    
    log_warn(SERVER, "Terminating server...");
    
    msgQueueRemove(&reportQueue);
    msgQueueRemove(&taskQueue);
    
    free(transitionGraphDesc);
    
    processWaitForAll();
    log_ok(SERVER, "Exit.");
    
    return 0;
}