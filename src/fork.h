/** @file
*
*  Various utilities for fork/wait/exec process management.
*
*  @author Piotr Styczyński <piotrsty1@gmail.com>
*  @copyright MIT
*  @date 2018-01-21
*/
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

/**
 * Fork process capturing its pid.
 *
 * Returns:
 *   * 1  in case of children
 *   * 0  in case of parent
 *   * -1 in case of error (set on errno)
 *
 * @param[in] pid : pointer to pid that will be set to spawned process pid
 * @returns Returns integer code
 */
int processFork(pid_t* pid) {
   
    switch (*pid = fork()) {
        case -1:
            return -1;
        case 0:
            return 1;
        default:
            return 0;
    }
    return 0;
}

/**
 * Execute file with args variadic list terminated by NULL.
 * Returns logic value to indicate errors. 
 *
 * @param[in] pid  : pointer to pid that will be set to spawned process pid
 * @param[in] path : path to the program
 * @param[in] arg  : first arg to be passed to the executed program
 * @param[in] ...  : variadic list of c-style strings (char*) terminated by NULL value
 * @returns Status: if the execution succeeded?
 */
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

/**
 * Wait for all children processes.
 * Check's if all processes terminate normally with 0 exit code.
 * If there's some unterminated children then this function will block.
 *
 * Returns:
 *   * -1 in case of error
 *   *  1 in case of success 
 *
 * @returns If all child processes were terminated normally with 0 exit code?
 */
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


/**
 * Execute native wait(0) function to wait for children processes.
 * It's offers only error check - see alternative: processWaitForAll().
 * If there's some unterminated children then this function will block.
 *
 * Returns:
 *   * -1 in case of error
 *   *  1 in case of success 
 *
 * @returns If wait(0) returned no error code?
 */
int processWait() {
    if(wait(0) == -1) {
        return -1;
    }
    return 1;
}

/**
 * Wait for all children processes.
 * Check's if all processes terminate normally with 0 exit code.
 * If there's some unterminated children then this function fall through.
 *
 * Returns:
 *   * -1 in case of error
 *   *  0 in case when there are some children process left unterminated
 *   *  1 in case of success
 *
 * @returns If all child processes were terminated normally with 0 exit code?
 */
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

/**
 * Terminate program exitting with the given exit code.
 * This function is equivalent to calling exit(status).
 * 
 * @param[in] status : exit code
 */
void processExit(int status) {
    exit(status);
}

#endif // __FORK_H__