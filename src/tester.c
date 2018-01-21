/**
 * Implementation of Automaton for studies on Warsaw Univeristy
 *
 * [Process tester]
 *   Queries in a form of words are sent by programs tester.
 *
 *   A program tester in an infinite loop read words form the standard input stream and forwards them to the server.
 *   Every word is written in a single line and the line feed symbol \n does not belong to any of the words.
 *   When the tester receives an answer from the server,
 *   he writes on the standard output stream the word he resecived answer for and the decision A if the word
 *   was accepted and N if not.
 *   The tester terminates when the server terminates of when tester receives the EOF symbol,
 *   i.e. the end of file symbol. When the tester terminates it sends no new queries,
 *   waits for the remaining answers from the server, and writes on the standard output a report.
 * 
 *   A Report of a tester consist of three lines
 *
 *     Snt: x\n
 *     Rcd: y\n
 *     Acc: z\n
 *
 * where x,y,z respectively are the numbers of: queries, received answers, and accepted words sent by this process.
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
#include "array_lists.h"
#include "memalloc.h"
#include "syslog.h"

#include "gcinit.h"

/*
 * Valid execution parameters:
 *
 *    tester [-v]
 *
 *      Use -v flag to enable verbosive logging.
 *
 */
int main(int argc, char *argv[]) {
    
    GC_SETUP();
    
    log_set(0);
    for(int i=1;i<argc;++i) {
        if(strcmp(argv[i], "-v") == 0) {
            log_set(1);
        }
    }
    
    // Create new queue for the tester - it will receive the results from server
    char* inputQueueName = MALLOCATE_ARRAY(char, 40);
    sprintf(inputQueueName, "/FinAutomTesterInQ%d", getpid());
    
    /*
     * Open registration queue
     * Each of the tester programs before doing anything registers itself on a server
     */
    MsgQueue registerQueue = msgQueueOpen("/FinAutomRegisterQueue", LINE_BUF_SIZE, MSG_QUEUE_SIZE);
    msgQueueWritef(registerQueue, "register_tester: %lld %s", (long long)getpid(), inputQueueName);
    msgQueueClose(&registerQueue);
    
    // Open the queue for sending data to the server
    MsgQueue reportQueue = msgQueueOpen("/FinAutomReportQueue", LINE_BUF_SIZE, MSG_QUEUE_SIZE);
    
    // Open input queue - server will send here the validation results
    MsgQueue inputQueue = msgQueueOpenNonBlocking(inputQueueName, LINE_BUF_SIZE, MSG_QUEUE_SIZE);
    
    //msgQueueWritef(reportQueue, "tester-register: %lld %s", (long long)getpid(), inputQueueName);
    
    /*
     * Allocates handful line buffer
     */ 
    char* line_buf = MALLOCATE_ARRAY(char, LINE_BUF_SIZE);
    size_t line_buf_size = LINE_BUF_SIZE;
    line_buf[0] = '\0';
   
    printf("PID: %d\n", getpid());
    
    /*
     * Array list with the result.
     * Each of the words has its mapped local id (loc_id)
     * The words are identified by their local id.
     * The server answer will not contain the input word but only it's loc_id.
     *
     *  ________                                ________
     * | TESTER |  --[ word, loc_id, ... ]-->  | SERVER |
     * |________|                              |________|
     *
     *  ________                                ________
     * | TESTER |  <-[ loc_id, results ... ]-- | SERVER |
     * |________|                              |________|
     *
     * Details of process communication is described further in file validator.c
     * 
     */
    ArrayList results = ArrayListNew();

    int req_count = 0;
    int ans_count = 0;
    int acc_count = 0;
    int loc_id = 0;
    
    int read_input = 1;
    
    while(1) {
        
        if(read_input) {
            ++loc_id;
            
            // Read word to be parsed
            int getline_size = getline(&line_buf, &line_buf_size, stdin);
            if(getline_size >= 0) {
                if(strcmp(line_buf, "!") == 0) {
                    log_warn(TESTER, "Sent termination request");
                    
                    // Send termination request to the server
                    msgQueueWrite(reportQueue, "exit");
                    read_input = 0;
                } else {
                    log(TESTER, "Sent work for verification: %s (loc_id=%d)", line_buf, loc_id);
                    
                    char* saved_word = MALLOCATE_ARRAY(char, strlen(line_buf)+3);
                    strcpy(saved_word, line_buf);
                    
                    // Save the word with it's loc_id locally
                    ArrayListSetValueAt(&results, loc_id, saved_word);
                    
                    // Send word to verification
                    msgQueueWritef(reportQueue, "parse: %lld %s %d %s", (long long)getpid(), inputQueueName, loc_id, line_buf);
                    ++req_count;
                }
            } else if(getline_size == -1) {
                log_warn(TESTER, "Ended input reading. Input has terminated.");
                read_input = 0;
            }
        }
        
        if(ans_count < req_count) {
            char* msg = msgQueueRead(inputQueue);
            if(msg != NULL) {
                int loc_id = 0, ans = 0;
                
                // Server has answered
                if(sscanf(msg, "%d answer: %d", &loc_id, &ans)) {
                    char* saved_word = (char*) ArrayListGetValueAt(&results, loc_id);
                    if(saved_word != NULL) {
                        
                        // Print the answer to the output
                        printf("%s %s\n", saved_word, ((ans)?"A":"N"));
                        if(ans) {
                            ++acc_count;
                        }
                        ++ans_count;
                        log(TESTER, "Got answer from server: %s %d (loc_id=%d)", saved_word, ans, loc_id);
                        
                        FREE(saved_word);
                        
                        // Remove the stored word from the loc_id table
                        ArrayListSetValueAt(&results, loc_id, NULL);
                        
                    } else {
                       log_err(TESTER, "Invalid locid in response from server: [%s]\n", msg); 
                    }
                // The server has terminated/crashed
                } else if(strcmp(msg, "exit") == 0) {
                    log_warn(TESTER, "Got exit request from server!");
                    break;
                // Invalid command from server
                } else {
                    log_err(TESTER, "Invalid response from server: [%s]\n", msg);
                }
            }
        }
        
        // If there's no input left and all request have been parsed then exit
        if(!read_input && !(ans_count < req_count)) {
            break;
        }
    }
    
    // Print final report
    printf("Snt: %d\nRcd: %d\nAcc: %d\n", req_count, ans_count, acc_count);
    
    log(TESTER, "Terminate.");
    
    LOOP_ARRAY_LIST(&results, i) {
        char* saved_word = (char*) ArrayListGetValue(i);
        if(saved_word != NULL) {
            FREE(saved_word);
        }
    }
    ArrayListDestroy(&results);
    
    // Close means of communication
    msgQueueClose(&reportQueue);
    msgQueueRemove(&inputQueue);
    
    FREE(inputQueueName);
    FREE(line_buf);
    
    
    return 0;
}