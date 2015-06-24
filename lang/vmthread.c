#include "vmthread.h"
#include "lang.h"


PThread *pthread_new(uint32_t size, uint8_t prio, void *fn, int cachesize) {
    /*int thsize = (size) ? THD_WA_SIZE(size) : 0;
    PThread *res = ALLOC_OBJ(PThread, PTHREAD, 0, 0);
    //thsize==0  --> main thread
    if (thsize) {
        PBuffer *wks = pbuffer_new(thsize, 1);
        res->wks = wks;
        SYSLOCK();
        res->id = __thid++;
        res->th = chThdCreateI(res->wks->buffer, thsize, prio, (fn) ? fn : vm_run, res);
        SYSUNLOCK();
        //add thread to vmlist
        gc_wait();
        res->next = _vm.thlist->next;
        res->prev = _vm.thlist;
        _vm.thlist->next->prev = res;
        _vm.thlist->next = res;
        gc_signal();
    } else res->th = THD_CURRENT();
    res->th->pthread = res;
    return res;*/
    PThread *res = ALLOC_OBJ(PThread, PTHREAD, 0, sizeof(ConstCache) * cachesize);
    res->cachesize = cachesize;
    //thsize==0  --> main thread
    if (size) {
        res->th = (VThread) vosThCreate(size, prio, (fn) ? fn : vm_run, res, res);
        //add thread to vmlist
        gc_wait();
        res->next = _vm.thlist->next;
        res->prev = _vm.thlist;
        _vm.thlist->next->prev = res;
        _vm.thlist->next = res;
        gc_signal();
    } else {
        res->th = vosThCurrent();
        vosThSetData(res->th, res);
    }
    return res;
}

PThread *pthread_get_by_id(uint32_t id) {
    PThread *ret = NULL;
    SYSLOCK();
    PThread *pth = _vm.thlist;
    do {
        if (pth->header.flags != 1 && vosThGetId(pth->th) == id) {
            ret = pth;
            break;
        }
        pth = pth->next;
    } while (pth != _vm.thlist);
    SYSUNLOCK();
    return ret;
}

// must be called between gc_wait --- gc_signal
PMethod *pthread_get_method_from_cache(PThread *th, PObject *self, PCallable *fn) {
    uint16_t cachepos = CONST_CACHE_POS(((uint32_t)fn) >> 2, th->cachesize);
    PMethod *obj = (PMethod*)th->cocache[cachepos].obj;
    if (obj && PHEADERTYPE(obj) == PMETHOD && obj->self == self && obj->fn == fn)
        return obj;
    gc_signal();
    obj = pmethod_new(NULL, self);
    gc_wait();
    obj->fn = fn;
    th->cocache[cachepos].obj=(PObject*)obj;
    return obj;
}

// must be called between gc_wait --- gc_signal
PObject *code_getSConst(PThread *th, PViperCode *code, uint16_t pos) {
    uint8_t *cnst = code->bytecode + code->nconsts * 2; //move to start of constants
    uint16_t *cheader = (uint16_t *)code->bytecode; //move to const header
    PObject *ret = NULL;
    uint16_t cachepos = CONST_CACHE_POS(pos, th->cachesize);
    cnst += cheader[pos];
    ret = (th->cocache[cachepos].addr == (uint32_t)cnst) ? (th->cocache[cachepos].obj) : NULL;
    if (!ret) {
        uint16_t len = *((uint16_t *)cnst);
        gc_signal();
        ret = (PObject *)pstring_new(len, cnst + 2);
        gc_wait();
        debug( "Getting const S %i @%x = %i\n", pos, cnst, len);
        th->cocache[cachepos].addr = (uint32_t)cnst;
        th->cocache[cachepos].obj = ret;
    }
    return ret;
}

PObject *code_getNConst(PViperCode *code, uint16_t pos, uint8_t opcode) {
    uint8_t *cnst = code->bytecode + code->nconsts * 2; //move to start of constants
    PObject *ret = NULL;
    cnst += pos; //move to offset
    debug("CONSTN: %x %i %i\n", cnst, pos, opcode);
    if (opcode == CONSTI) {
        int32_t val = *((int32_t *)cnst);
        ret = (PObject *)pinteger_new(val);
    } else {
        FLOAT_TYPE val = *((FLOAT_TYPE *)cnst);
        ret = (PObject *)pfloat_new(val);
    }
    return ret;
}
