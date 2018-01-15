#ifndef __SERIALIZE_H__
#define __SERIALIZE_H__

#include <stdlib.h>

#define serializeObj(TYPE, PTR)    (serializeMem((void*)(PTR), sizeof(TYPE)))
#define deserializeObj(TYPE, PTR)  ((TYPE*)deserializeMem(PTR))

char* serializeMem(void* data, const int len) {
    char* buffer = (char*) malloc(len+1);
    printf("[SAVE] Length of block = %d\n", len);
    
    const char* data_char = (char*) data;
    memcpy(buffer+1, data_char, len);
    
    buffer[0] = len;
    return buffer;
}

void* deserializeMem(char* data) {
    const int len = data[0];
    printf("[LOAD] Length of block = %d\n", len);
    
    char* buffer = (char*) malloc(len);
    memcpy((void*) buffer, (void*) (data+1), len);

    return buffer;
}


#endif // __SERIALIZE_H__