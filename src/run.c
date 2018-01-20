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
    
    const int current_letter = (int)word[depth];
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
    
    for(int i=0;i<branch_count;++i) {
        if(!acceptSync_rec(tg, word, word_len, tg->graph[current_state][current_letter][i], depth+1)) {
            return 0;
        }
    }
    return 1;
}

int acceptAsync_rec(TransitionGraph tg, char* word, int word_len, int current_state, int depth) {
    
    if(depth >= word_len) {
        return tg->acceptingStates[current_state];
    }
    
    log_warn(RUN, " > word %s at char # %d = %c, state = %d", word, depth, word[depth], current_state);
    
    const int current_letter = (int)word[depth];
    const int branch_count = tg->size[current_state][current_letter];
    
    MsgPipeID acceptAsyncDataPipeID[branch_count];
    MsgPipe acceptAsyncDataPipe[branch_count];
    pid_t acceptAsyncPid[branch_count];
    
    _trigForkErr_ = 1;
    
    if(current_state >= tg->U) {
        // Existential state
        for(int i=0;i<branch_count;++i) {
            
            acceptAsyncDataPipeID[i] = msgPipeCreate(5);
            int status = processFork(&acceptAsyncPid[i]);
            
            if(status == -1) {
                log_err(RUN, "Failed to fork subprocess");
                exit(-1);
            } else if(status == 1) {
                
                MsgPipe parentPipe = msgPipeOpen(acceptAsyncDataPipeID[i]);
                
                if(acceptAsync_rec(tg, word, word_len, tg->graph[current_state][current_letter][i], depth+1)) {
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
        
        if( processWaitForAll() == -1 ) {
            log_err(RUN, "Child exited abnormally, so terminate.");
            exit(-1);
        }
        
        for(int i=0;i<branch_count;++i) {
            char* rcv = msgPipeRead(acceptAsyncDataPipe[i]);
            if(strcmp(rcv, "A") == 0) {
                for(int j=0;j<branch_count;++j) {
                    msgPipeClose(&acceptAsyncDataPipe[j]);
                }
                return 1;
            }
        }
        
        for(int j=0;j<branch_count;++j) {
            msgPipeClose(&acceptAsyncDataPipe[j]);
        }
        
        return 0;
    }
    
    // Universal state
    for(int i=0;i<branch_count;++i) {
        
        acceptAsyncDataPipeID[i] = msgPipeCreate(5);
        int status = processFork(&acceptAsyncPid[i]);
        
        if(status == -1) {
            log_err(RUN, "Failed to fork subprocess");
            exit(-1);
        } else if(status == 1) {
            
            MsgPipe parentPipe = msgPipeOpen(acceptAsyncDataPipeID[i]);
            
            if(acceptAsync_rec(tg, word, word_len, tg->graph[current_state][current_letter][i], depth+1)) {
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
    
    if( processWaitForAll() == -1 ) {
        log_err(RUN, "Child exited abnormally, so terminate.");
        exit(-1);
    }
    
    for(int i=0;i<branch_count;++i) {
        char* rcv = msgPipeRead(acceptAsyncDataPipe[i]);
        if(strcmp(rcv, "A") != 0) {
            for(int j=0;j<branch_count;++j) {
                msgPipeClose(&acceptAsyncDataPipe[j]);
            }
            return 0;
        }
    }
    
    for(int j=0;j<branch_count;++j) {
        msgPipeClose(&acceptAsyncDataPipe[j]);
    }

    return 1;
}

int acceptSync(TransitionGraph tg, char* word) {
    return acceptSync_rec(tg, word, strlen(word), tg->q0, 0);
}

int acceptAsync(TransitionGraph tg, char* word) {
    return acceptAsync_rec(tg, word, strlen(word), tg->q0, 0);
}

int main(int argc, char *argv[]) {
    
    GC_SETUP();
    GC_LOG_ON();
    
    if(argc < 2) {
        fatal(RUN, "Wrong number of parameters should be at least 1.");
    }
    
    MsgQueue reportQueue = msgQueueOpen("/FinAutomReportQueue", 50, 10);
    MsgQueue taskQueue = msgQueueOpen("/FinAutomTaskQueue", 30, 10);
    
    MsgPipeID graphDataPipeID = msgPipeIDFromStr(argv[1]);
    MsgPipe graphDataPipe = msgPipeOpen(graphDataPipeID);
    
    log(RUN, "Ready.");
    
    char* transitionGraphDesc = msgPipeRead(graphDataPipe);
    log(RUN, "Received graph description: %d bytes", strlen(transitionGraphDesc));
    
    TransitionGraph tg = newTransitionGraph();
    char* transitionGraphDescIter = transitionGraphDesc;
    loadTransitionGraph(&transitionGraphDescIter, tg);
    
    char word_to_parse[LINE_BUF_SIZE];
    msgQueueReadf(taskQueue, "parse: %[^NULL]", word_to_parse);
    
    log(RUN, "Received word to parse: %s", word_to_parse);
    
    
    const int result = acceptAsync(tg, word_to_parse);
    if(result) {
        log_ok(RUN, "Result: %s A", word_to_parse);
    } else {
        log_ok(RUN, "Result: %s N", word_to_parse);
    }
    
    log(RUN, "Terminate.");
    msgQueueWritef(reportQueue, "run-terminate: %lld %d", (long long)getpid(), result);
    
    msgQueueClose(&taskQueue);
    msgQueueClose(&reportQueue);
    msgPipeClose(&graphDataPipe);
    
    return 0;
}