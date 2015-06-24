#include "lang.h"


PFrame *pframe_new(uint16_t ncode, PFrame *parent) {
    PCode *code = PCODE_MAKE(ncode);
    PTuple *freevars = NULL;
    uint32_t ssize = (code->stacksize + code->nlocals + code->bstacksize) * sizeof(PObject *);
    if (PCODE_HASVARS(code)) {
        ssize += 2 * sizeof(PObject *);
        if (PCODE_FREEVARS(code))
            freevars = ptuple_new(PCODE_FREEVARS(code), NULL);
    }
    PFrame *frame = ALLOC_OBJ(PFrame, PFRAME, 0, ssize);
    memset(frame->storage, 0, ssize);
    frame->parent = parent;
    frame->code = ncode;
    frame->sp = 0;
    frame->pc = 0;
    frame->why = 0;
    frame->sb = 0;
    if (freevars)
        PFRAME_SET_FREEVARS(frame, code, freevars);
    debug( "new frame %i with parent %i loc %i free %i stack %i code %i\r\n", frame, parent, code->nlocals, code->nfree, code->stacksize, code);
    return frame;
}


