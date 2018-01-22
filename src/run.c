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
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <sys/prctl.h>

#include "getline.h"
#include "automaton.h"
#include "msg_queue.h"
#include "msg_pipe.h"
#include "fork.h"
#include "syslog.h"

#include "gcinit.h"

/*
 * Signal used when parent is killed
 */
#define SIG_PARENT_KILLED SIGUSR1

static int parent_terminated_sig = 0;

/*
 * Handler invoked on SIG_PARENT_KILLED signal (when parent of the process is killed)
 */
void parent_killed_sig_handler(int sig) {
    (void) sig;
    parent_terminated_sig = 1;
}

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
    
    /*
     * When parent dies the program will receive SIG_PARENT_KILLED
     */
    if(prctl(PR_SET_PDEATHSIG, SIG_PARENT_KILLED) == -1) {
        syserr("Failed to setup parent kill signal kernel request.");
    }
    
    struct sigaction action;
    sigset_t block_mask;
    sigfillset (&block_mask);
    action.sa_handler = parent_killed_sig_handler;
    action.sa_mask = block_mask;
    action.sa_flags = 0;
    
    /*
     * Setup SIG_PARENT_KILLED signal handler
     */
    if (sigaction (SIG_PARENT_KILLED, &action, 0) == -1) {
        syserr("Failed to setup parent kill signal handler.");
    }
    
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
    
    // Check if parent has died
    if(parent_terminated_sig == 1) {
        log_err(RUN, "Ups! The parent process has died - terminate abnormally.");
        exit(-1);
    }
    
    // Queue to write termination status
    MsgQueue runOutputQueue = msgQueueOpen("/FinAutomRunOutQueue", LINE_BUF_SIZE, MSG_QUEUE_SIZE);

    // Capture pipe by which the server will send the graph representation
    MsgPipeID graphDataPipeID = msgPipeIDFromStr(argv[1]);
    MsgPipe graphDataPipe = msgPipeOpen(graphDataPipeID);
    
    log(RUN, "Ready.");
    
    // Check if parent has died
    if(parent_terminated_sig == 1) {
        log_err(RUN, "Ups! The parent process has died - terminate abnormally.");
        exit(-1);
    }
    
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
    
    // Check if parent has died
    if(parent_terminated_sig == 1) {
        log_err(RUN, "Ups! The parent process has died - terminate abnormally.");
        exit(-1);
    }
    
    // Commit results to the server
    msgQueueWritef(runOutputQueue, "run-terminate: %lld %d", (long long)getpid(), result);
    
    // Close all means of communication
    msgQueueClose(&runOutputQueue);
    msgPipeClose(&graphDataPipe);
    
    log(RUN, "Terminate.");
    
    return 0;
}
