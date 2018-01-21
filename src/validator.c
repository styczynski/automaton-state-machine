/**
 * Implementation of Automaton for studies on Warsaw Univeristy
 *
 * [Proces validator]
 *   The validator is a server accepting, or rejecting, words. A word is accepted if it is accepted by
 *   specified automaton. The program validator begins by reading from the standard input
 *   the description of the automaton and then in an infinite loop waits for the words to verify.
 *
 *   When a word is received he runs the program run which validates the word and vaidator waits
 *   for an another word or response from the program run. Afer receiving a message from one of the
 *   run processes validator forwords the message to the adequate tester.
 *
 *   When a tester send an unique stop word ! the server stops , i.e. he does not accept new words,
 *   collects the responses from the run processes, forwards the answers, writes a report on stdout, and, finally, terminates.
 *
 *   The validator report consist in three lines describing the numbers of received queries, sent answers and accepted words:
 *
 *     Rcd: x\n
 *     Snt: y\n
 *     Acc: z\n
 * 
 * where x,y,z respectively are the numbers of received queries, sent answers and accepted words; and a sequence of summaries of the interactions with the programs tester from which validator received at least one query. A summary for a tester with PID pid consists in:
 *
 *     [PID: pid\n
 *     Rcd: y\n
 *     Acc: z\n]
 *
 * where pid,y,z respectively are: the process' pid, the number of messages received from this process and the number of acceped words sent by this process.
 *
 * @author Piotr Styczyński <piotrsty1@gmail.com>
 * @copyright MIT
 * @date 2018-01-21
 */
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

int verboseMode = 0;

int main(int argc, char *argv[]) {
    
    GC_SETUP();
    
    log_set(0);
    for(int i=1;i<argc;++i) {
        if(strcmp(argv[i], "-v") == 0) {
            log_set(1);
            verboseMode = 1;
        }
    }
    
    int server_status_code = 0;
    
    char* transitionGraphDesc = loadTransitionGraphDescFromStdin();

    MsgQueue reportQueue = msgQueueOpen("/FinAutomReportQueue", LINE_BUF_SIZE, MSG_QUEUE_SIZE);
    MsgQueue runOutputQueue = msgQueueOpenNonBlocking("/FinAutomRunOutQueue", LINE_BUF_SIZE, MSG_QUEUE_SIZE);
    MsgQueue registerQueue = msgQueueOpenNonBlocking("/FinAutomRegisterQueue", LINE_BUF_SIZE, MSG_QUEUE_SIZE);
    
    char buffer[LINE_BUF_SIZE];
    char buffer2[LINE_BUF_SIZE];
    
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
    
    int throttled_mode = 0;
    
    while(1) {
        char* register_msg;
        do {
            register_msg = msgQueueRead(registerQueue);
            if(register_msg != NULL) {
                
                if(sscanf(register_msg, "register_tester: %lld %s", &buffer_pid, buffer2)) {
                    log_ok(SERVER, "Registered new tester with pid %lld for output queue: %s", buffer_pid, buffer2);
                        
                    pid_t tester_pid = (pid_t) buffer_pid;
                        
                    TesterSlot ts_new_val;
                    ts_new_val.pid = tester_pid;
                    ts_new_val.rcd_count = 0;
                    ts_new_val.acc_count = 0;
                    ts_new_val.testerInputQueue = msgQueueOpen(buffer2, LINE_BUF_SIZE, MSG_QUEUE_SIZE);
                        
                    strcpy((char*) &(ts_new_val.queueName), buffer);
                    HashMapSetV(&testerSlots, pid_t, TesterSlot, tester_pid, ts_new_val);
                }
            }
        } while(register_msg != NULL);
        
        
        if(!throttled_mode && activeTasksCount > SERVER_PROCESS_LIMIT) {
            throttled_mode = 1;
            msgQueueMakeBlocking(&runOutputQueue, 1);
            msgQueueMakeBlocking(&reportQueue,    0);
            log_warn(SERVER, "SERVER_PROCESS_LIMIT: Throttle (limit process) LOCK");
        }
        
        char* run_term_msg = msgQueueRead(runOutputQueue);
        
        if(run_term_msg != NULL) {
            if(sscanf(run_term_msg, "run-terminate: %lld %d", &buffer_pid, &buffer_result)) {
                --activeTasksCount;
                log(SERVER, "Run terminated: %lld for result: %d", buffer_pid, buffer_result);
                
                if(throttled_mode && activeTasksCount < SERVER_PROCESS_LIMIT) {
                    log_warn(SERVER, "SERVER_PROCESS_LIMIT: Throttle (limit process) UNLOCK");
                    msgQueueMakeBlocking(&runOutputQueue, 0);
                    msgQueueMakeBlocking(&reportQueue,    1);
                    throttled_mode = 0;
                }
                
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
                        log_ok(SERVER, "Sent answer to the tester with pid=%d (answer=%d, loc_id=%d, runpid=%d)", ts->pid, buffer_result, rs->loc_id, pid);
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
        
        if(activeTasksCount <= 0 && shouldTerminate && children_status == 0 && run_term_msg == NULL) {
            forceTermination = 1;
        }
        
        if(!shouldTerminate) {
            
            char* msg = msgQueueRead(reportQueue);
            
            if(msg != NULL) {
                buffer[0] = '\0';
                if(strcmp(msg, "exit") == 0) {
                    log_warn(SERVER, "Server received termination command and will close. Be aware.");
                    shouldTerminate = 1;
                    
                    //log_warn(SERVER, "Wait for subprocess termination... WAIT");
                    //processWaitForAll();
                    //log_warn(SERVER, "Wait for subprocess termination... END");
                    
                } else if(sscanf(msg, "parse: %lld %s %d %[^NULL]", &buffer_pid, buffer2, &loc_id, buffer)) {
                    ++rcd_count;
                    
                    log(SERVER, "Received word {%s} (loc_id=%d)", buffer, loc_id);
                    
                    TesterSlot* ts = HashMapGetV(&testerSlots, pid_t, TesterSlot, buffer_pid);
                    if(ts == NULL) {
                        log_ok(SERVER, "Registered new tester with pid %lld for output queue: %s", buffer_pid, buffer2);
                        
                        pid_t tester_pid = (pid_t) buffer_pid;
                        
                        TesterSlot ts_new_val;
                        ts_new_val.pid = tester_pid;
                        ts_new_val.rcd_count = 0;
                        ts_new_val.acc_count = 0;
                        ts_new_val.testerInputQueue = msgQueueOpen(buffer2, LINE_BUF_SIZE, MSG_QUEUE_SIZE);
                        
                        strcpy((char*) &(ts_new_val.queueName), buffer);
                        HashMapSetV(&testerSlots, pid_t, TesterSlot, tester_pid, ts_new_val);
                        ts = HashMapGetV(&testerSlots, pid_t, TesterSlot, buffer_pid);
                    }
                    
                    
                    ++(ts->rcd_count);
             
                    RunSlot rs;
                    rs.loc_id = loc_id;
                    rs.testerSourcePid = (pid_t) buffer_pid;
                    rs.graphDataPipeID = msgPipeCreate(100);
                    rs.graphDataPipe = msgPipeOpen(rs.graphDataPipeID);
                    
                    msgPipeWrite(rs.graphDataPipe, transitionGraphDesc);

                    char graphDataPipeIDStr[100];
                    msgPipeIDToStr(rs.graphDataPipeID, graphDataPipeIDStr);
                    
                    pid_t pid;
                    
                    ++activeTasksCount;
                    if(verboseMode) {
                        if(!processExec(&pid, "./run", "run", graphDataPipeIDStr, buffer, "-v", NULL)) {
                            log_err(SERVER, "Run fork failure");
                            exit(-1);
                        }
                    } else {
                        if(!processExec(&pid, "./run", "run", graphDataPipeIDStr, buffer, NULL)) {
                            log_err(SERVER, "Run fork failure");
                            exit(-1);
                        }
                    }
                    
                    log_ok(SERVER, "Forked run %d for word {%s} (loc_id=%d)", pid, buffer, loc_id);
                    
                    rs.pid = pid;
                    HashMapSetV(&runSlots, pid_t, RunSlot, pid, rs);
                    
                }
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
    msgQueueRemove(&registerQueue);
    
    FREE(transitionGraphDesc);
    
    log(SERVER, "Final check to determine if no subprocess is left...");
    processWaitForAll();
    log_ok(SERVER, "Exit.");
    
    if(server_status_code != 0) exit(server_status_code);
    
    return 0;
}