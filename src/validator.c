#include <stdio.h>
#include <string.h>
#include "getline.h"
#include "automaton.h"
#include "msg_queue.h"
#include "msg_pipe.h"
#include "fork.h"
#include "syslog.h"
#include "hashmap.h"

#include "gcinit.h"

typedef struct RunSlot RunSlot;
typedef struct TesterSlot TesterSlot;

struct RunSlot {
    MsgPipeID graphDataPipeID;
    MsgPipe graphDataPipe;
    pid_t pid;
    pid_t testerSourcePid;
    int loc_id;
};

struct TesterSlot {
    char queueName[100];
    pid_t pid;
    MsgQueue testerInputQueue;
    int rcd_count;
    int acc_count;
};

int main(void) {
    
    GC_SETUP();
    
    int server_status_code = 0;
    
    
    char* transitionGraphDesc = loadTransitionGraphDesc(stdin);

    MsgQueue reportQueue = msgQueueOpen("/FinAutomReportQueue", 50, 10);
    MsgQueue runOutputQueue = msgQueueOpenNonBlocking("/FinAutomRunOutQueue", 50, 10);
    MsgQueue taskQueue = msgQueueOpen("/FinAutomTaskQueue", 30, 10);
    
    char buffer[LINE_BUF_SIZE];
    long long buffer_pid;
    int buffer_result;
    int loc_id;
    
    log_ok(SERVER, "Server is up.");
    
    int activeTasksCount = 0;
    int shouldTerminate = 0;
    int forceTermination = 0;
    
    HashMap runSlots = HashMapNew(HashMapIntCmp);
    HashMap testerSlots = HashMapNew(HashMapIntCmp);
    
    int rcd_count = 0;
    int snt_count = 0;
    int acc_count = 0;
    
    while(1) {
        char* run_term_msg = msgQueueRead(runOutputQueue);
        
        if(run_term_msg != NULL) {
            if(sscanf(run_term_msg, "run-terminate: %lld %d", &buffer_pid, &buffer_result)) {
                --activeTasksCount;
                log(SERVER, "Run terminated: %lld for result: %d", buffer_pid, buffer_result);
                
                pid_t pid = (pid_t) buffer_pid;
                RunSlot* rs = HashMapGetV(&runSlots, pid_t, RunSlot, pid);
                
                if(rs == NULL) {
                    log_err(SERVER, "Missing run slot info for pid=%d", pid);
                } else {
                    msgPipeClose(&(rs->graphDataPipe));
                
                    TesterSlot* ts = NULL;
                    LOOP_HASHMAP(&testerSlots, i) {
                        ts = (TesterSlot*) HashMapGetValue(i);
                        if(ts != NULL) {
                            if(ts->pid == rs->testerSourcePid) {
                                break;
                            }
                        }
                    }
                    
                    if(ts == NULL) {
                        log_err(SERVER, "Missing tester slot info for run of pid=%d (tester pid=%d)", rs->pid, ts->pid);
                    } else {
                        ++snt_count;
                        if(buffer_result == 1) {
                            ++acc_count;
                            ++(ts->acc_count);
                        }
                        log_ok(SERVER, "Sent answer to the tester with pid=%d", ts->pid);
                        msgQueueWritef(ts->testerInputQueue, "%d answer: %d", rs->loc_id, buffer_result);
                    }
                    
                    HashMapRemoveV(&runSlots, pid_t, RunSlot, pid);
                
                }
                
            }
        }
        
        int children_status = processWaitForAllNonBlocking();
        if(children_status == -1) {
            // Error in some child
            log_err(SERVER, "Server detected crash in some RUN subprocess so will terminate.");
            log_warn(SERVER, "All current jobs were finished so execute terminate request.");

            log_warn(SERVER, "Wait for subprocess termination... WAIT");
            processWaitForAll();
            log_warn(SERVER, "Wait for subprocess termination... END");
            
            LOOP_HASHMAP(&testerSlots, i) {
                TesterSlot* ts = (TesterSlot*) HashMapGetValue(i);
                msgQueueWritef(ts->testerInputQueue, "exit");
            }
            
            server_status_code = -1;
            break;
        }
        
        if(shouldTerminate && children_status == 0 && run_term_msg == NULL) {
            forceTermination = 1;
        }
        
        if(!shouldTerminate) {
            
            char* msg = msgQueueRead(reportQueue);
            
            if(strcmp(msg, "exit") == 0) {
                log_warn(SERVER, "Server received termination command and will close. Be aware.");
                shouldTerminate = 1;
                
                log_warn(SERVER, "Wait for subprocess termination... WAIT");
                processWaitForAll();
                log_warn(SERVER, "Wait for subprocess termination... END");
                
            } else if(sscanf(msg, "parse: %lld %d %[^NULL]", &buffer_pid, &loc_id, buffer)) {
                ++rcd_count;
                
                log(SERVER, "Received word {%s}", buffer);
                pid_t pid;
         
                TesterSlot* ts = HashMapGetV(&testerSlots, pid_t, TesterSlot, buffer_pid);
                if(ts == NULL) {
                    log_err(SERVER, "Missing tester slot info for pid=%d", buffer_pid);
                } else {
                    ++(ts->rcd_count);
                }
         
                RunSlot rs;
                rs.loc_id = loc_id;
                rs.testerSourcePid = (pid_t) buffer_pid;
                rs.graphDataPipeID = msgPipeCreate(100);
                rs.graphDataPipe = msgPipeOpen(rs.graphDataPipeID);
                
                msgPipeWrite(rs.graphDataPipe, transitionGraphDesc);

                char graphDataPipeIDStr[100];
                msgPipeIDToStr(rs.graphDataPipeID, graphDataPipeIDStr);
                
                msgQueueWritef(taskQueue, "parse: %s", buffer);
                
                ++activeTasksCount;
                if(!processExec(&pid, "./run", "run", graphDataPipeIDStr, NULL)) {
                    log_err(SERVER, "Run fork failure");
                    exit(-1);
                }
                
                rs.pid = pid;
                HashMapSetV(&runSlots, pid_t, RunSlot, pid, rs);
                
            } else if(sscanf(msg, "tester-register: %lld %[^NULL]", &buffer_pid, buffer)) {
               
               log_ok(SERVER, "Registered new tester with pid %lld for output queue: %s", buffer_pid, buffer);
               
               pid_t pid = (pid_t) buffer_pid;
               
               TesterSlot ts;
               ts.pid = pid;
               ts.rcd_count = 0;
               ts.acc_count = 0;
               ts.testerInputQueue = msgQueueOpen(buffer, 50, 10);
               
               strcpy((char*) &(ts.queueName), buffer);
               HashMapSetV(&testerSlots, pid_t, TesterSlot, pid, ts);
               
            }
        }
        
        if(forceTermination) {
            log_warn(SERVER, "All current jobs were finished so execute terminate request.");
            
            LOOP_HASHMAP(&testerSlots, i) {
                TesterSlot* ts = (TesterSlot*) HashMapGetValue(i);
                msgQueueWritef(ts->testerInputQueue, "exit");
            }
            
            break;
        }
    }
    
    log_warn(SERVER, "Terminating server...");
    
    LOOP_HASHMAP(&runSlots, i) {
        RunSlot* rs = (RunSlot*) HashMapGetValue(i);
        msgPipeClose(&(rs->graphDataPipe));
    }
    
    printf("Rcd: %d\n", rcd_count);
    printf("Snt: %d\n", snt_count);
    printf("Acc: %d\n", acc_count);
    
    LOOP_HASHMAP(&testerSlots, i) {
        TesterSlot* ts = (TesterSlot*) HashMapGetValue(i);
        if(ts->rcd_count > 0) {
            printf("PID: %d\n", ts->pid);
            printf("Rcd: %d\n", ts->rcd_count);
            printf("Acc: %d\n", ts->acc_count);
        }
        msgQueueClose(&(ts->testerInputQueue));
    }
    
    HashMapDestroyV(&runSlots, pid_t, RunSlot);
    HashMapDestroyV(&testerSlots, pid_t, TesterSlot);
    
    msgQueueRemove(&reportQueue);
    msgQueueRemove(&runOutputQueue);
    msgQueueRemove(&taskQueue);
    
    FREE(transitionGraphDesc);
    
    log(SERVER, "Final check to determine if no subprocess is left...");
    processWaitForAll();
    log_ok(SERVER, "Exit.");
    
    if(server_status_code != 0) exit(server_status_code);
    
    return 0;
}