#ifndef __PFUNCTION__
#define __PFUNCTION__


#include "pobj.h"
#include "pcode.h"


typedef struct _pcallable {
    PObjectHeader header;
    uint8_t defargs;
    uint8_t defkwargs;
    uint16_t codeobj;
    uint16_t module;
    uint8_t codetype;
    uint8_t unused;
} PCallable;

typedef struct _pfunction {
    PObjectHeader header;
    uint8_t defargs;
    uint8_t defkwargs;
    uint16_t codeobj;
    uint16_t module;
    uint8_t codetype;
    uint8_t unused;
    PObject *storage[];
} PFunction;


typedef struct _pmethod {
    PObjectHeader header;
    PCallable *fn;
    PObject *self;
} PMethod;


typedef struct _arginfo {
    uint8_t defargs;            /* default positional args */
    uint8_t kwdefargs;          /* default keyword args */
    uint8_t nargs;              /* positional args in call */
    uint8_t nkwargs;            /* keyword args in call */
} ArgInfo;

typedef struct _argsource {
    PObject **args;             /*list of positional arguments passed in call */
    PObject **kwargs;           /*list of keyword arguments passed in call: it's (arg,name) */
    PObject *vargs;             /*vararg or NULL */
    PObject **defstore;         /*list of default arguments: first positional, then keywords */
    PObject *self;
} ArgSource;


#define PFLAG_CNATIVE   0x80
#define PFUNCTION_IS_CNATIVE(o)  (((PObject*)(o))->header.flags&PFLAG_CNATIVE)


PFunction *pfunction_new(uint8_t defargs, uint8_t defkwargs, int32_t ncodeobj);
PMethod *pmethod_new(PCallable *fn, PObject *self);


err_t pcallable_set_args(PCode *code, ArgInfo *arginfo, ArgSource *src, PObject **locals);

#define SET_DEFARG(fn,x,arg) ((fn)->storage[x]=(arg))
#define GET_DEFARG(fn,x)     ((fn)->storage[x])

#define SET_KWDEFARG(fn,x,arg) ((fn)->storage[2*(x)+1+((fn)->defargs)]=(PObject*)(arg))
#define SET_KWDEFARGNAME(fn,x,arg) ((fn)->storage[2*(x)+((fn)->defargs)]=(PObject*)(arg))


#define SET_DEFARG_EX(storage,x,arg) ((storage)[x]=(arg))
#define SET_KWDEFARG_EX(storage,defargs,x,arg) ((storage)[2*(x)+1+(defargs)]=(PObject*)(arg))
#define SET_KWDEFARGNAME_EX(storage,defargs,x,arg) ((storage)[2*(x)+(defargs)]=(PObject*)(arg))


#define GET_KWDEFARG(fn,x) ((fn)->storage[2*(x)+1+((fn)->defargs)])


#define GET_FREEVAR_START(fn) (((PFunction*)(fn))->storage+((PFunction*)(fn))->defargs+((PFunction*)(fn))->kwdefargs*2)

//#define PFUNCTION_HAS_CLOSURE(f) (PCODE_MAKE((f)->codeobj)->nfree)
#define PFUNCTION_GET_CLOSURE(f) (PTuple*)((f)->storage[(f)->defargs+(f)->defkwargs*2])
#define PFUNCTION_SET_CLOSURE(f,closure) ((f)->storage[(f)->defargs+(f)->defkwargs*2]=(closure))
        
/*====== C FUNCTION CALL============================================ */

int parse_py_args(const char *fmt, int nargs, PObject **args, ...);
int parse_py_arg(PObject **var, int ptype, int *nargs, PObject *** args, PObject *defval);

#define PARSE_PY_ARG(var,ptype,defval,nargs,args,err) err = parse_py_arg(&(var),ptype,&(nargs),&(args),defval)

#define PARSE_PY_ARGCODE(code) \
    if (nargs>0 && (PTYPE(args[0])==PSMALLINT)){    \
        code = PSMALLINT_VALUE(args[0]);            \
        nargs--; args++;                            \
    } else                                          \
        code = -1

#define PARSE_PY_INT(var) \
    if (nargs>0 && (PTYPE(args[0])==PSMALLINT)){    \
        var = PSMALLINT_VALUE(args[0]);            \
        nargs--; args++;                            \
    } else                                          \
        var = -1



#endif