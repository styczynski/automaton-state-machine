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
#include "onexit.h"
#include "fork.h"
#include "syslog.h"
#include "hashmap.h"

#include "gcinit.h"

typedef struct RunSlot RunSlot;
typedef struct TesterSlot TesterSlot;

/**
 * Strucutre to hold session details for run worker
 */
struct RunSlot {
    MsgPipeID graphDataPipeID;
    MsgPipe graphDataPipe;
    pid_t pid;
    pid_t testerSourcePid;
    int loc_id;
};

/**
 * Strucutre to hold session details for registered tester
 */
struct TesterSlot {
    char queueName[100];
    pid_t pid;
    MsgQueue testerInputQueue;
    int rcd_count;
    int acc_count;
};

/**
 * In verbose mode server actions logging is enabled.
 */
int verboseMode = 0;

int slots_inited = 0;
HashMap runSlots;
HashMap testerSlots;

/*
 * Custom server exit handler to send exit messages to all of registered the testers
 */
void onExit() {
    // There's nothing that we can do
    // In case of slots_inited = 1 the slots are broken for sure
    // We cannot read them
    if(!slots_inited) return;
    
    // Broadcast exit command to all testers
    LOOP_HASHMAP(&testerSlots, i) {
        TesterSlot* ts = (TesterSlot*) HashMapGetValue(i);
        msgQueueWritef(ts->testerInputQueue, "exit");
    }
}

int main(int argc, char *argv[]) {
    
    GC_SETUP();
    ExitHandlerSetup();
    
    log_set(0);
    for(int i=1;i<argc;++i) {
        if(strcmp(argv[i], "-v") == 0) {
            log_set(1);
            verboseMode = 1;
        }
    }
    
    // Register custom server exit handler (note: it's executed before GC cleanup)
    ExitHandlerAdd(onExit);
    
    // Server sessions for run and tester processes
    HashMap runSlots = HashMapNew(HashMapIntCmp);
    HashMap testerSlots = HashMapNew(HashMapIntCmp);
    // Indicate initialization of slots hashmap
    slots_inited = 1;
    
    // The final returned exit code during normal operation
    int server_status_code = 0;
    
    // Load transition graph description from the standard input
    char* transitionGraphDesc = loadTransitionGraphDescFromStdin();

    // Queue to receive commands from testers
    MsgQueue reportQueue = msgQueueOpen("/FinAutomReportQueue", LINE_BUF_SIZE, MSG_QUEUE_SIZE);
    
    // Queue to receive results from run workers
    MsgQueue runOutputQueue = msgQueueOpenNonBlocking("/FinAutomRunOutQueue", LINE_BUF_SIZE, MSG_QUEUE_SIZE);
    
    // Queue to receive "register" events from testers
    MsgQueue registerQueue = msgQueueOpenNonBlocking("/FinAutomRegisterQueue", LINE_BUF_SIZE, MSG_QUEUE_SIZE);
    
    // Two hdelper buffers 
    char buffer[LINE_BUF_SIZE];
    char buffer2[LINE_BUF_SIZE];
    
    // Helper uffers to store other values
    long long buffer_pid;
    int buffer_result;
    int loc_id;
    
    log_ok(SERVER, "Server is up.");
    
    // activeTasksCount is number of launched run workers
    int activeTasksCount = 0;
    
    // Flag to indicate request for termination
    int shouldTerminate = 0;
    
    // If conditions for termination are met and shouldTerminate is true
    // then this flag is set and in the next server loop iteration server will exit
    int forceTermination = 0;
    
    // Server operation statistics
    int rcd_count = 0;
    int snt_count = 0;
    int acc_count = 0;
    
    // Throttled mode flag
    int throttled_mode = 0;
    
    // Server event loop
    while(1) {
        ++cycle_num;
        
        /*
         * Read all available (if any) incoming register events and start sessions for all the testers.
         * This reading is NON BLOCKING.
         */
        char* register_msg;
        do {
            //log_info(SERVER, "Read register queue");
            register_msg = msgQueueRead(registerQueue);
            if(register_msg != NULL) {
                
                if(sscanf(register_msg, "register_tester: %lld %s", &buffer_pid, buffer2)) {
                    log_ok(SERVER, "Registered new tester with pid %lld for output queue: %s", buffer_pid, buffer2);
                        
                    pid_t tester_pid = (pid_t) buffer_pid;
                        
                    // Create new session for the tester
                    TesterSlot ts_new_val;
                    ts_new_val.pid = tester_pid;
                    ts_new_val.rcd_count = 0;
                    ts_new_val.acc_count = 0;
                    ts_new_val.testerInputQueue = msgQueueOpen(buffer2, LINE_BUF_SIZE, MSG_QUEUE_SIZE);
                        
                    strcpy((char*) &(ts_new_val.queueName), buffer);
                    
                    // Save the session
                    HashMapSetV(&testerSlots, pid_t, TesterSlot, tester_pid, ts_new_val);
                } else {
                    log_err(SERVER, "Invalid register command!");
                }
            }
        } while(register_msg != NULL);
        
        
        /*
         * If it's not throttled mode, but too much worker sessions are active then
         * enable throttling to limit number of running forked processes. 
         */
        if(!throttled_mode && activeTasksCount > SERVER_PROCESS_LIMIT) {
            throttled_mode = 1;
            
            /*
             * In throttling mode we prefer worker termination event over new parse request
             * So we change normal operating mode of both queues.
             *
             *   The queue with inputs will be NON BLOCKING
             *   And the queue with worker termination events will be BLOCKING
             *
             * Thanks to that change number of running workers will be not increased greatly
             * (just about +-1 / +0)
             *
             */
            msgQueueMakeBlocking(&runOutputQueue, 1);
            msgQueueMakeBlocking(&reportQueue,    0);
            log_warn(SERVER, "SERVER_PROCESS_LIMIT: Throttle (limit process) LOCK");
        }
        
        /*
         * Read termination message (if any) from the testers
         * This reading is NON BLOCKING.
         */
        //log_info(SERVER, "Read output queue");
        char* run_term_msg = msgQueueRead(runOutputQueue);
        
        if(run_term_msg != NULL) {
            if(sscanf(run_term_msg, "run-terminate: %lld %d", &buffer_pid, &buffer_result)) {
                --activeTasksCount;
                log(SERVER, "Run terminated: %lld for result: %d", buffer_pid, buffer_result);
                
                /*
                 * The number of active worker sessions has decremented.
                 * So check out if the throttled mode can be disabled?
                 */
                if(throttled_mode && activeTasksCount < SERVER_PROCESS_LIMIT) {
                    log_warn(SERVER, "SERVER_PROCESS_LIMIT: Throttle (limit process) UNLOCK");
                    
                    /*
                     * Switch back to normal operation mode.
                     */
                    msgQueueMakeBlocking(&runOutputQueue, 0);
                    msgQueueMakeBlocking(&reportQueue,    1);
                    throttled_mode = 0;
                }
                
                pid_t pid = (pid_t) buffer_pid;
                
                // Read the associated worker session
                RunSlot* rs = HashMapGetV(&runSlots, pid_t, RunSlot, pid);
                
                if(rs == NULL) {
                    /*
                     * Missing wroker session data!
                     * Probable reasons:
                     *    -> Internal error in the HashMap
                     *    -> Run process that was not property of the current server has terminated and saved its results
                     *    -> We received some outdated event (from the server running earlier that crashed)
                     */
                    log_err(SERVER, "Missing run slot info for pid=%d", pid);
                } else {
                    msgPipeClose(&(rs->graphDataPipe));
                
                    /*
                     * This code will try to find matching tester session that is the source
                     * of the terminated worker.
                     */
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
                        /*
                         * Missing tester session.
                         * Probable reasons:
                         *    -> Internal error in the HashMap
                         *    -> Run process that was not property of the current server has terminated and saved its results
                         *    -> We received some outdated event (from the server running earlier that crashed)
                         */
                        log_err(SERVER, "Missing tester slot info for run of pid=%d (tester pid=%d)", rs->pid, ts->pid);
                    } else {
                        // Update tester statistics
                        ++snt_count;
                        if(buffer_result == 1) {
                            ++acc_count;
                            ++(ts->acc_count);
                        }
                        
                        // Send the answer back to tester process
                        log_ok(SERVER, "Sent answer to the tester with pid=%d (answer=%d, loc_id=%d, runpid=%d)", ts->pid, buffer_result, rs->loc_id, pid);
                        msgQueueWritef(ts->testerInputQueue, "%d answer: %d", rs->loc_id, buffer_result);
                    }
                    
                    // Remove worker session
                    HashMapRemoveV(&runSlots, pid_t, RunSlot, pid);
                
                }
                
            } else {
                log_err(SERVER, "Invalid run response!");
            }
        }
        
        // Server SHOULD terminate on abnormal worker termination

        /*
         * Wait for worker termination events
         * This operation is NON BLOCKING
         */
        //log_info(SERVER, "Checkout children status");
        int children_status = processWaitForAllNonBlocking();
        if(children_status == -1) {
            /*
             * Error in some child (worker process)
             * -1 code means that:
             *   -> Some worker terminated abnormally via other method than exit(status) (signal etc.)
             *   -> Some worker has returned non-zero exit code
             */
#if SERVER_TERMINATE_ON_RUN_FAILURE == 1
            log_err(SERVER, "Server detected crash in some RUN subprocess so will terminate.");
            log_warn(SERVER, "All current jobs were finished so execute terminate request.");

            // Wait for all other processes
            log_warn(SERVER, "Wait for subprocess termination... WAIT");
            processWaitForAll();
            log_warn(SERVER, "Wait for subprocess termination... END");
            
            // Broadcast exit command to all of the testers
            LOOP_HASHMAP(&testerSlots, i) {
                TesterSlot* ts = (TesterSlot*) HashMapGetValue(i);
                msgQueueWritef(ts->testerInputQueue, "exit");
            }
            server_status_code = -1;
            break;
#else
            log_err(SERVER, "Server detected crash in some RUN subprocess but will NOT terminate.");
            --activeTasksCount;
#endif
        }
        
        
        
        /*
         * Enter force termination mode (the server will shut down in the next step).
         * The force termination is executed if all of the following conditions are met:
         *
         *   - shouldTerminate was set to 1 (means termination request)
         *   - children_status == 0         (no worker was terminated from the last iteration)
         *   - run_term_msg                 (no worker sent termination message)
         *   - activeTasksCount <= 0        (there are no left tasks that are beeing executed)
         *
         */
        int activeTasksCountM = 0;
        LOOP_HASHMAP(&runSlots, i) {
            ++activeTasksCountM;
        }
         
        if((activeTasksCount <= 0 || activeTasksCountM <= 0) && shouldTerminate && children_status == 0 && run_term_msg == NULL) {
            log_info(SERVER, "Request force termination (normal mode)");
            forceTermination = 1;
        }
        
        /*
         * If termination was not requested parse next input command.
         */
        if(!shouldTerminate) {
            //log_info(SERVER, "Read input queue");
            char* msg = msgQueueRead(reportQueue);
            
            if(msg != NULL) {
                buffer[0] = '\0';
                /*
                 * Received termination request from the tester
                 */
                if(strcmp(msg, "exit") == 0) {
                    
                    log_warn(SERVER, "Server received termination command and will close. Be aware.");
                    shouldTerminate = 1;
                    
                // Received word to be parsed
                } else if(sscanf(msg, "parse: %lld %s %d %[^NULL]", &buffer_pid, buffer2, &loc_id, buffer)) {
                    ++rcd_count;
                    
                    log(SERVER, "Received word {%s} (loc_id=%d)", buffer, loc_id);
                    
                    // Obtain the tester session
                    TesterSlot* ts = HashMapGetV(&testerSlots, pid_t, TesterSlot, buffer_pid);
                    if(ts == NULL) {
                        /*
                         * The session was not found so the tester has not been registered.
                         * It's okay as we can register the tester anyway.
                         *
                         * Missing session data on parse in not considered an error.
                         * As the register command is udeful but optional.
                         */
                        log_ok(SERVER, "Registered new tester with pid %lld for output queue: %s", buffer_pid, buffer2);
                        
                        pid_t tester_pid = (pid_t) buffer_pid;
                        
                        // Create new tester session
                        TesterSlot ts_new_val;
                        ts_new_val.pid = tester_pid;
                        ts_new_val.rcd_count = 0;
                        ts_new_val.acc_count = 0;
                        ts_new_val.testerInputQueue = msgQueueOpen(buffer2, LINE_BUF_SIZE, MSG_QUEUE_SIZE);
                        
                        strcpy((char*) &(ts_new_val.queueName), buffer);
                        
                        // Save the session
                        HashMapSetV(&testerSlots, pid_t, TesterSlot, tester_pid, ts_new_val);
                        ts = HashMapGetV(&testerSlots, pid_t, TesterSlot, buffer_pid);
                    }
                    
                    // Update request statistics
                    ++(ts->rcd_count);
                    
                    /*
                     * Create new worker session
                     */
                    RunSlot rs;
                    rs.loc_id = loc_id;
                    rs.testerSourcePid = (pid_t) buffer_pid;
                    rs.graphDataPipeID = msgPipeCreate(FILE_BUF_SIZE);
                    rs.graphDataPipe = msgPipeOpen(rs.graphDataPipeID);
                    
                    char graphDataPipeIDStr[1000];
                    msgPipeIDToStr(rs.graphDataPipeID, graphDataPipeIDStr);
                    
                    pid_t pid;
                    
                    /* 
                     * Spawn the worker.
                     * If the -v option is present then it's passed to the worker process.
                     */
                    char* vFlag = "-v";
                    char* vFlagArg = NULL;
                     
                    if(verboseMode) {
                        vFlagArg = vFlag;
                    } else {
                        vFlagArg = NULL;
                    }
                    
                    /*
                     * This loops do the spawning.
                     * If exec fails then rety a few times...
                     */
                    int retry_count = 0;
                    int worker_spawned = 1;
                    
                    log_info(SERVER, "Spawn worker...");
                    
                    while(!processExec(&pid, "./run", "run", graphDataPipeIDStr, buffer, vFlagArg, NULL)) {
                        log_err(SERVER, "Worker process has failed, try to retry...");
                        ++retry_count;
                        if(retry_count >= SERVER_FORK_RETRY_COUNT) {
                            worker_spawned = 0;
                            break;
                        }
                        sleep(1);
                    }
                    
                    if(worker_spawned) {
                        
                        log_ok(SERVER, "Forked run %d for word {%s} (loc_id=%d)", pid, buffer, loc_id);
                        rs.pid = pid;
                        
                        // Save worker session
                        HashMapSetV(&runSlots, pid_t, RunSlot, pid, rs);
                        
                        log_info(SERVER, "Push graph into pipe");
                        
                        // Send the graph to the worker
                        msgPipeWrite(rs.graphDataPipe, transitionGraphDesc);
                        ++activeTasksCount;

                    } else {
                        /*
                         * Log the event
                         *
                         * In this scenario the worker could not be spawned so we do not save the session info
                         * and try to continue normal execution.
                         *
                         * (we ommit one word)
                         */
                         log_err(SERVER, "Failed to fork worker, but continue anyway.");
                         
                    }
                    
                } else {
                    log_err(SERVER, "Invalid server input command!");
                }
            }
        }
        
        /*
         * If the termination can be done
         */
        if(forceTermination) {
            log_warn(SERVER, "All current jobs were finished so execute terminate request.");
            
            // Broadcast exit command to all testers
            LOOP_HASHMAP(&testerSlots, i) {
                TesterSlot* ts = (TesterSlot*) HashMapGetValue(i);
                msgQueueWritef(ts->testerInputQueue, "exit");
            }
            
            break;
        }
    }
    
    log_warn(SERVER, "Terminating server...");
    
    // Close all pipes for sending data to workers
    LOOP_HASHMAP(&runSlots, i) {
        RunSlot* rs = (RunSlot*) HashMapGetValue(i);
        msgPipeClose(&(rs->graphDataPipe));
    }
    
    /*
     * Print the server operation statistics
     */
    printf("Rcd: %d\n", rcd_count);
    printf("Snt: %d\n", snt_count);
    printf("Acc: %d\n", acc_count);
    
    /*
     * Close all the tester means of communication
     */
    LOOP_HASHMAP(&testerSlots, i) {
        TesterSlot* ts = (TesterSlot*) HashMapGetValue(i);
        if(ts->rcd_count > 0) {
            printf("PID: %d\n", ts->pid);
            printf("Rcd: %d\n", ts->rcd_count);
            printf("Acc: %d\n", ts->acc_count);
        }
        msgQueueClose(&(ts->testerInputQueue));
    }
    
    // Destroy all sessions
    HashMapDestroyV(&runSlots, pid_t, RunSlot);
    HashMapDestroyV(&testerSlots, pid_t, TesterSlot);
    
    // Remove server input/output queues
    msgQueueRemove(&reportQueue);
    msgQueueRemove(&runOutputQueue);
    msgQueueRemove(&registerQueue);
    
    FREE(transitionGraphDesc);
    
    /*
     * We should now have all children terminated, but wait for them if there's any worker left.
     */
    log(SERVER, "Final check to determine if no subprocess is left...");
    processWaitForAll();
    log_ok(SERVER, "Exit.");
    
    // Return server exit code
    if(server_status_code != 0) exit(server_status_code);
    
    return 0;
}
