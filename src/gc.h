#ifndef __GC_H__
#define __GC_H__

#include <stdio.h>

#ifndef ENABLE_GC

#define GC_STATUS (0)
#define GC_ON()
#define GC_OFF()
#define GC_ON_ALLOC(PTR)
#define GC_ON_FREE(PTR)
#define GC_SETUP()
#define GC_FREE_ALL()
#define GC_FORK_FREE()
#define GC_EXIT(CODE) exit(CODE)
#define GC_LOG_ON()
#defnie GC_LOG_OFF()

#else // ENABLE_GC

static void (*__gc_mem_tree__funct)(int, void*) = NULL;

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


static inline int __set_read_gc_status__(int w, int mode) {
    static int current_gc_mode = 1;
    
    if(w) {
        current_gc_mode = mode;
    }
    return current_gc_mode;
}

static inline void __gc_on__() {
    (void) __set_read_gc_status__(1, 1);
}

static inline void __gc_off__() {
    (void) __set_read_gc_status__(1, 0);
}

static inline int __is_gc__() {
    return __set_read_gc_status__(0, 0);
}

#endif // ENABLE_GC

#endif // __GC_H__