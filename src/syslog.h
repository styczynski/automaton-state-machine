#ifndef __SYS_LOG_H__
#define __SYS_LOG_H__

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef NO_LOG

#define log(LABEL, ...)      
#define log_info(LABEL, ...) 
#define log_ok(LABEL, ...)   
#define log_warn(LABEL, ...) 
#define log_err(LABEL, ...)  

void log_formated(int loglevel, const char* label, FILE* out, const char* function_name, const char* format, ...) {
    // Empty function
};

#else

#define log(LABEL, ...)      log_formated(0, LABEL, NULL, __func__, __VA_ARGS__)
#define log_info(LABEL, ...) log_formated(1, LABEL, NULL, __func__, __VA_ARGS__)
#define log_ok(LABEL, ...)   log_formated(2, LABEL, NULL, __func__, __VA_ARGS__)
#define log_warn(LABEL, ...) log_formated(3, LABEL, NULL, __func__, __VA_ARGS__)
#define log_err(LABEL, ...)  log_formated(4, LABEL, NULL, __func__, __VA_ARGS__)

void log_formated(int loglevel, const char* label, FILE* out, const char* function_name, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    if(out == NULL) { 
        out = stdout;
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
    } else if(loglevel == 4) {
        // Log level: Error
        fprintf(out, "\033[0;31m");
    }
    
    if(label != NULL) {
        fprintf(out, "%-8s ", label);
    } else {
        fprintf(out, " %-8s  ", "  ");
    }
    
    fprintf(out, " %-5d %-10s ", getpid(), function_name);
    vfprintf(out, format, args);
    fprintf(out, "\n");
    va_end(args);
    
    fprintf(out, "\033[0m");
    fflush(stdout);
}

#endif // NO_LOG

#endif // __SYS_LOG_H__