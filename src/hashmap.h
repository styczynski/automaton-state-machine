/** @file
*
*  Generics hashmap implementation. (C99 standard)
*
*  @author Piotr Styczyński <piotrsty1@gmail.com>
*  @copyright MIT
*  @date 2018-01-21
*/
#ifndef __HASHMAP_H__
#define __HASHMAP_H__

#include "memalloc.h"
#include "array_lists.h"
#include "dynamic_lists.h"

/**
 * @def HASH_MAP_FACT_P
 *   Hash map heuristics multiplier factor
 */
#define HASH_MAP_FACT_P 350

/**
 * @def HASH_MAP_FACT_Q
 *   Hash map heuristics modulo factor (should be prime)
 */
#define HASH_MAP_FACT_Q 1000000009

/**
 * @def HashMapSetV(HASHMAP, KEY_TYPE, VALUE_TYPE, KEY, VALUE)
 *
 * @param[in] HASHMAP     : Pointer to the hashmap
 * @param[in] KEY_TYPE    : Key value type name
 * @param[in] VALUE_TYPE  : Value type name
 * @param[in] KEY         : Key value
 * @param[in] VALUE       : Value associated with the given KEY
 * 
 * Performs HashMapSet but auto de-/allocates all values.
 */
#define HashMapSetV(HASHMAP, KEY_TYPE, VALUE_TYPE, KEY, VALUE) \
  do { \
      KEY_TYPE* __key_ptr__ = MALLOCATE(KEY_TYPE); \
      *__key_ptr__ = (KEY); \
      VALUE_TYPE* __val_ptr__ = MALLOCATE(VALUE_TYPE); \
      *__val_ptr__ = (VALUE); \
      HashMapSetDeep((HASHMAP), sizeof(KEY_TYPE), __key_ptr__, __val_ptr__); \
  } while(0)
  
/**
 * @def HashMapGetV(HASHMAP, KEY_TYPE, VALUE_TYPE, KEY, VALUE)
 *
 * @param[in] HASHMAP     : Pointer to the hashmap
 * @param[in] KEY_TYPE    : Key value type name
 * @param[in] VALUE_TYPE  : Value type name
 * @param[in] KEY         : Key value
 * 
 * Performs HashMapGet but auto de-/allocates all values.
 */
#define HashMapGetV(HASHMAP, KEY_TYPE, VALUE_TYPE, KEY) \
  (((VALUE_TYPE*)HashMapGet((HASHMAP), sizeof(KEY_TYPE), &(KEY))))

/**
 * @def HashMapHasV(HASHMAP, KEY_TYPE, VALUE_TYPE, KEY, VALUE)
 *
 * @param[in] HASHMAP     : Pointer to the hashmap
 * @param[in] KEY_TYPE    : Key value type name
 * @param[in] VALUE_TYPE  : Value type name
 * @param[in] KEY         : Key value
 * 
 * Performs HashMapHas but auto de-/allocates all values.
 */
#define HashMapHasV(HASHMAP,KEY_TYPE, VALUE_TYPE, KEY) \
  (HashMapHas((HASHMAP), sizeof(KEY_TYPE), &(K EY)))
  
/**
 * @def HashMapRemoveV(HASHMAP, KEY_TYPE, VALUE_TYPE, KEY, VALUE)
 *
 * @param[in] HASHMAP     : Pointer to the hashmap
 * @param[in] KEY_TYPE    : Key value type name
 * @param[in] VALUE_TYPE  : Value type name
 * @param[in] KEY         : Key value
 * 
 * Performs HashMapRemove but auto de-/allocates all values.
 */
#define HashMapRemoveV(HASHMAP, KEY_TYPE, VALUE_TYPE, KEY) \
  (HashMapRemoveDeep((HASHMAP), sizeof(KEY_TYPE), &(KEY)));
      
/**
 * @def HashMapDestroyV(HASHMAP, KEY_TYPE, VALUE_TYPE, KEY, VALUE)
 *
 * @param[in] HASHMAP     : Pointer to the hashmap
 * @param[in] KEY_TYPE    : Key value type name
 * @param[in] VALUE_TYPE  : Value type name
 * 
 * Performs HashMapDestroy but auto de-/allocates all values.
 */
#define HashMapDestroyV(HASHMAP, KEY_TYPE, VALUE_TYPE) \
 (HashMapDestroyDeep((HASHMAP)))

/**
* @def LOOP_HASHMAP(LIST, VAR_NAME)
* Macro for interating through values of HashMap
*
* Usage:
*
* @code
*   HashMap hm;
*   LOOP_HASHMAP(&hm, i) {
*         // Capture HashMapData from iterator:
*         HashMapData element = HashMapGetValue(i);
*   
*         // Capture HashMapKey from iterator:
*         HashMapKey element_key = HashMapGetKey(i);
*
*         // Convert the HashMapData to the int pointer:
*         int element_value = *((*int) element_value);
*
*         // Print data from the hashmap:
*         printf("void_ptr = %p\n", element_value);
*    }
* @endcode
*
* @param[in] LIST     : ArrayList name to be iterated
* @param[in] VAR_NAME : name of iterator variable
*/
#define LOOP_HASHMAP(HASHMAP, VAR_NAME) \
  for(HashMapIterator VAR_NAME = HashMapBegin(HASHMAP); \
  !HashMapIsEnd(VAR_NAME); VAR_NAME = HashMapNext(VAR_NAME))

  /*
* Declare data types needed for HashMap implementation
*/

/** Hashmap iterator */
typedef struct HashMapIterator HashMapIterator;

/** Type of value data held inside HashMap */
typedef void* HashMapData;

/** Type of key data held inside HashMap */
typedef void* HashMapKey;

/** Type of (key, value) pair object held in HashMap */
typedef struct HashMapElement HashMapElement;

/** Type of HashMap node that groups items under one hash value */
typedef struct HashMapNode HashMapNode;

/** Type of HashMap container itself */
typedef struct HashMap HashMap;

/** Type of HashMap comparator function */
typedef int (*HashMapComparatorFn)(HashMapKey, HashMapKey);

/** (key, value) pair object held in HashMap */
struct HashMapElement {
    HashMapKey key;     ///< Key of the object
    HashMapData value;  ///< Value of the object
};

/** Node that groups items under one hash value */
struct HashMapNode {
    List values; ///< List of values grouped under one hash
};

/** HashMap container */
struct HashMap {
    ArrayList nodes;          ///< Nodes for hash values
    HashMapComparatorFn cmp;  ///< Comparator function
};

/** Hashmap iterator */
struct HashMapIterator {
    HashMap* target;  ///< Pointer to the iterated HashMap
    int bucket;       ///< Hash value of item (bucket no.)
    int index;        ///< Index in the objects lists inside the bucket
};

/**
 * HashMap comparator that compares objects by its pointer.
 * This comparator is useful when storing plain numbers as void pointers.
 * Or when dealing with immutable singleton values.
 * 
 * @param[in] a : void* pointing to the key value residing in hashmap
 * @param[in] b : void* pointing to the key value residing in hashmap
 * @returns Are the two addresses are numerically equal?
 */
int HashMapVoidPtrCmp(void* a, void* b) {
    return a == b;
}

/**
 * HashMap comparator that compares itegers.
 *
 * The objects are dereferenced and interpreted as integers.
 * Then compared.
 * 
 * @param[in] a : void* pointing to the key value residing in hashmap
 * @param[in] b : void* pointing to the key value residing in hashmap
 * @returns Are the two integers under pointers indentical?
 */
int HashMapIntCmp(void* a, void* b) {
    return *((int*)a) == *((int*)b);
}

/**
 * HashMap comparator that compares cstrings.
 *
 * The objects are casted to char array and interpreted as cstrnigs.
 * 
 * @param[in] a : void* pointing to the key value residing in hashmap
 * @param[in] b : void* pointing to the key value residing in hashmap
 * @returns Are the strings under pointers equal (strcmp)?
 */
int HashMapStrCmp(void* a, void* b) {
    return strcmp(((char*)a), ((char*)b)) == 0;
}

/*
 * Helper function that creates empty HashMap node
 */
HashMapNode* HashMapCreateNode() {
    HashMapNode* node = MALLOCATE(HashMapNode);
    *node = (HashMapNode) {
        .values = ListNew()
    };
    return node;
}

/*
 * Helper function that creates empty HashMap element
 */
HashMapElement* HashMapCreateElement(HashMapKey key, HashMapData value) {
    HashMapElement* element = MALLOCATE(HashMapElement);
    *element = (HashMapElement) {
        .key = key,
        .value = value
    };
    return element;
}

/**
 * Create new empty HashMap
 * 
 * @param[in] keyComparator : Provided hashmap key comparator
 * @returns Returns new empty HashMap
 */
static inline HashMap HashMapNew(HashMapComparatorFn keyComparator) {
    HashMap hm = (HashMap) {
        .nodes = ArrayListNew(),
        .cmp = keyComparator
    };
    ArrayListResizeFill(&hm.nodes, 100);
    return hm;
}

/**
 * Calcualte the hash of the given key.
 * 
 * This function reads data under void* pointer and hashes bits to calculate the hash.
 * If @p key_size <= 0 then key_size value is used to calcualate the hash not the data
 * under the pointer.
 *
 * If you use HashMap to store integer values as void pointers and not the pointers
 * to actual memory locations then negative value is useful as otherwise
 * accessing data under such integer will couse segv.
 *
 * @param[in] key_size : Size in bytes of the given key
 * @param[in] key      : The key value
 * @returns Hash generated for the value
 */
static inline int HashMapCalcHash(const int key_size, HashMapKey key) {
    
    int hash = 0;
    if(key_size > 0) {
        char* ptr = (char*) key;
        for(int i=0;i<key_size;++i) {
            hash += ptr[i] * HASH_MAP_FACT_P;
            hash %= HASH_MAP_FACT_Q;
        }
    } else {
        hash = (key_size * HASH_MAP_FACT_P) % HASH_MAP_FACT_Q;
    }
    if(hash<0) hash*=-1;
    hash = 1000;
    
    return hash;
}

/**
 * Get element out of hashmap.
 * 
 * @param[in] hm       : Hashmap to be searched
 * @param[in] key_size : Size of the key to be provided for HashMapCalcHash
 * @param[in] key      : Key to be provided for HashMapCalcHash
 * @returns Returns value under the given key or NULL if it does not exist
 */
static inline HashMapData HashMapGet(HashMap* hm, const int key_size, HashMapKey key) {
    if(key == NULL) return NULL;
    
    const int hash = HashMapCalcHash(key_size, key);
    
    HashMapNode* currentNode = (HashMapNode*) ArrayListGetValueAt(&hm->nodes, hash);
    if(currentNode == NULL) {
        return NULL;
    }
    
    LOOP_LIST(&(currentNode->values), i) {
        HashMapElement* element = (HashMapElement*) ListGetValue(i);
        if(element != NULL) {
            if(element->key != NULL) {
                if(hm->cmp(element->key, key)) {
                    return element->value;
                }
            }
        }
    }
    
    return NULL;
}

/**
 * Checks if the key exists in the hashmap.
 * 
 * @param[in] hm       : Hashmap to be searched
 * @param[in] key_size : Size of the key to be provided for HashMapCalcHash
 * @param[in] key      : Key to be provided for HashMapCalcHash
 * @returns If the key is present is the HashMap?
 */
static inline int HashMapHas(HashMap* hm, const int key_size, HashMapKey key) {
    return HashMapGet(hm, key_size, key) != NULL;
}

/**
 * Removes element from the hashmap.
 * Returns value of the element that was removed.
 * If it does not exists nothing happens and HULL is returned.
 * 
 * NOTE:
 *   You msut manually free the returned pointer manually.
 *
 * @param[in] hm       : Hashmap to be searched
 * @param[in] key_size : Size of the key to be provided for HashMapCalcHash
 * @param[in] key      : Key to be provided for HashMapCalcHash
 * @returns Returns value under the given key or NULL if it does not exist
 */
static inline HashMapData HashMapRemove(HashMap* hm, const int key_size, HashMapKey key) {
    if(key == NULL) return NULL;
    
    const int hash = HashMapCalcHash(key_size, key);
    
    HashMapNode* currentNode = (HashMapNode*) ArrayListGetValueAt(&hm->nodes, hash);
    if(currentNode == NULL) {
        return NULL;
    }
    
    LOOP_LIST(&(currentNode->values), i) {
        HashMapElement* element = (HashMapElement*) ListGetValue(i);
        if(element != NULL) {
            if(element->key != NULL) {
                if(hm->cmp(element->key, key)) {
                    HashMapData ret = element->value;
                    ListDetachElement(&(currentNode->values), i);
                    FREE(element);
                    return ret;
                }
            }
        }
    }
    
    return NULL;
}


/**
 * Removes element from the hashmap.
 * If it does not exists nothing happens and HULL is returned.
 * 
 * NOTE:
 *  This function autodeallocates key value that was removed with free.
 *
 * @param[in] hm       : Hashmap to be searched
 * @param[in] key_size : Size of the key to be provided for HashMapCalcHash
 * @param[in] key      : Key to be provided for HashMapCalcHash
 * @returns Returns value under the given key or NULL if it does not exist
 */
static inline void HashMapRemoveDeep(HashMap* hm, const int key_size, HashMapKey key) {
    if(key == NULL) return;
    
    const int hash = HashMapCalcHash(key_size, key);
    
    HashMapNode* currentNode = (HashMapNode*) ArrayListGetValueAt(&hm->nodes, hash);
    if(currentNode == NULL) {
        return;
    }
    
    LOOP_LIST(&(currentNode->values), i) {
        HashMapElement* element = (HashMapElement*) ListGetValue(i);
        if(element != NULL) {
            if(element->key != NULL) {
                if(hm->cmp(element->key, key)) {
                    FREE(element->value);
                    FREE(element->key);
                    ListDetachElement(&(currentNode->values), i);
                    FREE(element);
                    return;
                }
            }
        }
    }
    
}

/**
 * Destroys entire HashMap.
 * 
 * NOTE:
 *   Values of (key, value) pairs must be earlier deallocated manually.
 *
 * @param[in] hm : HAshmap to be destroyed
 */
static inline void HashMapDestroy(HashMap* hm) {
    LOOP_ARRAY_LIST(&(hm->nodes), i) {
        HashMapNode* currentNode = (HashMapNode*) ArrayListGetValue(i);
        LOOP_LIST(&(currentNode->values), j) {
            HashMapElement* element = (HashMapElement*) ListGetValue(j);
            if(element != NULL) {
                FREE(element);
            }
        }
        ListDestroy(&(currentNode->values));
        FREE(currentNode);
    }
    ArrayListDestroy(&(hm->nodes));
}

/**
 * Destroys entire HashMap automatically freeing memory.
 * 
 * NOTE:
 *   Values of (key, value) pairs will be automatically free'd with free(ptr).
 *
 * @param[in] hm : HAshmap to be destroyed
 */
static inline void HashMapDestroyDeep(HashMap* hm) {
    LOOP_ARRAY_LIST(&(hm->nodes), i) {
        HashMapNode* currentNode = (HashMapNode*) ArrayListGetValue(i);
        LOOP_LIST(&(currentNode->values), j) {
            HashMapElement* element = (HashMapElement*) ListGetValue(j);
            if(element != NULL) {
                FREE(element->value);
                FREE(element->key);
                FREE(element);
            }
        }
        ListDestroy(&(currentNode->values));
        FREE(currentNode);
    }
    ArrayListDestroy(&(hm->nodes));
}

/**
 * Set the value under the given key.
 * Overrides element with the same key.
 * Returns old element that will be overwritten (with the same key).
 *
 * NOTE:
 *   Returned data pointer must be manually free'd.
 * 
 * @param[in] hm       : Hashmap to be searched
 * @param[in] key_size : Size of the key to be provided for HashMapCalcHash
 * @param[in] key      : Key to be provided for HashMapCalcHash
 * @param[in] value    : Value to be associated with the given key
 * @returns Returns old value from under the provided key (may be NULL)
 */
static inline HashMapData HashMapSet(HashMap* hm, const int key_size, HashMapKey key, HashMapData value) {
    if(key == NULL) return NULL;
    HashMapData oldData = HashMapRemove(hm, key_size, key);
  
    const int hash = HashMapCalcHash(key_size, key);
    
    HashMapNode* currentNode = (HashMapNode*) ArrayListGetValueAt(&hm->nodes, hash);
    if(currentNode == NULL) {
        currentNode = HashMapCreateNode();
    }
    
    HashMapElement* element = HashMapCreateElement(key, value);
    ListPushBack(&(currentNode->values), element);
    
    ArrayListSetValueAt(&hm->nodes, hash, currentNode);
    
    return oldData;
}

/**
 * Set the value under the given key.
 * Overrides element with the same key.
 * Returns old element that will be overwritten (with the same key).
 * Does the automatic deallocation of overriden pointer.
 *
 * @param[in] hm       : Hashmap to be searched
 * @param[in] key_size : Size of the key to be provided for HashMapCalcHash
 * @param[in] key      : Key to be provided for HashMapCalcHash
 * @param[in] value    : Value to be associated with the given key
 * @returns Returns old value from under the provided key (may be NULL)
 */
static inline void HashMapSetDeep(HashMap* hm, const int key_size, HashMapKey key, HashMapData value) {
    if(key == NULL) return;;
    HashMapRemoveDeep(hm, key_size, key);
  
    const int hash = HashMapCalcHash(key_size, key);
    
    HashMapNode* currentNode = (HashMapNode*) ArrayListGetValueAt(&hm->nodes, hash);
    if(currentNode == NULL) {
        currentNode = HashMapCreateNode();
    }
    
    HashMapElement* element = HashMapCreateElement(key, value);
    ListPushBack(&(currentNode->values), element);
    
    ArrayListSetValueAt(&hm->nodes, hash, currentNode);
}

/**
 * Gets iterator to the next element in the HashMap.
 * 
 * @param[in] iter : Iterator to the element in HashMap
 * @returns Iterator to the next element (one after the one provided)
 */
static inline HashMapIterator HashMapNext(HashMapIterator iter) {
    if(iter.target == NULL) {
        return (HashMapIterator) {
            .target = NULL,
            .bucket = 0,
            .index = 0
        };
    } else {
        ++iter.index;
        while(1) {
            if(ArrayListSize(&(iter.target->nodes)) <= iter.bucket) {
                return iter;
            } else {
                HashMapNode* items = (HashMapNode*) ArrayListGetValueAt(&(iter.target->nodes), iter.bucket);
                List* itemsList = &(items->values);
                HashMapElement* element = (HashMapElement*) ListGetValueAt(itemsList, iter.index);
                if(element == NULL) {
                    ++iter.bucket;
                    iter.index = 0;
                } else {
                    return iter;
                }
            }
        }
    }
}

/**
 * Returns the iterator to the HashMap begining.
 * 
 * @param[in] hm : HashMap to be iterated
 * @returns Returns iterator to the first element
 */
static inline HashMapIterator HashMapBegin(HashMap* hm) {
    if(hm == NULL) {
        return (HashMapIterator) {
            .target = NULL,
            .bucket = 0,
            .index = 0
        };
    } else {
        HashMapIterator iter = {
            .target = hm,
            .bucket = 0,
            .index = -1
        };
        iter = HashMapNext(iter);
        return iter;
    }
}

/**
 * Check's if the iterator points to the end of the HashMap.
 * 
 * @param[in] iter : Iterator to the HashMap
 * @returns If the iterator points to the HashMap end?
 */
static inline int HashMapIsEnd(const HashMapIterator iter) {
    if(iter.target == NULL) return 1;
    return ArrayListGetValueAt(&(iter.target->nodes), iter.bucket) == NULL;
}

/**
 * Obtain value of the element from the iterator.
 * 
 * @param[in] iter : Iterator to the HashMap
 * @returns Returns value from under location pointed by iterator
 */
static inline HashMapData HashMapGetValue(const HashMapIterator iter) {
    if(iter.target == NULL) return NULL;
    HashMapNode* items = (HashMapNode*) ArrayListGetValueAt(&(iter.target->nodes), iter.bucket);
    if(items == NULL) return NULL;
    List* itemsList = &(items->values);
    HashMapElement* element = (HashMapElement*) ListGetValueAt(itemsList, iter.index);
    if(element == NULL) return NULL;
    return element->value;
}

/**
 * Obtain key of the element from the iterator.
 * 
 * @param[in] iter : Iterator to the HashMap
 * @returns Returns key from under location pointed by iterator
 */
static inline HashMapKey HashMapGetKey(const HashMapIterator iter) {
    if(iter.target == NULL) return NULL;
    HashMapNode* items = (HashMapNode*) ArrayListGetValueAt(&(iter.target->nodes), iter.bucket);
    if(items == NULL) return NULL;
    List* itemsList = &(items->values);
    HashMapElement* element = (HashMapElement*) ListGetValueAt(itemsList, iter.index);
    if(element == NULL) return NULL;
    return element->key;
}

#endif // __HASHMAP_H__