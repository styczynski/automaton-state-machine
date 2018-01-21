/** @file
*
*  GC interface without implementation.
* 
*  Define ENABLE_GC macro to use GC
*  You must also call GC_SETUP() macro at the very begging of the function main.
*  (in the first line probably)
*
*  This file provides basic GC utilities without any detailed implementation to allow
*  inclusion of any matching interface collector.
*
*  If ENABLE_GC macro was not defined then the GC will fallback in not-supported mode.
*  In not-supported mode all macros and functions needed by them will be present but
*  will perform no action when it's not declared otherwise.
*
*  @author Piotr Styczyński <piotrsty1@gmail.com>
*  @copyright MIT
*  @date 2018-01-21
*/
#ifndef __GC_H__
#define __GC_H__

#include <stdio.h>

#ifndef ENABLE_GC

/**
 * @def GC_STATUS
 *   Macro equivalent to the current state of GC.
 *   GC can be enabled (1) or disabled (0)
 *
 * If the GC is not enabled by ENABLE_GC then the value of this macro is literal 0.
 */
#define GC_STATUS (0)

/**
 * @def GC_ON()
 *   Turn on the GC.
 *
 * If the GC is not enabled by ENABLE_GC then usage of this macro has no effect.
 */
#define GC_ON()

/**
 * @def GC_OFF()
 *   Turn on the GC.
 *
 * If the GC is not enabled by ENABLE_GC then usage of this macro has no effect.
 */
#define GC_OFF()

/**
 * @def GC_ON_ALLOC(PTR)
 *   Triggers GC "allocated" action on the provided pointer @p PTR.
 *
 * If the GC is not enabled by ENABLE_GC then usage of this macro has no effect.
 */
#define GC_ON_ALLOC(PTR)

/**
 * @def GC_ON_ALLOC(PTR)
 *   Triggers GC "freed" action on the provided pointer @p PTR.
 *
 * If the GC is not enabled by ENABLE_GC then usage of this macro has no effect.
 */
#define GC_ON_FREE(PTR)

/**
 * @def GC_SETUP()
 *   Initializes GC.
 *   This macro must be called as the first thing in main function.
 *   In other situations it may lead to undefined states.
 *
 *   If the GC is not enabled by ENABLE_GC then usage of this macro has no effect.
 */
#define GC_SETUP()

/**
 * @def GC_FREE_ALL()
 *   Manually trigger GC cleanup.
 *   GC will deallocate every allocated resource so the program after execution of GC_FREE_ALL()
 *   cannot rely on anything else than stack-allocated values or it can move itself into undefined state.
 *
 *   You don't have to call this by yourself.
 *   GC will cleanup automatically at the termination of program.
 * 
 *   WARNING!:
 *     Internal structures of GC will be also freed so you cannot perform any alloc/dealloc.
 *     For that purpose you must execute GC_OFF() to continue normal behaviour.
 * 
 *   If the GC is not enabled by ENABLE_GC then usage of this macro has no effect.
 */
#define GC_FREE_ALL()

/**
 * @def GC_FORK_FREE()
 *   Manually trigger GC cleanup.
 *   GC will deallocate every allocated resource so the program after execution of GC_FORK_FREE()
 *   cannot rely on anything else than stack-allocated values or it can move itself into undefined state.
 *
 *   You don't have to call this by yourself.
 *   GC will cleanup automatically at the termination of program.
 * 
 *   WARNING!:
 *     This macro does not free internal strucutres of GC (as GC_FREE_ALL() does)
 *     So you can normally call alloc/dealloc after execution of this macro function.
 * 
 *   If the GC is not enabled by ENABLE_GC then usage of this macro has no effect.
 */
#define GC_FORK_FREE()

/**
 * @def GC_EXIT(CODE)
 *   
 *   Exits program terminating it using the provided exit code.
 *   Before program exit the GC cleanup will be done.
 *
 *   You don't have to manually execute this macro function as GC normally is triggered
 *   when using exit(...) as it's installed as atexit hook.
 * 
 *   If the GC is not enabled by ENABLE_GC then this macro is equivalent to exit(CODE).
 */
#define GC_EXIT(CODE) exit(CODE)

/**
 * @def GC_LOG_ON()
 *   Enable verbosive GC allocation info.
 * 
 *   NOTE:
 *     Allocation logs can be huge.
 *     syslog.h logging must be enabled for this macro function to has any effect.
 *
 *   If the GC is not enabled by ENABLE_GC then this macro is equivalent to exit(CODE).
 */
#define GC_LOG_ON()

/**
 * @def GC_LOG_ON()
 *   Disable verbosive GC allocation info.
 * 
 *   NOTE:
 *     Allocation logs can be huge.
 *     syslog.h logging must be enabled for this macro function to has any effect.
 *
 *   If the GC is not enabled by ENABLE_GC then this macro is equivalent to exit(CODE).
 */
#defnie GC_LOG_OFF()

#else // ENABLE_GC

/*
 * Poitner to the GC implementation
 */
static void (*__gc_mem_tree__funct)(int, void*) = NULL;

/**
 * Proxy that executes the pointer implementation if it's not NULL.
 * It represents interface of the GC tree fucntion (__gc_mem_tree__funct).
 *
 * The function takes parameters (int mode, void* p)
 * Mode can be one of the following values:
 *
 *  ->  0 - p was just allocated
 *  ->  1 - p was just freed
 *  ->  2 - FREE_ALL was requested so you must free all the data and internal GC strucutres
 *  ->  3 - EXIT((int)p) was requested so you must do cleanup and call exit(...) function
 *  ->  4 - GC verbose allocation logging was enabled (p=1) or disabled (p=0)
 *  ->  5 - FORK_FREE was requested so you must free all the data without internal GC strucutres
 *          (these strucutres must be empty but not destroyed - we assume further usage of them)
 *
 **/
static inline void __gc_mem_tree__proxy(int mode, void* p) {
    if(__gc_mem_tree__funct != NULL) {
        __gc_mem_tree__funct(mode, p);
    }
}

#define GC_STATUS        (__is_gc__())
#define GC_ON()          (__gc_on__())
#define GC_OFF()         (__gc_off__())
#define GC_ON_ALLOC(PTR) (__gc_mem_tree__proxy(0, (PTR)))
#define GC_ON_FREE(PTR)  (__gc_mem_tree__proxy(1, (PTR)))
#define GC_SETUP()       (__gc_init__())
#define GC_FREE_ALL()    (__gc_mem_tree__proxy(2, NULL))
#define GC_FORK_FREE()   (__gc_mem_tree__proxy(5, NULL))
#define GC_EXIT(CODE)    (__gc_mem_tree__proxy(3, (void*)(CODE)))
#define GC_LOG_ON()      (__gc_mem_tree__proxy(4, (void*)(1)))
#define GC_LOG_OFF()     (__gc_mem_tree__proxy(4, (void*)(0)))

/*
 * Function to enable/disable GC during runtime
 */
static inline int __set_read_gc_status__(int w, int mode) {
    static int current_gc_mode = 1;
    
    if(w) {
        current_gc_mode = mode;
    }
    return current_gc_mode;
}

/*
 * Proxy to enable GC durign runtime
 */
static inline void __gc_on__() {
    (void) __set_read_gc_status__(1, 1);
}

/*
 * Proxy to disable GC durign runtime
 */
static inline void __gc_off__() {
    (void) __set_read_gc_status__(1, 0);
}

/*
 * Proxy to check if GC is running during runtime
 */
static inline int __is_gc__() {
    return __set_read_gc_status__(0, 0);
}

#endif // ENABLE_GC

#endif // __GC_H__