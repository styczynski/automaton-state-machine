/** @file
*
*  Set of utilities udeful for logging data. (C99 standard)
*
*  If the NO_LOG macro is used then all (non fatal) functions/macros have no effect.
* 
*  @author Piotr Styczyński <piotrsty1@gmail.com>
*  @copyright MIT
*  @date 2018-01-21
*/
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

/**
 * @def SYS_LOG_DEFAULT_FILE
 *  Default logging output file (by default it's stderr)
 */
#define SYS_LOG_DEFAULT_FILE stderr

#endif // SYS_LOG_DEFAULT_FILE

#ifndef SYS_LOG_DEFAULT_FLUSH

/**
 * @def SYS_LOG_DEFAULT_FLUSH
 *  If set to 1 then output will be flushed after each log execution (by default: 1)
 */
#define SYS_LOG_DEFAULT_FLUSH 1

#endif // SYS_LOG_DEFAULT_FLUSH

#ifndef SYS_LOG_PRINT_FNAMES

/**
 * @def SYS_LOG_PRINT_FNAMES
 *  If set to 1 then logs will print function names where the macro was called (by default: 1)
 */
#define SYS_LOG_PRINT_FNAMES 1

#endif // SYS_LOG_PRINT_FNAMES

#ifndef SYS_LOG_PROMOTE_TO_ERRORS

/**
 * @def SYS_LOG_PROMOTE_TO_ERRORS
 *  If set to 1 then all messages are promoted to errors (by default: 0)
 */
#define SYS_LOG_PROMOTE_TO_ERRORS 0

#endif // SYS_LOG_PROMOTE_TO_ERRORS

#ifndef SYS_LOG_HANDLE_FATALS

/**
 * @def SYS_LOG_HANDLE_FATALS
 *  If not set to 1 then fatal and log_fatal have no effects (default: 1)
 *  But syserr and syserrv are always terminating.
 */
#define SYS_LOG_HANDLE_FATALS 1


#endif // SYS_LOG_HANDLE_FATALS


#ifdef NO_LOG

/**
 * @def log_debug(ENABLE_SETTING, LABEL, ...)
 *  Log the message.
 *  If expression @p ENABLE_SETTING evaluates to logical true then the log is performed
 *  and have no effect in the other case
 *
 *  Note:
 *    All of the log macros/functions automatically append newline at the end of message.
 *
 *  Usage:
 *  @code
 *     #include <syslog.h>
 *      ...
 *     // Note that MY_CUSTOM_LABEL is not in the quotes
 *     log_debug(1, MY_CUSTOM_LABEL, "Log: %s, %d", "Hello", 42);
 *  @endcode
 *
 * @param[in] ENABLE_SETTING : If this log will execute?
 * @param[in] LABEL          : Pure (non-quoted) text label for the log instruction
 * @param[in] ...            : Printf-like formatting and paramters 
 */
#define log_debug(ENABLE_SETTING, LABEL, ...)

/**
 * @def log(LABEL, ...)
 *  Log the message. (no colour formatting is applied)
 *
 *  Note:
 *    All of the log macros/functions automatically append newline at the end of message.
 *
 *  Usage:
 *  @code
 *     #include <syslog.h>
 *      ...
 *     // Note that MY_CUSTOM_LABEL is not in the quotes
 *     log(MY_CUSTOM_LABEL, "Log: %s, %d", "Hello", 42);
 *  @endcode
 *
 * @param[in] LABEL          : Pure (non-quoted) text label for the log instruction
 * @param[in] ...            : Printf-like formatting and paramters 
 */
#define log(LABEL, ...)      

/**
 * @def log_info(LABEL, ...)
 *  Log the message (info is used in less important debug purpose information).
 *
 *  Note:
 *    All of the log macros/functions automatically append newline at the end of message.
 *
 *  Usage:
 *  @code
 *     #include <syslog.h>
 *      ...
 *     // Note that MY_CUSTOM_LABEL is not in the quotes
 *     log_info(MY_CUSTOM_LABEL, "Log: %s, %d", "Hello", 42);
 *  @endcode
 *
 * @param[in] LABEL          : Pure (non-quoted) text label for the log instruction
 * @param[in] ...            : Printf-like formatting and paramters 
 */
#define log_info(LABEL, ...) 

/**
 * @def log_ok(LABEL, ...)
 *  Log the message. (used to print success information/ok status to the screen)
 *
 *  Note:
 *    All of the log macros/functions automatically append newline at the end of message.
 *
 *  Usage:
 *  @code
 *     #include <syslog.h>
 *      ...
 *     // Note that MY_CUSTOM_LABEL is not in the quotes
 *     log_ok(MY_CUSTOM_LABEL, "Log: %s, %d", "Hello", 42);
 *  @endcode
 *
 * @param[in] LABEL          : Pure (non-quoted) text label for the log instruction
 * @param[in] ...            : Printf-like formatting and paramters 
 */
#define log_ok(LABEL, ...)   

/**
 * @def log_warn(LABEL, ...)
 *  Log the message. (used to print warning message)
 *
 *  Note:
 *    All of the log macros/functions automatically append newline at the end of message.
 *
 *  Usage:
 *  @code
 *     #include <syslog.h>
 *      ...
 *     // Note that MY_CUSTOM_LABEL is not in the quotes
 *     log_warn(MY_CUSTOM_LABEL, "Log: %s, %d", "Hello", 42);
 *  @endcode
 *
 * @param[in] LABEL          : Pure (non-quoted) text label for the log instruction
 * @param[in] ...            : Printf-like formatting and paramters 
 */
#define log_warn(LABEL, ...) 

/**
 * @def log_err(LABEL, ...)
 *  Log the message. (used to log erorr messages)
 *
 *  Note:
 *    All of the log macros/functions automatically append newline at the end of message.
 *
 *  Usage:
 *  @code
 *     #include <syslog.h>
 *      ...
 *     // Note that MY_CUSTOM_LABEL is not in the quotes
 *     log_err(MY_CUSTOM_LABEL, "Log: %s, %d", "Hello", 42);
 *  @endcode
 *
 * @param[in] LABEL          : Pure (non-quoted) text label for the log instruction
 * @param[in] ...            : Printf-like formatting and paramters 
 */
#define log_err(LABEL, ...)  

/**
 * @def log_set(STATE)
 *  Enable/disable runtime logging depending on the logic value of @p STATE expression.
 *
 * @param[in] STATE  : Enable logging or not?
 */
#define log_set(STATE)       

#if SYS_LOG_HANDLE_FATALS == 1

/**
 * @def fatal(LABEL, ...)
 *  Log the message. (used to log erorr messages)
 *  Automatically terminates application with -1 exit code if SYS_LOG_HANDLE_FATALS = 1
 *  If SYS_LOG_HANDLE_FATALS <> 1 then this function will be non terminating.
 *
 *  Note:
 *    All of the log macros/functions automatically append newline at the end of message.
 *
 *  Usage:
 *  @code
 *     #include <syslog.h>
 *      ...
 *     // Note that MY_CUSTOM_LABEL is not in the quotes
 *     fatal(MY_CUSTOM_LABEL, "Log: %s, %d", "Hello", 42);
 *  @endcode
 *
 * @param[in] LABEL          : Pure (non-quoted) text label for the log instruction
 * @param[in] ...            : Printf-like formatting and paramters 
 */
#define fatal(LABEL, ...)     fatal_formated(0, 1, #LABEL, stderr, __func__, __VA_ARGS__)

/**
 * @def log_fatal(LABEL, ...)
 *  Log the message. (used to log erorr messages)
 *  Automatically terminates application with -1 exit code if SYS_LOG_HANDLE_FATALS = 1
 *  If SYS_LOG_HANDLE_FATALS <> 1 then this function will be non terminating.
 *
 *  Note:
 *    All of the log macros/functions automatically append newline at the end of message.
 *
 *  Usage:
 *  @code
 *     #include <syslog.h>
 *      ...
 *     // Note that MY_CUSTOM_LABEL is not in the quotes
 *     log_fatal(MY_CUSTOM_LABEL, "Log: %s, %d", "Hello", 42);
 *  @endcode
 *
 * @param[in] LABEL          : Pure (non-quoted) text label for the log instruction
 * @param[in] ...            : Printf-like formatting and paramters 
 */
#define log_fatal(LABEL, ...) fatal_formated(0, 1, #LABEL, stderr, __func__, __VA_ARGS__)

/**
 * @def syserr(...)
 *  Log the message and (always) terminate application with -1 exit code.
 *  This function always print errno error description.
 *  It's useful when handling system function that can fail setting errno to their eror code.
 *
 *  Note:
 *    All of the log macros/functions automatically append newline at the end of message.
 *
 *  Usage:
 *  @code
 *     #include <syslog.h>
 *      ...
 *     // Note that we do not provide any label (it's fixed to SYSERR)
 *     syserr("Log: %s, %d", "Hello", 42);
 *  @endcode
 *
 * @param[in] ...            : Printf-like formatting and paramters 
 */
#define syserr(...)           fatal_formated(1, 1, "SYSERR", stderr, __func__, __VA_ARGS__)

/**
 * @def syserrv(...)
 *  Log the message and (always) terminate application with -1 exit code.
 *  This function does not print errno error description as syserr(...) does.
 *  
 *  Note:
 *    All of the log macros/functions automatically append newline at the end of message.
 *
 *  Usage:
 *  @code
 *     #include <syslog.h>
 *      ...
 *     // Note that we do not provide any label (it's fixed to SYSERR)
 *     syserrv("Log: %s, %d", "Hello", 42);
 *  @endcode
 *
 * @param[in] ...            : Printf-like formatting and paramters 
 */
#define syserrv(...)          fatal_formated(0, 1, "SYSERR", stderr, __func__, __VA_ARGS__)

#else // SYS_LOG_HANDLE_FATALS == 1
#define log_fatal(LABEL, ...) 
#define fatal(LABEL, ...)     
#define syserr(...)           fatal_formated(1, 1, "SYSERR", stderr, __func__, __VA_ARGS__)
#define syserrv(...)          fatal_formated(0, 1, "SYSERR", stderr, __func__, __VA_ARGS__)
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
#define syserr(...)           log_formated(5, 1, "SYSERR", NULL, __func__, __VA_ARGS__)
#define syserrv(...)          log_formated(5, 0, "SYSERR", NULL, __func__, __VA_ARGS__)
#define log_set(STATE)        ((STATE)?(_log_on_()):(_log_off_()))

/*
 * Helper value that hold current (enabled/disabled) runtime state of logging facilities
 */
static int _runtime_log_status_ = 1;

/*
 * Helper function that enables runtime logging
 */
int _log_on_() {
    _runtime_log_status_ = 1;
    return 1;
}

/*
 * Helper function that disables runtime logging
 */
int _log_off_() {
    _runtime_log_status_ = 0;
    return 1;
}

/*
 * Helper function to log the message
 * It's used by logging macros
 */
void log_formated(int loglevel, const int print_errno, const char* label, FILE* out, const char* function_name, const char* format, ...) {
    
    // When logging is disabled log only fatals
    if(!_runtime_log_status_ && loglevel < 5) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    
    // Default logging output
    if(out == NULL) { 
        out = SYS_LOG_DEFAULT_FILE;
    }
    
    // Promote message to the error level
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
    
    // Print label
    if(label != NULL) {
        fprintf(out, "%-8s ", label);
    } else {
        fprintf(out, " %-8s  ", "  ");
    }
    
    // Print function name
    if(SYS_LOG_PRINT_FNAMES) {
        fprintf(out, " %-5d %-10s ", getpid(), function_name);
    }
    vfprintf(out, format, args);
    
    va_end(args);
    
    // Print stringified errno
    if(print_errno) {
        fprintf(out, "errno %d: %s", errno, strerror(errno));
    }
    
    fprintf(out, "\033[0m");
    fprintf(out, "\n");
    
    // Flush error output
    if(SYS_LOG_DEFAULT_FLUSH) {
        fflush(out);
    }
    
    if(loglevel > 4) {
        // Log level: Fatal
        if(SYS_LOG_HANDLE_FATALS) {
            // Execute termination
            exit(-1);
        }
    }
}

#endif // NO_LOG

/*
 * Helper function to log fatal (terminating) messages.
 * Used extensively by fatal/log_fatal/syserr/syserrv macros.
 */
void fatal_formated(const int print_errno, const int if_exit, const char* label, FILE* out, const char* function_name, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    // Default logging output
    if(out == NULL) { 
        out = SYS_LOG_DEFAULT_FILE;
    }
    
    // Log level: Fatal
    fprintf(out, "\033[0;31m [Process ");
    
    // Print label
    if(label != NULL) {
        fprintf(out, "[%s] ", label);
    } else {
        fprintf(out, "[%s] ", "?");
    }
    
    // Print function name
    if(SYS_LOG_PRINT_FNAMES) {
        fprintf(out, " (pid) %d in function %s] ", getpid(), function_name);
    }
    
    vfprintf(out, format, args);
    
    // Print stringified errno
    if(print_errno) {
        fprintf(out, "errno %d: %s", errno, strerror(errno));
    }
    
    fprintf(out, "\n");
    va_end(args);
    
    fprintf(out, "\033[0m");
    fflush(out);
    
    // Execute termination
    if(if_exit) {
        exit(-1);
    }
}

#endif // __SYS_LOG_H__