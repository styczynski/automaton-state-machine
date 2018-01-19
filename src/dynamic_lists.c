/*
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "memalloc.h"
#include "dynamic_lists.h"


/*
* Makes new List from given values
*/
static ListNode* ListMakeNode(ListNode* left, ListData value, ListNode* right) {
  ListNode* l = MALLOCATE(ListNode);
  l->right = right;
  l->left = left;
  l->value = value;
  return l;
}

/*
* Null/empty objects - used for memory allocation
*/
static void ListDestroyRecLeft(ListNode* l) {
  if(l == NULL) return;
  if(l->left != NULL) {
    ListDestroyRecLeft(l->left);
  }
  DBG {printf("FREE Lists.MListDestroyRecLeft %p\n", (void*)l);fflush(stdout);}
  free(l);
}

/*
* Push element to the front of the given List
*/
ListNode* ListPushFront(List* l, ListData value) {
  if(l == NULL) {
    return NULL;
  } else {
    ListNode* new_node = ListMakeNode(NULL, value, l->begin);
    if(l->begin != NULL) {
      (l->begin)->left = new_node;
    }
    (l->begin) = new_node;
    if(l->end == NULL) {
      (l->end) = new_node;
    }
    return new_node;
  }
}

/*
* Push element to the end of the given List
*/
ListNode* ListPushBack(List* l, ListData value) {
  if(l == NULL) {
    return NULL;
  } else {
    ListNode* new_node = ListMakeNode(l->end, value, NULL);
    if(l->end != NULL) {
      (l->end)->right = new_node;
    }
    (l->end) = new_node;
    if(l->begin == NULL) {
      (l->begin) = new_node;
    }
    return new_node;
  }
}

/*
* Remove the first element of the given List
* Return data pointer held by the removed element.
* If List is empty return NULL.
*/
ListData ListPopFront(List* l) {
  if(l == NULL) {
    return NULL;
  } else if (l->begin == NULL) {
    return NULL;
  } else {
    void *val = (l->begin)->value;
    ListNode* new_begin = (l->begin)->right;
    if(new_begin != NULL) {
      new_begin->left = NULL;
    } else {
      (l->end) = NULL;
    }
    DBG {printf("FREE Lists.ListPopFront %p\n", (void*)(l->begin));fflush(stdout);}
    free(l->begin);
    (l->begin) = new_begin;
    return val;
  }
}

/*
* Remove the last element of the given List
* Return data pointer held by the removed element.
* If List is empty return NULL.
*/
ListData ListPopBack(List* l) {
  if(l == NULL) {
    return NULL;
  } else if (l->end == NULL) {
    return NULL;
  } else {
    ListData val = (l->end)->value;
    ListNode* new_end = (l->end)->left;
    if(new_end != NULL) {
      new_end->right = NULL;
    } else {
      (l->begin) = NULL;
    }
    free(l->end);
    (l->end) = new_end;
    return val;
  }
}

/*
* Remove all elements of the given List
*/
void ListClear(List* l) {
  if(l == NULL) return;
  if(l->end != NULL) {
    ListDestroyRecLeft(l->end);
  }
  (l->begin)=NULL;
  (l->end)=NULL;
}

/*
* Deallocate List
*/
void ListDestroy(List* l) {
  if(l == NULL) return;
  //clearList(l);
  ListNode* it = l->begin;
  while(it != NULL) {
    ListNode* next = it->right;
    //DBG {printf("FREE Lists.MListDestroy %p\n", (void*)it);fflush(stdout);}
    free(it);
    it = next;
  }
  l->begin = NULL;
  l->end = NULL;
  //free(l);
}

/*
* Get first element data pointer or NULL if the List is empty
*/
ListData ListFirst(const List* l) {
  if(l == NULL) {
    return NULL;
  }
  if(l->begin == NULL) {
    return NULL;
  }
  if(l->begin != NULL) return (l->begin)->value;
  return NULL;
}

/*
* Get lst element data pointer or NULL if the List is empty
*/
ListData ListLast(const List* l) {
  if(l == NULL) {
    return NULL;
  }
  if(l->end == NULL) {
    return NULL;
  }
  if(l->end != NULL) return (l->end)->value;
  return NULL;
}

/*
* Measure List size in O(n) time
*/
int ListSize(const List* l) {
  if(l == NULL) return 0;
  int size = 0;
  LOOP_LIST(l, it) {
    ++size;
  }
  return size;
}

/*
* Add all elements of <src> to the <tgt>
*/
void ListCopyInto( const List* src, List* tgt ) {
  if(src == NULL) return;
  if(tgt == NULL) return;

  LOOP_LIST(src, it) {
    ListPushBack(tgt, it->value);
  }
}

/*
* Print List nodes starting at <l> and going right
*/
static void ListPrintNodes(const ListNode* l, GenericsPrinter printer) {
  if(l==NULL) return;
  printer(l->value);
  printf("; ");
  //printf("%p; ", printer(l->value));
  ListPrintNodes(l->right, printer);
}

/*
* Print List nodes of a given List
*/
void ListPrint(const List* l, GenericsPrinter printer) {
  printf("[ ");
  ListPrintNodes(l->begin, printer);
  printf("] ");
}

/*
* Make a copy of a List
*/
List ListCopy( const List* l ) {
  if(l == NULL) return ListNew();

  const int size = ListSize(l);

  if(size == 0) return ListNew();
  if(size < 5) {
    List ret = ListNew();
    ListCopyInto(l, &ret);
    return ret;
  }

  //ListNode* chunk_d = (ListNode*) malloc(size*sizeof(ListNode)); //MALLOCATE_ARRAY(ListNode, size);
  ListNode* chunk[size];
  for(int i=0;i<size;++i) {
    chunk[i] = ListMakeNode(NULL, NULL, NULL);
  }

  for(int i=0;i<size;++i) {
    chunk[i]->left = NULL;
    chunk[i]->right = NULL;
    chunk[i]->value = NULL;
  }
  ListNode* iter = l->begin;
  for(int i=0;i<size;++i) {
    if(iter == NULL) {
      break; //TODO: Throw sth?
    } else {
        chunk[i]->value = iter->value;
        iter = iter->right;
    }
  }
  chunk[0]->left = NULL;
  chunk[size-1]->right = NULL;
  for(int i=0;i<size;++i) {
    if(i!=0) {
      chunk[i]->left = (chunk[i-1]);
    }
    if(i!=size-1) {
      chunk[i]->right = (chunk[i+1]);
    }
  }

  ListRoot ret = { .begin = (chunk[0]), .end = (chunk[size-1]) };
  return ret;
}


/*
* Maps (ListData)->(ListData) function on node <l> going to its right side
*/
static void ListMapNodes( ListNode* l, ListModifierFn mapping, int preserveValue ) {
  if(l==NULL) return;
  ListData ret = mapping(l->value);
  if(!preserveValue) {
    l->value = ret;
  }
  ListMapNodes(l->right, mapping, preserveValue);
}

/*
* Iterate (ListData)->(ListData) function through the given List
*/
void ListIterate(const List* l, ListModifierFn iterator) {
  if(l == NULL) return;
  if(l->begin == NULL) return;
  ListMapNodes(l->begin, iterator, 1);
}

/*
* Maps (ListData)->(ListData) function on the given List
*/
void ListMap(List* l, ListModifierFn mapping) {
  if(l == NULL) return;
  if(l->begin == NULL) return;
  ListMapNodes(l->begin, mapping, 0);
}

/*
* Make a deep copy of a List
*/
List ListDeepCopy( const List* l, ListModifierFn assigner ) {
  if(l == NULL) return ListNew();
  List ret = ListCopy(l);
  ListMap(&ret, assigner);

  return ret;
}

/*
* Remove given element from the List
*/
void ListDetachElement( List* l, ListNode* node ) {
  if(node == NULL) return;

  ListNode* left_neighbour = node->left;
  ListNode* right_neighbour = node->right;

  if(left_neighbour != NULL) {
    left_neighbour->right = right_neighbour;
  } else {
    l->begin = right_neighbour;
  }
  if(right_neighbour != NULL) {
    right_neighbour->left = left_neighbour;
  } else {
    l->end = left_neighbour;
  }
  DBG {printf("FREE Lists.ListDetachElement %p\n", (void*)node);fflush(stdout);}
  free(node);
}

/*
* Create new element container not attached to any List.
*/
ListNode* ListNewDetachedElement() {
  ListNode* ret = MALLOCATE(ListNode);
  DBG {printf("MALLOC Lists.ListNewDetachedElement %p\n", (void*)ret);fflush(stdout);}
  ret->left = NULL;
  ret->right = NULL;
  ret->value = NULL;
  return ret;
}

/*
* Append List <src> to the left of <node> in List <tgt>
* leaving <src> empty.
* Tgt parameter MUST BE NON-NULL only if <node> is first/last element.
* Otherwise it may be NULL
*/
void ListInsertListAt( List* tgt, ListNode* node, List* src ) {
  if(node == NULL) return;
  if(src == NULL) return;
  if((src->begin == NULL) || (src->end == NULL)) {
    return;
  }
  if(node->left == NULL) {
    tgt->begin = src->begin;
    src->end->right = node;
    node->left = src->end;

    src->begin = NULL;
    src->end = NULL;
    return;
  } else if(node->right == NULL) {
    node->left->right = src->begin;
    src->begin->left = node->left;
    node->left = src->end;
    src->end->right = node;

    src->begin = NULL;
    src->end = NULL;
    return;
  } else {
    node->left->right = src->begin;
    src->begin->left = node->left;
    node->left = src->end;
    src->end->right = node;

    src->begin = NULL;
    src->end = NULL;
    return;
  }
}


/*
* Append a new value to the left of <node> in List <tgt>
* Tgt parameter MUST BE NON-NULL only if <node> is first/last element.
* Otherwise it may be NULL
*/
void ListInsertElementAt( List* tgt, ListNode* node, ListData value ) {
  List wrapper = ListNew();
  ListPushBack(&wrapper, value);
  ListInsertListAt(tgt, node, &wrapper);
  ListDestroy(&wrapper);
}


/*
* Transfer all right-side neighbours of <splitter> to a new List.
* Return that new List.
*/
List ListSplit( List* src, ListNode* splitter ) {
  if(src == NULL) return ListNew();
  List ret = ListNew();
  if(splitter == NULL) {
    return ret;
  }

  ListNode* real_end = src->end;
  src->end = splitter;

  if(splitter->right != NULL) {
    ret.begin = splitter->right;
    ret.end = real_end;
    splitter->right = NULL;
    ret.begin->left = NULL;
  }
  return ret;
}