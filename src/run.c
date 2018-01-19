#include <stdio.h>
#include "getline.h"
#include "automaton.h"
#include "msg_queue.h"
#include "msg_pipe.h"
#include "fork.h"
#include "syserr.h"
#include "syslog.h"


int acceptSync_rec(TransitionGraph tg, char* word, int word_len, int current_state, int depth) {
    
    if(depth >= word_len) {
        return tg->acceptingStates[current_state];
    }
    
    const char current_letter = word[depth];
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
    
    const char current_letter = word[depth];
    const int branch_count = tg->size[current_state][current_letter];
    
    MsgPipeID acceptAsyncDataPipeID[branch_count];
    MsgPipe acceptAsyncDataPipe[branch_count];
    pid_t acceptAsyncPid[branch_count];
    
    if(current_state >= tg->U) {
        // Existential state
        for(int i=0;i<branch_count;++i) {
            
            acceptAsyncDataPipeID[i] = msgPipeCreate(5);
            if(processFork(&acceptAsyncPid[i])) {
                
                MsgPipe parentPipe = msgPipeOpen(acceptAsyncDataPipeID[i]);
                
                if(acceptAsync_rec(tg, word, word_len, tg->graph[current_state][current_letter][i], depth+1)) {
                    msgPipeWrite(parentPipe, "A");
                    msgPipeClose(&parentPipe);
                } else {
                    msgPipeWrite(parentPipe, "N");
                    msgPipeClose(&parentPipe);
                }
                
                processExit(0);
            } else {
                acceptAsyncDataPipe[i] = msgPipeOpen(acceptAsyncDataPipeID[i]);
            }
        }
        
        for(int i=0;i<branch_count;++i) {
            char* rcv = msgPipeRead(acceptAsyncDataPipe[i]);
            if(strcmp(rcv, "A") == 0) {
                for(int j=0;j<branch_count;++j) {
                    msgPipeClose(&acceptAsyncDataPipe[j]);
                }
                processWaitForAll();
                return 1;
            }
        }
        
        for(int j=0;j<branch_count;++j) {
            msgPipeClose(&acceptAsyncDataPipe[j]);
        }
        processWaitForAll();
        return 0;
    }
    
    // Universal state
    for(int i=0;i<branch_count;++i) {
        
        acceptAsyncDataPipeID[i] = msgPipeCreate(5);
        if(processFork(&acceptAsyncPid[i])) {
            
            MsgPipe parentPipe = msgPipeOpen(acceptAsyncDataPipeID[i]);
            
            if(acceptAsync_rec(tg, word, word_len, tg->graph[current_state][current_letter][i], depth+1)) {
                msgPipeWrite(parentPipe, "A");
                msgPipeClose(&parentPipe);
            } else {
                msgPipeWrite(parentPipe, "N");
                msgPipeClose(&parentPipe);
            }
            
            processExit(0);
        } else {
            acceptAsyncDataPipe[i] = msgPipeOpen(acceptAsyncDataPipeID[i]);
        }
    }
    
    for(int i=0;i<branch_count;++i) {
        char* rcv = msgPipeRead(acceptAsyncDataPipe[i]);
        if(strcmp(rcv, "A") != 0) {
            for(int j=0;j<branch_count;++j) {
                msgPipeClose(&acceptAsyncDataPipe[j]);
            }
            processWaitForAll();
            return 0;
        }
    }
    
    for(int j=0;j<branch_count;++j) {
        msgPipeClose(&acceptAsyncDataPipe[j]);
    }
    processWaitForAll();
    return 1;
}

int acceptSync(TransitionGraph tg, char* word) {
    return acceptSync_rec(tg, word, strlen(word), tg->q0, 0);
}

int acceptAsync(TransitionGraph tg, char* word) {
    return acceptAsync_rec(tg, word, strlen(word), tg->q0, 0);
}

int main(int argc, char *argv[]) {
    
    
    MsgQueue reportQueue = msgQueueOpen("/FinAutomReportQueue", 30, 10);
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
    msgQueueWritef(reportQueue, "run-terminate: %lld", (long long)getpid());
    
    msgQueueClose(&taskQueue);
    msgQueueClose(&reportQueue);
    msgPipeClose(&graphDataPipe);
    
    
    return 0;
}