#ifndef __HASHMAP_H__
#define __HASHMAP_H__

#include "utils.h"

#include "memalloc.h"
#include "array_lists.h"
#include "dynamic_lists.h"

#define HashMapSetV(HASHMAP, KEY_TYPE, VALUE_TYPE, KEY, VALUE) \
  do { \
      KEY_TYPE* __key_ptr__ = MALLOCATE(KEY_TYPE); \
      *__key_ptr__ = (KEY); \
      VALUE_TYPE* __val_ptr__ = MALLOCATE(VALUE_TYPE); \
      *__val_ptr__ = (VALUE); \
      HashMapSet((HASHMAP), __key_ptr__, __val_ptr__); \
  } while(0)
  
#define HashMapGetV(HASHMAP,KEY_TYPE, VALUE_TYPE, KEY) \
  (*((VALUE_TYPE*)HashMapGet((HASHMAP), &(KEY))))

#define HashMapHasV(HASHMAP,KEY_TYPE, VALUE_TYPE, KEY) \
  (HashMapHas((HASHMAP), &(KEY)))
  
#define HashMapRemoveV(HASHMAP, KEY_TYPE, VALUE_TYPE, KEY) \
  (HashMapRemoveDeep((HASHMAP), &(KEY)));
      
#define HashMapDestroyV(HASHMAP, KEY_TYPE, VALUE_TYPE) \
 (HashMapDestroyDeep((HASHMAP)))
  
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

static inline void HashMapSet(HashMap hm, HashMapKey key, HashMapData value) {
    if(key == NULL) return;
    
    const int hash = 42;
    
    HashMapNode* currentNode = (HashMapNode*) ArrayListGetValueAt(&hm.nodes, hash);
    if(currentNode == NULL) {
        currentNode = HashMapCreateNode();
    }
    
    HashMapElement* element = HashMapCreateElement(key, value);
    ListPushBack(&(currentNode->values), element);
    
    ArrayListSetValueAt(&hm.nodes, hash, currentNode);
}

static inline HashMapData HashMapGet(HashMap hm, HashMapKey key) {
    if(key == NULL) return NULL;
    
    const int hash = 42;
    
    HashMapNode* currentNode = (HashMapNode*) ArrayListGetValueAt(&hm.nodes, hash);
    if(currentNode == NULL) {
        return NULL;
    }
    
    LOOP_LIST(&(currentNode->values), i) {
        HashMapElement* element = (HashMapElement*) ListGetValue(i);
        if(element != NULL) {
            if(element->key != NULL) {
                if(hm.cmp(element->key, key)) {
                    return element->value;
                }
            }
        }
    }
    
    return NULL;
}

static inline HashMapData HashMapHas(HashMap hm, HashMapKey key) {
    return HashMapGet(hm, key) != NULL;
}

static inline HashMapData HashMapRemove(HashMap hm, HashMapKey key) {
    if(key == NULL) return NULL;
    
    const int hash = 42;
    
    HashMapNode* currentNode = (HashMapNode*) ArrayListGetValueAt(&hm.nodes, hash);
    if(currentNode == NULL) {
        return NULL;
    }
    
    LOOP_LIST(&(currentNode->values), i) {
        HashMapElement* element = (HashMapElement*) ListGetValue(i);
        if(element != NULL) {
            if(element->key != NULL) {
                if(hm.cmp(element->key, key)) {
                    HashMapData ret = element->value;
                    ListDetachElement(&(currentNode->values), i);
                    free(element);
                    return ret;
                }
            }
        }
    }
    
}

static inline void HashMapRemoveDeep(HashMap hm, HashMapKey key) {
    if(key == NULL) return;
    
    const int hash = 42;
    
    HashMapNode* currentNode = (HashMapNode*) ArrayListGetValueAt(&hm.nodes, hash);
    if(currentNode == NULL) {
        return;
    }
    
    LOOP_LIST(&(currentNode->values), i) {
        HashMapElement* element = (HashMapElement*) ListGetValue(i);
        if(element != NULL) {
            if(element->key != NULL) {
                if(hm.cmp(element->key, key)) {
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

static inline void HashMapDestroy(HashMap hm) {
    LOOP_ARRAY_LIST(&(hm.nodes), i) {
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
    ArrayListDestroy(&(hm.nodes));
}

static inline void HashMapDestroyDeep(HashMap hm) {
    LOOP_ARRAY_LIST(&(hm.nodes), i) {
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
    ArrayListDestroy(&(hm.nodes));
}

#endif // __HASHMAP_H__