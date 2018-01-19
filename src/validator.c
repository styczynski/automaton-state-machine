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
#include "hashmap.h"

typedef struct RunSlot RunSlot;
typedef struct TesterSlot TesterSlot;

struct RunSlot {
    MsgPipeID graphDataPipeID;
    MsgPipe graphDataPipe;
    pid_t pid;
};

struct TesterSlot {
    char queueName[100];
    pid_t pid;
    MsgQueue testerInputQueue;
};

int main(void) {
    
    char* transitionGraphDesc = loadTransitionGraphDesc(stdin);

    MsgQueue reportQueue = msgQueueOpen("/FinAutomReportQueue", 50, 10);
    MsgQueue taskQueue = msgQueueOpen("/FinAutomTaskQueue", 30, 10);
    
    char buffer[LINE_BUF_SIZE];
    long long buffer_pid;
    int buffer_result;
    
    log_ok(SERVER, "Server is up.");
    
    int activeTasksCount = 0;
    int shouldTerminate = 0;
    
    HashMap runSlots = HashMapNew(HashMapIntCmp);
    HashMap testerSlots = HashMapNew(HashMapIntCmp);
    
    while(1) {
        char* msg = msgQueueRead(reportQueue);
        
        if(strcmp(msg, "exit") == 0) {
            log_warn(SERVER, "Server received termination command and will close. Be aware.");
            shouldTerminate = 1;
        } else if(!shouldTerminate && sscanf(msg, "parse: %[^NULL]", buffer)) {
            log(SERVER, "Received word {%s}", buffer);
            pid_t pid;
     
            RunSlot rs;
            rs.graphDataPipeID = msgPipeCreate(100);
            rs.graphDataPipe = msgPipeOpen(rs.graphDataPipeID);
            
            msgPipeWrite(rs.graphDataPipe, transitionGraphDesc);

            char graphDataPipeIDStr[100];
            msgPipeIDToStr(rs.graphDataPipeID, graphDataPipeIDStr);
            
            msgQueueWritef(taskQueue, "parse: %s", buffer);
            
            ++activeTasksCount;
            if(processExec(&pid, "./run", "run", graphDataPipeIDStr, NULL)) return 0;
            
            rs.pid = pid;
            HashMapSetV(runSlots, pid_t, RunSlot, pid, rs);
            
        } else if(sscanf(msg, "tester-register: %lld %[^NULL]", &buffer_pid, buffer)) {
           
           log_ok(SERVER, "Registered new tester with pid %lld for output queue: %s", buffer_pid, buffer);
           
           pid_t pid = (pid_t) buffer_pid;
           
           TesterSlot ts;
           ts.pid = pid;
           ts.testerInputQueue = msgQueueOpen(buffer, 50, 10);
           
           strcpy(&(ts.queueName), buffer);
           HashMapSetV(testerSlots, pid_t, TesterSlot, pid, ts);
           
        } else if(sscanf(msg, "run-terminate: %lld %d", &buffer_pid, &buffer_result)) {
            --activeTasksCount;
            log(SERVER, "Run terminated: %lld for result: %d", buffer_pid, buffer_result);
            
            pid_t pid = (pid_t) buffer_pid;
            RunSlot* rs = HashMapGetV(runSlots, pid_t, RunSlot, pid);
            
            if(rs == NULL) {
                log_err(SERVER, "Missing run slot info for pid=%d", pid);
            } else {
                msgPipeClose(&(rs->graphDataPipe));
                HashMapRemoveV(runSlots, pid_t, RunSlot, pid);
            }
            
            if(activeTasksCount <= 0 && shouldTerminate) {
                log_warn(SERVER, "All current jobs were finished so execute terminate request.");
                break;
            }
            
        }
    }
    
    log_warn(SERVER, "Terminating server...");
    
    HashMapDestroyV(runSlots, pid_t, RunSlot);
    HashMapDestroyV(testerSlots, pid_t, TesterSlot);
    
    msgQueueRemove(&reportQueue);
    msgQueueRemove(&taskQueue);
    
    free(transitionGraphDesc);
    
    processWaitForAll();
    log_ok(SERVER, "Exit.");
    
    return 0;
}