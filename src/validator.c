#include <stdio.h>
#include "getline.h"
#include "automaton.h"
#include "msg_queue.h"
#include "syserr.h"


int main(void) {
    
    TransitionGraph tg = newTransitionGraph();
    loadTransitionGraph(stdin, tg);
    printTransitionGraph(tg);
    
    MsgQueue taskQueue = msgQueueOpen("/fin_autom_tsk_queue", 30, 10);
    
    pid_t pid;
    switch (pid = fork()) {
        case -1:
          syserr("Error in fork\n");
          return 0;
        case 0:
          printf("I am a child and my pid is %d\n", getpid());      
          execlp("./run", "run", NULL);
          syserr("Error in execlp\n");
          return 0;
        default:
          printf("I am a parent and my pid is %d\n", getpid());
          msgQueueSend(taskQueue, "Hello world!");
          if (wait(0) == -1) syserr("Error in wait\n");
          msgQueueRemove(taskQueue);
          return 0;
    } 
    
    return 0;
}