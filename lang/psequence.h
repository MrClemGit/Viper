#ifndef __PSEQUENCE__
#define __PSEQUENCE__

#include "pobj.h"


typedef struct _pbuffer {
    PObjectHeader header;
    void *buffer[];
} PBuffer;

#define PBUFFER_SIZE(b) ((( (PObject*)(b))->header.size-sizeof(PObjectHeader))/PFLAG_OSIZE(b))
#define POBJ_BUFFER(b) ((PObject**)((b)->buffer))

typedef struct _psequence {
    PObjectHeader header;
    uint16_t elements;
    uint16_t unused;
} PSequence, PContainer;

typedef struct _pisequence {
    PObjectHeader header;
    uint16_t elements;
    uint16_t unused;
    void *seq[];
} PImmutableSequence, PTuple, PString, PBytes,PShorts;

typedef struct _pmsequence {
    PObjectHeader header;
    uint16_t elements;
    uint16_t unused;
    PBuffer *seq;
} PMutableSequence, PByteArray, PShortArray, PList;

typedef struct _prange {
    PObjectHeader header;
    uint16_t elements;
    uint16_t unused;
    INT_TYPE start;
    INT_TYPE step;
    INT_TYPE stop;
} PRange;


typedef struct _piterator {
    PObjectHeader header;
    uint16_t current;
    int16_t temp;
    PObject *iterable;
} PIterator;

typedef struct _pslice {
    PObjectHeader header;
    PObject *start;
    PObject *stop;
    PObject *step;
} PSlice;



#define IS_TYPE_SEQ_MUTABLE(type) ((type)==PBYTEARRAY || (type)==PLIST || (type)==PSHORTARRAY)
#define MAKE_SEQ_FLAGS(mut,b4_obj) (((mut)<<4)|(b4_obj))
#define IS_SEQ_MUTABLE(seq) (((PSequence*)seq)->header.flags&0x10)
#define SEQ_OBJ_SIZE(seq) ((((PSequence*)seq)->header.flags&0x0f))
#define IS_PSEQUENCE(seq) ((PTYPE(seq)>=PSTRING)&&(PTYPE(seq)<=PSEQUENCE))
#define IS_PSEQUENCE_TYPE(t) ((t>=PSTRING)&&(t<=PSEQUENCE))
#define IS_BYTE_PSEQUENCE_TYPE(t) ((t)>=PSTRING && (t)<=PBYTEARRAY)
#define IS_SHORT_PSEQUENCE_TYPE(t)((t)>=PSHORT && (t)<=PSHORTARRAY)
#define IS_OBJ_PSEQUENCE_TYPE(t) ((t)>=PLIST && (t)<=PTUPLE)
#define IS_PSLICE(o) (PTYPE(o)==PSLICE)


#define PCONTAINER_ELEMENTS(o) ((PContainer*)(o))->elements


#define POBJ_ISEQ(sq) ((PObject **)((PImmutableSequence*)(sq))->seq)

#define _PMS(o) ((PMutableSequence*)(o))
#define _PIS(o) ((PImmutableSequence*)(o))
#define PSEQUENCE_ELEMENTS(o) (((PSequence*)(o))->elements)
#define PSEQUENCE_BYTES(o) ( (IS_SEQ_MUTABLE(o)) ? ( (_PMS(o)->seq) ? ((uint8_t*)_PMS(o)->seq->buffer):(NULL)  ):((uint8_t*)_PIS(o)->seq))
#define PSEQUENCE_SHORTS(o) (uint16_t*)(PSEQUENCE_BYTES(o))
#define _PIS_BYTES(o) ((uint8_t*)(_PIS(o))->seq)
#define _PMS_BYTES(o) ((uint8_t*)(_PMS(o))->seq->buffer)
#define PSEQUENCE_OBJECTS(o) ((PObject**)PSEQUENCE_BYTES(o))
#define PSEQUENCE_SIZE(o) ((IS_SEQ_MUTABLE(o)) ? (_PMS(o)->seq ? PBUFFER_SIZE(_PMS(o)->seq):0) :_PIS(o)->elements)
#define PSEQUENCE_BUFFER(o) ((IS_SEQ_MUTABLE(o)) ? ((PMutableSequence*)(o))->seq:NULL)

#define _PMS_SET_BYTE(o,i,v)  (((uint8_t*)(_PMS(o))->seq->buffer)[i]=(uint8_t)(v))
#define _PMS_SET_SHORT(o,i,v)  (((uint16_t*)(_PMS(o))->seq->buffer)[i]=(uint16_t)(v))
#define _PMS_SET_OBJECT(o,i,v) (_PMS(o)->seq->buffer[i] = (void*)(v))

#define _PIS_SET_BYTE(o,i,v) (((uint8_t*)(_PIS(o))->seq)[i]=(uint8_t)(v))
#define _PIS_SET_OBJECT(o,i,v) (_PIS(o)->seq[i] = (void*)(v))


#define PLIST_SET_ITEM(lst,i,item) ((PObject**)((PList*)lst)->seq->buffer)[i]=(PObject*)(item)
#define PTUPLE_SET_ITEM(tpl,i,item)  (((PTuple*)tpl)->seq[i]=(void*)(item))
#define PTUPLE_OBJECTS(tpl) ((PObject**)(((PTuple*)(tpl))->seq))
//((PObject**)((PTuple*)(tpl))->seq)[i]=(PObject*)(item)

#define PLIST_ITEM(lst,i) (( (PObject**)((PList*)lst)->seq->buffer)[i])
#define PTUPLE_ITEM(tpl,i) ((PObject*)(((PTuple*)tpl)->seq[i]))
//((PObject*)(((PTuple*)tpl)->seq+((i)*sizeof(PObject*))))
//(((PObject**)(((PTuple*)tpl)->seq))[i])


#define PSHORTS_SET_ITEM(tpl,i,item)   (((uint16_t*)((PShorts*)tpl)->seq)[i]=(uint16_t)item)


#define PSLICE_START(o) (((PSlice*)(o))->start)
#define PSLICE_STOP(o) (((PSlice*)(o))->stop)
#define PSLICE_STEP(o) (((PSlice*)(o))->step)


#define PRANGE_START(o) (((PRange*)(o))->start)
#define PRANGE_STOP(o) (((PRange*)(o))->stop)
#define PRANGE_STEP(o) (((PRange*)(o))->step)

#define PITERATOR_NORMAL    0
#define PITERATOR_ENUMERATE 1
#define PITERATOR_KEYS      2
#define PITERATOR_VALUES    3
#define PITERATOR_ITEMS     4
#define PITERATOR_REVERSED  5
#define PITERATOR_SET_TYPE(p,tp) ((p)->header.flags|=(tp))
#define PITERATOR_GET_TYPE(p) ((p)->header.flags&0xf)



PBuffer *pbuffer_new(uint16_t elements, uint8_t typesize);

PSequence *psequence_new(uint8_t type, uint16_t elements);
PRange *prange_new(INT_TYPE start, INT_TYPE end, INT_TYPE step);
PIterator *piterator_new(PObject *seq);
PObject *piterator_next(PIterator *p);
PString *pstring_new(uint16_t len, uint8_t *buf);
PList *plist_new(uint16_t len, PObject **buf);
PTuple *ptuple_new(uint16_t len, PObject **buf);
PSlice *pslice_new(PObject *start, PObject *stop, PObject *step);
PBytes *pbytes_new(uint16_t len, uint8_t *buf);
PBytes *pshorts_new(uint16_t len, uint16_t *buf);



err_t psequence_compare(uint32_t op, PSequence *a, PSequence *b, PObject **res);
err_t psequence_mul(PObject *n, PSequence *seq, PSequence **res, uint8_t);
err_t psequence_concat(PSequence *a, PSequence *b, PSequence **res, uint8_t);
int psequence_contains(PSequence *a, PObject *b);
err_t psequence_setitem(PSequence *a, PObject *pos, PObject *b);
err_t psequence_getitem(PSequence *a, PObject *pos, PObject **res);
err_t psequence_getslice(PSequence *a, PSlice *s, PObject **res);
err_t psequence_setslice(PSequence *a, PSlice *s, PObject *itr);
err_t psequence_binary_op(uint32_t op, PObject *a, PObject *b, PObject **res);
err_t psequence_subscr(PObject *a, PObject *idx, PObject *b, PObject **res);

err_t plist_append(PObject *l, PObject *x);

PMutableSequence *psequence_grow(PMutableSequence *seq, uint16_t size);


#define PSEQUENCE_INDEX_NORMALIZE(seq,i)  \
    do { \
        if((i)<0){ \
            (i)+=(seq)->elements; \
            if((i)<0) (i)=0; \
        }else if((i)>(seq)->elements) \
            (i)=(seq)->elements; \
    } while(0)


/*

              0   1   2   3   4   5   6   7   8   9              elements = 10

                                                            results
[::]          b--------------------------------------e         0,1,...9    (10)
[::-1]        e--------------------------------------b         9,8,...0    (10)
[:0:-1]       e--------------------------------------b         9,8,...1    (9)
[2:7:2]              bo-------o-------o---e                    2,4,6       (3)
[7:2:-2]             e----o-------o------ob                    7,5,3       (3)


                                                            indexes after PREPARE_INDEXES
[::]          b--------------------------------------e         0:10:1
[::-1]     e--------------------------------------b            9:-11:-1
[:0:-1]       e-----------------------------------b            9:0:-1
[2:7:2]              bo-------o-------o---e                    2:7:2
[7:2:-2]             e----o-------o------ob                    7:2:-2


                                                            indexes after NORMALIZE_INDEXES
[0:10:1]      b--------------------------------------e         0:10:1         10-0 = 10
[9:-11:-1]  e-------------------------------------b            9:-1:-1        -1-9 = -10
[9:0:-1]      e-----------------------------------b            9:0:-1         0-9 = -9
[2:7:2]              bo-------o-------o---e                    2:7:2          7-2+1 = 6/2 = 3
[7:2:-2]             e----o-------o------ob                    7:2:-2         2-7-1 = -6/-2 = 3

[7::-2]   e-------o-------o-------o------0b   [7:-1:-2]     -1-7-1 -9/-2 = 5

                                                            indexes in loops
                                                               [0,10[
                                                               [9,-1[
                                                               [9,0[
                                                               [2,7[
                                                               [7,2[

[100:-100:-1]   ->  [5:-1:-1]  ->  -1-5 = -6 []
*/

#define PSLICE_PREPARE_INDEXES(seq,slice,a,b,s)\
    do {   \
        if ((slice)->step == P_NONE || (slice)->step==P_ZERO) (s) = 1; \
        else if (!IS_INTEGER( (slice)->step)) {\
            return ERR_TYPE_EXC;\
        } else (s) = INTEGER_VALUE( (slice)->step);\
        if ((slice)->start == P_NONE) (a) = ((s)<0) ? ((seq)->elements-1):(0); \
        else if (!IS_INTEGER( (slice)->start)) {\
            return ERR_TYPE_EXC;\
        }else (a) = INTEGER_VALUE( (slice)->start);\
        if ((slice)->stop == P_NONE) (b) = ( (s)<0)? (-(seq)->elements-1):(seq)->elements; \
        else if (!IS_INTEGER( (slice)->stop)) {\
            return ERR_TYPE_EXC;\
        } else (b) = INTEGER_VALUE( (slice)->stop);\
    } while (0)



#define PSLICE_INDEX_NORMALIZE(seq,start,stop,step,ssize) \
    do{\
        if((start)<0){ (start)+=(seq)->elements; if((start)<0) (start)=0;} \
        else if((start)>=(seq)->elements) (start) = ((step)<0) ? ((seq)->elements-1):(seq)->elements;\
        \
        if((stop)<0){ (stop)+=(seq)->elements; if((stop)<0) (stop)=((step)<0)?-1:0;}\
        else if((stop)>(seq)->elements) (stop) = (seq)->elements;\
        \
        if((step)==1 || (step)==-1)\
            (ssize)=((stop)-(start))/(step);\
        else\
            (ssize) = ((stop) - (start) + (((step) < 0) ? -1 : 1) ) / (step); \
        if((ssize)<0) (ssize)=0;\
    } while (0)

#define PSEQUENCE_INDEX_CHECK_BOUNDS(seq,i) if ((i)<0 || (i)>=(seq)->elements) return ERR_INDEX_EXC

#endif