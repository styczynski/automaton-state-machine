/** @file
*
*  Set of exit status handling utilites. (C99 standard)
*
*  @author Piotr Styczyński <piotrsty1@gmail.com>
*  @copyright MIT
*  @date 2018-01-21
*/
#ifndef __ONEXIT_H__
#define __ONEXIT_H__

#include <stdlib.h>

#define ON_EXIT_MAX_HANDLERS_COUNT 10

/** Type of exit callback function */
typedef void (*OnExitHandler)();

static int __on_exit_called_mode__ = 0;

static inline void __onexit_handler_op__(int mode, OnExitHandler handler) {

    static int once = 0;
    static OnExitHandler __gc_handler_slot__ = NULL;
    static OnExitHandler __normal_handlers_slots__[ON_EXIT_MAX_HANDLERS_COUNT];
    static int __normal_handlers_slots_i__ = 0;
    
    if(once == 0) {
        once = 1;
        for(int i=0;i<ON_EXIT_MAX_HANDLERS_COUNT;++i) {
            __normal_handlers_slots__[i] = NULL;
        }
    }
    
    if(mode == 1) {
        __normal_handlers_slots__[__normal_handlers_slots_i__++] = handler;
    } else if(mode == 0) {
        __gc_handler_slot__ = handler;
    } else if(mode == -1) {
        __on_exit_called_mode__ = 1;
        for(int i=0;i<__normal_handlers_slots_i__;++i) {
            if(__normal_handlers_slots__[i] != NULL) {
                __normal_handlers_slots__[i]();
            }
        }
        if(__gc_handler_slot__ != NULL) {
            __gc_handler_slot__();
        }
    }
}

static inline void ExitHandlerOverrideGC(OnExitHandler handler) {
    __onexit_handler_op__(0, handler);
}

static inline void ExitHandlerAdd(OnExitHandler handler) {
    __onexit_handler_op__(1, handler);
}

static inline void ExitHandlerExec() {
    __onexit_handler_op__(-1, NULL);
}

static inline int ExitHandlerIsExitting() {
    return __on_exit_called_mode__;
}

static inline void ExitHandlerSetup() {
    static int once = 0;
    if(once == 0) {
        once = 1;
        if(atexit(ExitHandlerExec) != 0) {
            syserr("Could not initialize ExitHandler");
            return;
        }
    }
}

#endif // __ONEXIT_H__