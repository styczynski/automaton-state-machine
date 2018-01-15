#include <stdio.h>
#include "getline.h"
#include "automaton.h"
#include "msg_queue.h"
#include "msg_pipe.h"
#include "fork.h"
#include "syserr.h"

int main(void) {
    
    MsgQueue taskQueue = msgQueueOpen("/FinAutomTskQueue", 30, 10);
    
    char* line_buf = (char*) malloc(LINE_BUF_SIZE * sizeof(char));
    size_t line_buf_size = LINE_BUF_SIZE;
    
    while(getline(&line_buf, &line_buf_size, stdin)) {
        printf("Tester sending {%s}\n", line_buf);
        msgQueueWritef(taskQueue, "parse: %s", line_buf);
    }
    
    msgQueueClose(&taskQueue);
    
    return 0;
}