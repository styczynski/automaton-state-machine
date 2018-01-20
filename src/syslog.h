#ifndef __SYS_LOG_H__
#define __SYS_LOG_H__

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>

#ifndef SYS_LOG_DEFAULT_FILE
#define SYS_LOG_DEFAULT_FILE stderr
#endif // SYS_LOG_DEFAULT_FILE

#ifndef SYS_LOG_DEFAULT_FLUSH
#define SYS_LOG_DEFAULT_FLUSH 1
#endif // SYS_LOG_DEFAULT_FLUSH

#ifndef SYS_LOG_PRINT_FNAMES
#define SYS_LOG_PRINT_FNAMES 1
#endif // SYS_LOG_PRINT_FNAMES

#ifndef SYS_LOG_PROMOTE_TO_ERRORS
#define SYS_LOG_PROMOTE_TO_ERRORS 0
#endif // SYS_LOG_PROMOTE_TO_ERRORS

#ifndef SYS_LOG_HANDLE_FATALS
#define SYS_LOG_HANDLE_FATALS 1
#endif // SYS_LOG_HANDLE_FATALS


#ifdef NO_LOG

#define log_debug(ENABLE_SETTING, LABEL, ...)

#define log(LABEL, ...)      
#define log_info(LABEL, ...) 
#define log_ok(LABEL, ...)   
#define log_warn(LABEL, ...) 
#define log_err(LABEL, ...)  

#if SYS_LOG_HANDLE_FATALS == 1
#define fatal(LABEL, ...)     fatal_formated(0, 1, #LABEL, stderr, __func__, __VA_ARGS__)
#define log_fatal(LABEL, ...) fatal_formated(0, 1, #LABEL, stderr, __func__, __VA_ARGS__)
#define syserr(...)           fatal_formated(1, 0, "SYSERR", stderr, __func__, __VA_ARGS__)
#define syserrv(...)          fatal_formated(0, 0, "SYSERR", stderr, __func__, __VA_ARGS__)
#else // SYS_LOG_HANDLE_FATALS == 1
#define log_fatal(LABEL, ...) 
#define fatal(LABEL, ...)     
#define syserr(...)           fatal_formated(1, 0, "SYSERR", stderr, __func__, __VA_ARGS__)
#define syserrv(...)          fatal_formated(0, 0, "SYSERR", stderr, __func__, __VA_ARGS__)
#endif // SYS_LOG_HANDLE_FATALS == 1

#define log_formated(...)     

/*
void log_formated(int loglevel, const int print_errno, const char* label, FILE* out, const char* function_name, const char* format, ...) {
    // Empty function
};
*/

#else // NO_LOG

#define log_debug(ENABLE_SETTING, LABEL, ...) if(ENABLE_SETTING) { log_info(LABEL, __VA_ARGS__); }
    
#define log(LABEL, ...)       log_formated(0, 0, #LABEL, NULL, __func__, __VA_ARGS__)
#define log_info(LABEL, ...)  log_formated(1, 0, #LABEL, NULL, __func__, __VA_ARGS__)
#define log_ok(LABEL, ...)    log_formated(2, 0, #LABEL, NULL, __func__, __VA_ARGS__)
#define log_warn(LABEL, ...)  log_formated(3, 0, #LABEL, NULL, __func__, __VA_ARGS__)
#define log_err(LABEL, ...)   log_formated(4, 0, #LABEL, NULL, __func__, __VA_ARGS__)
#define log_fatal(LABEL, ...) log_formated(5, 0, #LABEL, NULL, __func__, __VA_ARGS__)
#define fatal(LABEL, ...)     log_formated(5, 0, #LABEL, NULL, __func__, __VA_ARGS__)
#define syserr(...)           log_formated(4, 1, "SYSERR", NULL, __func__, __VA_ARGS__)
#define syserrv(...)          log_formated(4, 0, "SYSERR", NULL, __func__, __VA_ARGS__)


void log_formated(int loglevel, const int print_errno, const char* label, FILE* out, const char* function_name, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    if(out == NULL) { 
        out = SYS_LOG_DEFAULT_FILE;
    }
    
    if(SYS_LOG_PROMOTE_TO_ERRORS) {
        loglevel = 4;
    }
    
    if(loglevel == 0) {
        // Log level: Default
        fprintf(out, "\033[0m");
    } else if(loglevel == 1) {
        // Log level: Info
        fprintf(out, "\033[0;34m");
    } else if(loglevel == 2) {
        // Log level: Success
        fprintf(out, "\033[0;32m");
    } else if(loglevel == 3) {
        // Log level: Warn
        fprintf(out, "\033[0;33m");
    } else if(loglevel >= 4) {
        // Log level: Error
        fprintf(out, "\033[0;31m");
    }
    
    if(label != NULL) {
        fprintf(out, "%-8s ", label);
    } else {
        fprintf(out, " %-8s  ", "  ");
    }
    
    if(SYS_LOG_PRINT_FNAMES) {
        fprintf(out, " %-5d %-10s ", getpid(), function_name);
    }
    vfprintf(out, format, args);
    
    va_end(args);
    
    if(print_errno) {
        fprintf(out, "errno %d: %s", errno, strerror(errno));
    }
    
    fprintf(out, "\033[0m");
    fprintf(out, "\n");
    
    if(SYS_LOG_DEFAULT_FLUSH) {
        fflush(out);
    }
    
    
    if(loglevel > 4) {
        // Log level: Fatal
        if(SYS_LOG_HANDLE_FATALS) {
            exit(-1);
        }
    }
}

#endif // NO_LOG


void fatal_formated(const int print_errno, const int if_exit, const char* label, FILE* out, const char* function_name, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    if(out == NULL) { 
        out = SYS_LOG_DEFAULT_FILE;
    }
    
    // Log level: Fatal
    fprintf(out, "\033[0;31m [Process ");
    
    if(label != NULL) {
        fprintf(out, "[%s] ", label);
    } else {
        fprintf(out, "[%s] ", "?");
    }
    
    if(SYS_LOG_PRINT_FNAMES) {
        fprintf(out, " (pid) %d in function %s] ", getpid(), function_name);
    }
    
    vfprintf(out, format, args);
    
    if(print_errno) {
        fprintf(out, "errno %d: %s", errno, strerror(errno));
    }
    
    fprintf(out, "\n");
    va_end(args);
    
    fprintf(out, "\033[0m");
    fflush(out);
    
    if(if_exit) {
        exit(-1);
    }
}

#endif // __SYS_LOG_H__