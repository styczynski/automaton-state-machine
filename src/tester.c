#include <stdio.h>
#include <string.h>
#include "getline.h"
#include "automaton.h"
#include "msg_queue.h"
#include "msg_pipe.h"
#include "fork.h"
#include "syslog.h"
#include "syserr.h"

int main(void) {
    
    char* inputQueueName = "/alamakota";
    
    MsgQueue reportQueue = msgQueueOpen("/FinAutomReportQueue", 50, 10);
    MsgQueue inputQueue = msgQueueOpen(inputQueueName, 50, 10);
    
    msgQueueWritef(reportQueue, "tester-register: %lld %s", (long long)getpid(), inputQueueName);
    
    char* line_buf = (char*) malloc(LINE_BUF_SIZE * sizeof(char));
    size_t line_buf_size = LINE_BUF_SIZE;
    
    int req_count = 0;
    while(getline(&line_buf, &line_buf_size, stdin)>0) {
        
        if(strcmp(line_buf, "!") == 0) {
            log(TESTER, "Sent termination request");
            msgQueueWrite(reportQueue, "exit");
            break;
        } else {
            log(TESTER, "Sent work for verification: %s", line_buf);
            msgQueueWritef(reportQueue, "parse: %lld %d %s", (long long)getpid(), req_count, line_buf);
            ++req_count;
        }
    }
    
    log(TESTER, "Awaiting response...");
    
    for(int i=0;i<req_count;++i) {
        char* msg = msgQueueRead(inputQueue);
        int loc_id, ans;
        if(!sscanf(msg, "%d answer: %d", &loc_id, &ans)) {
            
        } else {
            log_ok(TESTER, "Answer for id=%d is %d", loc_id, ans);
        }
    }
    
    log(TESTER, "Terminate.");
    
    msgQueueClose(&reportQueue);
    msgQueueClose(&inputQueue);
    
    return 0;
}