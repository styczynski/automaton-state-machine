#ifndef __FORK_H__
#define __FORK_H__

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/wait.h>
#include "msg_pipe.h"

static int _trigForkErr_ = 0;

int processFork(pid_t* pid) {
    
    static int cnt = 0;
    ++cnt;
    if(cnt > 2 && _trigForkErr_) {
        syserr("Failed due to fork() error");
        return -1;
    };
    
    switch (*pid = fork()) {
        case -1:
            syserr("Failed due to fork() error");
            return -1;
        case 0:
            return 1;
        default:
            return 0;
    }
    return 0;
}

int processExec(pid_t* pid, const char* path, const char* arg, ...) {
    
    int status = processFork(pid);
    
    if(status == -1) {
        return 0;
    }
    
    if(status == 1) {
        ptrdiff_t argc;
        va_list ap;
        va_start (ap, arg);
        for(argc=1; va_arg(ap, const char *); argc++) {
            if (argc == INT_MAX) {
                va_end(ap);
                errno = E2BIG;
                return 0;
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
            return 0;
        }
        
        return 1;
    }
    
    return 1;
}

int processWaitForAll() {
    int wstatus;
    int f_status = 1;
    int cont = 1;
    
    while(cont) {
        if(wait(&wstatus) == -1) {
            if(errno == ECHILD) {
                break;
            } else {
                f_status = 0;
            }
        } else {
            if (WIFEXITED(wstatus)) {
                if(WEXITSTATUS(wstatus) == 0) {
                    // OK
                } else {
                    f_status = 0;
                }
            } else {
                f_status = 0;
            }
        }
    }
    
    if(f_status == 0) {
        return -1;
    }
    return 1;
}

int processWait() {
    if(wait(0) == -1) {
        return -1;
    }
    return 1;
}

int processWaitForAllNonBlocking() {
    int wstatus;
    int ret = waitpid(-1, &wstatus, WNOHANG);
    
    if(ret == -1) {
        if(errno == ECHILD) {
            return 0;
        }
        syserr("processWaitForAllNonBlocking waitpid err");
        return -1;
    }
    
    if(ret == 0) return 0;
    
    if(ret != -1) {
        if (WIFEXITED(wstatus)) {
            if(WEXITSTATUS(wstatus) == 0) {
                return 1;
            } else {
                return -1;
            }
        } else {
            return -1;
        }
    }
    
    return -1;
}

void processExit(int status) {
    exit(status);
}

#endif // __FORK_H__