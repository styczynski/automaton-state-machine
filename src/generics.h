/** @file
*  Generic collections support and utilities
*
*  @author Piotr Styczyński <piotrsty1@gmail.com>
*  @copyright MIT
*  @date 2017-05-13
*/
#include <stdio.h>

#ifndef __STY_COMMON_GENERICS_H__
#define __STY_COMMON_GENERICS_H__

/**
* Generic type printer.
* Function that accept generic (void*) data.
* And return (may return NULL) some generic data.
* It's the most generic unary operation.
*
* @param[in] : data input
* @return some data or NULL
*/
typedef void* (*GenericsPrinter)(void* data);

/**
* Printer function for generic collections.
* Converts void* to int* and prints its value.
*
* @param[in] data : data input
* @return NULL
*/
static inline void* GenericsIntPrinter(void* data) {
  if(data == NULL) {
    printf("nil");
  } else {
    printf("%d", *((int*)data));
  }
  return NULL;
}

/**
* Printer function for generic collections.
* Prints void* pointer address.
*
* @param[in] data : data input
* @return NULL
*/
static inline void* GenericsPtrPrinter(void* data) {
  printf("%p", data);
  return NULL;
}


#endif /* __STY_COMMON_GENERICS_H__ */