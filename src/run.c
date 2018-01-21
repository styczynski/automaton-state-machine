/**
 * Implementation of Automaton for studies on Warsaw Univeristy
 *
 * [Process run]
 *   Program run receives from validator a word to verify and the description of the automaton.
 *   Then he begins the verification. When the verification stops, the process sends a message to the server and terminates.
 *
 * @author Piotr Styczyński <piotrsty1@gmail.com>
 * @copyright MIT
 * @date 2018-01-21
 */
#include <stdio.h>
#include "getline.h"
#include "automaton.h"
#include "msg_queue.h"
#include "msg_pipe.h"
#include "fork.h"
#include "syslog.h"

#include "gcinit.h"

int acceptSync_rec(TransitionGraph tg, char* word, int word_len, int current_state, int depth) {
    
    if(depth >= word_len) {
        return tg->acceptingStates[current_state];
    }
    
#if DEBUG_ACCEPT_RUN == 1    
    log_warn(RUN, "At state %d in word {%s} at pos {%d/%d}", current_state, word, depth, word_len);
#endif
    
    const int current_letter = (int)(word[depth] - 'a');
    const int branch_count = tg->size[current_state][current_letter];
    
    if(current_state >= tg->U) {
        // Existential state
        for(int i=0;i<branch_count;++i) {
            if(acceptSync_rec(tg, word, word_len, tg->graph[current_state][current_letter][i], depth+1)) {
                return 1;
            }
        }
        return 0;
    }
   
    // Universal state
    for(int i=0;i<branch_count;++i) {
        if(!acceptSync_rec(tg, word, word_len, tg->graph[current_state][current_letter][i], depth+1)) {
            return 0;
        }
    }
    return 1;
}

static int acceptAsync_rec(TransitionGraph tg, char* word, int word_len, int current_state, int depth, int* workload);

static int acceptAsync_node(int is_existential_state, TransitionGraph tg, char* word, int word_len, int current_state, int depth, int* workload) {
    
    const int current_letter = (int)(word[depth]-'a');
    const int branch_count = tg->size[current_state][current_letter];
    
    MsgPipeID acceptAsyncDataPipeID[branch_count];
    MsgPipe acceptAsyncDataPipe[branch_count];
    pid_t acceptAsyncPid[branch_count];
    
    if(branch_count <= 0) {
        return !is_existential_state;
    }
    
    for(int i=1;i<branch_count;++i) {
        
        acceptAsyncDataPipeID[i] = msgPipeCreate(5);
        int status = processFork(&acceptAsyncPid[i]);
        
        if(status == -1) {
            log_err(RUN, "Failed to fork subprocess");
            exit(-1);
        } else if(status == 1) {
            
            MsgPipe parentPipe = msgPipeOpen(acceptAsyncDataPipeID[i]);
            
            int new_workload = 0;
            
            if(acceptAsync_rec(tg, word, word_len, tg->graph[current_state][current_letter][i], depth+1, &new_workload)) {
                msgPipeWrite(parentPipe, "A");
                msgPipeClose(&parentPipe);
            } else {
                msgPipeWrite(parentPipe, "N");
                msgPipeClose(&parentPipe);
            }
            
            processExit(0);
        } else if(status == 0) {
            acceptAsyncDataPipe[i] = msgPipeOpen(acceptAsyncDataPipeID[i]);
        }
    }
    
    int originValue = acceptAsync_rec(tg, word, word_len, tg->graph[current_state][current_letter][0], depth+1, workload);
    
    if(processWaitForAll() == -1) {
        log_err(RUN, "Child exited abnormally, so terminate.");
        exit(-1);
    }
    
    if((is_existential_state && originValue) || (!is_existential_state && !originValue)) {
        for(int j=1;j<branch_count;++j) {
            msgPipeClose(&acceptAsyncDataPipe[j]);
        }
        return is_existential_state;
    }
    
    for(int i=1;i<branch_count;++i) {
        char* rcv = msgPipeRead(acceptAsyncDataPipe[i]);
        if((is_existential_state && strcmp(rcv, "A") == 0) || (!is_existential_state && strcmp(rcv, "A") != 0)) {
            for(int j=1;j<branch_count;++j) {
                msgPipeClose(&acceptAsyncDataPipe[j]);
            }
            return is_existential_state;
        }
    }
    
    for(int j=1;j<branch_count;++j) {
        msgPipeClose(&acceptAsyncDataPipe[j]);
    }
    
    return !is_existential_state;
}

static int acceptAsync_rec(TransitionGraph tg, char* word, int word_len, int current_state, int depth, int* workload) {
    
    ++(*workload);
    
    if(depth >= word_len) {
        return tg->acceptingStates[current_state];
    }
    
#if DEBUG_ACCEPT_RUN == 1    
    log_warn(RUN, "At state %d in word {%s} at pos {%d/%d}", current_state, word, depth, word_len);
#endif
    
    if(*workload < RUN_WORKLOAD_LIMIT) {
        // Sync version
        
        const int current_letter = (int)(word[depth] - 'a');
        const int branch_count = tg->size[current_state][current_letter];
        if(current_state >= tg->U) {
            // Existential state
            for(int i=0;i<branch_count;++i) {
                if(acceptAsync_rec(tg, word, word_len, tg->graph[current_state][current_letter][i], depth+1, workload)) {
                    return 1;
                }
            }
            return 0;
        }
        
        // Universal state
        for(int i=0;i<branch_count;++i) {
            if(!acceptAsync_rec(tg, word, word_len, tg->graph[current_state][current_letter][i], depth+1, workload)) {
                return 0;
            }
        }
        return 1;
    } else {
        // Async version
        
        if(current_state >= tg->U) {
            // Existential state
            return acceptAsync_node(1, tg, word, word_len, current_state, depth, workload);
        }
        
        // Universal state
        return acceptAsync_node(0, tg, word, word_len, current_state, depth, workload);
    }
}

int acceptSync(TransitionGraph tg, char* word) {
    return acceptSync_rec(tg, word, strlen(word), tg->q0, 0);
}

int acceptAsync(TransitionGraph tg, char* word) {
    int workload = 1;
    return acceptAsync_rec(tg, word, strlen(word), tg->q0, 0, &workload);
}

int main(int argc, char *argv[]) {
    
    GC_SETUP();
    
    if(argc < 2) {
        fprintf(stderr, "This command should not be manually run by user.\nIt's worker of validator server.\nAs it was executed manually it will terminate.\n");
        return -1;
    }
    
    log_set(0);
    for(int i=1;i<argc;++i) {
        if(strcmp(argv[i], "-v") == 0) {
            log_set(1);
        }
    }
    
    char* word_to_parse = argv[2];
    
    MsgQueue runOutputQueue = msgQueueOpen("/FinAutomRunOutQueue", LINE_BUF_SIZE, MSG_QUEUE_SIZE);

    MsgPipeID graphDataPipeID = msgPipeIDFromStr(argv[1]);
    MsgPipe graphDataPipe = msgPipeOpen(graphDataPipeID);
    
    log(RUN, "Ready.");
    
    char* transitionGraphDesc = msgPipeRead(graphDataPipe);
    if(transitionGraphDesc == NULL) {
        fatal(RUN, "Received empty graph description.");
    }
    log(RUN, "Received graph description: %d bytes", strlen(transitionGraphDesc));
    
    TransitionGraph tg = newTransitionGraph();
    char* transitionGraphDescIter = transitionGraphDesc;
    initTransitionGraph(tg);
    loadTransitionGraph(&transitionGraphDescIter, tg);

#if DEBUG_TRANSFERRED_GRAPH == 1
    printTransitionGraph(tg);
#endif
    
    log(RUN, "Received word to parse: %s", word_to_parse);
    
#if USE_ASYNC_ACCEPT == 1
    const int result = acceptAsync(tg, word_to_parse);
#else
    const int result = acceptSync(tg, word_to_parse);
#endif

    if(result) {
        log_ok(RUN, "Result: %s A", word_to_parse);
    } else {
        log_ok(RUN, "Result: %s N", word_to_parse);
    }
    
    log(RUN, "Terminate.");
    msgQueueWritef(runOutputQueue, "run-terminate: %lld %d", (long long)getpid(), result);
    
    msgQueueClose(&runOutputQueue);
    msgPipeClose(&graphDataPipe);
    
    return 0;
}