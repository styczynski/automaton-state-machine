#include <stdio.h>
#include "msg_pipe.h"
#include "fork.h"
#include "syserr.h"

/*include "hashmap.h"

typedef struct custom_type custom_type;
struct custom_type {
    int x;
    int y;
};

int main(void) {
    HashMap hm = HashMapNew(HashMapIntCmp);
    
    custom_type val;
    val.x = 21;
    val.y = 37;
    
    HashMapSetV(hm, int, custom_type, 3, val);
    
    val.x = 1;
    val.y = 2;
    HashMapSetV(hm, int, custom_type, 4, val);
    
    LOOP_HASHMAP(&hm, i) {
        custom_type* ptr = (custom_type*) HashMapGetValue(i);
        printf("custom_type x=%d, y=%d\n", ptr->x, ptr->y);
    }
    
    HashMapDestroyV(hm, int, custom_type);
}*/


int main(void) {
    
    MsgPipeID taskPipeID = msgPipeCreate(100);
    
    pid_t pid;
    if(processFork(&pid)) {
        
        MsgPipe taskPipe = msgPipeOpen(taskPipeID);
        
        printf("? Child\n");
        msgPipeCloseWrite(&taskPipe);
        
        for(int t=0;t<1;++t) {
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

