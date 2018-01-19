#ifndef __HASHMAP_H__
#define __HASHMAP_H__

#include "memalloc.h"
#include "array_lists.h"
#include "dynamic_lists.h"

#define HASH_MAP_FACT_P 350
#define HASH_MAP_FACT_Q 1000000009

#define HashMapSetV(HASHMAP, KEY_TYPE, VALUE_TYPE, KEY, VALUE) \
  do { \
      KEY_TYPE* __key_ptr__ = MALLOCATE(KEY_TYPE); \
      *__key_ptr__ = (KEY); \
      VALUE_TYPE* __val_ptr__ = MALLOCATE(VALUE_TYPE); \
      *__val_ptr__ = (VALUE); \
      HashMapSetDeep((HASHMAP), sizeof(KEY_TYPE), __key_ptr__, __val_ptr__); \
  } while(0)
  
#define HashMapGetV(HASHMAP, KEY_TYPE, VALUE_TYPE, KEY) \
  (((VALUE_TYPE*)HashMapGet((HASHMAP), sizeof(KEY_TYPE), &(KEY))))

#define HashMapHasV(HASHMAP,KEY_TYPE, VALUE_TYPE, KEY) \
  (HashMapHas((HASHMAP), sizeof(KEY_TYPE), &(K EY)))
  
#define HashMapRemoveV(HASHMAP, KEY_TYPE, VALUE_TYPE, KEY) \
  (HashMapRemoveDeep((HASHMAP), sizeof(KEY_TYPE), &(KEY)));
      
#define HashMapDestroyV(HASHMAP, KEY_TYPE, VALUE_TYPE) \
 (HashMapDestroyDeep((HASHMAP)))

#define LOOP_HASHMAP(HASHMAP, VAR_NAME) \
  for(HashMapIterator VAR_NAME = HashMapBegin(HASHMAP); \
  !HashMapIsEnd(VAR_NAME); VAR_NAME = HashMapNext(VAR_NAME))

typedef struct HashMapIterator HashMapIterator;
 
typedef void* HashMapData;

typedef void* HashMapKey;

typedef struct HashMapElement HashMapElement;

typedef struct HashMapNode HashMapNode;

typedef struct HashMap HashMap;

typedef int (*HashMapComparatorFn)(HashMapKey, HashMapKey);

struct HashMapElement {
    HashMapKey key;
    HashMapData value;
};

struct HashMapNode {
    List values;
};

struct HashMap {
    ArrayList nodes;
    HashMapComparatorFn cmp;
};

struct HashMapIterator {
    HashMap* target;
    int bucket;
    int index;
};

int HashMapIntCmp(void* a, void* b) {
    return *((int*)a) == *((int*)b);
}

int HashMapStrCmp(void* a, void* b) {
    return strcmp(((char*)a), ((char*)b)) == 0;
}

HashMapNode* HashMapCreateNode() {
    HashMapNode* node = MALLOCATE(HashMapNode);
    *node = (HashMapNode) {
        .values = ListNew()
    };
    return node;
}

HashMapElement* HashMapCreateElement(HashMapKey key, HashMapData value) {
    HashMapElement* element = MALLOCATE(HashMapElement);
    *element = (HashMapElement) {
        .key = key,
        .value = value
    };
    return element;
}

static inline HashMap HashMapNew(HashMapComparatorFn keyComparator) {
    HashMap hm = (HashMap) {
        .nodes = ArrayListNew(),
        .cmp = keyComparator
    };
    ArrayListResizeFill(&hm.nodes, 100);
    return hm;
}

static inline int HashMapCalcHash(const int key_size, HashMapKey key) {
    
    int hash = 0;
    char* ptr = (char*) key;
    for(int i=0;i<key_size;++i) {
        hash += ptr[i] * HASH_MAP_FACT_P;
        hash %= HASH_MAP_FACT_Q;
    }
    if(hash<0) hash*=-1;
    hash = 1000;
    
    //log_info(HASHMAP, "hash is %d", hash);
    
    return hash;
}

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

static inline int HashMapHas(HashMap* hm, const int key_size, HashMapKey key) {
    return HashMapGet(hm, key_size, key) != NULL;
}

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
                    free(element);
                    return ret;
                }
            }
        }
    }
    
}



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
                    free(element->value);
                    free(element->key);
                    ListDetachElement(&(currentNode->values), i);
                    free(element);
                    return;
                }
            }
        }
    }
    
}


static inline void HashMapDestroy(HashMap* hm) {
    LOOP_ARRAY_LIST(&(hm->nodes), i) {
        HashMapNode* currentNode = (HashMapNode*) ArrayListGetValue(i);
        LOOP_LIST(&(currentNode->values), j) {
            HashMapElement* element = (HashMapElement*) ListGetValue(j);
            if(element != NULL) {
                free(element);
            }
        }
        ListDestroy(&(currentNode->values));
        free(currentNode);
    }
    ArrayListDestroy(&(hm->nodes));
}

static inline void HashMapDestroyDeep(HashMap* hm) {
    LOOP_ARRAY_LIST(&(hm->nodes), i) {
        HashMapNode* currentNode = (HashMapNode*) ArrayListGetValue(i);
        LOOP_LIST(&(currentNode->values), j) {
            HashMapElement* element = (HashMapElement*) ListGetValue(j);
            if(element != NULL) {
                free(element->value);
                free(element->key);
                free(element);
            }
        }
        ListDestroy(&(currentNode->values));
        free(currentNode);
    }
    ArrayListDestroy(&(hm->nodes));
}

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

static inline int HashMapIsEnd(const HashMapIterator iter) {
    if(iter.target == NULL) return 1;
    return ArrayListGetValueAt(&(iter.target->nodes), iter.bucket) == NULL;
}

static inline HashMapData HashMapGetValue(const HashMapIterator iter) {
    if(iter.target == NULL) return NULL;
    HashMapNode* items = (HashMapNode*) ArrayListGetValueAt(&(iter.target->nodes), iter.bucket);
    if(items == NULL) return NULL;
    List* itemsList = &(items->values);
    HashMapElement* element = (HashMapElement*) ListGetValueAt(itemsList, iter.index);
    if(element == NULL) return NULL;
    return element->value;
}

#endif // __HASHMAP_H__