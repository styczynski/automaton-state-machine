#include <stdio.h>
#include "getline.h"
#include "automaton.h"
#include "msg_queue.h"
#include "msg_pipe.h"
#include "syserr.h"
#include "syslog.h"

int main(int argc, char *argv[]) {
    
    
    MsgQueue dataQueue = msgQueueOpen("/FinAutomDataQueue", 1000, 2);
    MsgQueue reportQueue = msgQueueOpen("/FinAutomReportQueue", 30, 10);
    MsgQueue taskQueue = msgQueueOpen("/FinAutomTaskQueue", 30, 10);
    
    log_ok("RUN", "Ready.");
    
    char* transitionGraphDesc = msgQueueSeek(dataQueue);
    log("RUN", "Received graph description: %d bytes", strlen(transitionGraphDesc));
    
    char word_to_parse[LINE_BUF_SIZE];
    msgQueueReadf(taskQueue, "parse: %[^NULL]", word_to_parse);
    
    log_ok("RUN", "Received word to parse: %s", word_to_parse);
    
    msgQueueWritef(reportQueue, "run-terminate: %lld", (long long)getpid());
    
    log("RUN", "Terminate.");
    
    msgQueueClose(&taskQueue);
    msgQueueClose(&reportQueue);
    msgQueueClose(&dataQueue);
    
    return 0;
}