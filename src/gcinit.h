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

static void __gc_mem_tree__(int mode, void* p) {
    
    (void) mode;
    (void) p;
    
    if(!__is_gc__()) return;
    __gc_off__();
    
    static int once = 0;
    static HashMap GCmemMap;
    static int logEnabled = 0;
    
    if(once == 0) {
        once = 1;
        GCmemMap = HashMapNew(HashMapVoidPtrCmp);
    }
    
    if(mode == 0) {
        if(logEnabled) {
            fprintf(stderr, "GC_ALLOC(%p)\n", (void*) p);
        }
        
        // allocated
        int phv = (int) (intptr_t) p;
        if(phv > 0) {
            phv *= -1;
        }
        HashMapSet(&GCmemMap, phv, (void*) p, (void*)p);
        
        int mem_size = 0;
        LOOP_HASHMAP(&GCmemMap, i) {
            ++mem_size;
        }
        
        //fprintf(stderr, "GC: Alloc %p (left: %d)\n", p, mem_size);
        
    } else if(mode == 1) {
        // freed
        int phv = (int) (intptr_t) p;
        if(phv > 0) {
            phv *= -1;
        }
        
        HashMapRemove(&GCmemMap, phv, (void*) p);
        /*LOOP_HASHMAP(&GCmemMap, i) {
            void* ptr = HashMapGetValue(i);
             fprintf(stderr, "GC: LEFT %p\n", ptr);
            //free(ptr);
        }*/
        
        
        int mem_size = 0;
        LOOP_HASHMAP(&GCmemMap, i) {
            ++mem_size;
        }
        
        if(logEnabled) {
            fprintf(stderr, "GC_FREE(NORMAL, %p)\n", (void*) p);
        }
        
        //fprintf(stderr, "GC: Free %p (left: %d) = %d\n", p, mem_size, HashMapHas(&GCmemMap, sizeof(void*), (void*) &p));
    } else if(mode == 2 || mode == 3 || mode == 5) {
        LOOP_HASHMAP(&GCmemMap, i) {
            void* ptr = HashMapGetValue(i);
            if(logEnabled) {
                fprintf(stderr, "GC_FREE(GC_CLEANUP, %p)\n", (void*) p);
            }
            free(ptr);
        }
        
        if(mode != 5) {
            HashMapDestroy(&GCmemMap);
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
    } else if(mode == 4) {
        logEnabled = (int) (intptr_t) p;
    }
    
    __gc_on__();
}

static inline void __gc_exit_hook__() {
    GC_FREE_ALL();
}

static void __gc_init__() {
    
    ExitHandlerSetup();
    ExitHandlerOverrideGC(__gc_exit_hook__);
    
    __gc_mem_tree__funct = __gc_mem_tree__;
}

#endif // __GCINIT_H__