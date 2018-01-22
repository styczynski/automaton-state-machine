/** @file
*
*  GC interface implementation.
* 
*  This implementation uses hashmap to store allocated pointers values and free them at exit.  
*
*  @author Piotr Styczyński <piotrsty1@gmail.com>
*  @copyright MIT
*  @date 2018-01-21
*/
#ifndef __GCINIT_H__
#define __GCINIT_H__

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <stdint.h>

#include "hashmap.h"
#include "fork.h"
#include "onexit.h"
#include "syslog.h"

#include <stdio.h>

typedef struct custom_ptr_holder custom_ptr_holder;
typedef struct custom_ptr_destr custom_ptr_destr;

struct custom_ptr_holder {
    void* ptr;
    void* type_id;
};

struct custom_ptr_destr {
    void* ptr;
    void* destr;
};


/**
 * Implemention of the GC tree function.
 * 
 * Mode can be one of the following values:
 *
 *  ->  0 - p was just allocated
 *  ->  1 - p was just freed
 *  ->  2 - FREE_ALL was requested so you must free all the data and internal GC strucutres
 *  ->  3 - EXIT((int)p) was requested so you must do cleanup and call exit(...) function
 *  ->  4 - GC verbose allocation logging was enabled (p=1) or disabled (p=0)
 *  ->  5 - FORK_FREE was requested so you must free all the data without internal GC strucutres
 *          (these strucutres must be empty but not destroyed - we assume further usage of them)
 *  ->  6 - GC_N - p (data of typeid = r) was allocated with handler so add its cleanup handler (passed as q parameter)
 *  ->  7 - GC_D - p (data of typeid = r) was allocated with handler so remove its cleanup handler
 *
 *
 * @param[in] mode : Integer representing call mode
 * @param[in] p    : Data provided for the call
 */
static void __gc_mem_tree__(int mode, void* p, void* q, void* r) {
    
    (void) mode;
    (void) p;
    (void) q;
    (void) r;
    
    if(!__is_gc__()) return;
    __gc_off__();
    
    static int once = 0;
    static HashMap GCmemMap;
    static HashMap GChandlersMap;
    static int logEnabled = 0;
    
    if(once == 0) {
        once = 1;
        GCmemMap = HashMapNew(HashMapVoidPtrCmp);
        GChandlersMap = HashMapNew(HashMapVoidPtrCmp);
    }
    
    // For GC_ALLOC
    if(mode == 0) {
        if(logEnabled) {
            fprintf(stderr, "GC_ALLOC(%p)\n", (void*) p);
        }
        
        // Pointer was allocated
        int phv = (int) (intptr_t) p;
        if(phv > 0) {
            phv *= -1;
        }
        HashMapSet(&GCmemMap, phv, (void*) p, (void*)p);
        
        int mem_size = 0;
        LOOP_HASHMAP(&GCmemMap, i) {
            ++mem_size;
        }
        
    // For GC_FREE
    } else if(mode == 1) {
        
        // Pointer was freed
        int phv = (int) (intptr_t) p;
        if(phv > 0) {
            phv *= -1;
        }
        
        HashMapRemove(&GCmemMap, phv, (void*) p);
        
        int mem_size = 0;
        LOOP_HASHMAP(&GCmemMap, i) {
            ++mem_size;
        }
        
        if(logEnabled) {
            fprintf(stderr, "GC_FREE(NORMAL, %p)\n", (void*) p);
        }

    // For FORK_FREE, FREE_ALL, GC_EXIT
    } else if(mode == 2 || mode == 3 || mode == 5) {
        
        LOOP_HASHMAP(&GChandlersMap, i) {
            custom_ptr_holder* data = (custom_ptr_holder*) HashMapGetKey(i);
            custom_ptr_destr* destr = (custom_ptr_destr*) HashMapGetValue(i);
            
            if(data != NULL && destr != NULL) {
                GCCustomDestructor destructor = (GCCustomDestructor) (intptr_t) (destr->destr);
                
                if(logEnabled) {
                    fprintf(stderr, "GC_D(GC_CLEANUP, %p, typeid=%p, handle=%p)\n", (void*) data->ptr, (void*) data->type_id, (void*) destr->destr );
                }
                
                __gc_on__();
                destructor(data->ptr);
                __gc_off__();
                
            }
            
        }
        
        LOOP_HASHMAP(&GCmemMap, i) {
            void* ptr = HashMapGetValue(i);
            if(logEnabled) {
                fprintf(stderr, "GC_FREE(GC_CLEANUP, %p)\n", (void*) ptr);
            }
            free(ptr);
        }
        
        // If that wasn't FORK_FREE
        if(mode != 5) {
            HashMapDestroy(&GCmemMap);
            HashMapDestroyV(&GChandlersMap, custom_ptr_holder, custom_ptr_destr);
            processWaitForAll();
        }
        
        if(mode == 3) {
            if(logEnabled) {
                log_info(GC, "GC Automatic cleanup done at exit (code = %d).", (int) (intptr_t) p);
            }
            exit((int) (intptr_t) p);
        } else {
            if(logEnabled) {
                log_info(GC, "GC Automatic cleanup done.");
            }
        }
    // For GC_LOG_ON/OFF
    } else if(mode == 4) {
        logEnabled = (int) (intptr_t) p;
    // Add GC_N handler
    } else if(mode == 6) {
        
        if(p == NULL) {
            __gc_on__();
            return;
        }
        
        custom_ptr_holder data;
        data.ptr = p;
        data.type_id = r;
        
        custom_ptr_destr destr;
        destr.ptr = p;
        destr.destr = q;
        
        if(logEnabled) {
            fprintf(stderr, "GC_N(%p, typeid=%p, handle=%p)\n", (void*) p, (void*) r, (void*) q);
        }
        
        HashMapSetV(&GChandlersMap, custom_ptr_holder, custom_ptr_destr, data, destr);
    // Remove GC_D handler
    } else if(mode == 7) {
        custom_ptr_holder data;
        data.ptr = p;
        data.type_id = r;
        
        if(logEnabled) {
            custom_ptr_destr* destr = HashMapGetV(&GChandlersMap, custom_ptr_holder, custom_ptr_destr, data);
            if(destr == NULL) {
                __gc_on__();
                return;
            }
            fprintf(stderr, "GC_D(%p, typeid=%p, handle=%p)\n", (void*) p, (void*) r, (void*) (destr->destr));
        }
        
        HashMapRemoveV(&GChandlersMap, custom_ptr_holder, custom_ptr_destr, data);
    }
    
    __gc_on__();
}

// Proxy function to execute FREE_ALL at exit
static inline void __gc_exit_hook__() {
    GC_FREE_ALL();
}

// GC setup implementation that adds __gc_exit_hook__() to atexit hooks
static void __gc_init__() {
    
    ExitHandlerSetup();
    ExitHandlerOverrideGC(__gc_exit_hook__);
    
    __gc_mem_tree__funct = __gc_mem_tree__;
}

#endif // __GCINIT_H__