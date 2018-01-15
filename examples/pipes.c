#include <stdio.h>
#include "msg_pipe.h"
#include "fork.h"
#include "syserr.h"

int main(void) {
    
    MsgPipeID taskPipeID = msgPipeCreate(100);
    
    pid_t pid;
    if(processFork(&pid)) {
        
        MsgPipe taskPipe = msgPipeOpen(taskPipeID);
        
        printf("? Child\n");
        msgPipeCloseWrite(&taskPipe);
        
        for(int t=0;t<1;++t) {
            int v = 0;
            printf("--> %s\n", msgPipeRead(taskPipe));
        }
        
        msgPipeClose(&taskPipe);
    } else {
        MsgPipe taskPipe = msgPipeOpen(taskPipeID);
        
        printf("? Parent\n");
        msgPipeCloseRead(&taskPipe);
        
        msgPipeWritef(taskPipe, "1.2.3.4.5.6.7.8.9.10.11.12.13.14.15.16.17.18:%d", 42);
        processWait();
        printf("? Parent finish\n");
        
        msgPipeClose(&taskPipe);
    }
    
    return 0;
}