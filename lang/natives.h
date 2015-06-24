#ifndef __NATIVES__
#define __NATIVES__

/* ========================================================================
     NATIVES
   ======================================================================== */

typedef err_t (*native_fn)(int, PObject *, PObject **, PObject **);


/* args: 16 bits   [0..6] argmin [7] blocking [8..15] argmax */
typedef struct _nativefnfo {
	const native_fn fn;
	const uint8_t argmin;
	const uint8_t argmax;
	const uint16_t name;
} NativeFn, NativeMth;

typedef err_t (*binary_op_fn)(uint32_t, PObject *, PObject *b, PObject **);
typedef err_t (*subscr_fn)(PObject *, PObject *, PObject *, PObject **);
typedef err_t (*attr_fn)(PObject *, uint16_t, PObject *, PObject **);

typedef struct _typeinfo {
	const binary_op_fn bop;
	const attr_fn attr;
	const subscr_fn subscr;
	const uint16_t nmth;
	const NativeMth *mths;
} Type;

//#define NATIVE_TAG ((PNATIVE<<2)|0x2)
#define NATIVE_TAG ((5<<2)|0x2)
#define PNATIVE_METHOD_MAKE(idx,tt)  ((PObject*)((uint32_t)(((idx)<<16)|((tt)<<8)|NATIVE_TAG)))
#define PNATIVE_MAKE(idx)  PNATIVE_METHOD_MAKE(idx,NTYPES)
#define PNATIVE_CODE(o)  ((((uint32_t)(o))&0x00ff0000)>>16)
#define PNATIVE_TYPE(o)  ((((uint32_t)(o))&0x0000ff00)>>8)
#define PNATIVE_IS_BLOCKING(fn) ((fn)->argmin&0x80)
#define PNATIVE_ARGMIN(fn) ((fn)->argmin&0x7f)
#define PNATIVE_ARGMAX(fn) ((fn)->argmax)
#define PNATIVE_NAME(fn) ((fn)->name)
#define PNATIVE_IS_METHOD(fn) ((fn)->name)


#define CNATIVE_GET(n) ((native_fn)(_cnative_table[n]))


/* ========================================================================
     NATIVE TABLES & UTILS
   ======================================================================== */

extern const NativeFn const native_fns[];
extern const Type const _types[];


/* macros for native defs */
#define CHECK_ARG(x,t) if( PTYPE(x)!=(t) ) return ERR_TYPE_EXC
#define CHECK_NARGS(x,Mx,mx) if((x)<(mx) || (x)>(Mx)) return ERR_TYPE_EXC

#define NATIVE_NAME(name)  ntv__##name
#define NATIVE_FN(name) err_t NATIVE_NAME(name)(int nargs, PObject *self, PObject **args, PObject **res)
#define NATIVE_UNWARN() (void)nargs;(void)self;(void)args;(void)res
#define NATIVE_FN_CALL(name,nargs,self,args,res) NATIVE_NAME(name)(nargs,self,args,res)

#define NATIVE_BOP_FN(name) err_t name(uint32_t, PObject *, PObject *b, PObject **)
#define NATIVE_ATTR_FN(name) err_t name(PObject *, uint16_t, PObject *, PObject **)
#define NATIVE_SUBSCR_FN(name) err_t name(PObject *, PObject *, PObject *, PObject **)


#define _NMTH(t) (sizeof(t##_mths)/sizeof(NativeMth)),(t##_mths)
#define BUILD_NATIVE(fn,argmax,argmin,blocking) {(native_fn)NATIVE_NAME(fn),argmin|((blocking)?0x80:0x00),argmax,0}
#define BUILD_NATIVE_MTH(fn,argmax,argmin,blocking,name) {(native_fn)NATIVE_NAME(fn),argmin|((blocking)?0x80:0x0),argmax,name}

int find_native_mth(const NativeMth *table, uint16_t name, int size);
PObject *get_native_mth(PObject *self, uint16_t name);

#endif