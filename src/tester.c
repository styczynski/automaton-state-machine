#include <stdio.h>
#include <string.h>
#include "getline.h"
#include "automaton.h"
#include "msg_queue.h"
#include "msg_pipe.h"
#include "fork.h"
#include "array_lists.h"
#include "memalloc.h"
#include "syslog.h"
#include "syserr.h"

int main(void) {
    
    char* inputQueueName = MALLOCATE_ARRAY(char, 40);
    sprintf(inputQueueName, "/FinAutomTesterInQ");
    log_err(TESTER, "queue name = %s", inputQueueName);
    
    MsgQueue reportQueue = msgQueueOpen("/FinAutomReportQueue", 50, 10);
    MsgQueue inputQueue = msgQueueOpen(inputQueueName, 50, 10);
    
    msgQueueWritef(reportQueue, "tester-register: %lld %s", (long long)getpid(), inputQueueName);
    
    char* line_buf = (char*) malloc(LINE_BUF_SIZE * sizeof(char));
    size_t line_buf_size = LINE_BUF_SIZE;
    line_buf[0] = '\0';
   
    printf("PID: %d\n", getpid());
    
    ArrayList results = ArrayListNew();
    
    int req_count = 0;
    int ans_count = 0;
    int acc_count = 0;
    int loc_id = 0;
    
    while(getline(&line_buf, &line_buf_size, stdin)>0) {
        ++loc_id;
        
        if(strcmp(line_buf, "!") == 0) {
            log(TESTER, "Sent termination request");
            msgQueueWrite(reportQueue, "exit");
            break;
        } else {
            log(TESTER, "Sent work for verification: %s", line_buf);
            
            char* saved_word = MALLOCATE_ARRAY(char, strlen(line_buf)+3);
            strcpy(saved_word, line_buf);
            
            ArrayListSetValueAt(&results, loc_id, saved_word);
            
            msgQueueWritef(reportQueue, "parse: %lld %d %s", (long long)getpid(), req_count, line_buf);
            ++req_count;
        }
    }
    
    log(TESTER, "Awaiting response...");
    
    for(int i=0;i<req_count;++i) {
        char* msg = msgQueueRead(inputQueue);
        int loc_id = 0, ans = 0;
        if(sscanf(msg, "%d answer: %d", &loc_id, &ans)) {
            char* saved_word = (char*) ArrayListGetValueAt(&results, loc_id);
            if(saved_word != NULL) {
                printf("%s %s\n", saved_word, ((ans)?"A":"N"));
                
                free(saved_word);
                ArrayListSetValueAt(&results, loc_id, NULL);
            }
        } else {
            log_err(TESTER, "Invalid response from server: [%s]\n", msg);
        }
    }
    
    printf("Snt: %d\nRcd: %d\nAcc: %d\n", req_count, ans_count, acc_count);
    
    log(TESTER, "Terminate.");
    
    LOOP_ARRAY_LIST(&results, i) {
        char* saved_word = (char*) ArrayListGetValue(i);
        if(saved_word != NULL) {
            free(saved_word);
        }
    }
    ArrayListDestroy(&results);
    
    msgQueueClose(&reportQueue);
    msgQueueRemove(&inputQueue);
    
    free(inputQueueName);
    free(line_buf);
    
    return 0;
}