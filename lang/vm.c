#include "lang.h"
#include "port.h"


VM *vmpnt = NULL;

#if VM_DEBUG

const char *const opstrings[] = {
    "NOP", "STOP", "CONSTI", "CONSTF", "CONSTS", "CONST_NONE", "CONSTI_0", "CONSTI_1", "CONSTI_2", "CONSTI_M1", "CONSTF_0", "CONSTF_1", "CONSTF_M1", "CONSTS_0", "CONSTS_S", "CONSTS_N",
    "LOAD_FAST", "LOAD_GLOBAL", "LOAD_DEREF", "LOAD_SUBSCR", "LOAD_ATTR", "LOOKUP_BUILTIN",
    "STORE_FAST", "STORE_GLOBAL", "STORE_DEREF", "STORE_SUBSCR", "STORE_ATTR",
    "DELETE_FAST", "DELETE_GLOBAL", "DELETE_DEREF", "DELETE_SUBSCR", "DELETE_ATTR",
    "UPOS", "UNEG", "NOT", "INVERT",
    "ADD", "IADD", "SUB", "ISUB", "MUL", "IMUL", "DIV", "IDIV", "FDIV", "IFDIV", "MOD", "IMOD", "POW", "IPOW", "LSHIFT", "ILSHIFT",
    "RSHIFT", "IRSHIFT", "BIT_OR", "IBIT_OR", "BIT_XOR", "IBIT_XOR", "BIT_AND", "IBIT_AND", "EQ", "NOT_EQ",
    "LT", "LTE", "GT", "GTE", "AND", "OR", "IS", "IS_NOT", "IN", "IN_NOT",
    "BUILD_LIST", "BUILD_TUPLE", "BUILD_SLICE", "BUILD_DICT", "BUILD_SET", "BUILD_CLASS", "END_CLASS", "UNPACK",
    "JUMP_IF_TRUE", "JUMP_IF_FALSE", "IF_TRUE", "IF_FALSE", "JUMP", "SETUP_LOOP", "POP_BLOCK", "BREAK", "CONTINUE", "GET_ITER", "FOR_ITER",
    "MAKE_FUNCTION", "MAKE_CLOSURE", "CALL", "CALL_VAR", "RET", "POP", "DUP", "DUP_TWO", "ROT_TWO", "ROT_THREE", "LOOKUP_CODE", "LOOKUP_NAME", "LOOKUP_NATIVE",
    "RAISE", "SETUP_FINALLY", "SETUP_EXCEPT", "POP_EXCEPT", "END_FINALLY", "BUILD_EXCEPTION", "CHECK_EXCEPTION", "IMPORT_NAME", "IMPORT_BUILTINS", "MAP_STORE", "LIST_STORE", "CONST_TRUE", "CONST_FALSE", "MAKE_CELL", "YIELD"
};

const char *const blockstrings[] = {
    "LOOP", "EXCEPT", "FINALLY", "HANDLER"
};
#endif

const char *const exceptionstrings[] = {
    "Exception", "ArithmeticException", "ZeroDivisionError", "FloatingPointError", "OverflowError", "LookupError", "IndexError", "KeyError",
    "NameError", "RuntimeError", "NotImplementedError", "TypeError", "ValueError", "AttributeError", "UnsupportedError"
};

const char *excmsg(uint16_t exc) {
    int pos = exc - NAME_Exception;
    if (pos < 0 || pos >= 15)
        return "UnknownException";
    return exceptionstrings[pos];
}


VSysTimer  vt;
VSysTimer  vtimers;
uint32_t _vm_thread_quantum;

void blink(int, int);
void do_switchtime(void *);
void reset_switchtime(void);


PThread *vm_init(VM *vm) {


    //TODO: substitute with macros
    _vm_thread_quantum = 100;

    vmpnt = vm;
    memset(&_vm, 0, sizeof(VM));
    _vm.program = (CodeHeader *)codemem;
    _vm.nmodules = 0;
    _vm.irqn = 0;
    _vm.etable = (PException *) (_vm.program->codeobjs + _vm.program->nobjs + _vm.program->ncnatives);



    debug( "Program: @%i\r\n", _vm.program);
    debug( "- magic: %x\r\n", _vm.program->magic);
    debug( "- flags: %i\r\n", _vm.program->flags);
    debug( "- nmods: %i\r\n", _vm.program->nmodules);
    debug( "- nobjs: %i\r\n", _vm.program->nobjs);
    debug( "- nexcp: %i\r\n", _vm.program->nexcp);
    debug( "- ncnat: %i\r\n", _vm.program->ncnatives);
    debug( "- sdata: %x\r\n", _vm.program->data_start);
    debug( "- edata: %x\r\n", _vm.program->data_end);
    debug( "- dbsss: %i\r\n", _vm.program->data_bss);
    debug( "- ssram: %x\r\n", _vm.program->ram_start);
    debug( "- cobjs: %i\r\n", _vm.program->codeobjs);
    debug( "- etabl: %x\r\n", _vm.etable);


    if (_vm.program->magic != 0x44474747)
        return NULL;


    /* Point to C Natives table */
    _vm.cnatives = (void **) & (_vm.program->codeobjs[_vm.program->nobjs]);
    /* Alloc C Natives */
    uint8_t *ramss = gc_malloc(_vm.program->data_bss);
    debug( "- ramss: %x\n", ramss);
    memcpy(ramss, (uint8_t *)_vm.program->data_start, _vm.program->data_end - _vm.program->data_start);

    _vm.timers = NULL;
    vt = vosTimerCreate();
    vtimers = vosTimerCreate();
    /* Init Consts */
    debug( "Init Consts\r\n");
    P_NONE = MAKE_NONE();
    P_ONE_F = (PObject *)pfloat_new(FLOAT_ONE);
    P_ZERO_F = (PObject *)pfloat_new(FLOAT_ZERO);
    P_M_ONE_F = (PObject *)pfloat_new(-FLOAT_ONE);

    P_EMPTY_S = (PObject *)psequence_new(PSTRING, 0);
    P_SPACE_S = (PObject *)psequence_new(PSTRING, 1);
    P_NEWLINE_S = (PObject *)psequence_new(PSTRING, 1);
    _PIS_BYTES(P_SPACE_S)[0] = ' ';
    _PIS_BYTES(P_NEWLINE_S)[0] = '\n';
    //((PString *)P_SPACE_S)->seq[0] = ' ';
    //((PString *)P_NEWLINE_S)->seq[0] = '\n';

    _vm.modules = (PModule **)(pbuffer_new(_vm.program->nmodules, sizeof(PObject *))->buffer);
    _vm.thlist = pthread_new(0, VOS_PRIO_NORMAL, NULL, 4);
    PTHREAD_SET_MAIN(_vm.thlist);
    _vm.thlist->next = _vm.thlist;
    _vm.thlist->prev = _vm.thlist;
    _vm.thlist->frame = pframe_new(0, NULL);
    pmodule_new(_vm.thlist->frame);
    _vm.irqthread = pthread_new(640, VOS_PRIO_HIGHER, vm_irqthread, 1);
    vosThResume(_vm.irqthread->th);

    vosTimerRecurrent(vt, TIME_U(_vm_thread_quantum, MILLIS), do_switchtime, NULL);
    debug( "END INIT\r\n");
    gc_start();
    return _vm.thlist;
}


void utoh(uint32_t num, uint8_t *buf) {
    uint32_t i, j;
    for (i = 0; i < 8; i ++) {
        j = (num & ((0xf0000000) >> (4 * i))) >> (28 - 4 * i);
        debug( "utoh %i %x %x\n", i, ((0xf0000000) >> (4 * i)), j);
        buf[i] = (uint8_t)((j < 10) ? ('0' + j) : ('A' + (j - 10)));
    }
}

uint32_t htou(uint8_t *buf) {
    uint32_t res = 0, i;
    for (i = 0; i < 8; i++) {
        res |= ((buf[7 - i] <= '9') ? (buf[i] - '0') : (10 + buf[i] - 'A')) << (4 * i);
    }
    return res;
}


int vm_upload() {
    uint32_t data = 0;
    uint32_t size = 0;
    uint32_t *origin;
    uint32_t wrt;
    uint32_t adler;
    uint8_t buffer[16];
    uint32_t drv = VM_UPLOAD_INTERFACE;
    uint8_t *uidstr = vhalNfoGetUIDStr();
    uint32_t ram_start = GC_HEDGE_START();
    uint8_t *buf = ((uint8_t *)ram_start);


    PORT_LED_ON();

    if (vhalSerialInit(drv, 115200, SERIAL_PARITY_NONE, SERIAL_STOP_ONE, SERIAL_BITS_8, VM_RXPIN(drv), VM_TXPIN(drv)) < 0)
        goto cleanup;

    debug( "Starting Upload\r\n");

    PORT_PRE_UPLOAD_HOOK();

    vhalSerialWrite(drv, (uint8_t *)"\n", 1);
    vhalSerialWrite(drv, (uint8_t *)VM_VERSION, VM_VERSIONL);
    vhalSerialWrite(drv, (uint8_t *)" ", 1);
    vhalSerialWrite(drv, (uint8_t *)VM_BOARD, VM_BOARD_LEN);
    vhalSerialWrite(drv, (uint8_t *)" ", 1);
    vhalSerialWrite(drv, (uint8_t *)VM_ARCH, VM_ARCH_LEN);
    vhalSerialWrite(drv, (uint8_t *)" ", 1);
    vhalSerialWrite(drv, uidstr, vhalNfoGetUIDLen() * 2);
    vhalSerialWrite(drv, (uint8_t *)" VIPER\n", 7);

    for (wrt = 0; wrt < VM_UPLOAD_TIMEOUT / 100; wrt++) {
        debug( "data %i %i\r\n", data, wrt);
        vosThSleep(TIME_U(100, MILLIS));
        data = vhalSerialAvailable(drv);
        if (data) break;
    }


    if (data) {
        //check command:
        debug( "data ok\r\n");
        data = vhalSerialRead(drv, buf, 1);
        if (data) {
            debug( "cmd %c %i/%i\r\n", buf[0], data, vhalSerialAvailable(drv));
            switch (buf[0]) {
                case 'U':
                    //upload
                    goto upload_cmd;
                case '#':
                    goto cleanup;
                default:
                    goto no_good;
            }
        } else goto no_good;


        //try uploading
upload_cmd:
        //debug("Uploading...\r\n");
        buffer[8] = '\n';
        vhalSerialWrite(drv, (uint8_t *)"OK\n", 3);
        utoh((uint32_t)_vsymbase, buffer);
        vhalSerialWrite(drv, buffer, 9);
        debug( ">>%s %x\n", buffer, (uint32_t)_vsymbase);

        utoh((uint32_t)ram_start, buffer);
        vhalSerialWrite(drv, buffer, 9);
        debug( ">>%s %x\n", buffer, (uint32_t)ram_start);

        utoh((uint32_t)codemem, buffer);
        vhalSerialWrite(drv, buffer, 9);
        debug( ">>%s %x\n", buffer, (uint32_t)codemem);


        vhalSerialRead(drv, (uint8_t *)&size, sizeof(uint32_t));
        buffer[9] = 0;
        //size = htou(buffer);
        debug( "read size %s=%i\n", buffer, size);

        if (!size) goto no_good;
        wrt = 0;
        vhalSerialWrite(drv, (uint8_t *)"OK\n", 3);
        origin = begin_bytecode_storage(size);
        debug( "Uploading %i bytes to %x\r\n", size, origin);
        while (wrt < size) {
retry_block:
            data = adler = 0;
            PORT_LED_ON();
            while (data < VM_UPLOAD_CHUNK && (wrt + data) < size) {
                //debug(".%i %i\r\n",data,stream_available(drv));
                data += vhalSerialRead(drv, buf + data, MIN(VM_UPLOAD_CHUNK, size - wrt)/*1*//*VM_UPLOAD_CHUNK - data*/);
            }
            //get crc
            vhalSerialRead(drv, (uint8_t *)&adler, sizeof(uint32_t));
            debug( "checking block of %i/%i/%i bytes %x\r\n", data, wrt, size, adler);

            PORT_LED_OFF();
            if (adler == adler32(buf, data)) {
                uint32_t *neworigin = bytecode_store(origin, buf, data);
                debug( "Writing %i bytes to %x -> %x\r\n", data, origin, neworigin);
                if (neworigin) {
                    if (memcmp(origin, buf, data) != 0) {
                        begin_bytecode_storage(size);
                        goto no_good;
                    }
                    origin = neworigin;
                    wrt += data;
                    vhalSerialWrite(drv, (uint8_t *)"OK\n", 3);
                } else {
                    begin_bytecode_storage(size);
                    goto no_good;
                }
            } else {
                debug( "retrying block\r\n");
                vhalSerialWrite(drv, (uint8_t *)"RB\n", 3);
                goto retry_block;
                //goto no_good;
            }
        }
        PORT_AFTER_UPLOAD_HOOK();
        //stream_close(drv);
        return 0;
    }
no_good:
    vhalSerialWrite(drv, (uint8_t *)"KO\n", 3);
    debug( "KO\n");
cleanup:
    PORT_AFTER_UPLOAD_HOOK();
    //stream_close(drv);
    debug( "close\n");
    return 1;
}




VSemaphore _vmirqsem;
volatile uint8_t switchtime = 0;
volatile uint8_t irqtime = 0;



void do_switchtime(void *p) {
    (void)p;
    switchtime = 1;
}

void reset_switchtime() {

    switchtime = 0;
}


void vm_add_irq_slot_isr(int slot, int dir) {
    vosSysLockIsr();
    if (_vm.irqn < VM_IRQS  && _vm.irqslots[dir][slot].fn) {
        _vm.irqstack[_vm.irqn].fn = _vm.irqslots[dir][slot].fn;
        _vm.irqstack[_vm.irqn].args = _vm.irqslots[dir][slot].args;
        _vm.irqn++;
        //VM_SIGNAL_I();
        vosSemSignalIsr(_vmirqsem);
        irqtime = 1;
    }
    vosSysUnlockIsr();
}

err_t vm_fill_irq_slot(int slot, int dir, PObject *fn, PObject *args) {
    if (fn == P_NONE) {
        _vm.irqslots[dir][slot].fn = NULL;
        _vm.irqslots[dir][slot].args = NULL;
    } else {
        _vm.irqslots[dir][slot].fn = fn;
        _vm.irqslots[dir][slot].args =  (PTuple *)args;
    }
    return ERR_OK;
}


//Must be called in a lock!
void vm_add_irq(PObject *fn, PObject *args) {
    if (_vm.irqn < VM_IRQS) {
        _vm.irqstack[_vm.irqn].fn = fn;
        _vm.irqstack[_vm.irqn].args = (PTuple*)args;
        _vm.irqn++;
        vosSemSignalIsr(_vmirqsem);
        irqtime = 1;
    }
}

void vm_timer_callback(void *timer) {
    (void)timer;
    SYSLOCK_I();
    PSysObject *t = (PSysObject *)timer;
    t->sys.timer.next->sys.timer.prev = t->sys.timer.prev;
    t->sys.timer.prev->sys.timer.next = t->sys.timer.next;
    if ((PObject*)t == _vm.timers) {
        _vm.timers = (PObject*)t->sys.timer.next;
        if (_vm.timers == (PObject*)t)
            _vm.timers = NULL;
    }
    vosTimerReset(t->sys.timer.vtm);
    vm_add_irq(t->sys.timer.fn.fn, (PObject*)t->sys.timer.fn.args);
    SYSUNLOCK_I();
    //printf("head is %x\n",_vm.timers);
}



err_t vm_add_timer(PObject *tm, uint32_t msec) {
    SYSLOCK();
    PSysObject *t = (PSysObject *)tm;
    if (_vm.timers) {
        PSysObject *cur = (PSysObject *)_vm.timers;
        t->sys.timer.prev = cur;
        t->sys.timer.next = cur->sys.timer.next;
        cur->sys.timer.next->sys.timer.prev = t;
        cur->sys.timer.next = t;
    } else {
        _vm.timers = tm;
        t->sys.timer.next = (PSysObject*)tm;
        t->sys.timer.prev = (PSysObject*)tm;
    }
    vosTimerOneShot(t->sys.timer.vtm, TIME_U(msec, MILLIS), vm_timer_callback, t);
    SYSUNLOCK();
    return ERR_OK;
}

err_t vm_del_timer(PObject *tm) {
    SYSLOCK();
    PSysObject *t = (PSysObject *)tm;
    t->sys.timer.next->sys.timer.prev = t->sys.timer.prev;
    t->sys.timer.prev->sys.timer.next = t->sys.timer.next;
    if ((PObject*)t == _vm.timers) {
        _vm.timers = (PObject*)t->sys.timer.next;
        if (_vm.timers == (PObject*)t)
            _vm.timers = NULL;
    }
    vosTimerReset(t->sys.timer.vtm);
    SYSUNLOCK();
    return ERR_OK;
}




int vm_irqthread(void *prm) {
    (void)prm;
    PThread *th = _vm.irqthread;
    _vmirqsem = vosSemCreate(0);

    PTHREAD_SET_IRQ(th);
    for (;;) {
        th->frame = NULL;
        debug( "> th (%i): waiting for irq\n", vosThGetId(th->th));
        vosSemWait(_vmirqsem);
        debug( "> th (%i): serving irq...\n", vosThGetId(th->th));

        SYSLOCK();
        _vm.irqcur = &_vm.irqstack[--_vm.irqn];
        SYSUNLOCK();

        PObject *callable = _vm.irqcur->fn;
        int tt = PTYPE(callable);
        debug( "> th (%i): serving irq... %i %i\n", vosThGetId(th->th), callable, tt);
        /* TODO: should raise an exception...but who will catch it? */
        if (tt != PFUNCTION && tt != PMETHOD)
            continue;
        PFunction *fn;
        if (tt == PFUNCTION) {
            fn = ((PFunction *)callable);
        } else {
            fn = (PFunction *)(((PMethod *)callable)->fn);
            if (PTYPE(fn) != PFUNCTION)
                continue;
        }
        PTuple *args = _vm.irqcur->args;
        PCode *code = PCODE_MAKE(fn->codeobj);
        th->frame = pframe_new(fn->codeobj, NULL);
        th->frame->module = fn->module;
        ArgInfo ainfo;
        ArgSource asrc;
        ainfo.defargs = fn->defargs;
        ainfo.kwdefargs = fn->defkwargs;
        asrc.defstore = fn->storage;
        ainfo.nargs = PSEQUENCE_ELEMENTS(args) + ((tt == PFUNCTION) ? 0 : 1);
        ainfo.nkwargs = 0;
        asrc.args = PTUPLE_OBJECTS(args);//(PObject **)(PSEQUENCE_BYTES(args));
        asrc.kwargs = NULL;
        asrc.vargs = NULL;
        asrc.self = (tt == PFUNCTION) ? NULL : (((PMethod *)callable)->self);
        debug( "> th (%i): new frame in irq thread: nargs %i args %i | %i\n", vosThGetId(th->th), ainfo.nargs, asrc.args, tt);
        //pcallable_set_args(code, &ainfo, &asrc, (code->codetype == CODETYPE_CNATIVE) ? (th->frame->storage) : (th->frame->storage + ((PViperCode *)code)->stacksize));
        pcallable_set_args(code, &ainfo, &asrc, PFRAME_PLOCALS(th->frame, code));
        SYSLOCK();
        _vm.irqcur = NULL;
        irqtime = _vm.irqn;
        SYSUNLOCK();
        vm_run(th);
    }
}


#ifdef VM_DEBUG
void pframe_print(PFrame *frm) {
    debug("\n/--------------%x\n", frm);
    debug("| module %i code %i why %i pc %i sp %i sb %i\n", frm->module, frm->code, frm->why, frm->pc,
          frm->sp, frm->sb);
    debug("S--------------\n");
    int i = 0;
    PObject **stack = PFRAME_PSTACK(frm);//(PObject **)(frm->storage);
    for (i = 0; i < frm->sp; i++) {
        debug( "| %i --> %s @%x\n", i, stack[i] ? typestrings[PTYPE(stack[i])] : "NULL", stack[i]);
    }
    debug("L--------------\n");
    //locals
    PCode *code = PCODE_MAKE(frm->code);
    PObject **locals = PFRAME_PLOCALS(frm, code); //(PObject **)(frm->storage + code->stacksize);
    for (i = 0; i < code->nlocals; i++) {
        debug( "| %i --> %s @%x\n", i, locals[i] ? typestrings[PTYPE(locals[i])] : "NULL", locals[i]);
    }

    if (PCODE_FREEVARS(code)) {
        PTuple *fvs = PFRAME_TFREEVARS(frm, code);
        PObject **frvss = PTUPLE_OBJECTS(fvs);
        debug("F-------------- %x\n", fvs);
        for (i = 0; i < PSEQUENCE_ELEMENTS(fvs); i++) {
            debug("| %i --> %s @%x\n", i, frvss[i] ? typestrings[PTYPE(frvss[i])] : "NULL", frvss[i]);
        }
    }

    if (PCODE_CELLVARS(code)) {
        PTuple *fvs = PFRAME_TCELLVARS(frm, code);
        PObject **frvss = PSEQUENCE_OBJECTS(fvs);
        debug("C-------------- %x\n", fvs);

        for (i = 0; i < PSEQUENCE_ELEMENTS(fvs); i++) {
            PObject *obj = PCELL_GET(frvss[i]);
            debug("| %i -->  @%x | %i :: %s = %x\n", i, PCELL_ARG(frvss[i]), PCELL_IDX(frvss[i]), obj ? typestrings[PTYPE(obj)] : "NULL", obj);
        }
    }

    PBlock *block = PFRAME_PBLOCKS(frm, code);

    if (code->bstacksize) {
        debug("B--------------\n");
        for (i = 0; i < frm->sb; i++) {
            block++;
            debug("| %i --> %s @ %i => %x\n", i, blockstrings[block->type], block->sp, block->jump);
        }
    }
    debug("\\--------------\n\n");
    /*
    if (PCODE_FREEVARS(code)){
        debug( "\n");
        PTuple* fvs = PFRAME_FREEVARS(frm);
        PObject **frvss = PSEQUENCE_OBJECTS(fvs);
        debug("FREEVARS: %x %x %i\n",fvs,frvss,PCODE_FREEVARS(code));
        for (i=0;i<PSEQUENCE_ELEMENTS(fvs);i++){

            debug("%i >>> %s @%x\n",i,frvss[i] ? typestrings[PTYPE(frvss[i])]:"NULL",frvss[i]);
        }
    }


    if (PCODE_CELLVARS(code)){
        debug( "\n");
        PTuple* fvs = PFRAME_CELLVARS(frm);
        PObject **frvss = PSEQUENCE_OBJECTS(fvs);
        debug("CELLVARS: %x %x %i\n",fvs,frvss,PCODE_CELLVARS(code));

        for (i=0;i<PSEQUENCE_ELEMENTS(fvs);i++){
            PObject *obj = PCELL_GET(frvss[i]);
            debug("%i <->  @%x | %i :: %s\n",i,PCELL_ARG(frvss[i]),PCELL_IDX(frvss[i]),obj ? typestrings[PTYPE(obj)]:"NULL");
        }
    }
    */

}

void pcode_print(PCode *code) {
    debug("CODE %x\n", code);
    debug("    type: %i %x\n", code->codetype, &code->codetype);
    debug("    args: %i %x\n", code->args, &code->args);
    debug("  kwargs: %i %x\n", code->kwargs, &code->kwargs);
    debug(" varargs: %i %x\n", code->vararg, &code->vararg);
    debug(" nlocals: %i %x\n", code->nlocals, &code->nlocals);
    debug("   nfree: %i %x\n", code->nfree, &code->nfree);
    debug("   stack: %i %x\n", code->stacksize, &code->stacksize);
    debug("  bstack: %i %x\n", code->bstacksize, &code->bstacksize);
    debug("    name: %i %x\n", code->name, &code->name);


}
#else

#define pframe_print(a)
#define pcode_print(a)
#endif




/* ========================================================================
    VM_RUN DEFINES
   ======================================================================== */

//#define FRAME th->frame
#define CODEOBJ FRAME->code
#define PC    FRAME->pc
#define BASESP PFRAME_PSTACK(FRAME)
//FRAME->storage
#define SP    FRAME->sp

#define TOS   (BASESP[SP-1])
#define TOS1  (BASESP[SP-2])
#define TOS2  (BASESP[SP-3])
#define TOS3  (BASESP[SP-4])

#define TOSn(n) (BASESP[SP-(n+1)])
#define TOS2n(n) (BASESP[SP-(n)])
#define PTOSn(n) (&(BASESP[SP-(n+1)]))
#define PTOS2n(n) (&(BASESP[SP-(n)]))


#define POPT()  (--SP)
#define POPS()  (--SP,BASESP[SP])
#define POPn(n)  (SP-=n)
#define PUSHS(o) (BASESP[SP]=(PObject*)(o),++SP)
#define PUTn(o,i) (BASESP[SP-i]=(o))

#define UNWIND(n)  (SP-=(n))


#define ARG_BYTE()  (bytecode[PC++])
#define ARG_WORD()  (uint16_t)(PC+=sizeof(uint16_t),(bytecode[PC-1]<<8)+(bytecode[PC-2]))
#define ARG_OFFS()  (int16_t)(PC+=sizeof(uint16_t),(bytecode[PC-1]<<8)+(bytecode[PC-2]))

#define LOCALS(n)   locals[n]
#define GLOBALS(n)  globals[n]
//#define FREEVARS(n) freevars[n]
//#define CELLVARS(n) cellvars[n]
#define FREEVARS_GET(idx) PTUPLE_ITEM(FREEVARS,idx)
#define CELLVARS_GET(idx) PTUPLE_ITEM(CELLVARS,idx)

#define FREEVARS_SET(idx,obj) PTUPLE_SET_ITEM(FREEVARS,idx,obj)
#define CELLVARS_SET(idx,obj) PTUPLE_SET_ITEM(CELLVARS,idx,obj)

#define ERR(x) ((x)!=ERR_OK)


#define PUSH_NEWBLOCK(btype,offs)                            \
    FRAME->sb++;                                            \
    block[FRAME->sb].type = btype;                           \
    block[FRAME->sb].sp = (SP);                             \
    block[FRAME->sb].jump = (PC+(int16_t)offs)

#define POP_CURBLOCK() FRAME->sb--

#define CURBLOCK() (&block[FRAME->sb])

#define HAS_BLOCKS(f) (f)->sb


#define WHY_NOT         0
#define WHY_CONTINUE    1
#define WHY_BREAK       2
#define WHY_RETURN      3
#define WHY_EXCEPTION   4
#define WHY_STOP        5
#define WHY_ENDCLASS    6


#define tbyte fl4
#define backjumpreg fl4


#define PREPARE_FRAME(f,backjumpid) \
    th->frame = (PFrame*)(f);\
    backjumpreg = backjumpid; \
    goto prepare_frame;\
    backjump_##backjumpid: \
    // back!


PModule *vm_has_module(uint16_t mcode) {
    int i;
    PModule *mod;
    for (i = 0; i < _vm.nmodules; i++) {
        mod = VM_MODULE(i);
        if (PMODULE_IS_LOADED(mod)) {
            if (mod->nfo.nums.code == mcode)
                return mod;
        } else {
            if (mod->nfo.frame->module == mcode)
                return mod;
        }
    }
    return NULL;
}

#define DO_DELEGATION(obj,name,nargs,popping)\
    callable = (PCallable*) pobj_getattr(obj, NAME_##name);\
    if(callable){\
        fl1=nargs;fl2=0;\
        is_delegation=opcode;\
        POPn(popping);\
        goto do_call;\
    } else {\
        res = (PObject*) PEXCEPTION_MAKE(exception_search(ERR_TYPE_EXC));\
    }



#define FREEVARS th->status.freevars
#define CELLVARS th->status.cellvars
#define callable th->status.callable
#define opcode th->status.opcode
#define is_delegation th->status.is_delegation
#define tword th->status.tword
#define fl1 th->status.fl1
#define fl2 th->status.fl2
#define fl3 th->status.fl3
#define fl4 th->status.fl4
#define err th->status.err
#define res th->status.res
#define fcode th->status.fcode
#define block th->status.block

int vm_run(PThread *th) {
    register PObject *arg1;
    register PObject *arg2;
    register PObject *arg3;
    //register uint16_t tword = 0;
    //register err_t err = ERR_OK;
    //register uint8_t fl1 = 0, fl2 = 0, fl3 = 0, fl4 = 0;
    register PObject **locals;
    register PObject **globals;
    register uint8_t *bytecode;
    //register PCode *fcode;
    register PFrame *FRAME = NULL;
    //PObject *res;
    //PTuple *freevars;
    //PTuple *cellvars;
    PFrame *frm = FRAME;
    //PBlock *block = NULL;
    //PCallable *callable = NULL;
    //uint8_t opcode = NOP;
    //uint8_t is_delegation = 0;
    fl1 = fl2 = fl3 = fl4 = 0;
    tword = 0;
    err = ERR_OK;
    block = NULL;

    /* Don't judge me for goto's...this reduces code size :P */
prepare_frame:
    debug( "> th (%i): prepare frame %x n%x o%x\r\n", vosThGetId(th->th), th, th->frame, FRAME);
    //pframe_print(th->frame);
    //th->frame = newframe;
    FRAME = th->frame;
    if (PMODULE_IS_LOADED(VM_MODULE(FRAME->module)))
        globals = VM_MODULE(FRAME->module)->globals;
    else {
        fcode = PCODE_MAKE(VM_MODULE(FRAME->module)->nfo.frame->code);
        globals = PFRAME_PLOCALS(VM_MODULE(FRAME->module)->nfo.frame, fcode);
        //globals = VM_MODULE(FRAME->module)->nfo.frame->storage + fcode->stacksize+fcode->bstacksize;
    }
    fcode = PCODE_MAKE(FRAME->code);
    pcode_print(fcode);
    locals = PFRAME_PLOCALS(FRAME, fcode);
    FREEVARS = PFRAME_TFREEVARS_SAFE(FRAME, fcode);
    CELLVARS = PFRAME_TCELLVARS_SAFE(FRAME, fcode);
    block = PFRAME_PBLOCKS(FRAME, fcode);
    bytecode = PCODE_GET_BYTECODE(fcode);
    debug("Prepared frame:\nlocals %x\nfreevars %x\ncellvars %x\nblocks %x\nbytecode %x\n", locals, FREEVARS, CELLVARS, block, bytecode);
    //locals = FRAME->storage + fcode->stacksize;
    //freevars = (PCODE_FREEVARS(fcode)) ? PSEQUENCE_OBJECTS(PFRAME_FREEVARS(FRAME)) : NULL; //locals + fcode->nlocals;
    //cellvars = (PCODE_CELLVARS(fcode)) ? PSEQUENCE_OBJECTS(PFRAME_CELLVARS(FRAME)) : NULL; //locals + fcode->nlocals;
    //block = (PBlock *)(freevars + ViperCode(fcode)->nfree - 1);
    //block = (PBlock *)(locals +fcode->stacksize+ fcode->nlocals - 1 + ((freevars) ? 1 : 0) + ((cellvars) ? 1 : 0));
    //bytecode = ViperCode(fcode)->bytecode + ViperCode(fcode)->bcstart;
    switch (backjumpreg) {
        //case 1: goto backjump_1;
        case 2: goto backjump_2;
        case 3: goto backjump_3;
        case 4: goto backjump_4;
        case 5: goto backjump_5;
        case 6: goto backjump_6;
    }

    debug( "> th (%i): Before Main loop\r\n", vosThGetId(th->th));
    ACQUIRE_GIL();
    for (;;) {

        opcode = bytecode[PC++];
        debug( "> th (%i): exec >> %s %i %x @ %i %i\r\n", vosThGetId(th->th), opstrings[opcode], opcode, opstrings[opcode], PC, FRAME->code);

        if (th->stage) {
            /* has staged objects! unstage them all: here is safe */
            debug( "> th (%i): Let's unstage!\n", vosThGetId(th->th));
            gc_wait();
            res = th->stage;
            do  {
                GC_UNSTAGE(res);
                res = GCH_NEXT(res);
            } while (res);
            th->stage = NULL;
            gc_signal();
            debug( "> th (%i): Unstaged!\n", vosThGetId(th->th));

        }

        if (switchtime) {
            switchtime = 0;
yield_point:
            RELEASE_GIL();
            vosThYield();
            ACQUIRE_GIL();
        }

        if (irqtime && th != _vm.irqthread)
            goto yield_point;

        pframe_print(FRAME);

        switch (opcode) {
            case NOP:
                continue;
            case STOP:
                FRAME->why = WHY_STOP;
                goto block_handler;
            case CONSTI:
            case CONSTF:
                tword = ARG_WORD();
                arg1 = code_getNConst(ViperCode(fcode), tword, opcode);
                PUSHS(arg1);
                continue;
            case CONSTS:
                gc_wait();
                tword = ARG_WORD();
                arg1 = code_getSConst(th, ViperCode(fcode), tword);
                PUSHS(arg1);
                gc_signal();
                continue;
            case CONST_NONE:
                PUSHS(P_NONE);
                continue;
            case CONSTI_0:
                PUSHS(P_ZERO);
                continue;
            case CONSTI_1:
                PUSHS(P_ONE);
                continue;
            case CONSTI_2:
                PUSHS(P_TWO);
                continue;
            case CONSTI_M1:
                PUSHS(P_M_ONE);
                continue;
            case CONSTF_0:
                PUSHS(P_ZERO_F);
                continue;
            case CONSTF_1:
                PUSHS(P_ONE_F);
                continue;
            case CONSTF_M1:
                PUSHS(P_M_ONE_F);
                continue;
            case CONSTS_0:
                PUSHS(P_EMPTY_S);
                continue;
            case CONSTS_S:
                PUSHS(P_SPACE_S);
                continue;
            case CONSTS_N:
                PUSHS(P_NEWLINE_S);
                continue;
            case LOAD_FAST:
                tbyte = ARG_BYTE();
                arg1 = LOCALS(tbyte);
                if (!arg1) {
                    err = ERR_NAME_EXC;
                    break;
                }
                PUSHS(arg1);
                continue;
            case LOAD_GLOBAL:
                tbyte = ARG_BYTE();
                arg1 = GLOBALS(tbyte);
                if (!arg1) {
                    err = ERR_NAME_EXC;
                    break;
                }
                PUSHS(arg1);
                continue;
            case LOAD_DEREF:
                tbyte = ARG_BYTE();
                fl1 = PCODE_FREEVARS(fcode);
                //debug("LOAD_DEREF: %i %i %x %x\n", tbyte, fl1, freevars, cellvars);
                //vosThSleep(TIME_U(1,SECONDS));
                if (tbyte < fl1) {
                    //freevar
                    if (!FREEVARS)
                        goto load_deref_fail;
                    arg1 = FREEVARS_GET(tbyte);
                } else {
                    //cellvar
                    tbyte -= fl1;
                    if (!CELLVARS)
                        goto load_deref_fail;
                    arg1 = CELLVARS_GET(tbyte);
                    //debug("CELLVAR(%i)=%x => %x %i %x\n", tbyte, arg1, PCELL_ARG(arg1), PCELL_IDX(arg1), PCELL_GET(arg1));
                    //vosThSleep(TIME_U(1,SECONDS));
                    arg1 = PCELL_GET(arg1);
                }
                if (!arg1) {
load_deref_fail:
                    err = ERR_NAME_EXC;
                    break;
                }
                //debug("LOAD_DEREF: push %x\n", arg1);
                //vosThSleep(TIME_U(1,SECONDS));
                PUSHS(arg1);
                continue;
            case LOAD_SUBSCR:
                arg1 = TOS;
                arg2 = TOS1;
                err = _types[PTYPE(arg2)].subscr(arg2, arg1, P_MARKER, &res); /* res = arg2[arg1] */
                if (ERR(err)) {
                    if (err == ERR_UNSUPPORTED_EXC) {
                        DO_DELEGATION(arg2, __getitem__, 1, 0);
delegate_load_subscr:
                        POPT(); //pop arg2
                        PUSHS(res);
                        if (!IS_EXCEPTION(res)) {
                            continue;
                        } else goto do_raise;
                    }
                    POPn(2);
                    break;
                }
                PUTn(res, 2);
                POPT();
                continue;

            case LOAD_ATTR:
                tword = ARG_WORD();
                arg1 = TOS;
                debug("LOAD_ATTR %i for %x| locals %x, stack %x, sp %i, stacksize %i\n", tword, arg1, locals, PFRAME_PSTACK(FRAME), FRAME->sp, fcode->stacksize);
                err = _types[PTYPE(arg1)].attr(arg1, tword, P_MARKER, &res); /* res = arg1.tword */
                POPT();
                if (ERR(err)) break;
                PUSHS(res);
                continue;
            case LOOKUP_BUILTIN:
                tword = ARG_WORD();
                if (tword > 0xff) {// --> reserved names
                    switch (tword) {
                        case __RES_NAME___BUILTINS__: PUSHS(VM_BUILTINS()); break;
                        case __RES_NAME___MODULE__: PUSHS(VM_MODULE(FRAME->module)); break;
                    }
                } else {
                    /*TODO: can we really access globals like that? what if __builtins__ is not loaded yet? */
                    PUSHS((VM_BUILTINS()->globals[tword]));
                }
                continue;
            case STORE_FAST:
                tbyte = ARG_BYTE();
                LOCALS(tbyte) = TOS;
                POPT();
                continue;
            case STORE_GLOBAL:
                tbyte = ARG_BYTE();
                GLOBALS(tbyte) = TOS;
                POPT();
                continue;
            case STORE_DEREF:
                tbyte = ARG_BYTE();
                fl1 = PCODE_FREEVARS(fcode);
                if (tbyte < fl1) {
                    //freevar
                    if (!FREEVARS)
                        goto store_deref_fail;
                    FREEVARS_SET(tbyte, TOS);
                    POPT();
                } else {
                    //cellvar
                    tbyte -= fl1;
                    if (!CELLVARS)
                        goto store_deref_fail;
                    arg1 = CELLVARS_GET(tbyte);
                    PCELL_SET(arg1, TOS);
                    POPT();
                }
                continue;
store_deref_fail:
                err = ERR_NAME_EXC;
                break;
            case STORE_SUBSCR:
                arg1 = TOS;
                arg2 = TOS1;
                arg3 = TOS2;
                err = _types[PTYPE(arg2)].subscr(arg2, arg1, arg3, &res); /* arg2[arg1]=arg3 */
                if (ERR(err)) {
                    if (err == ERR_UNSUPPORTED_EXC) {
                        //swap things around: TOS1 = k TOS = v
                        SYSLOCK();
                        TOS1 = arg3;
                        TOS = arg2;
                        TOS2 = arg1;
                        SYSUNLOCK();
                        DO_DELEGATION(arg2, __setitem__, 2, 1);
delegate_store_subscr:
                        if (IS_EXCEPTION(res)) {
                            PUSHS(res);
                            goto do_raise;
                        } else continue;
                    }
                    POPn(3);
                    break;
                }
                POPn(3);
                continue;
            case STORE_ATTR:
                tword = ARG_WORD();
                arg1 = TOS;
                arg2 = TOS1;
                err = _types[PTYPE(arg1)].attr(arg1, tword, arg2, &res); /* arg1.tword=arg2 */
                if (ERR(err))
                    break;
                POPn(2);
                continue;
            case DELETE_FAST:
                tbyte = ARG_BYTE();
                if (LOCALS(tbyte))
                    LOCALS(tbyte) = NULL;
                else {
                    err = ERR_NAME_EXC;
                    break;
                }
                continue;
            case DELETE_GLOBAL:
                tbyte = ARG_BYTE();
                if (GLOBALS(tbyte))
                    GLOBALS(tbyte) = NULL;
                else {
                    err = ERR_NAME_EXC;
                    break;
                }
                continue;
            case DELETE_DEREF:
                tbyte = ARG_BYTE();
                fl1 = PCODE_FREEVARS(fcode);
                if (tbyte < fl1) {
                    //freevar
                    if (!FREEVARS || !FREEVARS_GET(tbyte))
                        goto delete_deref_fail;
                    FREEVARS_SET(tbyte, NULL);
                } else {
                    //cellvar
                    tbyte -= fl1;
                    if (!CELLVARS)
                        goto delete_deref_fail;
                    arg1 = CELLVARS_GET(tbyte);
                    if (!PCELL_GET(arg1))
                        goto delete_deref_fail;
                    PCELL_SET(arg1, NULL);
                }
                continue;
delete_deref_fail:
                err = ERR_NAME_EXC;
                break;
            case DELETE_SUBSCR:
                arg1 = TOS;
                arg2 = TOS1;
                err = _types[PTYPE(arg2)].subscr(arg2, arg1, NULL, &res); /* res = arg2[arg1] */
                if (ERR(err)) {
                    if (err == ERR_UNSUPPORTED_EXC) {
                        DO_DELEGATION(arg2, __delitem__, 1, 0);
delegate_delete_subscr:
                        POPT(); //pop arg2
                        if (!IS_EXCEPTION(res)) {
                            continue;
                        } else {
                            PUSHS(res);
                            goto do_raise;
                        }
                    }
                    POPn(2);
                    break;
                }
                POPn(2);
                continue;
            case DELETE_ATTR:
                /* TODO */
                continue;

            /* TODO: unary ops */
            case UPOS:
                err = pnumber_unary_op(opcode, TOS, &res);
                if (ERR(err))
                    break;
                PUTn(res, 1);
                continue;
            case UNEG:
                err = pnumber_unary_op(opcode, TOS, &res);
                if (ERR(err))
                    break;
                PUTn(res, 1);
                continue;
            case NOT:
                if (pobj_is_true(TOS)) {
                    PUTn(P_FALSE, 1);
                } else {
                    PUTn(P_TRUE, 1);
                }
                continue;
            case INVERT:
                err = pnumber_unary_op(opcode, TOS, &res);
                if (ERR(err))
                    break;
                PUTn(res, 1);
                continue;
            /* binary ops */
            case ADD:
            case IADD:
            case SUB:
            case ISUB:
            case MUL:
            case IMUL:
            case DIV:
            case IDIV:
            case FDIV:
            case IFDIV:
            case MOD:
            case IMOD:
            case POW:
            case IPOW:
                arg1 = TOS;
                arg2 = TOS1;

                err = _types[PTYPE(arg2)].bop(_BIN_OP(opcode), arg2, arg1, &res);
                goto binop_check;
            case LSHIFT:
            case ILSHIFT:
            case RSHIFT:
            case IRSHIFT:
            case BIT_OR:
            case IBIT_OR:
            case BIT_XOR:
            case IBIT_XOR:
            case BIT_AND:
            case IBIT_AND:
                arg1 = TOS;
                arg2 = TOS1;

                err = _types[PTYPE(arg2)].bop(_BIN_OP(opcode), arg2, arg1, &res);
                goto binop_check;
            case EQ:
            case NOT_EQ:
            case LT:
            case LTE:
            case GT:
            case GTE:
                /* arg2 op arg1. arg1 is popped, arg2 is used to hold the result */
                arg1 = TOS;
                arg2 = TOS1;
                fl1 = PTYPE(arg2);
                if(fl1==PINSTANCE || fl1==PCLASS){
                    //check delegation
                }
                err = pobj_compare(_BIN_OP(opcode),arg2,arg1,&res);
binop_check:
                if (ERR(err)) goto binop_ko;
binop_ok:
                POPn(2);
                PUSHS(res);
                continue;
binop_ko:
                POPn(2);
                break;

            case L_AND:
                arg1 = TOS;
                arg2 = TOS1;
                if (!pobj_is_true(arg2))
                    arg3 = arg2;
                else
                    arg3 = arg1;
                PUTn(arg3, 2);
                POPT();
                continue;
            case L_OR:
                arg1 = TOS;
                arg2 = TOS1;
                if (pobj_is_true(arg2))
                    arg3 = arg2;
                else
                    arg3 = arg1;
                PUTn(arg3, 2);
                POPT();
                continue;
            case IS:
                arg1 = TOS;
                arg2 = TOS1;
                if (arg1 == arg2) PUTn(P_TRUE, 2);
                else PUTn(P_FALSE, 2);
                POPT();
                continue;
            case IS_NOT:
                arg1 = TOS;
                arg2 = TOS1;
                if (arg1 != arg2) PUTn(P_TRUE, 2);
                else PUTn(P_FALSE, 2);
                POPT();
                continue;
            case IN:
            case IN_NOT:
                arg1 = TOS;
                arg2 = TOS1;
                fl1 = PTYPE(arg1);
                fl2 = PTYPE(arg2);

                err = _types[fl1].bop(_BIN_OP(opcode), arg1, arg2, &res);
                if (err == ERR_OK) {
                    PUTn(res, 2);
                    POPT();
                    continue;
                } else {
                    DO_DELEGATION(TOS, __contains__, 1, 1);
delegate_in:
                    if (!IS_EXCEPTION(res)) {
                        if (opcode == IN_NOT) {
                            res = (res == P_TRUE) ? P_FALSE : P_TRUE;
                        }
                        PUSHS(res);
                        continue;
                    } else {
                        PUSHS(res);
                        goto do_raise;
                    }
                }
                break;
            case BUILD_LIST:
                tword = ARG_WORD();
                arg1 = (PObject *) plist_new(tword, PTOS2n(tword));
                UNWIND(tword);
                PUSHS(arg1);
                continue;
            case BUILD_TUPLE:
                tword = ARG_WORD();
                arg1 = (PObject *)ptuple_new(tword, PTOSn(tword - 1));
                UNWIND(tword);
                PUSHS(arg1);
                continue;

            case BUILD_SLICE:
                arg1 = TOS2;  /* start */
                arg2 = TOS1;  /* stop */
                arg3 = TOS;   /* step */
                res = (PObject *)pslice_new(arg1, arg2, arg3);
                PUTn(res, 3);
                POPn(2);
                continue;

            case BUILD_DICT:
                tword = ARG_WORD();
                res = (PObject *)pdict_new(tword);
                PUSHS(res);
                continue;

            case BUILD_SET:
                /*TODO*/
                continue;

            case BUILD_CLASS: {
                fl4 = ARG_BYTE();
                res = pclass_new(fl4, &TOS2);
                GC_STAGE(((PClass *)res)->bases);
                tword = PSMALLINT_VALUE(TOS);
                PCode *tcode;
                ArgInfo ainfo;
                ArgSource asrc;
                PFrame *frame;
                debug( "> th (%i): building class with %i parents, code %i, starting at %i\r\n", vosThGetId(th->th), fl4, tword, &TOS2);
                tcode = PCODE_MAKE(tword);
                ainfo.defargs = 0;
                ainfo.kwdefargs = 0;
                asrc.defstore = NULL;
                frame = pframe_new(tword, FRAME);
                frame->module = FRAME->module;
                ainfo.nargs = 0;
                ainfo.nkwargs = 0;
                asrc.args = NULL;
                asrc.kwargs = NULL;
                asrc.vargs = NULL;
                debug( "> th (%i): class frame @ %i %i\r\n", vosThGetId(th->th), SP, frame);
                err = pcallable_set_args(tcode, &ainfo, &asrc, PFRAME_PLOCALS(frame, tcode));
                if (err != ERR_OK)
                    break;
                /* call it */
                PUTn(res, (fl4 + 2));
                pframe_print(FRAME);
                POPn(fl4 + 1);
                pframe_print(FRAME);
                PREPARE_FRAME(frame, 5);
                //gc_trace();
            }
            continue;

            case END_CLASS:
                FRAME->why = WHY_ENDCLASS;
                goto block_handler;

            case UNPACK:
                fl2 = ARG_BYTE();
                /*TODO: protect this from gc */
                arg1 = POPS();
                while (fl2 > 0) {
                    fl2--;
                    psequence_getitem((PSequence *)arg1, PSMALLINT_NEW(fl2), &res);
                    //res = PTUPLE_ITEM(arg1,fl2);
                    //debug("unpack %i[%i] = %i\r\n", arg1, fl2, res);
                    PUSHS(res);
                }
                continue;

            case JUMP_IF_TRUE:
                tword = ARG_WORD();
                if (pobj_is_true(TOS)) PC += (int16_t)tword;
                else UNWIND(1);
                continue;
            case JUMP_IF_FALSE:
                tword = ARG_WORD();
                if (!pobj_is_true(TOS)) PC += (int16_t)tword;
                else UNWIND(1);
                continue;
            case IF_TRUE:
                tword = ARG_WORD();
                if (pobj_is_true(TOS)) PC += (int16_t)tword;
                POPT();
                continue;

            case IF_FALSE:
                tword = ARG_WORD();
                if (!pobj_is_true(TOS)) PC += (int16_t)tword;
                POPT();
                continue;

            case JUMP:
                tword = ARG_WORD();
                PC += (int16_t)tword;
                continue;

            case SETUP_LOOP:
                tword = ARG_WORD();
                fl4 = PBLOCK_LOOP;
setup_block:
                PUSH_NEWBLOCK(fl4, tword);
                continue;

            case POP_BLOCK:
pop_block:
                POP_CURBLOCK();
                continue;

            case BREAK:
                FRAME->why = WHY_BREAK;
                goto block_handler;

            case CONTINUE:
                tword = ARG_WORD();
                FRAME->why = WHY_CONTINUE;
                goto block_handler;

            case GET_ITER:
                arg2 = TOS;
                if (PTYPE(arg2) != PITERATOR)
                    arg2 = (PObject *)piterator_new(TOS);
                if (arg2 == NULL) {
                    //delegate call
                    debug("delegate_get_iter0: %x %i %i %i\n", FRAME, res, PC, SP);
                    DO_DELEGATION(TOS, __iter__, 0, 0);
delegate_get_iter:
                    debug("delegate_get_iter1: %x %i %i %i\n", FRAME, res, PC, SP);
                    if (IS_EXCEPTION(res)) {
                        PUTn(res, 1);
                        goto do_raise;
                    }
                    arg2 = res;
                }
                debug("delegate_get_iter2: %x %i %i %i\n", FRAME, res, PC, SP);
                PUTn(arg2, 1);
                continue;

            case FOR_ITER:
                tword = ARG_WORD();
                arg2 = TOS;
                if (PTYPE(arg2) != PITERATOR) {
                    debug("delegate_for_iter0: %x %i %i %i\n", FRAME, res, PC, SP);
                    FRAME->temp = tword;
                    DO_DELEGATION(TOS, __next__, 0, 0);
delegate_for_iter:
                    debug("delegate_for_iter1: %x %i %i %i\n", FRAME, res, PC, SP);
                    if (IS_EXCEPTION(res)) {
                        if (res == PEXCEPTION_MAKE(exception_search(ERR_STOP_ITERATION))) {
                            res = NULL;
                        } else {
                            PUSHS(res);
                            goto do_raise;
                        }
                    }
                    arg1 = res;
                    tword = FRAME->temp;
                } else {
                    arg1 = piterator_next((PIterator *)TOS);
                }
                debug("delegate_for_iter2: %x %i %i %i\n", FRAME, arg1, PC, SP);
                if (arg1 != NULL) {
                    PUSHS(arg1);
                } else {
                    UNWIND(1);
                    PC += (int16_t)tword;
                }
                continue;

            case MAKE_FUNCTION:
            case MAKE_CLOSURE: {
                tword = ARG_WORD();
                fl1 = tword & 0xff; /* defargs */
                fl2 = (tword >> 8); /* kwdefargs */
                fl3 = fl1;
                fl4 = fl2;

                /* code obj */
                //PCode *mfcode = PCODE_MAKE(PSMALLINT_VALUE(TOS));
                if (opcode == MAKE_CLOSURE) {
                    res = TOS; //get closure
                    tword = PSMALLINT_VALUE(TOS1);
                    TOS1 = res; //PUTn(res, 1);
                    POPT();
                } else {
                    res = NULL;
                    tword = PSMALLINT_VALUE(TOS);
                }
                debug( "> th (%i): Make Fun for code %i\r\n", vosThGetId(th->th), tword);
                PObject **storage;
                {
                    //debug("make fun @ %i with %i %i\r\n", SP, fl1, fl2);
                    PFunction *fn = pfunction_new(fl1, fl2, tword);
                    //debug("make fun @ %i after new %i\r\n", SP, fn);
                    fn->module = FRAME->module;
                    storage = fn->storage;
                    if (opcode == MAKE_CLOSURE) {
                        PFUNCTION_SET_CLOSURE(fn, res);
                    }
                    res = (PObject *)fn;
                }
                POPT(); /* pop code smallint */
                /* kwdefargs */
                tword = 0;
                while (fl2-- > 0) {
                    //debug("make fun @ %i kwarg %i in %i\r\n", SP, fl2, tword);
                    arg1 = TOSn(tword);//POPS();   /* kwdef */
                    tword++;
                    arg2 = TOSn(tword);//POPS(); /* name as smallint */
                    tword++;
                    SET_KWDEFARG_EX(storage, fl3, fl2, arg1);
                    SET_KWDEFARGNAME_EX(storage, fl3, fl2, arg2);
                }
                /* defargs */
                while (fl1-- > 0) {
                    //debug("make fun @ %i arg %i in %i\r\n", SP, fl1, tword);
                    arg1 = TOSn(tword); /* def */
                    tword++;
                    SET_DEFARG_EX(storage, fl1, arg1);
                }
                if (tword) {
                    PUTn(res, tword);
                    POPn(tword - 1);
                } else PUSHS(res);
                debug( "> th (%i): made fun @ %i\r\n", vosThGetId(th->th), SP);
            }
            continue;

            case CALL:
            case CALL_VAR: {
do_call:
                ;
                PFrame *frame;
                PCode *tcode;
                ArgInfo ainfo;
                ArgSource asrc;
                fl4 = 0;
                if (!is_delegation) {
                    tword = ARG_WORD();
                    fl1 = tword & 0xff; /* nargs */
                    fl2 = tword >> 8; /* nkwargs */
                    fl3 = (opcode == CALL_VAR) ? 1 : 0;
                    tword = fl1 + 2 * fl2 + fl3; /* index of callable on the stack */
                    callable = (PCallable *)TOSn(tword); /* get codeobj */
                } else {
                    //callable is set by delegator opcode
                    //fl1 and fl2 set by delegator
                    fl3 = 0;
                    tword = fl1;
                }
                asrc.self = NULL;
                asrc.vargs = (fl3) ? TOS : NULL;
                fl3 = 0; /* <--- set fl3 to 0. It will be set to 1 by PCLASS as needed, otherwise should be 0, also in CALL_VAR: already added in tword */
                debug( "> th (%i): calling %i %i %i %i %i @ %i [%i,%i] \r\n", vosThGetId(th->th), fl1, fl2, fl3, fl4, tbyte, SP, tword, callable);
                switch (PTYPE(callable)) {
                    case PDRIVER: {
                        //debug("calling driver %i %i\r\n", callable, PDRIVER_ID(callable));
                        if (fl2 || !fl1) { //no kwargs or nargs=0
                            err = ERR_TYPE_EXC;
                            goto error_handler;
                        }
                        err = PDRIVER_TO_CTL(callable)(fl1, (PObject *)callable, PTOSn(tword - 1), &res);
                        goto native_fn_ret;
                    }
                    break;
                    case PFUNCTION: {
callable_function:
                        //debug("function %i @ %i with code %i\r\n", callable, SP, callable->codeobj);
                        tcode = PCODE_MAKE(callable->codeobj);
                        ainfo.defargs = callable->defargs;
                        ainfo.kwdefargs = callable->defkwargs;
                        asrc.defstore = ((PFunction *)callable)->storage;
                        frame = pframe_new(callable->codeobj, FRAME);
                        if (is_delegation) {
                            PFRAME_DELEGATE(frame, opcode);
                        }
                        frame->module = ((PFunction *)callable)->module;
                        if (PCODE_CELLVARS(tcode)) {
                            PFRAME_SET_CELLVARS(frame, tcode, PFUNCTION_GET_CLOSURE((PFunction *)callable));
                        }
                        //debug("function: set %i %i\r\n", fl4, fl3);
                    }
                    break;
                    case PMETHOD: {
                        asrc.self = ((PMethod *)callable)->self;
                        //debug("method %i @ %i with self %i\r\n", callable, SP, asrc.self);
                        callable = ((PMethod *)callable)->fn;
                        fl3 = 0; fl4 = 1;
                        //debug("method: set %i %i\r\n", fl4, fl3);
                        if (IS_TAGGED(callable))
                            goto native_fn_call;
                        else
                            goto callable_function;
                    }
                    break;
                    case PCLASS: {
                        PMethod *mth;
                        //debug("class %i @ %i\r\n", callable, SP);
                        res = (PObject *)pinstance_new((PClass *)callable);
                        PUTn(res, tword - 1);
                        pframe_print(FRAME);
                        mth = (PMethod *)pinstance_get((PInstance *)res, NAME___init__);
                        if (!mth) {
                            POPn(tword);
                            continue;
                        }
                        tcode = PCODE_MAKE(mth->fn->codeobj);
                        ainfo.defargs = mth->fn->defargs;
                        ainfo.kwdefargs = mth->fn->defkwargs;
                        asrc.defstore = ((PFunction *)mth->fn)->storage;
                        asrc.self = res;
                        frame = pframe_new(mth->fn->codeobj, FRAME);
                        frame->module = ((PFunction *)mth->fn)->module;
                        PFRAME_NO_RET(frame);
                        fl4 = 1;
                        fl3 = 1;
                        //debug("class: set %i %i\r\n", fl4, fl3);
                    }
                    break;
                    case PNATIVE: {
native_fn_call:
                        /* only non-method natives can be in the stack */
                        /* a native method is inside a PMETHOD object */
                        debug( "> th (%i): native %i @ %i %i and type %i\r\n", vosThGetId(th->th), callable, SP, PNATIVE_CODE(callable), PNATIVE_TYPE(callable));
                        //NativeFn *nfo = &(type_fns[PNATIVE_TYPE(callable)][PNATIVE_CODE(callable)]);
                        const NativeFn *nfo = &_types[PNATIVE_TYPE(callable)].mths[PNATIVE_CODE(callable)];
                        debug( "> th (%i): native info: code %i argsM %i, argsm %i, block %i, name %i\r\n", vosThGetId(th->th), PNATIVE_CODE(callable), PNATIVE_ARGMAX(nfo),
                               PNATIVE_ARGMIN(nfo), PNATIVE_IS_BLOCKING(nfo), PNATIVE_NAME(nfo));
                        if ((fl1 < PNATIVE_ARGMIN(nfo)) || (fl1 > PNATIVE_ARGMAX(nfo)) || (fl2) || (PNATIVE_IS_METHOD(nfo)
                                && asrc.self == NULL)) {
                            err = ERR_TYPE_EXC;
                            goto error_handler;
                        }

                        if (PNATIVE_IS_BLOCKING(nfo)) {
                            RELEASE_GIL();
                        }
                        err = nfo->fn(fl1, asrc.self, PTOSn(tword - 1), &res);
                        if (PNATIVE_IS_BLOCKING(nfo)) {
                            ACQUIRE_GIL();
                        }

native_fn_ret:
                        debug( "> th (%i): returning from native in CALL with err %i and res %i\r\n", vosThGetId(th->th), err, res);
                        if (ERR(err)) goto error_handler;
                        PUTn(res, tword - 1);
                        POPn(tword);
                        continue;
                    }
                    break;
                    default:
                        /*not callable */
                        err = ERR_TYPE_EXC;
                        goto error_handler;
                }
                ainfo.nargs = fl1 + fl4;
                ainfo.nkwargs = fl2;
                asrc.args = PTOSn(tword - 1);
                asrc.kwargs = PTOSn(tword - 1 - fl1);

                //debug("prepare call: %i %i %i %i %i\r\n", fl1, fl2, fl3, fl4, tbyte);
                //debug("new frame %i, curr stack %i\r\n", frame, SP);
                err = pcallable_set_args(tcode, &ainfo, &asrc, PFRAME_PLOCALS(frame, tcode));
                if (err != ERR_OK)
                    break;

                /* call it */
                //debug("popn @ %i of %i\r\n", SP, (tword + (1 - fl3)));
                if (is_delegation) {
                    fl3++; //leave one on the stack
                    is_delegation = 0;
                }
                POPn( (tword + (1 - fl3)));
                if (tcode->codetype == CODETYPE_CNATIVE) {
                    tword = tcode->args + tcode->kwargs + tcode->vararg;
                    debug( "Calling CNative %i @ %x with args %i\n", CNativeCode(tcode)->tableidx, _vm.cnatives[CNativeCode(tcode)->tableidx], tword);
                    err = ((native_fn)_vm.cnatives[CNativeCode(tcode)->tableidx])(tword, asrc.self, PFRAME_PSTACK(frame), &res);
                    if (err != ERR_OK)
                        break;
                    PUSHS(res);
                    continue;
                }

                //debug("prepare frame @ %i\r\n", SP);
                PREPARE_FRAME(frame, 2);
            }
            continue;
            case RET:
                FRAME->why = WHY_RETURN;
                goto block_handler;
            case POP:
                UNWIND(1);
                continue;

            case DUP:
                PUSHS(TOS);
                continue;

            case DUP_TWO:
                PUSHS(TOS1);
                PUSHS(TOS1);
                continue;

            case ROT_TWO:
                arg1 = TOS;
                arg2 = TOS1;
                GC_START_STAGING();
                TOS = arg2;
                TOS1 = arg1;
                GC_STOP_STAGING();
                continue;

            case ROT_THREE:
                arg1 = TOS;
                arg2 = TOS1;
                arg3 = TOS2;
                GC_START_STAGING();
                TOS = arg2;
                TOS1 = arg3;
                TOS2 = arg1;
                GC_STOP_STAGING();
                continue;

            case LOOKUP_CODE:
            case LOOKUP_NAME:
                tword = ARG_WORD();
                PUSHS(PSMALLINT_NEW(tword));
                continue;
            case LOOKUP_NATIVE:
                tword = ARG_WORD();
                res = PNATIVE_MAKE(tword);
                //debug("looking up native %i = %i\r\n", tword, res);
                PUSHS(res);
                continue;

            case RAISE:
do_raise:
                arg1 = TOS;
                FRAME->why = WHY_EXCEPTION;
                if (!IS_EXCEPTION(arg1)) {
                    POPT();
                    err = ERR_TYPE_EXC;
                    goto error_handler;
                } else if (PEXCEPTION_TH(arg1) == EXCEPTION_NO_TH) {
                    //this exception is not linked to a thread
                    //link to the current one: it must be an explicit "raise Exc"
                    TOS = PEXCEPTION_MAKE_TH(PEXCEPTION_ERR(arg1), vosThGetId(th->th));
                    th->exc_code = FRAME->code;
                    th->exc_offs = PC;
                    th->exc_idx = PEXCEPTION_ERR(arg1);
                }
                goto block_handler;

            case SETUP_FINALLY:
                tword = ARG_WORD();
                fl4 = PBLOCK_FINALLY;
                goto setup_block;

            case SETUP_EXCEPT:
                tword = ARG_WORD();
                fl4 = PBLOCK_EXCEPT;
                goto setup_block;

            case POP_EXCEPT:
                if (CURBLOCK()->type != PBLOCK_HANDLER) {
                    err = ERR_RUNTIME_EXC;
                    break;
                }
                goto pop_block;

            case END_FINALLY:
                arg1 = POPS();
                if (IS_EXCEPTION(arg1)) {
                    //reraise ex
                    FRAME->why = WHY_EXCEPTION;
                    err = _vm.etable[PEXCEPTION_ERR(arg1)].name;
                    break;
                } else if (IS_PSMALLINT(arg1)) {
                    //ret or continue
                    FRAME->why = PSMALLINT_VALUE(arg1);
                    if (FRAME->why == WHY_CONTINUE) {
                        arg1 = POPS();
                        tword = (uint16_t)PSMALLINT_VALUE(arg1);
                        //debug("popping %i\r\n", tword);
                    }
                } else if (arg1 != P_NONE) {
                    debug( "what the hell?!?!?\r\n");
                }
                goto block_handler;

            case BUILD_EXCEPTION:
                tword = ARG_WORD();
                PUSHS(PEXCEPTION_MAKE(tword));
                continue;

            case CHECK_EXCEPTION:
                tword = ARG_WORD();
                arg1 = TOS;
                if (!IS_EXCEPTION(arg1)) {
                    err = ERR_RUNTIME_EXC;
                    POPT();
                    break;
                }
                if (exception_matches(tword, arg1)) {
                    PUTn(P_TRUE, 1);
                } else {
                    PUTn(P_FALSE, 1);
                }
                continue;

            case IMPORT_NAME:
                tword = ARG_WORD();
import_module:
                res = (PObject *)vm_has_module(tword);
                if (res) {
                    PUSHS(res);
                    continue;
                }
                th->frame = pframe_new(tword, FRAME);
                PUSHS(pmodule_new(th->frame));
                PREPARE_FRAME(th->frame, 3);
                continue;


            case IMPORT_BUILTINS:
                tword = 1;
                goto import_module;

            case MAP_STORE:
                //val- key - n iterators - dict
                tbyte = ARG_BYTE();
                arg1 = TOSn(tbyte + 2); /* dict */
                arg2 = TOS1;  /* key */
                arg3 = TOS;   /* val */
                pdict_put((PDict *)arg1, arg2, arg3);
                POPn(2);
                continue;
            case LIST_STORE:
                //val - n iterators - list
                tbyte = ARG_BYTE(); /*number of comprehensions*/
                arg1 = TOS;   /* val */
                arg2 = TOSn(tbyte + 1); /* get the list */
                plist_append(arg2, arg1);
                POPT();
                continue;
            case CONST_TRUE:
                PUSHS(P_TRUE);
                continue;
            case CONST_FALSE:
                PUSHS(P_FALSE);
                continue;
            case MAKE_CELL: {
                tbyte = ARG_BYTE();
                res = NULL;
                debug("MAKE_CELL: %i\n", tbyte);
                fl1 = PCODE_FREEVARS(fcode);
                fl2 = PCODE_CELLVARS(fcode);
                fl3 = fl2 + fl1;
                debug("MAKE_CELL: vars are %i\n", fl3);
                if (tbyte >= fl3) {
                    ;
                } else {
                    if (tbyte < fl1) {
                        //it's in freevars
                        //arg1 = (PObject *)PFRAME_FREEVARS(FRAME);
                        res = PCELL_MAKE(tbyte, FREEVARS); //must make a cell
                        debug("MAKE_CELL: freevars %i %x %x %x\n", tbyte, arg1, FREEVARS, res);
                    } else {
                        //it's in cellvars
                        tbyte -= fl1;
                        //arg1 = (PObject *)PFRAME_CELLVARS(FRAME);
                        //res = PTUPLE_ITEM(arg1, tbyte); //already a cell
                        res = CELLVARS_GET(tbyte);
                        debug("MAKE_CELL: cellvars %i %x %x %x\n", tbyte, arg1, CELLVARS, res);
                    }
                    PUSHS(res);
                }
                if (!res) {
                    //this should not happen...however :)
                    err = ERR_RUNTIME_EXC;
                    break;
                }
            }
            continue;

        } /* end switch */
error_handler:
        if (err != ERR_OK) {
            debug( "> th (%i): EH> %i %s\r\n", vosThGetId(th->th), err, excmsg(err));
            FRAME->why = WHY_EXCEPTION;
            th->exc_code = FRAME->code;
            th->exc_offs = PC;
            th->exc_idx = exception_search(err);
            PUSHS(PEXCEPTION_MAKE_TH(th->exc_idx, vosThGetId(th->th)));
        }

#define BLOCKSTR(b) blockstrings[(b)->type]

block_handler:
        debug( "> th (%i): BH> reason %i with block %i and err %i @ %i of %i\r\n", vosThGetId(th->th), FRAME->why, CURBLOCK(), err, PC, FRAME->code);

        while (FRAME->why != WHY_NOT && HAS_BLOCKS(FRAME)) {
            /* peek block */
            PBlock *b = CURBLOCK();
            debug( "> th (%i): ROLLING> reason %i with block %x = %s and next %x, frame %x\r\n", vosThGetId(th->th), FRAME->why, CURBLOCK(), BLOCKSTR(block),
                   FRAME->sb, FRAME);

            if (b->type == PBLOCK_LOOP && FRAME->why == WHY_CONTINUE) {
                //debug("LOOP && CONTINUE\r\n");
                FRAME->why = WHY_NOT;
                //debug("jumping to %i from %i\r\n", tword, PC);
                PC = tword;
                break; /* from block_handler loop */
            }

            /* pop block & unwind stack: TODO: make safe for gc */

            POP_CURBLOCK();

            if (b->type == PBLOCK_LOOP && FRAME->why == WHY_BREAK) {
                //debug("LOOP && BREAK\r\n");
                PC = b->jump;
                FRAME->sp = b->sp;
                FRAME->why = WHY_NOT;
                break; /* from block_handler loop */
            }

            if (b->type == PBLOCK_EXCEPT && FRAME->why == WHY_EXCEPTION) {
                //debug("EXCEPT && EXCEPTION\r\n");
                PC = b->jump;
                /*b->next = FRAME->block;
                FRAME->block = b;
                b->jump = 0;
                b->sp = (SP);
                b->header.flags = PBLOCK_HANDLER;*/
                PUSH_NEWBLOCK(PBLOCK_HANDLER, 0);
                FRAME->why = WHY_NOT;
                break;
            }

            if (b->type == PBLOCK_FINALLY && FRAME->why == WHY_EXCEPTION) {
                //debug("FINALLY && EXCEPTION\r\n");
                PC = b->jump;
                FRAME->why = WHY_NOT;
                break;
            }


            if (b->type == PBLOCK_FINALLY) {
                //debug("FINALLY && NOT EXCEPTION\r\n");
                if (FRAME->why == WHY_CONTINUE) {
                    //debug("pushing %i\r\n", tword);
                    PUSHS(PSMALLINT_NEW(tword)); //push continue target
                } // ret value is already on the stack if WHY_RETURN
                PUSHS(PSMALLINT_NEW(FRAME->why)); //push reason
                PC = b->jump;
                FRAME->why = WHY_NOT;
                break;
            }

            if (b->type == PBLOCK_EXCEPT) {
                //FRAME->sp = b->sp;
                continue;
            }

            if (b->type == PBLOCK_HANDLER) {
                //debug("HANDLER && ...\r\n");
                FRAME->sp = b->sp; //revert to tos=exception
                continue;
            }

            debug( "> th (%i): KEEP ON ROLLING\r\n", vosThGetId(th->th));

        }

        if (FRAME->why == WHY_RETURN || FRAME->why == WHY_STOP || FRAME->why == WHY_ENDCLASS || ((FRAME->why == WHY_EXCEPTION
                && !HAS_BLOCKS(FRAME)))) {
            /* get the return value */
            res = TOS;
            frm = FRAME;

            /* get parent frame */
            if (FRAME->why == WHY_STOP) {
                /* end of module */
                PModule *mod = VM_MODULE(FRAME->module);
                debug( "> th (%i): BH> WHY_STOP with module %i = %x\r\n", vosThGetId(th->th), FRAME->module, mod);
                mod->nfo.nums.nglobals = fcode->nlocals;
                mod->nfo.nums.code = FRAME->code;
                debug( "> th (%i): BH> Transferring globals %i\r\n", vosThGetId(th->th), mod->nfo.nums.nglobals);
                memcpy(mod->globals, locals, sizeof(PObject *) * (mod->nfo.nums.nglobals));
                PMODULE_SET_LOADED(mod);
            } else if (FRAME->why == WHY_ENDCLASS) {
                PFrame *pp = FRAME->parent;
                /* Can't be NULL*/
                PObject *cls = PFRAME_PSTACK(pp)[pp->sp - 1];
                debug( "> th (%i): calling class init for %i and %x %i\r\n", vosThGetId(th->th), cls, FRAME, PTYPE(cls));
                pframe_print(FRAME);
                pclass_init((PClass *)cls, FRAME);
                debug( "> th (%i): called class init for %i and %x\r\n", vosThGetId(th->th), cls, FRAME);
            }

            if (FRAME->parent == NULL) {
                /* return or stop = end of thread */
                debug( "> th (%i): BH> no upper frame\r\n", vosThGetId(th->th));
                th->frame = NULL;
                goto exit_thread;
            }

            debug( "> th (%i): BH> frame management: frame %x parent %x res %i\r\n", vosThGetId(th->th), FRAME, FRAME->parent, res);
            /*PREPARE_FRAME(frm->parent, 4);
            frm->parent = NULL;
            if ((frm->why == WHY_RETURN && !PFRAME_IS_NO_RET(frm)) || (frm->why == WHY_EXCEPTION)) {
                PUSHS(res);
            }
            if (frm->why == WHY_EXCEPTION) {
                FRAME->why = WHY_EXCEPTION;
                goto block_handler;
            }
            */
            //PREPARE_FRAME(frm->parent, 4);
            //frm->parent = NULL;
            if ((frm->why == WHY_RETURN && !PFRAME_IS_NO_RET(frm)) || (frm->why == WHY_EXCEPTION)) {
                fl1 = PFRAME_DELEGATED_OP(frm);
                if (!fl1) {
                    PFRAME_PSTACK(frm->parent)[frm->parent->sp] = res;
                    frm->parent->sp++;
                }
                PREPARE_FRAME(frm->parent, 4);
                frm->parent = NULL;
                debug( "> th (%i): BH> jumping to delegate %i\n", vosThGetId(th->th), fl1);
                switch (fl1) {
                    case IN:
                    case IN_NOT:
                        goto delegate_in;
                    case GET_ITER:
                        goto delegate_get_iter;
                    case FOR_ITER:
                        goto delegate_for_iter;
                    case LOAD_SUBSCR:
                        goto delegate_load_subscr;
                    case STORE_SUBSCR:
                        goto delegate_store_subscr;
                    case DELETE_SUBSCR:
                        goto delegate_delete_subscr;
                }
            } else {
                //STOP or ENDCLASS
                PREPARE_FRAME(frm->parent, 6);
                frm->parent = NULL;
            }
            //delegated frames don't get here
            if (frm->why == WHY_EXCEPTION) {
                FRAME->why = WHY_EXCEPTION;
                goto block_handler;
            }
            continue; /* with main loop */
        }

        if (FRAME->why != WHY_NOT)
            goto exit_thread;
        //break; /* from main loop */

    } /* end for */
exit_thread:
    RELEASE_GIL();
    // if (!PTHREAD_IS_IRQ(th)&&!PTHREAD_IS_MAIN(th)) {
    //     SYSLOCK();
    //     PTHREAD_SET_TERMINATED(th);
    //     //th->wks = NULL; //unlink workspace
    //     //TODO: unstage
    //     //TODO: remove dependency on ChibiOS
    //     chThdExitS(err);
    // }
    return 0;
}


PObject *P_TRUE = PBOOL_TRUE();
PObject *P_FALSE = PBOOL_FALSE();
PObject *P_ONE = PSMALLINT_NEW(1);
PObject *P_TWO = PSMALLINT_NEW(2);
PObject *P_ZERO = PSMALLINT_NEW(0);
PObject *P_M_ONE = PSMALLINT_NEW(-1);
PObject *P_NONE;
PObject *P_ONE_F;
PObject *P_ZERO_F;
PObject *P_M_ONE_F;
PObject *P_EMPTY_S;
PObject *P_SPACE_S;
PObject *P_NEWLINE_S;


#define SYM(x) (void*)(x)
#define VAR(x) (void*)&(x)

const void *const _vsymbase[] STORED = {
    SYM(memcpy),
    SYM(memset),
    SYM(memmove),
    SYM(gc_malloc),
    SYM(gc_free),
    SYM(strlen),
    VAR(_systime_millis),
    VAR(_systime_seconds),
    VAR(_system_frequency),
    VAR(_gillock),

    /* Threads */
    SYM(vosSysLock),
    SYM(vosSysUnlock),
    SYM(vosSysLockIsr),
    SYM(vosSysUnlockIsr),
    SYM(vosThCreate),
    SYM(vosThDestroy),
    SYM(vosThGetId),
    SYM(vosThGetStatus),
    SYM(vosThSetPriority),
    SYM(vosThGetPriority),
    SYM(vosThCurrent),
    SYM(vosThSleep),
    SYM(vosThYield),
    SYM(vosThResume),


    /*Semaphores*/
    SYM(vosSemCreate),
    SYM(vosSemReset),
    SYM(vosSemGetValue),
    SYM(vosSemSignal),
    SYM(vosSemSignalIsr),
    SYM(vosSemWaitTimeout),
    SYM(vosSemDestroy),

    /* Mutexes */
    SYM(vosMtxCreate),
    SYM(vosMtxLock),
    SYM(vosMtxTryLock),
    SYM(vosMtxUnlock),
    SYM(vosMtxDestroy),

    /* Timers */
    SYM(vosTimerCreate),
    SYM(vosTimerReadMillis),
    SYM(vosTimerReadMicros),
    SYM(vosTimerOneShot),
    SYM(vosTimerRecurrent),
    SYM(vosTimerReset),
    SYM(vosTimerDestroy),

    /*VHAL*/
    SYM(vhalGetPeripheralForPin),
    SYM(vhalPinSetMode),
    SYM(vhalPinRead),
    SYM(vhalPinWrite),
    SYM(vhalPinToggle),
    SYM(vhalPinAttachInterrupt),
    SYM(vhalPwmStart),
    SYM(vhalIcuStart),
    SYM(vhalHtmGetFreeTimer),
    SYM(vhalHtmOneShot),
    SYM(vhalHtmRecurrent),
    SYM(vhalSpiInit),
    SYM(vhalSpiLock),
    SYM(vhalSpiUnlock),
    SYM(vhalSpiSelect),
    SYM(vhalSpiUnselect),
    SYM(vhalSpiExchange),
    SYM(vhalSpiDone),


    SYM(psequence_new),
    SYM(netaddress_to_object),

    SYM(parse_py_args),
#if VM_DEBUG
    SYM(viper_debug),
#else
    NULL,
#endif
    SYM(vosTicks),
    SYM(vhalPinGetPort),
    SYM(vhalPinGetPad),
    SYM(vhalPinFastSet),
    SYM(vhalPinFastClear),
    SYM(vhalPinFastRead),
    SYM(vosThGetData)
};
