#ifndef __PHASH__
#define __PHASH__

#include "psequence.h"
#include "natives.h"


typedef struct _setentry {
    PObject *key;
    uint16_t next;
    uint16_t index;
} SetEntry;

typedef struct _dictentry {
    PObject *key;
    uint16_t next;
    uint16_t index;
    PObject *value;
} DictEntry, HashEntry;


typedef struct _pdict {
    PObjectHeader header;
    uint16_t     elements;
    uint16_t     size;
    PBuffer     *entry;
} PDict, PSet, PFSet, PHash;




#define IS_SET_TYPE(tt)((tt)>=PFSET&&(tt)<=PSET)
#define IS_MAP_TYPE(tt) ((tt)>=PFSET&&(tt)<=PDICT)
#define PMAP_ELEMENTS(o) ((PContainer*)(o))->elements

#define PDICT_ELEMENTS(o)  ((PDict*)(o))->elements
#define PDICT_SIZE(o)  ((PDict*)(o))->size
#define PDICT_BUFFER(o)  ((PDict*)(o))->entry
#define PDICT_ENTRY(o) ((DictEntry*)((PDict*)(o))->entry->buffer)
#define PDICT_KEY(o,i)  PDICT_ENTRY(o)[PDICT_ENTRY(o)[i].index].key
#define PDICT_VAL(o,i)  PDICT_ENTRY(o)[PDICT_ENTRY(o)[i].index].value
#define PDICT_IDX(o,i)  PDICT_ENTRY(o)[i].index
#define IS_PDICT(o) (PTYPE(o)==PDICT)

#define PHASH_ESIZE(o) (((PObject*)(o))->header.flags&0x1f)

#define PHASH_ELEMENTS(o)  ((PHash*)(o))->elements
#define PHASH_SIZE(o)  ((PHash*)(o))->size
#define PHASH_BYTES(o) ((uint8_t*)(((PHash*)(o))->entry->buffer))
#define PHASH_BYTES_SIZE(o) (PHASH_SIZE(o)*PHASH_ESIZE(o))


#define PSET_ELEMENTS(o)  ((PSet*)(o))->elements
#define PSET_SIZE(o)  ((PSet*)(o))->size
#define PSET_BYTES(o) ((uint8_t*)(((PSet*)(o))->entry->buffer))
#define PSET_BYTES_SIZE(o) (PSET_SIZE(o)*sizeof(SetEntry))



#define PHASH_ELEMENTS(o)  ((PHash*)(o))->elements


PDict *pdict_new(int size);
PSet *pset_new(int type, int size);

//int pdict_put(PDict *f, PObject *key, PObject *value);
//PObject *pdict_get(PDict *f, PObject *key);
//PObject *pdict_del(PDict *f, PObject *key);

int phash_put(PDict *f, PObject *key, PObject *val);
PObject *phash_get(PDict *f, PObject *key);
HashEntry *phash_getentry(PDict *f, int n);
PObject *phash_del(PDict *f, PObject *key);
PHash *phash_clone(PHash *src);
#if VM_DEBUG
void phash_print(PHash *);
#else
#define phash_print(f)
#endif

#define pdict_put(f,k,v) phash_put(f,k,v)
#define pset_put(f,k) phash_put(f,k,NULL)
#define pdict_get(f,k) phash_get(f,k)
#define pset_get(f,k) phash_get(f,k)
#define pdict_del(f,k) phash_del(f,k)
#define pset_del(f,k) phash_del(f,k)
#define pdict_getentry(f,n) phash_getentry(f,n)
#define pset_getentry(f,n) phash_getentry(f,n)
#define pdict_print(f) phash_print(f)
#define pset_print(f) phash_print(f)


NATIVE_FN(__set_difference);
NATIVE_FN(__set_intersection);
NATIVE_FN(__set_isdisjoint);
NATIVE_FN(__set_issubset);
NATIVE_FN(__set_issuperset);
NATIVE_FN(__set_symmetric_difference);
NATIVE_FN(__set_union);
NATIVE_FN(__set_pop);
NATIVE_FN(__set_remove);
NATIVE_FN(__hash_clear);
NATIVE_FN(__hash_copy);
NATIVE_FN(__set_update);
NATIVE_FN(__set_intersection_update);
NATIVE_FN(__set_difference_update);
NATIVE_FN(__set_symmetric_difference_update);
NATIVE_FN(__set_add);
NATIVE_FN(__set_discard);
NATIVE_FN(__dict_pop);
NATIVE_FN(__dict_get);
NATIVE_FN(__dict_keys);
NATIVE_FN(__dict_items);
NATIVE_FN(__dict_values);
NATIVE_FN(__dict_update);
NATIVE_FN(__dict_popitem);


NATIVE_BOP_FN(pset_binary_op);
NATIVE_BOP_FN(pdict_binary_op);

NATIVE_SUBSCR_FN(pdict_subscr_fn);


typedef union _minitable {
    struct _entry {
        uint16_t name;
        uint8_t next;
        uint8_t pos;
    } entry;
    uint32_t size;
} MiniTable;

int minitable_lookup(MiniTable *table, Name name);

#define MINITABLE_BYTES(table) (((table)->size+1)*sizeof(MiniTable))
#define MINITABLE_NO_ENTRY INT_TYPE_MIN

#endif