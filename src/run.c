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

/*
 * Valid execution parameters:
 *
 *    run <stringified_MsgPipe_object> <word_to_parse> [-v]
 *
 *   -v flag is used to indicate verbosive logging
 * 
 *   The run command should not be ever executed by user.
 *   It's internal worker of the server.
 *
 * Details of process communication is described further in file validator.c
 */
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
    
    // Queue to write termination status
    MsgQueue runOutputQueue = msgQueueOpen("/FinAutomRunOutQueue", LINE_BUF_SIZE, MSG_QUEUE_SIZE);

    // Capture pipe by which the server will send the graph representation
    MsgPipeID graphDataPipeID = msgPipeIDFromStr(argv[1]);
    MsgPipe graphDataPipe = msgPipeOpen(graphDataPipeID);
    
    log(RUN, "Ready.");
    
    // Load transition graph description
    char* transitionGraphDesc = msgPipeRead(graphDataPipe);
    if(transitionGraphDesc == NULL) {
        fatal(RUN, "Received empty graph description.");
    }
    log(RUN, "Received graph description: %d bytes", strlen(transitionGraphDesc));
    
    // Load transition graph from its description
    TransitionGraph tg = newTransitionGraph();
    char* transitionGraphDescIter = transitionGraphDesc;
    initTransitionGraph(tg);
    loadTransitionGraph(&transitionGraphDescIter, tg);

#if DEBUG_TRANSFERRED_GRAPH == 1
    printTransitionGraph(tg);
#endif
    
    log(RUN, "Received word to parse: %s", word_to_parse);
    
    // Run sync/async accept on the received word
    
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
    
    // Commit results to the server
    msgQueueWritef(runOutputQueue, "run-terminate: %lld %d", (long long)getpid(), result);
    
    // Close all means of communication
    msgQueueClose(&runOutputQueue);
    msgPipeClose(&graphDataPipe);
    
    return 0;
}