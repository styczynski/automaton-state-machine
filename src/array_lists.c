/*
*  Unidirectional list implementation based on arrays (C99 standard)
*  Usage:
*  @code
*     #include <array_lists.h>
*      ...
*     ArrayList l = ArrayListNew();
*     int a = 42;
*     int b = 64;
*
*     ArrayListPushBack(&l, &a);
*     ArrayListPushBack(&l, &b);
*
*     printf("%d", *((int*)ArrayListPopBack(&l))); //Output: 64
*     printf("%d", ArrayListSize(&l));             //Output: 1
*
*     ArrayListDestroy(&l);
*  @endcode
*
*
*  @author Piotr Styczyński <piotrsty1@gmail.com>
*  @copyright MIT
*  @date 2017-05-13
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "memalloc.h"
#include "array_lists.h"

/*
* Function resizing capacity of an ArrayList to @p min_size + 1
*/
void ArrayListResizeCapacity(ArrayList* l, const int min_size) {
  if(l->alloc_size > min_size) return;
  if(min_size <= 0) return;
  l->data = MREALLOCATE_ARRAY(ArrayListData, min_size+1, l->data);
  l->alloc_size = min_size+1;
}

/*
* Function resizing automagically capacity of an array
* at least to size of @p min_size
*/
void ArrayListResize(ArrayList* l, const int min_size) {
  if(l->alloc_size > min_size) return;
  if(min_size <= 0) {
    ArrayListResizeCapacity(l, 2);
  } else {
    ArrayListResizeCapacity(l, (l->alloc_size) * ARRAY_LIST_AUTORESIZE_FACTOR);
    ArrayListResizeCapacity(l, min_size);
  }
}

/*
* Function resizing the list to have at least @p min_size elements
*/
void ArrayListResizeFill(ArrayList* l, const int min_size) {
  
  if(l->size >= min_size) return;
  int oldSize = l->size;
  ArrayListResize(l, min_size + 30);
  for(int i=oldSize;i<min_size;++i) {
      (l->data)[i] = NULL;
  }
  l->size = min_size;
  
}

/*
* Function resizing the list to have at least @p min_size elements
* And filling new elements with given list modifier (ctor)
*/
void ArrayListResizeFillWith(ArrayList* l, const int min_size, ArrayListModifierFn ctor) {
  if(l->size >= min_size) return;
  int oldSize = l->size;
  ArrayListResize(l, min_size + 30);
  for(int i=oldSize;i<min_size;++i) {
      (l->data)[i] = ctor(NULL);
  }
  l->size = min_size;
}

/*
* Push element to the end of the given ArrayList
*/
ArrayListIterator ArrayListPushBack(ArrayList* l, ArrayListData value) {
  if(l == NULL) {
    return (ArrayListIterator){ .target = NULL, .position = 0 };
  } else {
    ArrayListResize(l, l->size + 1);
    (l->data)[l->size] = value;
    (l->size)++;
    return (ArrayListIterator){ .target = l, .position = (l->size)-1 };
  }
}

/*
* Remove the last element of the given ArrayList
* Return data pointer held by the removed element.
* If ArrayList is empty return NULL.
*/
ArrayListData ArrayListPopBack(ArrayList* l) {
  if(l == NULL) {
    return NULL;
  } else {
    if(l->size == 0) return NULL;
    ArrayListData popped_element = (l->data)[(l->size)-1];
    (l->data)[(l->size)-1] = NULL;
    (l->size)--;
    return popped_element;
  }
}

/*
* Remove all elements of the given ArrayList
*/
void ArrayListClear(ArrayList* l) {
  if(l == NULL) return;
  for(int i=0; i<l->size; ++i) {
    (l->data)[i] = NULL;
  }
  l->size = 0;
}

/*
* Deallocate ArrayList
*/
void ArrayListDestroy(ArrayList* l) {
  if(l == NULL) return;
  if(l->alloc_size <= 0) return;
  if(l->data != NULL) {
    FREE(l->data);
  }
  l->data = NULL;
  l->size = 0;
  l->alloc_size = 0;
}

/*
* Get first element data pointer or NULL if the ArrayList is empty
*/
ArrayListData ArrayListFirst(const ArrayList* l) {
  if(l == NULL) return NULL;
  if(l->size == 0) return NULL;
  return (l->data)[0];
}

/*
* Get lst element data pointer or NULL if the ArrayList is empty
*/
ArrayListData ArrayListLast(const ArrayList* l) {
  if(l == NULL) return NULL;
  if(l->size == 0) return NULL;
  return (l->data)[(l->size)-1];
}

/*
* Measure ArrayList size in constant time
*/
int ArrayListSize(const ArrayList* l) {
  if(l == NULL) return 0;
  return l->size;
}

/*
* Add all elements of src to the tgt
*/
void ArrayListCopyInto( const ArrayList* src, ArrayList* tgt ) {
  if(src == NULL) return;
  if(tgt == NULL) return;

  LOOP_ARRAY_LIST(src, it) {
    ArrayListPushBack(tgt, ArrayListGetValue(it));
  }
}

/*
* Print ArrayList iterators starting at <l> and going right
*/
void ArrayListPrintNodes(const ArrayListIterator iterator, GenericsPrinter printer) {
  if(ArrayListIsEnd(iterator)) return;
  printer(ArrayListGetValue(iterator));
  printf("; ");
  ArrayListPrintNodes(ArrayListNext(iterator), printer);
}

/*
* Print ArrayList iterators of a given ArrayList
*/
void ArrayListPrint(const ArrayList* l, GenericsPrinter printer) {
  printf("[ ");
  ArrayListPrintNodes(ArrayListBegin(l), printer);
  printf("] ");
}


/*
* Make a copy of a ArrayList
*/
ArrayList ArrayListCopy( const ArrayList* l ) {
  if(l == NULL) return ArrayListNew();
  ArrayList new_list = ArrayListNew();
  ArrayListResize(&new_list, l->size);
  for(int i=0;i<l->size;++i) {
    (new_list.data)[i] = (l->data)[i];
  }
  new_list.size = l->size;
  return new_list;
}


/*
* Maps (ArrayListData)->(ArrayListData) function on iterator <l> going to its right side
*/
static void ArrayListMapNodes( ArrayList* l, ArrayListModifierFn mapping, int preserveValue ) {
  if(l==NULL) return;
  LOOP_ARRAY_LIST(l, it) {
    ArrayListData ret = mapping(ArrayListGetValue(it));
    if(!preserveValue) {
      ArrayListSetValue(it, ret);
    }
  }
}

/*
* Iterate (ArrayListData)->(ArrayListData) function through the given ArrayList
*/
void ArrayListIterate(const ArrayList* l, ArrayListModifierFn iterator) {
  if(l == NULL) return;
  ArrayListMapNodes((ArrayList*) l, iterator, 1);
}

/*
* Maps (ArrayListData)->(ArrayListData) function on the given ArrayList
*/
void ArrayListMap(ArrayList* l, ArrayListModifierFn mapping) {
  if(l == NULL) return;
  ArrayListMapNodes(l, mapping, 0);
}

/*
* Make a deep copy of a ArrayList
*/
ArrayList ArrayListDeepCopy( const ArrayList* l, ArrayListModifierFn assigner ) {
  if(l == NULL) return ArrayListNew();
  ArrayList ret = ArrayListCopy(l);
  ArrayListMap(&ret, assigner);

  return ret;
}