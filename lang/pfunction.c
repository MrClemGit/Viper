#include "lang.h"

PFunction *pfunction_new(uint8_t defargs, uint8_t defkwargs, int32_t ncodeobj) {
    int ssize = sizeof(PObject *) * (defargs + 2 * defkwargs);
    PCode *code = PCODE_MAKE(ncodeobj);
    if (PCODE_CELLVARS(code)) {
        ssize += sizeof(PObject *);
    }
    PFunction *fn = ALLOC_OBJ(PFunction, PFUNCTION, PFLAG_HASHABLE, ssize);
    fn->codeobj = ncodeobj;
    fn->defargs = defargs;
    fn->defkwargs = defkwargs;
    //if present, closure is null because of gc zeroing
    return fn;
}


PMethod *pmethod_new(PCallable *fn, PObject *self) {
    PMethod *res = ALLOC_OBJ(PMethod, PMETHOD, PFLAG_HASHABLE, 0);
    res->fn = fn;
    res->self = self;
    return res;
}


/*
    set arguments to the new frame locals when a callable is called.

    The code obj referenced by the callable contains the necessary number of arguments: args, kwargs and varargs
    It also contains a MiniTable associating a kwarg name to its position in the locals of the code
    Following the MiniTable there is the list of positional arg positions in locals. Last one is vararg if present

    |-----------------------------------------||------------------------------------|
         MiniTable(kwname)=kpos ----|               argpos---------------------
                                    |                                         |
                                   \|/                                       \|/
    |----||----||----||----||----||----||----||----||----||----||----||----||----||----|
*/

err_t pcallable_set_args(PCode *code, ArgInfo *arginfo, ArgSource *src, PObject **locals) {
    //uint8_t nnames = code->args + code->kwargs + code->vararg;
    uint8_t obargs = code->args - arginfo->defargs;
    MiniTable *mtable;
    PTuple *freevars = PFRAME_TFREEVARS_FROM_LOCALS(locals, code); //(PTuple*)*(locals + code->nlocals);
    if (code->codetype == CODETYPE_VIPER) {
        mtable = (MiniTable *)( ViperCode(code)->bytecode + ViperCode(code)->nmstart);
    } else {
        //cnative
        mtable = (MiniTable *)(CNativeCode(code)->minitable);
    }
    int8_t *argpos = ((int8_t *)( ((uint8_t *)mtable) + MINITABLE_BYTES(mtable)));
    int argset = 0;
    int i = 0, j, pos;
    int nvargs = 0; //number of vargs in CALL_VAR
    int unvargs = 0; //number of used vargs
    PObject **nvarglist = NULL;
    if (src->vargs) {
        nvargs = PSEQUENCE_ELEMENTS(src->vargs);
        nvarglist = PSEQUENCE_OBJECTS(src->vargs);
    }

    debug( "+++preparing call\r\n");
    debug( "called with %i arguments and %i kwargs and %i unpackable vargs\r\n", arginfo->nargs, arginfo->nkwargs, nvargs);
    debug( "default arguments are %i, kwonly defaults are %i\r\n", arginfo->defargs, arginfo->kwdefargs);
    debug( "expected arguments are %i, expected kwonly are %i\r\n", code->args, code->kwargs);
    debug( "expected varargs %i, passed vararg %i. Has self %i\r\n", code->vararg, src->vargs != NULL, src->self != NULL);
    debug( "needed positional arguments are %i - %i = %i / %i\r\n", code->args, arginfo->defargs, obargs, arginfo->nargs + nvargs);
    debug( "needed kwonly arguments are %i - %i = %i / %i\r\n", code->kwargs, arginfo->kwdefargs,
            code->kwargs - arginfo->kwdefargs, arginfo->nkwargs);
    debug( "defstore @%i\r\n", src->defstore);
    debug( "----\r\n");


    j = (src->self) ? 1 : 0;
    /* not enough positional aguments or too many keyword args */
    if ((arginfo->nargs + nvargs +j < obargs) /* || (arginfo->nkwargs > code->kwargs)*/ ) {
        debug( "wrong! not enough positional arguments");
        return ERR_TYPE_EXC;
    }


    /* set keyword args and eventually defaults */
    uint8_t kwguard = 0;
    for (j = 1; j >= 0; j--) {
        PObject **kbuf = (j == 0) ? (src->defstore + arginfo->defargs) : src->kwargs;
        kwguard = (j == 0) ? (arginfo->kwdefargs) : arginfo->nkwargs;
        debug( "pass %i:\r\n", j);
        for (i = 0; i < kwguard; i++) {
            PObject *karg = kbuf[2 * i + 1];
            uint16_t kname = (uint16_t) PSMALLINT_VALUE(kbuf[2 * i]);
            pos = minitable_lookup(mtable, kname);
            debug( "processing name %i=>%x at %i\r\n", kname, karg, pos);
            if (pos == MINITABLE_NO_ENTRY) {
                /* wrong keyword argument passed */
                debug( "wrong! wrong keyword argument passed %i %i\r\n", j, kname);
                return ERR_TYPE_EXC;
            }
            /* if not set, increment */
            // check for pos < code->args ---> positional argument
            // if is passed as a keyword arg, the default is already in local or freevars
            // it gets overwritten...if more than one keyword arg with the same name is passed
            // argset overflows
            if (pos < 0) {
                pos = -(pos + 1);
                debug("check freevars at %i %x\n", pos, PTUPLE_ITEM(freevars, pos));
                if (PTUPLE_ITEM(freevars, pos) == NULL) {
                    argset++;
                    debug( "setting arg %i at freevars[%i]: already set %i\r\n", karg, pos, argset);
                    PTUPLE_SET_ITEM(freevars, pos, karg);
                }
            } else {
                debug("check locals at %i %x\n", pos, locals[pos]);
                if (locals[pos] == NULL) {
                    argset++;
                    debug( "setting arg %i at locals[%i]: already set %i %i\r\n", karg, pos, argset, code->args);
                    locals[pos] = karg;
                }
            }
            
        }
    }

    j = (src->self) ? 1 : 0;

    /* set positional args and eventually defaults */
    for (i = 0; i < code->args; i++) {
#if VM_DEBUG
        if (i < arginfo->nargs) {
            debug( "setting positional %i: it goes to locals[%i] and it's taken from %s[%i]\n", i, argpos[i], (j && !i) ? "self" : "args", i - j);
        } else if (i < arginfo->nargs + nvargs) {
            debug( "setting positional %i: it goes to locals[%i] and it's taken from vargs[%i]\n", i, argpos[i], i - j);
        } else {
            debug( "setting positional %i: it goes to locals[%i] and it's taken from defstore[%i]\n", i, argpos[i], i - obargs);
        }
#endif
        PObject *parg;
        if (i < arginfo->nargs) {
            if (src->self) {
                parg = (i) ? src->args[i - 1] : src->self;
            } else {
                parg = src->args[i];
            }
        } else if (i < arginfo->nargs + nvargs) {
            parg = nvarglist[unvargs++];
        } else {
            parg = src->defstore[i - obargs];
        }
        //store parg appropriately
        int8_t apos = argpos[i];
        if (!parg){
            debug("NULL ARG at %i %i %x %i %i",i,j,src->self,arginfo->nargs,code->args);
            return ERR_TYPE_EXC;
        }
        if (apos >= 0) {
            //it's a local
            if (!locals[apos]) { //could be already set by a keyword arg
                locals[apos] = parg;
                debug("setting %x to %i of locals\n", parg, apos);
                argset++;
            }
        } else {
            //it's a freevar: calc the index
            apos = -(apos + 1);
            if (!PTUPLE_ITEM(freevars, apos)) { //could be already set by a keyword arg
                PTUPLE_SET_ITEM(freevars, apos, parg);
                debug("setting %x to %i of freevars\n", parg, apos);
                argset++;
            }
        }
    }

    /* handle varargs */
    if (code->vararg) {
        PTuple *tuple;
        PObject **tpb;
        tuple = ptuple_new(arginfo->nargs + nvargs - unvargs - code->args, NULL/*src->args + code->args*/);
        tpb = PSEQUENCE_OBJECTS(tuple);
        if (arginfo->nargs > code->args) {
            //copy remaining nargs to *args
            memcpy(tpb, src->args + code->args, (arginfo->nargs - code->args)*sizeof(PObject *));
            tpb += (arginfo->nargs - code->args);
        }
        if (nvargs && (nvargs > unvargs)) {
            //copy remaining vargs to *args
            memcpy(tpb, nvarglist + unvargs, (nvargs - unvargs)*sizeof(PObject *));
        }
        locals[argpos[code->args]] = (PObject *)tuple;
        argset++;
    } else if (nvargs && (nvargs > unvargs)) {
        /* too many arguments passed! */
        debug( "wrong! too many positional arguments\r\n");
        return ERR_TYPE_EXC;
    }



    /* check missing args */
    if (argset != code->args + code->kwargs + code->vararg) {
        debug( "wrong! wrong number of args! %i vs %i\r\n", argset, code->args + code->kwargs + code->vararg);
        return ERR_TYPE_EXC;
    }
    return ERR_OK;
}

