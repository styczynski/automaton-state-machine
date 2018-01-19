#ifndef __GETLINE_H__
#define __GETLINE_H__

#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "memalloc.h"
#include "syslog.h"

int getline(char **lineptr, size_t *n, FILE *stream) {
    static char line[256];
    char *ptr;
    unsigned int len;
    if(lineptr == NULL || n == NULL) {
       errno = EINVAL;
       return -1;
    }
    if(ferror(stream)) {
        return -1;
    }
    if(feof(stream)) {
        return -1;
    }
    char* fgetsptr = fgets(line, 256, stream);
    if(!fgetsptr) {
        return -1;
    }
    ptr = strchr(line,'\n');
    if(ptr) {
       *ptr = '\0';
    }
    len = strlen(line);
    if((len+1) < 256) {
        ptr = (char*) MREALLOCATE_BLOCKS(1, 256, *lineptr);
        if (ptr == NULL) {
            return -1;
        }
        *lineptr = ptr;
        *n = 256;
    }
    strcpy(*lineptr,line); 
    return len;
}

/*
 * Read characters from string until newline or EOF.
 */
char* strGets(char* out, int n, char** str_stream) {
    
    if(str_stream == NULL) return NULL;
    if(*str_stream == NULL) return NULL;
    
    char* outi = out;
    for(int i=0;i<n;++i) {
        if((*str_stream)[i] == '\n') {
            *outi = (*str_stream)[i];
            *(outi+1) = '\0';
            *str_stream = &((*str_stream)[i+1]);
            return out;
        } else if((*str_stream)[i] == '\0') {
            *outi = (*str_stream)[i];
            *str_stream = &((*str_stream)[i]);
            return out;
        } else {
            *outi = (*str_stream)[i];
            ++outi;
        }
    }
    
    *outi = '\0';
    *str_stream = &((*str_stream)[n]);
    return out;
}


int strGetline(char **lineptr, size_t *n, char** str_stream) {
    static char line[256];
    char *ptr;
    unsigned int len;
    if(lineptr == NULL || n == NULL) {
       errno = EINVAL;
       (*lineptr)[0] = '\0';
       return -1;
    }
    if(str_stream == NULL) {
        (*lineptr)[0] = '\0';
        return -1;
    }
    if(*str_stream == NULL) {
        (*lineptr)[0] = '\0';
        return -1;
    }
    char* fgetsptr = strGets(line, 256, str_stream);
    if(!fgetsptr) {
        return -1;
    }
    ptr = strchr(line,'\n');
    if(ptr) {
       *ptr = '\0';
    }
    len = strlen(line);
    if((len+1) < 256) {
        ptr = (char*) MREALLOCATE_BLOCKS(1, 256, *lineptr);
        if (ptr == NULL) {
            return -1;
        }
        *lineptr = ptr;
        *n = 256;
    }
    strcpy(*lineptr,line); 
    
    return len;
}

#endif // __GETLINE_H__