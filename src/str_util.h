#ifndef __STR_UTIL_H__
#define __STR_UTIL_H__

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

/*char* strjoin(char* text, ...) {
    va_list ap;
    int i, size;
    char* result;
    char* list[num];
    va_start (ap, num);

    size = strlen(text);
    for(i=0; i<num; i++) {
        list[i] = va_arg(ap, char*);
        if(list[i] != NULL) {
            size = size + strlen(list[i]);
        }
    }
    
    result = malloc(sizeof(char)*(size+1));
    result[0] = '\0';
    strcpy(result, text);
    
    for(i=0; i<num; i++) {
        if(list[i] != NULL) {
            result = strcat(result, list[i]);
        }
    }
    
    va_end(ap);
    return result;
}*/


#endif // __STR_UTIL_H__