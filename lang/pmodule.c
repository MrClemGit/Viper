#include "lang.h"


PModule *pmodule_new(PFrame *frame) {
    PViperCode *code = (PViperCode *)PCODE_MAKE(frame->code);
    int msize = (code->nlocals * sizeof(PObject *));
    PModule *mod = ALLOC_OBJ(PModule, PMODULE, 0, msize);
    memset(mod->globals, 0, msize);
    mod->nfo.frame = frame;
    mod->dict = (MiniTable *)(code->bytecode + code->nmstart);
    frame->module = VM_ADD_MODULE(mod);
    debug( "New Module %i= %i with globals %i\r\n", frame->module, mod, msize / sizeof(PObject *));
    return mod;
}


err_t pmodule_attr_fn(PObject *o, uint16_t name, PObject *b, PObject **res) {
    int16_t fni;
    PModule *mod = (PModule *)o;
    fni = minitable_lookup(mod->dict, name);
    if (fni < 0)
        return ERR_ATTRIBUTE_EXC;

    if (b == P_MARKER) {
        /* load attr */
        debug( "looking up name %i for module %i is %i\r\n", name, mod, fni);
        *res = mod->globals[fni];
    } else if (b) {
        /*store attr */
        mod->globals[fni] = b;
    } else {
        /*del attr*/
        return ERR_UNSUPPORTED_EXC;
    }
    return ERR_OK;
}