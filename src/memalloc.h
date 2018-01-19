/** @file
*  Error-detecing memory allocators.
*
*  @author Piotr Styczyński <piotrsty1@gmail.com>
*  @copyright MIT
*  @date 2017-05-13
*/
#ifndef __STY_COMMON_MEMALLOC_H__
#define __STY_COMMON_MEMALLOC_H__

#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include "syslog.h"


/**
* @def MALLOCATE(STRUCT)
*
* Macro giving value of pointer to the allocated structure @p STRUCT
*
* NOTICE: Assertion checking is done for allocation errors.
*
* @param[in] STRUCT : Type to be allocated
*/
#define MALLOCATE(STRUCT) \
( (STRUCT*) AllocateMemoryBlock(sizeof(STRUCT)) )

/**
* @def MALLOCATE_ARRAY(STRUCT, LEN)
*
* Macro giving value of pointer to the allocated array of size @p LEN
* of structures @p STRUCT
*
* NOTICE: Assertion checking is done for allocation errors.
*
* @param[in] STRUCT : Type to be allocated
* @param[in] LEN    : Length of array to be allocated
*/
#define MALLOCATE_ARRAY(STRUCT, LEN) \
( (STRUCT*) AllocateMemoryBlockArray((LEN), sizeof(STRUCT)) )

/**
* @def MREALLOCATE(STRUCT, PTR)
*
* Macro giving value of pointer to the reallocated structure @p STRUCT
*
* NOTICE: Assertion checking is done for allocation errors.
*
* @param[in] STRUCT : Type to be allocated
* @param[in] PTR    : Pointer to currently allocated structure
*/
#define MREALLOCATE(STRUCT, PTR) \
( (STRUCT*) ReallocateMemoryBlock((PTR), sizeof(STRUCT)) )

/**
* @def MREALLOCATE_ARRAY(STRUCT, LEN, PTR)
*
* Macro giving value of pointer to the reallocated array of size @p LEN
* of structures @p STRUCT
*
* NOTICE: Assertion checking is done for allocation errors.
*
* @param[in] STRUCT : Type to be allocated
* @param[in] LEN    : Length of new array
* @param[in] PTR    : Pointer to currently allocated structure array
*/
#define MREALLOCATE_ARRAY(STRUCT, LEN, PTR) \
( (STRUCT*) ReallocateMemoryBlockArray((PTR), (LEN), sizeof(STRUCT)) )

/**
* @def MREALLOCATE_BLOCKS(BLOCK_SIZE, LEN)
*
* Macro giving value of pointer to the reallocated array of size @p LEN
* of blocks of size @p SIZE bytes
*
* NOTICE: Assertion checking is done for allocation errors.
*
* @param[in] BLOCK_SIZE : Length of single block in bytes
* @param[in] LEN        : Length of array to be allocated
* @param[in] PTR    : Pointer to currently allocated structure array
*/
#define MREALLOCATE_BLOCKS(BLOCK_SIZE, LEN, PTR) \
( (void*) ReallocateMemoryBlockArray((PTR), (LEN), (BLOCK_SIZE)) )


/**
* @def MALLOCATE_BLOCKS(SIZE, LEN)
*
* Macro giving value of pointer to the allocated array of size @p LEN
* of blocks of size @p SIZE bytes
*
* NOTICE: Assertion checking is done for allocation errors.
*
* @param[in] BLOCK_SIZE : Length of single block in bytes
* @param[in] LEN        : Length of array to be allocated
*/
#define MALLOCATE_BLOCKS(BLOCK_SIZE, LEN) \
( (void*) AllocateMemoryBlockArray((LEN), (BLOCK_SIZE)) )

/**
* Function allocating @p size bytes.
*
* NOTICE: Assertion checking is done for allocation errors.
*
* @param[in] size : int
* @return void* to allocated memory block
*/
static inline void* AllocateMemoryBlock(int size) {
  assert(size > 0);

  void* data = malloc(size);
  if(data == NULL) {
      syserr("AllocateMemoryBlock() failed due to malloc failure");
  }

  return data;
}

/**
* Function reallocating @p size bytes.
*
* NOTICE: Assertion checking is done for allocation errors.
*
* @param[in] p    : reused void* pointer
* @param[in] size : int
* @return void* to allocated memory block
*/
static inline void* ReallocateMemoryBlock(void *p, int size) {
  if(p == NULL) return AllocateMemoryBlock(size);
  
  if(size <= 0) {
      syserrv("ReallocateMemoryBlock() failed because size=%d < 0", size);
  }
  
  void* data = realloc(p, size);
  if(data == NULL) {
      syserr("ReallocateMemoryBlock() failed due to realloc failure");
  }
  
  return data;
}

/**
* Function allocating @p count block each of size @p size bytes.
*
* NOTICE: Assertion checking is done for allocation errors.
*
* @param[in] count : int
* @param[in] size  : int
* @return void* to allocated memory array
*/
static inline void* AllocateMemoryBlockArray(int count, int size) {
  
  if(size <= 0) {
      syserrv("AllocateMemoryBlockArray() failed because size=%d < 0", size);
  }
  
  if(count <= 0) {
      syserrv("AllocateMemoryBlockArray() failed because count=%d < 0", count);
  }

  void* data = calloc(count, size);
  if(data == NULL) {
      syserr("AllocateMemoryBlock() failed due to calloc failure");
  }

  return data;
}


/**
* Function reallocating @p count block each of size @p size bytes.
*
* NOTICE: Assertion checking is done for allocation errors.
*
* @param[in] p     : reused void* pointer
* @param[in] count : int
* @param[in] size  : int
* @return void* to allocated memory array
*/
static inline void* ReallocateMemoryBlockArray(void* p, int count, int size) {
  if(size <= 0) {
      syserrv("ReallocateMemoryBlockArray() failed because size=%d < 0", size);
  }
  
  if(count <= 0) {
      syserrv("ReallocateMemoryBlockArray() failed because count=%d < 0", count);
  }
  
  void* data = realloc(p, count * size);
  if(data == NULL) {
      syserr("ReallocateMemoryBlockArray() failed due to realloc failure");
  }

  return data;
}


#endif /* __STY_COMMON_MEMALLOC_H__ */