/** @file
*  Error-detecing memory allocators.
*
*  @author Piotr Styczyński <piotrsty1@gmail.com>
*  @copyright MIT
*  @date 2017-05-13
*/
#ifndef __STY_COMMON_MEMALLOC_H__
#define __STY_COMMON_MEMALLOC_H__

#include "utils.h"

#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>


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
  assert(data != NULL);

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
  assert(size > 0);

  void* data = realloc(p, size);

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
  assert(count > 0);
  assert(size > 0);

  void* data = calloc(count, size);
  assert(data != NULL);

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
  //if(p == NULL) return AllocateMemoryBlockArray(count, size);
  assert(count > 0);
  assert(size > 0);
  
  void* data = realloc(p, count * size);
  assert(data != NULL);

  return data;
}


#endif /* __STY_COMMON_MEMALLOC_H__ */