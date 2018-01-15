#ifndef __FORK_H__
#define __FORK_H__

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
#include "syserr.h"

int processFork(pid_t* pid) {
    switch (*pid = fork()) {
        case -1:
            syserr("Error in fork\n");
            return -1;
        case 0:
            //printf("I am a child and my pid is %d\n", getpid());    
            return 1;
        default:
            //printf("I am a parent and my pid is %d\n", getpid());
            return 0;
    }
    return -1;
}

int processExec(pid_t* pid, const char* path, const char* arg, ...) {
    
    if(processFork(pid) == 1) {
        ptrdiff_t argc;
        va_list ap;
        va_start (ap, arg);
        for(argc=1; va_arg(ap, const char *); argc++) {
            if (argc == INT_MAX) {
                va_end(ap);
                errno = E2BIG;
                return -1;
            }
        }
        va_end(ap);

        ptrdiff_t i;
        char *argv[argc + 1];
        va_start(ap, arg);
        argv[0] = (char*) arg;
        for(i=1; i<=argc; i++) {
            argv[i] = va_arg(ap, char *);
        }
        va_end(ap);

        if(execve(path, argv, NULL) == -1) {
            return -1;
        }
        
        return 1;
    }
    
    return 0;
}

int processWait() {
    if(wait(0) == -1) {
        syserr("Error in wait\n");
        return -1;
    }
    return 1;
}

#endif // __FORK_H__