/** @file
*  Bidirectional List implementation (C99 standard)
*  Usage:
*  @code
*     #include <dynamic_lists.h>
*      ...
*     List l = ListNew();
*  @endcode
*
*
*  @author Piotr Styczyński <piotrsty1@gmail.com>
*  @copyright MIT
*  @date 2017-05-13
*/
#include "utils.h"

#include <stdio.h>
#include "memalloc.h"
#include "generics.h"

#ifndef __STY_COMMON_DYNAMIC_LISTS_H__
#define __STY_COMMON_DYNAMIC_LISTS_H__

/**
* @def LOOP_LIST(LIST, VAR_NAME)
* Macro for interating through List
*
* Usage:
*
* @code
*   LOOP_LIST(List_object, i) {
*         ListData element = ListsGetValue(i);
*         printf("void_ptr = %p\n", i);
*    }
* @endcode
*
* @param[in] LIST     : List name to be iterated
* @param[in] VAR_NAME : name of iterator variable
*/
#define LOOP_LIST(LIST, VAR_NAME) \
  for(ListIterator VAR_NAME = ListBegin(LIST); \
  VAR_NAME != NULL; VAR_NAME = ListNext(VAR_NAME))

/*
* Declare data types needed for Lists implementation
*/

/** Data type held by list */
typedef void* ListData;

/** Type of Lists nodes (elements) */
typedef struct ListNode ListNode;

/** Type of List root element */
typedef struct ListRoot ListRoot;


/**
* Actual type of List - syntax sugar
*/
typedef ListRoot List;

/**
* Structure representing one element of List
* It's got two neighbours (may be NULL)
* Element also contains ListData pointer to the actual data.
*/
struct ListNode {
  ListNode* right; ///< pointer to the right neighbour
  ListData value; ///< pointer to the data held in List element
  ListNode* left; ///< pointer to the right neighbour
};

/**
* Root element of the List containing pointers
* to the two ends of a List
*/
struct ListRoot {
  ListNode* begin; ///< pointer to the begining of the List
  ListNode* end; ///< pointer to the end of the List
};

/**
* List iterator
*/
typedef ListNode* ListIterator;

/**
* Function of type
* (ListData) -> (ListData)
* Used as List data manipulators/iterators
*/
typedef ListData (*ListModifierFn)(ListData);


/**
* Create new List
* All Lists must be then freed with Lists.free(List).
*
* @return List
*/
static inline List ListNew() {
  return (List) {
    .begin = NULL,
    .end = NULL
  };
}

/**
* Destroy given List freeing up memory.
*
* WARN: Invalidates ListIterators
*
* @param[in] l : List*
*/
void ListDestroy(List* l);

/**
* Push @p element to the front of a given List.
* Method returns pointer to the newly created List node.
*
* NOTICE: All ListIterators are valid until used operation does not
*         keep pointers validity.
*
* @param[in] l       : List*
* @param[in] element : ListData
* @return ListIterator
*/
ListIterator ListPushFront( List* l, ListData element );


/**
* Push @p element to the end of a given List.
* Method returns pointer to the newly created List node.
*
* NOTICE: All ListIterators are valid until used operation does not
*         keep pointers validity.
*
* @param[in] l       : List*
* @param[in] element : ListData
* @return ListIterator
*/
ListIterator ListPushBack( List* l, ListData element );

/**
* Removes first element of the given List or does nothing if it's empty.
* Returns data pointer held by the removed element.
* If no element was removed (List is empty) NULL is returned.
*
* WARN: Invalidates ListIterators when elment under pointers
*       will be popped.
*
* @param[in] l : List*
* @return ListData (popped element)
*/
ListData ListPopFront( List* l );

/**
* Removes last element of the given List or does nothing if it's empty.
* Returns data pointer held by the removed element.
* If no element was removed (List is empty) NULL is returned.
*
* WARN: Invalidates ListIterators when elment under pointers
*       will be popped.
*
* @param[in] l : List*
* @return ListData (popped element)
*/
ListData ListPopBack( List* l );


/**
* Clears the entire List.
*
* WARN: Invalidates ListIterators for all elements of List
*
* @param[in] l : List*
*/
void ListClear( List* l );

/**
* Obtain first element of the List.
* Function return ListData pointer to the data under first element.
*
* If the List is empty NULL is returned.
*
* @param[in] l : const List*
* @return first element if exists
*/
ListData ListFirst( const List* l );

/**
* Obtain last element of the List.
* Function return ListData pointer to the data under first element.
*
* If the List is empty NULL is returned.
*
* @param[in] l : const List*
* @return last element if exists
*/
ListData ListLast( const List* l );

/**
* Obtain the size of a List.
*
* WARN: Works in O(n) time where n is the length of the List
*
* @param[in] l : const List*
* @return size of the List
*/
int ListSize( const List* l );

/**
* Check's if List is empty
*
* @param[in] l : const List*
* @return If List is empty?
*/
static inline int ListEmpty( const List* l ) {
  if(l == NULL) return 1;
  return (l->begin == NULL && l->end == NULL);
}

/**
* Copy the List into a new one.
*
* WARN: Each element will be a new one, but the data
*       pointers will be still pointing to the same
*       memory locations (there're still the same
*       objects under ListData pointers)
*
* @param[in] l : const List*
* @return shallow copy of a given List
*/
List ListCopy( const List* l );

/**
* Performs deep copy of the List returning new one.
* The given (ListData)->(ListData) function is used as assigner.
* The function should create new element value, copy the value of
* the given one and return pointer to this element.
*
* @param[in] l                : const List*
* @param[in] elementAllocator : ListModifierFn
* @return deep copy of a given List
*/
List ListDeepCopy( const List* l, ListModifierFn elementAllocator );

/**
* Copy the List @p source into other List -  @p target (shallow copy)
*
* WARN: Each element will be a new one, but the data
*       pointers will be still pointing to the same
*       memory locations (there're still the same
*       objects under ListData pointers)
*
* @param[in] source : const List*
* @param[in] target : List*
*/
void ListCopyInto( const List* source, List* target );

/**
* Iterate through List using given
* (ListData)->(ListData) function.
* Function is executed for every List element value
* The return value is ignored.
*
* @param[in] l        : const List*
* @param[in] iterator : ListModifierFn
*/
void ListIterate( const List* l, ListModifierFn iterator );

/**
* Map List values using given
* (ListData)->(ListData) function
* Function is executed for every List element value
* Then the result of function is assigned to the
* element's data pointer.
*
* NOTICE: Mapping is made in place.
*
* @param[in] l       : const List*
* @param[in] mapping : ListModifierFn
*/
void ListMap( List* l, ListModifierFn mapping );

/**
* Print given List to stdout.
* Prints only adresses of values not exact values.
*
* @param[in] l       : const List*
* @param[in] printer : GenericsPrinter
*/
void ListPrint( const List* l, GenericsPrinter printer );

/**
* Print given List to stdout.
* Prints only adresses of values not exact values.
* Variant displaying new line at the end of stringified List.
*
* @param[in] l       : const List*
* @param[in] printer : GenericsPrinter
*/
static inline void ListPrintln( const List* l, GenericsPrinter printer ) {
  if(l==NULL) {
    printf("NULL\n");
    return;
  }
  ListPrint(l, printer);
  printf("\n");
}

/**
* Print given List to stdout.
* Prints only adresses of values not exact values.
*
* @param[in] l : const List*
*/
static inline void ListPrintData( const List* l ) {
  ListPrint(l, GenericsPtrPrinter);
}

/**
* Print given List to stdout.
* Prints only adresses of values not exact values.
* Variant displaying new line at the end of stringified List.
*
* @param[in] l : const List*
*/
static inline void ListPrintlnData( const List* l ) {
  ListPrintln(l, GenericsPtrPrinter);
}

/**
* Get the first element container pointer.
* If the List is empty then NULL is returned.
*
* NOTICE: All ListIterators are valid until used operation does not
*         keep pointers validity.
*
* @param[in] l : const List*
* @return ListIterator pointing to the List begining
*/
static inline ListIterator ListBegin( const List* l ) {
  if(l == NULL) return NULL;
  return l->begin;
}

/**
* Get the last element container pointer.
* If the List is empty then NULL is returned.
*
* NOTICE: All ListIterators are valid until used operation does not
*         keep pointers validity.
*
* @param[in] l : const List*
* @return ListIterator pointing to the List end
*/
static inline ListIterator ListEnd( const List* l ) {
  if(l == NULL) return NULL;
  return l->end;
}

/**
* Removes element from the List using given container pointer.
* The List parameter MUST BE NON NULL for nodes that are first or last
* (isSideElement return true)
* For all other situations it may be NULL
*
* WARN: Invalidates pointers to the removed elements.
*
* @param[in] l    : List*
* @param[in] node : ListIterator
*/
void ListDetachElement( List* l, ListIterator node );

/**
* Create node that is not attached to anything.
* This functionality may be used in situations when you need
* List nodes outside actual List.
*
* NOTICE: All ListIterators are valid until used operation does not
*         keep pointers validity.
*
* @return ListIterator to that not-attached node
*/
ListIterator ListNewDetachedElement( );

/**
* Checks if given node is the last or first element.
*
* NOTICE: For detached nodes this function returns true
*         Formally detached nodes are simultaniously
*         on the left and right side of the List as they have no
*         neightbours on any side.
*
* @param[in] node : ListIterator
* @return If the node is on the left/side of the List
*/
static inline int ListIsSideElement( ListIterator node ) {
  if(node == NULL) return 0;
  return ( (node->left == NULL) || (node->right == NULL) );
}

/**
* Inserts List @p source to the left side of @p node of List @p target
* leaving @p source empty.
* Note that @p target MUST BE NON-NULL only when the @p node is first/last
* element of the List (isSideElement return true).
* For all other situations it may be NULL
*
* @param[in] target : List*
* @param[in] node   : ListIterator
* @param[in] source : List*
*/
void ListInsertListAt( List* target, ListIterator node, List* source );

/**
* Inserts value  to the left side of @p node of List @p target
*
* Note that @p target MUST BE NON-NULL only when the @p node is first/last
* element of the List (isSideElement return true).
* For all other situations it may be NULL
*
* @param[in] target : List*
* @param[in] node   : ListIterator
* @param[in] value  : ListData
*/
void ListInsertElementAt( List* target, ListIterator node, ListData value );


/**
* Checks if given node is the last element
*
* NOTICE: For detached nodes this function returns true
*         Formally detached nodes are simultaniously
*         on the left and right side of the List as they have no
*         neightbours on any side.
*
* @param[in] node : ListIterator
* @return If the node is on the List's end?
*/
static inline int ListIsEnd( ListIterator node ) {
  if(node == NULL) return 0;
  return (node->right == NULL);
}

/**
* Checks if given node is the first element.
*
* NOTICE: For detached nodes this function returns true
*         Formally detached nodes are simultaniously
*         on the left and right side of the List as they have no
*         neightbours on any side.
*
* @param[in] node : ListIterator
* @return If the node is on the List's begining?
*/
static inline int ListIsBegin( ListIterator node ) {
  if(node == NULL) return 0;
  return (node->left == NULL);
}

/**
* All elements on the right side of @p node are transferred to the new List
* that is returned.
*
* @param[in] l    : List*
* @param[in] node : ListIterator
* @return List
*/
List ListSplitList( List* l, ListIterator node );

/**
* Get next element on the List.
* Returns NULL if node is the last element.
*
* NOTICE: All ListIterators are valid until used operation does not
*         keep pointers validity.
*
* @param[in] node : ListIterator
* @return next node (the right neighbour of the current node)
*/
static inline ListIterator ListNext( ListIterator node ) {
  if(node==NULL) return NULL;
  return node->right;
}

/**
* Get prevous element on the List.
* Returns NULL if node is the last element.
*
* NOTICE: All ListIterators are valid until used operation does not
*         keep pointers validity.
*
* @param[in] node : ListIterator
* @return previous node (the left neighbour of the current node)
*/
static inline ListIterator ListPrevious( ListIterator node ) {
  if(node==NULL) return NULL;
  return node->left;
}

/**
* Get value of the List element. Returns void pointer to underlying data.
* Returns NULL if element is NULL.
*
* NOTICE: All ListIterators are valid until used operation does not
*         keep pointers validity.
*
* @param[in] node : ListIterator
* @return value under the given node
*/
static inline ListData ListGetValue( ListIterator node ) {
  if(node==NULL) return NULL;
  return node->value;
}

/**
* Sets value of the List element.
* Does nothing if element is NULL.
*
* NOTICE: All ListIterators are valid until used operation does not
*         keep pointers validity.
*
* @param[in] node  : ListIterator
* @param[in] value : ListData
*/
static inline void ListSetValue( ListIterator node, ListData value ) {
  if(node==NULL) return;
  node->value = value;
}


/**
* Destroy given ArrayList freeing up memory.
*
* WARN: Invalidates ArrayListIterators
*
* @param[in] l           : ArrayList*
* @param[in] deallocator : ListModifierFn
*/
static inline void ListDestroyDeep(List* l, ListModifierFn deallocator) {
  LOOP_LIST(l, iter) {
    deallocator(ListGetValue(iter));
  }
  ListDestroy(l);
}

/**
 * Get value of the element at the position @p position.
 * If the position does not exist in the list then NULL is returned.
 *
 * As the list is dynamic, it does not allow random acceess.
 * That's why this function is linear due to list size.
 *
 * NOTICE: This operation's time complexity is O(n).
 *
 * @param[in] l        : List*
 * @param[in] position : const int
 * @return ListData
 */
static inline ListData ListGetValueAt(List* l, const int position) {
    if(l == NULL) return NULL;
    if(position < 0) return NULL;
    
    int pos_index = 0;
    LOOP_LIST(l, i) {
         ListData element = ListGetValue(i);
         if(position == pos_index) {
             return element;
         }
         ++pos_index;
    }
    return NULL;
}

#include "dynamic_lists.c"

#endif /* __STY_COMMON_DYNAMIC_LISTS_H__ */