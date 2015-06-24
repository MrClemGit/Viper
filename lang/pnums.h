#ifndef __PNUMS__
#define __PNUMS__

#include "pobj.h"




typedef struct _p_integer {
    PObjectHeader header;
    INT_TYPE val;
} PInteger;

typedef struct _p_float {
    PObjectHeader header;
    FLOAT_TYPE val;
} PFloat;


typedef struct _p_complex {
    PObjectHeader header;
    FLOAT_TYPE re;
    FLOAT_TYPE im;
} PComplex;

typedef PObject *PSmallInt;

typedef PObject *PBool;


#define PSMALLINT_MASK 0x1
#define PSMALLINT_VALUE(o) (((int32_t)(o))>>1)
#define PSMALLINT_NEW(o) (PObject*)((((int32_t)o)<<1)|PSMALLINT_MASK)
#define IS_PSMALLINT(o) (((uint32_t)(o))&PSMALLINT_MASK)

#define PSMALLINT_SLICE_MARKER ((PObject*)(0x0fffffff))
#define PSMALLINT_SLICE_MARKER_VALUE (0x0fffffff>>1)
#define PSMALLINT_ZERO ((PObject*)(0x1))

#define PSMALLINT_OVERFLOWED(o) (!((((o)&0xC0000000)==0)||(((o)&0xC0000000)==0xC0000000)))

#define IS_NUMBER_TYPE(x)  ((x)<=PNUMBER)
#define IS_PNUMBER(o) ((PTYPE(o))<=PNUMBER)
#define IS_PNUMBER_TYPE(o) ((o)<=PNUMBER)
#define IS_INTEGER(o) ((PTYPE(o))<=PINTEGER)
#define INTEGER_VALUE(o) ((IS_PSMALLINT(o)) ? (PSMALLINT_VALUE(o)): (((PInteger*)(o))->val))
#define FLOAT_VALUE(o) ((PFloat*)(o))->val
#define COMPLEX_RE(o) ((PComplex*)(o))->re
#define COMPLEX_IM(o) ((PComplex*)(o))->im


PFloat *pfloat_new(FLOAT_TYPE n);
PInteger *pinteger_new(INT_TYPE n);
PComplex *pcomplex_new(FLOAT_TYPE re, FLOAT_TYPE im);
void pnumber_destroy(PObject *number);

err_t pnumber_binary_op(uint32_t op, PObject *a, PObject *b, PObject **res);
err_t pnumber_unary_op(uint32_t op, PObject *a, PObject **res);


#endif