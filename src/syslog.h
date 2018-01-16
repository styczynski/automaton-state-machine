#ifndef __SYS_LOG_H__
#define __SYS_LOG_H__

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "syserr.h"

#ifndef SYS_LOG_DEFAULT_FILE
#define SYS_LOG_DEFAULT_FILE stdout
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
#define log_fatal(LABEL, ...) fatal(__VA_ARGS__)
#else // SYS_LOG_HANDLE_FATALS == 1
#define log_fatal(LABEL, ...) 
#endif // SYS_LOG_HANDLE_FATALS == 1

void log_formated(int loglevel, const char* label, FILE* out, const char* function_name, const char* format, ...) {
    // Empty function
};

#else // NO_LOG

#define log_debug(ENABLE_SETTING, LABEL, ...) if(ENABLE_SETTING) { log_info(LABEL, __VA_ARGS__); }
    
#define log(LABEL, ...)       log_formated(0, #LABEL, NULL, __func__, __VA_ARGS__)
#define log_info(LABEL, ...)  log_formated(1, #LABEL, NULL, __func__, __VA_ARGS__)
#define log_ok(LABEL, ...)    log_formated(2, #LABEL, NULL, __func__, __VA_ARGS__)
#define log_warn(LABEL, ...)  log_formated(3, #LABEL, NULL, __func__, __VA_ARGS__)
#define log_err(LABEL, ...)   log_formated(4, #LABEL, NULL, __func__, __VA_ARGS__)
#define log_fatal(LABEL, ...) log_formated(5, #LABEL, NULL, __func__, __VA_ARGS__)


void log_formated(int loglevel, const char* label, FILE* out, const char* function_name, const char* format, ...) {
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
    fprintf(out, "\n");
    va_end(args);
    
    fprintf(out, "\033[0m");
    if(SYS_LOG_DEFAULT_FLUSH) {
        fflush(out);
    }
    
    if(loglevel > 4) {
        // Log level: Fatal
        if(SYS_LOG_HANDLE_FATALS) {
            fatal("Syslog.h reported fatal error. See logs.");
        }
    }
}

#endif // NO_LOG

#endif // __SYS_LOG_H__