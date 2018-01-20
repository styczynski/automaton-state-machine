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

#include "gcinit.h"

int main(void) {
    
    GC_SETUP();
    
    char* inputQueueName = MALLOCATE_ARRAY(char, 40);
    sprintf(inputQueueName, "/FinAutomTesterInQ%d", getpid());
    
    MsgQueue reportQueue = msgQueueOpen("/FinAutomReportQueue", 50, 10);
    MsgQueue inputQueue = msgQueueOpenNonBlocking(inputQueueName, 50, 10);
    
    msgQueueWritef(reportQueue, "tester-register: %lld %s", (long long)getpid(), inputQueueName);
    
    char* line_buf = MALLOCATE_ARRAY(char, LINE_BUF_SIZE);
    size_t line_buf_size = LINE_BUF_SIZE;
    line_buf[0] = '\0';
   
    printf("PID: %d\n", getpid());
    
    ArrayList results = ArrayListNew();
    
    
    int req_count = 0;
    int ans_count = 0;
    int acc_count = 0;
    int loc_id = 0;
    
    int read_input = 1;
    
    while(1) {
        
        if(read_input) {
            ++loc_id;
            if(getline(&line_buf, &line_buf_size, stdin) > 0) {
                if(strcmp(line_buf, "!") == 0) {
                    log(TESTER, "Sent termination request");
                    msgQueueWrite(reportQueue, "exit");
                    read_input = 0;
                } else {
                    log(TESTER, "Sent work for verification: %s", line_buf);
                    
                    char* saved_word = MALLOCATE_ARRAY(char, strlen(line_buf)+3);
                    strcpy(saved_word, line_buf);
                    
                    ArrayListSetValueAt(&results, loc_id, saved_word);
                    
                    msgQueueWritef(reportQueue, "parse: %lld %d %s", (long long)getpid(), loc_id, line_buf);
                    ++req_count;
                }
            } else {
                read_input = 0;
            }
        }
        
        if(ans_count < req_count) {
            char* msg = msgQueueRead(inputQueue);
            if(msg != NULL) {
                int loc_id = 0, ans = 0;
                if(sscanf(msg, "%d answer: %d", &loc_id, &ans)) {
                    char* saved_word = (char*) ArrayListGetValueAt(&results, loc_id);
                    if(saved_word != NULL) {
                        printf("%s %s\n", saved_word, ((ans)?"A":"N"));
                        if(ans) {
                            ++acc_count;
                        }
                        ++ans_count;
                        log(TESTER, "Got answer from server: %s %d", saved_word, ans);
                        
                        FREE(saved_word);
                        ArrayListSetValueAt(&results, loc_id, NULL);
                        
                    } else {
                       log_err(TESTER, "Invalid locid in response from server: [%s]\n", msg); 
                    }
                } else if(strcmp(msg, "exit") == 0) {
                    log_warn(TESTER, "Got exit request from server!");
                    break;
                } else {
                    log_err(TESTER, "Invalid response from server: [%s]\n", msg);
                }
            }
        }
        
        if(!read_input && !(ans_count < req_count)) {
            break;
        }
    }
    
    printf("Snt: %d\nRcd: %d\nAcc: %d\n", req_count, ans_count, acc_count);
    
    log(TESTER, "Terminate.");
    
    LOOP_ARRAY_LIST(&results, i) {
        char* saved_word = (char*) ArrayListGetValue(i);
        if(saved_word != NULL) {
            FREE(saved_word);
        }
    }
    ArrayListDestroy(&results);
    
    msgQueueClose(&reportQueue);
    msgQueueRemove(&inputQueue);
    
    FREE(inputQueueName);
    FREE(line_buf);
    
    
    return 0;
}