#include "lang.h"
#include "port.h"


const uint8_t const _typetags[] STORED = {
    0,
    PDRIVER,
    PNONE,
    PEXCEPTION,
    PBOOL,
    PNATIVE,
    PCELL
};


/*========================================================================== */
/*====== NATIVE FUNCTIONS ================================================== */
/*========================================================================== */


#define THREAD_NEW     0
#define THREAD_START   1
#define THREAD_JOIN    2
#define THREAD_STATUS  3
#define THREAD_SETPRIO 4
#define THREAD_GETPRIO 5

NATIVE_FN(__thread) {
    NATIVE_UNWARN();

    int tt = PTYPE(args[0]);
    if (tt != PFUNCTION && tt != PMETHOD)
        return ERR_TYPE_EXC;
    CHECK_ARG(args[1], PSMALLINT);
    CHECK_ARG(args[2], PSMALLINT);
    CHECK_ARG(args[3], PTUPLE);

    /*TODO: check the type of callable */
    PFunction *fn;
    if (tt == PFUNCTION) {
        fn = (((PFunction *)args[0]));
    } else {
        fn = (PFunction *)((PMethod *)args[0])->fn;
        if (PTYPE(fn) != PFUNCTION) {
            return ERR_TYPE_EXC;
        }
    }
    int size = PSMALLINT_VALUE(args[1]);
    int prio = PSMALLINT_VALUE(args[2]);
    PTuple *params = (PTuple *)args[3];
    PCode *code = PCODE_MAKE(fn->codeobj);

    PFrame *frame = pframe_new(fn->codeobj, NULL);

    debug( "new frame in thread native @\n");
    frame->module = fn->module;
    ArgInfo ainfo;
    ArgSource asrc;
    ainfo.defargs = fn->defargs;
    ainfo.kwdefargs = fn->defkwargs;
    asrc.defstore = fn->storage;
    ainfo.nargs = PSEQUENCE_ELEMENTS(params) + ((tt == PFUNCTION) ? 0 : 1);
    ainfo.nkwargs = 0;
    asrc.args = PSEQUENCE_OBJECTS(params);
    asrc.kwargs = NULL;
    asrc.vargs = NULL;
    asrc.self = (tt == PFUNCTION) ? NULL : (((PMethod *)args[0])->self);

    err_t err = pcallable_set_args(code, &ainfo, &asrc, PFRAME_PLOCALS(frame, code));

    if (err == ERR_OK) {
        PThread *pth = pthread_new(size, prio, NULL, 4);
        pth->frame = frame;
        *res = (PObject *)pth;
    }
    return err;
}



#define RTOS__SEM_CREATE   0
#define RTOS__SEM_SIGNAL   1
#define RTOS__SEM_WAIT     2
#define RTOS__SEM_RESET    3
#define RTOS__SEM_TRYLOCK  4
#define RTOS__SEM_SIGNAL_WAIT 5


NATIVE_FN(__rtos_do) {
    NATIVE_UNWARN();
    CHECK_ARG(args[0], PSMALLINT);
    int32_t code = PSMALLINT_VALUE(args[0]);
    PSysObject *oo;
    *res = P_NONE;


    switch (code) {
        case RTOS__SEM_CREATE: {
            int32_t value = 1;
            if (nargs >= 2) {
                CHECK_ARG(args[1], PSMALLINT);
                value = PSMALLINT_VALUE(args[1]);
                if (value < 0) return ERR_VALUE_EXC;
            }
            oo = psysobj_new(PSYS_SEMAPHORE);
            oo->sys.sem = vosSemCreate(value);
            *res = (PObject*)oo;
        }
        break;
        case RTOS__SEM_SIGNAL: {
            if (nargs != 2)
                return ERR_TYPE_EXC;
            CHECK_ARG(args[1], PSYSOBJ);
            oo  = (PSysObject *)args[1];
            debug("signalling sem for cmd %i with %i\r\n", code, timeout);
            vosSemSignal(oo->sys.sem);
        }
        break;

        case RTOS__SEM_WAIT: {
            CHECK_ARG(args[1], PSYSOBJ);
            int32_t timeout = -1;
            if (nargs >= 3) {
                CHECK_ARG(args[2], PSMALLINT);
                timeout = PSMALLINT_VALUE(args[2]);
            }
            oo  = (PSysObject *)args[1];
            //printf("RTOS LOCK\n");
            timeout = vosSemWaitTimeout(oo->sys.sem, timeout <= 0 ? VTIME_INFINITE : (uint32_t)TIME_U(timeout, MILLIS));
            *res = (PObject*)PSMALLINT_VALUE(timeout);
        }
        break;

        case RTOS__SEM_RESET: {
            CHECK_ARG(args[1], PSYSOBJ);
            oo  = (PSysObject *)args[1];
            vosSemReset(oo->sys.sem);
        }
        break;
        case RTOS__SEM_TRYLOCK: {
            if (nargs != 2)
                return ERR_TYPE_EXC;
            CHECK_ARG(args[1], PSYSOBJ);
            oo  = (PSysObject *)args[1];
            SYSLOCK();
            int32_t cnt = vosSemGetValue(oo->sys.sem);
            if (cnt > 0) {
                //semaphore not taken, take it
                vosSemWait(oo->sys.sem);// !!! this call will remove the lock
                *res = P_ZERO;
                return ERR_OK;
            } else *res = P_M_ONE; //semaphore taken
            SYSUNLOCK();
        }
        break;
        case RTOS__SEM_SIGNAL_WAIT: {
            if (nargs < 3)
                return ERR_TYPE_EXC;
            CHECK_ARG(args[1], PSYSOBJ);
            oo  = (PSysObject *)args[1]; //sem to be signaled
            SYSLOCK();
            debug("signalling sem for cmd %i with %i\r\n", code, timeout);
            vosSemSignalIsr(oo->sys.sem);
            int32_t timeout = -1;
            if (nargs >= 4) {
                CHECK_ARG(args[3], PSMALLINT);
                timeout = PSMALLINT_VALUE(args[3]);
            }
            oo  = (PSysObject *)args[2]; //sem to be waited
            //printf("RTOS LOCK\n");
            timeout = vosSemWaitTimeout(oo->sys.sem, timeout <= 0 ? VTIME_INFINITE : (uint32_t)TIME_U(timeout, MILLIS));
            *res =(PObject*) PSMALLINT_VALUE(timeout);
        }
        break;
        default:
            return ERR_UNSUPPORTED_EXC;
    }
    return ERR_OK;

}


NATIVE_FN(getattr) {
    NATIVE_UNWARN();
    CHECK_ARG(args[1], PSMALLINT);
    PObject *o = pobj_getattr(args[0], PSMALLINT_VALUE(args[1]));
    if (!o)
        return ERR_ATTRIBUTE_EXC;
    *res = o;
    return ERR_OK;
}

NATIVE_FN(setattr) {
    NATIVE_UNWARN();

    int tt = PTYPE(args[0]);
    if (tt == PINSTANCE || tt == PCLASS) {
        CHECK_ARG(args[1], PSMALLINT);
        pdict_put( ((PAttributable *)(args[0]))->dict, args[1], args[2]);
        *res = P_NONE;
        return ERR_OK;
    }
    return ERR_UNSUPPORTED_EXC;
}

NATIVE_FN(hasattr) {
    NATIVE_UNWARN();
    CHECK_ARG(args[1], PSMALLINT);
    PObject *o = pobj_getattr(args[0], PSMALLINT_VALUE(args[1]));
    *res = (o) ? P_TRUE : P_FALSE;
    return ERR_OK;
}




NATIVE_FN(random) {
    NATIVE_UNWARN();
    if (nargs == 0) {
        int r = vhalRngGenerate() % (INT_TYPE_MAX >> 2);
        *res = (PObject *)pinteger_new(r);
    } else if (nargs == 2) {
        CHECK_ARG(args[0], PSMALLINT);
        CHECK_ARG(args[1], PSMALLINT);
        *res = (PObject *)pinteger_new( (vhalRngGenerate() % (PSMALLINT_VALUE(args[1]) - PSMALLINT_VALUE(args[0]))) + PSMALLINT_VALUE(args[0]));
    } else return ERR_TYPE_EXC;
    return ERR_OK;
}

NATIVE_FN(enumerate) {
    NATIVE_UNWARN();
    int16_t start = 0;
    if (nargs == 2) {
        CHECK_ARG(args[1], PSMALLINT);
        start = PSMALLINT_VALUE(args[1]);
    }
    PIterator *rr = piterator_new(args[0]);
    if (!rr)
        return ERR_TYPE_EXC;
    PITERATOR_SET_TYPE(rr, PITERATOR_ENUMERATE);
    rr->temp = start;
    *res = (PObject *)rr;
    return ERR_OK;
}

NATIVE_FN(reversed) {
    NATIVE_UNWARN();
    PIterator *rr = piterator_new(args[0]);
    if (!rr)
        return ERR_TYPE_EXC;
    PITERATOR_SET_TYPE(rr, PITERATOR_REVERSED);
    *res = (PObject *)rr;
    return ERR_OK;
}



NATIVE_FN(ord) {
    NATIVE_UNWARN();
    CHECK_ARG(args[0], PSTRING);
    *res = PSMALLINT_NEW(PSEQUENCE_BYTES(args[0])[0]);
    return ERR_OK;
}

NATIVE_FN(chr) {
    NATIVE_UNWARN();
    CHECK_ARG(args[0], PSMALLINT);
    int32_t tt = PSMALLINT_VALUE(args[0]);
    if (tt < 0 || tt > 255)
        return ERR_TYPE_EXC;
    uint8_t bb = (uint8_t)tt;
    *res = (PObject *)pstring_new(1, &bb);
    return ERR_OK;
}






NATIVE_FN(__new_timer) {
    NATIVE_UNWARN();
    PSysObject *oo = psysobj_new(PSYS_TIMER);
    oo->sys.timer.fn.fn = NULL;
    oo->sys.timer.fn.args = NULL;
    oo->sys.timer.vtm = vosTimerCreate();
    *res = (PObject *)oo;
    return ERR_OK;
}


NATIVE_FN(__timer_set) {
    NATIVE_UNWARN();

    CHECK_ARG(args[0], PSYSOBJ);
    CHECK_ARG(args[1], PSMALLINT);

    PSysObject *oo = (PSysObject *)args[0];
    if (oo->type != PSYS_TIMER)
        return ERR_TYPE_EXC;
    int32_t msec = PSMALLINT_VALUE(args[1]);
    if (msec > 0) {
        oo->sys.timer.fn.fn = args[2];
        oo->sys.timer.fn.args = ptuple_new(nargs - 3, args + 3);
        debug( "setting timer %i in %i msec with fn %i and nargs %i @ %i\r\n", oo, msec, args[2], nargs - 3, args + 3);
        vm_add_timer((PObject*)oo, msec);
    } else {
        vm_del_timer((PObject*)oo);
    }
    *res = (PObject *)oo;


    return ERR_OK;
}


NATIVE_FN(__timer_get) {
    NATIVE_UNWARN();
    uint32_t   now = vosMillis();
    if (now > 0x3fffffff)
        now = 0xffffffff - now;
    *res = PSMALLINT_NEW(now);
    return ERR_OK;
}




NATIVE_FN(onPinRise) {
    NATIVE_UNWARN();

    CHECK_ARG(args[0], PSMALLINT);
    int32_t pin = PSMALLINT_VALUE(args[0]);
    PObject *fn1 = args[1];

    if (!VHAL_IS_VALID_VPIN(pin) || !PIN_HAS_PRPH(pin, PRPH_EXT))
        return ERR_UNSUPPORTED_EXC;

    PObject *tuple = (fn1 != P_NONE) ? (PObject *)ptuple_new(nargs - 2, args + 2) : P_NONE;
    SYSLOCK();
    int slot = vhalPinAttachInterrupt(pin, PINMODE_EXT_RISING, vm_add_irq_slot_isr);
    if (slot < 0) {
        SYSUNLOCK();
        return ERR_RUNTIME_EXC;
    } else {
        vm_fill_irq_slot(slot, 1, fn1, tuple);
    }
    SYSUNLOCK();
    *res = P_NONE;
    return ERR_OK;
}


NATIVE_FN(onPinFall) {
    NATIVE_UNWARN();

    CHECK_ARG(args[0], PSMALLINT);
    int32_t pin = PSMALLINT_VALUE(args[0]);
    PObject *fn1 = args[1];

    //debug("onFall: vpin %x, pinclass %x, pinclasspad %i, classid %i, pinprph: %x, pinport %i, pinpad %i\n",pin,PIN_CLASS(pin),PIN_CLASS_PAD(pin),PIN_CLASS_ID(pin),PIN_PRPH(pin),PIN_PORT_NUMBER(pin),PIN_PAD(pin));
    if (!VHAL_IS_VALID_VPIN(pin) || !PIN_HAS_PRPH(pin, PRPH_EXT))
        return ERR_UNSUPPORTED_EXC;

    PObject *tuple = (fn1 != P_NONE) ? (PObject *)ptuple_new(nargs - 2, args + 2) : P_NONE;
    SYSLOCK();
    int slot = vhalPinAttachInterrupt(pin, PINMODE_EXT_FALLING, vm_add_irq_slot_isr);
    if (slot < 0) {
        SYSUNLOCK();
        return ERR_RUNTIME_EXC;
    } else {
        vm_fill_irq_slot(slot, 0, fn1, tuple);
    }
    SYSUNLOCK();
    *res = P_NONE;
    return ERR_OK;
}


NATIVE_FN(sleep) {
    NATIVE_UNWARN();

    int32_t delay;
    int32_t time_unit = MILLIS;

    CHECK_ARG(args[0], PSMALLINT);
    if (nargs > 1) {
        CHECK_ARG(args[1], PSMALLINT);
        time_unit = PSMALLINT_VALUE(args[1]);
    }

    delay = PSMALLINT_VALUE(args[0]);

    if (time_unit < MICROS || time_unit > SECONDS || delay < 0 ) {
        return ERR_TYPE_EXC;
    }

    vosThSleep(TIME_U(delay, time_unit));

    *res = P_NONE;
    return ERR_OK;
}

NATIVE_FN(__len) {
    NATIVE_UNWARN();

    int tt = PTYPE(self);

    if (IS_PSEQUENCE_TYPE(tt)) {
        *res = PSMALLINT_NEW(PSEQUENCE_ELEMENTS(self));
        return ERR_OK;
    } else if (IS_MAP_TYPE(tt)) {
        *res = PSMALLINT_NEW(PMAP_ELEMENTS(self));
        return ERR_OK;
    }
    /*TODO: implement the rest */
    return ERR_NOT_IMPLEMENTED_EXC;
}


NATIVE_FN(___len) {
    NATIVE_UNWARN();
    PObject *arg1 = args[0];
    PObject **args2 = &args[1];
    return ntv____len(nargs - 1, arg1, args2, res);
}


NATIVE_FN(range) {
    NATIVE_UNWARN();

    INT_TYPE start = 0;
    INT_TYPE stop = 0;
    INT_TYPE step = 1;

    if (nargs >= 1) {
        CHECK_ARG(args[0], PSMALLINT);
        start = PSMALLINT_VALUE(args[0]);
    }

    if (nargs >= 2) {
        CHECK_ARG(args[1], PSMALLINT);
        stop = PSMALLINT_VALUE(args[1]);
    }

    if (nargs >= 3) {
        CHECK_ARG(args[2], PSMALLINT);
        step = PSMALLINT_VALUE(args[2]);
    }

    *res = (PObject *)prange_new(start, stop, step);
    return ERR_OK;
}


#define ISSPACE(c) ( (c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == 12)
#define ISDIGIT(c) ( (c) >= '0' && (c) <= '9')
#define ISALPHA(c) ( ((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z'))
#define ISUPPER(c) ( (c) >= 'A' && (c) <= 'Z')
#define ISLOWER(c) ( (c) >= 'a' && (c) <= 'z')

INT_TYPE vatoi(uint8_t *buf, int32_t len, uint16_t base, err_t *err) {
    const uint8_t *s = buf;
    INT_TYPE acc;
    int c, neg = 0, any;
    INT_TYPE cutoff, cutlim;


    debug( "vatoi: %i of %i base %i\r\n", buf, len, base);
    *err = ERR_VALUE_EXC;
    do {
        c = *s++;
        len--;
    } while (ISSPACE(c) && len > 0);

    if (len < 0) return 0;

    if (c == '-') {
        neg = 1;
        c = *s++;
        len--;
    } else if (c == '+') {
        c = *s++;
        len--;
    }

    if (len < 0) return 0;

    if ((base == 0 || base == 16) && c == '0' && (*s == 'x' || *s == 'X')) {
        c = s[1];
        s += 2;
        len -= 2;
        base = 16;
    } else if ((base == 0 || base == 2) && c == '0' && (*s == 'b' || *s == 'B')) {
        c = s[1];
        s += 2;
        len -= 2;
        base = 2;
    }
    if (base == 0)
        base = (c == '0') ? 8 : 10;


    if (len < 0) return 0;

    cutoff = neg ? -(UINT_TYPE)INT_TYPE_MIN : INT_TYPE_MAX;
    cutlim = cutoff % (UINT_TYPE)base;
    cutoff /= (UINT_TYPE)base;

    debug( "vatoi: cutoff %i cutlim %i len %i\r\n", cutoff, cutlim, len);

    *err = ERR_OK;
    for (acc = 0, any = 0; len >= 0; c = *s++, len--) {
        debug( "vatoi: %i %c %i %i\r\n", c, c, acc, any);
        if (ISDIGIT(c))
            c -= '0';
        else if (ISALPHA(c))
            c -= ISUPPER(c) ? ('A' - 10) : ('a' - 10);
        else
            break;
        if (c >= base)
            break;
        if ( (any < 0) || (acc > cutoff) || (acc == cutoff && c > cutlim))
            any = -1;
        else {
            any = 1;
            acc *= base;
            acc += c;
        }
    }
    if (any < 0) {
        acc = neg ? INT_TYPE_MIN : INT_TYPE_MAX;
        *err = ERR_OVERFLOW_EXC;
    } else if (any == 0) {
        *err = ERR_VALUE_EXC;
    } else if (neg)
        acc = -acc;
    return (acc);
}

NATIVE_FN(int) {
    NATIVE_UNWARN();

    if (nargs == 0) {
        *res = P_ZERO;
        return ERR_OK;
    } else {
        int tt = PTYPE(args[0]);
        if (tt == PFLOAT && nargs == 1) {
            *res = (PObject *)pinteger_new( (INT_TYPE)FLOAT_TRUNC(FLOAT_VALUE(args[0])));
            return ERR_OK;
        } else if (tt == PSTRING || tt == PBYTES || tt == PBYTEARRAY) {
            err_t err;
            int base = 10;
            if (nargs == 2) {
                CHECK_ARG(args[1], PSMALLINT);
                base = PSMALLINT_VALUE(args[1]);
            }

            INT_TYPE val = vatoi(PSEQUENCE_BYTES(args[0]), PSEQUENCE_ELEMENTS(args[0]), base, &err);
            if (err != ERR_OK)
                return err;
            *res = (PObject *)pinteger_new(val);
            return ERR_OK;
        } else if (tt == PSMALLINT) {
            *res = args[0];
            return ERR_OK;
        }
    }
    return ERR_TYPE_EXC;
}

#include <math.h>
#define PZERO 38            /* index of 1e0 in powten[] */
#define PMAX 76             /* highest index of powten[]    */
#define HUGE_NOR 1.70141183460469228        /* normalised HUGE  */

FLOAT_TYPE vatof(uint8_t *s, int32_t len, err_t *err) {
    //extern double powten[];
    FLOAT_TYPE val, man;
    int i, sign, esign, ex, ex1;

    *err = ERR_VALUE_EXC;

    //skip spaces
    for (i = 0; (s[i] == ' ' || s[i] == '\n' || s[i] == '\t') && len > 0; i++, len--);

    if (len < 0) return 0;

    //sign
    sign = 1;
    if (s[i] == '+' || s[i] == '-') {
        len--;
        sign = (s[i++] == '+') ? 1 : -1;
        if (len < 0) return 0;
    }

    // read digits
    ex = 0;
    for (val = FLOAT_ZERO; s[i] >= '0' && s[i] <= '9'; i++, len--) {
        val = val * 10 + (s[i] - '0');
        debug( "VATOF: %c\r\n", s[i]);
        ex++;
        if (len < 0) return 0;
    }
    debug( "VATOF: digits %i %i\r\n", ex, (int32_t)val);

    /* if first non-digit is decimal point skip it and keep going */
    /* if it is not a decimal point we fall through to test for exponent */
    if (s[i] == '.') {
        i++;
        len--;
    }
    /* continue to build value while the digits keep coming */
    for (man = FLOAT_ONE; s[i] >= '0' && s[i] <= '9'; i++, len--) {
        man *= 0.1f;
        val += (s[i] - '0') * man;
        debug( "VATOF: .%c\r\n", s[i]);
        if (len < 0) return 0;
    }
    debug( "VATOF: digits %f\r\n", val);

    /* if non-digit was an exponent flag deal with exponent */
    if (s[i] == 'e' || s[i] == 'E' || s[i] == 'd' || s[i] == 'D') {
        i++;
        len--;
        if (len < 0) return 0;

        esign = 1;
        /* check for explicit + or - sign in exponent and deal with it */
        if (s[i] == '+' || s[i] == '-') {
            esign = (s[i++] == '+') ? 1 : -1;
            len--;
            if (len < 0) return 0;
        }
        for (ex1 = 0; s[i] >= '0' && s[i] <= '9'; i++, len--) {
            ex1 = 10 * ex1 + s[i] - '0';
            if (len < 0) return 0;
        }
        ex += ex1 * esign;
    }
    /* incorporate exponent into val */
    if (ex < 0) {
        while (ex < 0) {
            val *= 0.1f;
            ex++;
        }
    } else if (ex < PMAX || (ex == PMAX && val < HUGE_NOR))
        while (ex > 1) {
            val *= 10;
            ex--;
        }
    else {
        val = INFINITY;
    }
    *err = ERR_OK;
    debug( "VATOF %i - %i\r\n", (int32_t)sign * val, sign);
    return (sign * val);
}
NATIVE_FN(float) {
    NATIVE_UNWARN();

    if (nargs == 0) {
        *res = P_ZERO_F;
        return ERR_OK;
    } else {
        int tt = PTYPE(args[0]);
        if (tt == PSMALLINT || tt == PINTEGER) {
            *res = (PObject *)pfloat_new( (FLOAT_TYPE) INTEGER_VALUE(args[0]));
            return ERR_OK;
        } else if (tt == PSTRING || tt == PBYTES || tt == PBYTEARRAY) {
            err_t err;
            FLOAT_TYPE val = vatof(PSEQUENCE_BYTES(args[0]), PSEQUENCE_ELEMENTS(args[0]), &err);
            if (err != ERR_OK)
                return err;
            *res = (PObject *)pfloat_new(val);
            return ERR_OK;
        }
    }
    return ERR_TYPE_EXC;
}



#define PO10_LIMIT (INT_TYPE_MAX/10)


int nDigits(int i)
{
    int n, po10;

    if (i < 0) i = -i;
    n = 1;
    po10 = 10;
    while (i >= po10)
    {
        n++;
        if (po10 > PO10_LIMIT) break;
        po10 *= 10;
    }
    return n;
}

#define NTV__STR_BUF 32
char strbuf[NTV__STR_BUF];


static void strreverse(char *begin, char *end) {
    char aux;
    while (end > begin)
        aux = *end, *end-- = *begin, *begin++ = aux;
}

int modp_itoa10(int32_t value, char *str) {
    char *wstr = str;
    /* Take care of sign */
    uint32_t uvalue = (value < 0) ? (uint32_t)(-value) : (uint32_t)(value);
    /* Conversion. Number is reversed. */
    do *wstr++ = (char)(48 + (uvalue % 10));
    while (uvalue /= 10);
    if (value < 0) *wstr++ = '-';
    *wstr = '\0';

    /* Reverse string */
    strreverse(str, wstr - 1);
    return (int)(wstr - str);
}

typedef union {
    long    L;
    float   F;
}   LF_t;

int ftoa(FLOAT_TYPE f) {
    long mantissa, int_part, frac_part;
    short exp2;
    LF_t x;
    char *p;
    char *outbuf = strbuf;

    if (f == 0.0f) {
        outbuf[0] = '0';
        outbuf[1] = '.';
        outbuf[2] = '0';
        return 3;
    }
    x.F = f;

    exp2 = (unsigned char)(x.L >> 23) - 127;
    mantissa = (x.L & 0xFFFFFF) | 0x800000;
    frac_part = 0;
    int_part = 0;

    if (exp2 >= 31) {
        outbuf[0] = 'I';
        outbuf[1] = 'n';
        outbuf[2] = 'f';
        return 3;
    } else if (exp2 < -23) {
        outbuf[0] = 'i';
        outbuf[1] = 'n';
        outbuf[2] = 'f';
        return 3;
    } else if (exp2 >= 23)
        int_part = mantissa << (exp2 - 23);
    else if (exp2 >= 0) {
        int_part = mantissa >> (23 - exp2);
        frac_part = (mantissa << (exp2 + 1)) & 0xFFFFFF;
    } else /* if (exp2 < 0) */
        frac_part = (mantissa & 0xFFFFFF) >> -(exp2 + 1);

    p = outbuf;

    if (x.L < 0)
        *p++ = '-';

    if (int_part == 0)
        *p++ = '0';
    else {
        p += modp_itoa10(int_part, p);
    }
    *p++ = '.';

    if (frac_part == 0)
        *p++ = '0';
    else {
        char m, max;

        max = NTV__STR_BUF - (p - outbuf) - 1;
        if (max > 7)
            max = 7;
        /* print BCD */
        for (m = 0; m < max; m++) {
            /* frac_part *= 10; */
            frac_part = (frac_part << 3) + (frac_part << 1);

            *p++ = (frac_part >> 24) + '0';
            frac_part &= 0xFFFFFF;
        }
        /* delete ending zeroes */
        for (--p; p[0] == '0' && p[-1] != '.'; --p)
            ;
        ++p;
    }
    return p - outbuf;
}


NATIVE_FN(__str) {
    NATIVE_UNWARN();


    int32_t tt = PTYPE(args[0]);

    //protect strbuf
    PString *str;
    switch (tt) {
        case PSMALLINT:
        case PINTEGER: {
            SYSLOCK();
            int32_t n = INTEGER_VALUE(args[0]);
            //debug("__str of %i %i\r\n", n, str);
            int i = NTV__STR_BUF;
            int32_t isNeg = n < 0;
            uint32_t n1 = isNeg ? -n : n;

            do {
                strbuf[--i] = (n1 % 10) + '0';
                n1 = n1 / 10;
            } while (n1 != 0);

            if (isNeg)
                strbuf[--i] = '-';
            //debug("__str: %i (%.*s)%i\r\n", i, NTV__STR_BUF - i, strbuf + i, NTV__STR_BUF - i);
            str = pstring_new(NTV__STR_BUF - i, (uint8_t *)(strbuf + i));
            SYSUNLOCK();
        }
        break;
        case PFLOAT: {
            //int i = snprintf(strbuf, NTV__STR_BUF, "%g", FLOAT_VALUE(args[0]));
            SYSLOCK();
            int i = ftoa(FLOAT_VALUE(args[0]));
            str = pstring_new(i, (uint8_t *)strbuf);
            SYSUNLOCK();
        }
        break;
        case PBOOL:
            if (args[0] == P_TRUE)
                str = pstring_new(4, (uint8_t *)"True");
            else
                str = pstring_new(5, (uint8_t *)"False");
            break;

        case PBYTES:
            str = pstring_new(((PBytes *)args[0])->elements, (uint8_t *)((PBytes *)args[0])->seq);
            break;
        case PBYTEARRAY:
            str = pstring_new(((PByteArray *)args[0])->elements, _PMS_BYTES(args[0]));
            break;
        case PSTRING:
            str = (PString *)args[0]; //pstring_new(((PString *)args[0])->elements, ((PString *)args[0])->seq);
            break;
        case PNONE:
            str = pstring_new(4, (uint8_t *)"None");
            break;
        case PEXCEPTION: {
            uint16_t elen = VM_EXCEPTION_MSG_LEN(args[0]);
            uint8_t *emsg = VM_EXCEPTION_MSG(args[0]);
            PThread *th = (PEXCEPTION_TH(args[0]) == EXCEPTION_NO_TH) ? NULL : pthread_get_by_id(PEXCEPTION_TH(args[0]));
            uint16_t pntlen = (!th) ? 0 : (5 + nDigits(th->exc_code) + nDigits(th->exc_offs));
            str = pstring_new(elen + pntlen, NULL);
            uint8_t *pstrbuf = PSEQUENCE_BYTES(str);
            warning("EXC to STRING: %i %i | %i | %i %i\n", elen, pntlen, PEXCEPTION_ERR(args[0]), emsg, th);
            memcpy(pstrbuf, emsg, elen);
            pstrbuf += elen;
            if (th) {
                memcpy(pstrbuf, " @[", 3);
                pstrbuf += 3;
                pstrbuf += modp_itoa10(th->exc_code, (char *)pstrbuf);
                *pstrbuf++ = ':';
                pstrbuf += modp_itoa10(th->exc_offs, (char *)pstrbuf);
                *pstrbuf++ = ']';
            }
        }
        break;
        default:
            return ERR_TYPE_EXC;
    }


    //debug("__str ret %i %.*s %i\r\n", str,str->elements,str->seq,str->elements);
    *res = (PObject *)str;

    return ERR_OK;
}


NATIVE_FN(digitalRead) {
    NATIVE_UNWARN();

    CHECK_ARG(args[0], PSMALLINT);

    int32_t pin = PSMALLINT_VALUE(args[0]);

    if (!VHAL_IS_VALID_VPIN(pin))
        return ERR_UNSUPPORTED_EXC;

    *res = (vhalPinRead(pin)) ? P_ONE : P_ZERO;

    return ERR_OK;
}

NATIVE_FN(pinToggle) {
    NATIVE_UNWARN();

    CHECK_ARG(args[0], PSMALLINT);

    int32_t pin = PSMALLINT_VALUE(args[0]);

    if (!VHAL_IS_VALID_VPIN(pin))
        return ERR_UNSUPPORTED_EXC;

    vhalPinToggle(pin);

    *res = P_NONE;
    return ERR_OK;
}

NATIVE_FN(digitalWrite) {
    NATIVE_UNWARN();

    CHECK_ARG(args[0], PSMALLINT);
    CHECK_ARG(args[1], PSMALLINT);

    int32_t pin = PSMALLINT_VALUE(args[0]);
    int32_t val = PSMALLINT_VALUE(args[1]);

    if (!VHAL_IS_VALID_VPIN(pin))
        return ERR_UNSUPPORTED_EXC;

    *res = P_NONE;

    vhalPinWrite(pin, val);

    return ERR_OK;
}

NATIVE_FN(pinMode) {
    NATIVE_UNWARN();

    CHECK_ARG(args[0], PSMALLINT);
    CHECK_ARG(args[1], PSMALLINT);

    int32_t pin = PSMALLINT_VALUE(args[0]);
    int32_t mode = PSMALLINT_VALUE(args[1]);

    //debug("pinmode: mode %i vpin %x, pinclass %x, pinclasspad %i, classid %i, pinprph: %x, pinport %i, pinpad %i\n",mode,pin,PIN_CLASS(pin),PIN_CLASS_PAD(pin),PIN_CLASS_ID(pin),PIN_PRPH(pin),PIN_PORT_NUMBER(pin),PIN_PAD(pin));
    if (!VHAL_IS_VALID_VPIN(pin) || mode < 0 || mode > PINMODE_GPIO_MODES)
        return ERR_UNSUPPORTED_EXC;

    *res = P_NONE;

    vhalPinSetMode(pin, mode);

    return ERR_OK;
}



NATIVE_FN(__lock) {
    NATIVE_UNWARN();
    //vm_lock();
    *res = P_NONE;
    return ERR_OK;
}


NATIVE_FN(__unlock) {
    NATIVE_UNWARN();
    //vm_unlock();
    *res = P_NONE;
    return ERR_OK;
}

NATIVE_FN(type) {
    NATIVE_UNWARN();
    *res = (PObject *)pinteger_new(PTYPE(args[0]));
    return ERR_OK;
}



NATIVE_FN(__driver) {
    NATIVE_UNWARN();

    CHECK_ARG(args[0], PSMALLINT);

    int32_t drvname = PSMALLINT_VALUE(args[0]);

    if (drvname < 0)
        return ERR_TYPE_EXC;

    int drv = get_driver(drvname);
    if (drv < 0)
        *res = P_NONE;
    else
        *res = (PObject *)PDRIVER_MAKE(drv);

    return ERR_OK;
}

err_t __ints_from_seq(PSequence *seq, int32_t *i) {

    int secobjsize = SEQ_OBJ_SIZE(seq);
    if (secobjsize == 1) {
        uint8_t *buf = PSEQUENCE_BYTES(seq);
        *i = buf[*i];

    } else if (secobjsize == 2) {
        uint16_t *buf = PSEQUENCE_SHORTS(seq);
        *i = buf[*i];
    } else {
        PObject **buf = PSEQUENCE_OBJECTS(seq);
        if (!IS_PSMALLINT(buf[*i]))
            return ERR_TYPE_EXC;
        *i = PSMALLINT_VALUE(buf[*i]);
    }
    return ERR_OK;
}

err_t __make_new_bytes(int nargs, PObject *self, PObject **args, PObject **res, int type) {
    NATIVE_UNWARN();

    PSequence *bt = NULL;
    if (nargs > 0) {
        int tt = PTYPE(args[0]);
        switch (tt) {
            case PSTRING:
                bt = (PSequence *)psequence_new(type, PSEQUENCE_ELEMENTS(args[0]));
                bt->elements = PSEQUENCE_ELEMENTS(args[0]);
                uint8_t *bb = PSEQUENCE_BYTES(args[0]);
                uint8_t *tb = PSEQUENCE_BYTES(bt);
                if (bb)
                    memcpy(tb, bb, bt->elements);
                break;
            case PSMALLINT:
                bt = (PSequence *)psequence_new(type, PSMALLINT_VALUE(args[0]));
                bt->elements = PSMALLINT_VALUE(args[0]);
                break;
            default:
                return ERR_NOT_IMPLEMENTED_EXC;
        }
    } else bt = (PSequence *)psequence_new(type, 0);
    *res = (PObject *)bt;
    return ERR_OK;

}

/*TODO: optimize bytearray and bytes: same code! */
NATIVE_FN(bytearray) {
    return __make_new_bytes(nargs, self, args, res, PBYTEARRAY);
}

err_t __make_new_shorts(int nargs, PObject *self, PObject **args, PObject **res, int type) {
    NATIVE_UNWARN();
    PSequence *bt = NULL;
    if (nargs > 0) {
        int tt = PTYPE(args[0]);
        if (tt == PSMALLINT) {
            bt = psequence_new(type, PSMALLINT_VALUE(args[0]));
            bt->elements = PSMALLINT_VALUE(args[0]);
            //memset(bt->seq->buffer, 0, bt->elements);
        } else if (IS_PSEQUENCE_TYPE(tt)) {
            int elems = PSEQUENCE_ELEMENTS(args[0]);
            bt = (PSequence *)psequence_new(type, elems);
            uint16_t *btbuf = PSEQUENCE_SHORTS(bt);
            for (tt = 0; tt < elems; tt++) {
                int32_t val = tt;
                if (__ints_from_seq((PSequence *)args[0], &val) != ERR_OK)
                    return ERR_TYPE_EXC;
                if (val < 0 || val > 0xffff)
                    return ERR_TYPE_EXC;
                btbuf[tt] = val;
            }
            /*
            int secobjsize = SEQ_OBJ_SIZE(args[0]);
            int elems = PSEQUENCE_ELEMENTS(args[0]);
            bt = (PSequence *)psequence_new(type, elems);
            uint16_t *btbuf = PSEQUENCE_SHORTS(bt);
            if (secobjsize == 1) {
                uint8_t *buf = PSEQUENCE_BYTES(args[0]);
                for (tt = 0; tt < elems; tt++) {
                    btbuf[tt] = buf[tt];
                }
            } else if (secobjsize == 2) {
                uint16_t *buf = PSEQUENCE_SHORTS(args[0]);
                memcpy(btbuf, buf, elems * 2);
            } else {
                PObject **buf = PSEQUENCE_OBJECTS(args[0]);
                for (tt = 0; tt < elems; tt++) {
                    if (!IS_PSMALLINT(buf[tt]))
                        return ERR_TYPE_EXC;
                    int32_t val = PSMALLINT_VALUE(buf[tt]);
                    if (val < 0 || val > 0xffff)
                        return ERR_TYPE_EXC;
                    btbuf[tt] = val;
                }
            }*/
        }
        else
            return ERR_NOT_IMPLEMENTED_EXC;
    } else bt = (PSequence *)psequence_new(type, 0);
    *res = (PObject *)bt;
    return ERR_OK;

}

NATIVE_FN(shortarray) {
    return __make_new_shorts(nargs, self, args, res, PSHORTARRAY);
}

NATIVE_FN(shorts) {
    return __make_new_shorts(nargs, self, args, res, PSHORTS);
}


NATIVE_FN(bytes) {
    return __make_new_bytes(nargs, self, args, res, PBYTES);
}



NATIVE_FN(isinstance) {
    NATIVE_UNWARN();

    int tt = PTYPE(args[1]);
    int ta = PTYPE(args[0]);

    if (tt == PSMALLINT) {
        *res = (ta == tt) ? P_TRUE : P_FALSE;
    } else if (tt >= PBYTES && tt <= PTUPLE) {
        *res = (psequence_contains((PSequence *)args[0], args[1])) ? P_TRUE : P_FALSE;
    } else if (tt == PCLASS) {
        if (ta == PINSTANCE) {
            PInstance *pi = (PInstance *)args[0];
            if (pclass_has_parent(pi->base, (PClass *)args[1]))
                *res = P_TRUE;
            else
                *res = P_FALSE;
        } else *res = P_FALSE;
    } else return ERR_TYPE_EXC;
    return ERR_OK;
}


#define GC_CMD_INFO     0
#define GC_CMD_COLLECT  1
#define GC_CMD_DISABLE  2
#define GC_CMD_ENABLE   3

NATIVE_FN(__gc) {
    NATIVE_UNWARN();
    CHECK_ARG(args[0], PSMALLINT);
    int32_t code = PSMALLINT_VALUE(args[0]);

    *res = P_NONE;

    switch (code) {
        case GC_CMD_INFO: {
            //total mem, free mem, fragmentation, blocks, fblocks, gctime
            PTuple *pt = ptuple_new(7, NULL);
            PTUPLE_SET_ITEM(pt, 0, PSMALLINT_NEW(GC_TOTAL_MEMORY()));
            PTUPLE_SET_ITEM(pt, 1, PSMALLINT_NEW(GC_FREE_MEMORY()));
            PTUPLE_SET_ITEM(pt, 2, PSMALLINT_NEW(100 - GC_FRAGMENTATION()));
            PTUPLE_SET_ITEM(pt, 3, PSMALLINT_NEW(GC_USED_BLOCKS()));
            PTUPLE_SET_ITEM(pt, 4, PSMALLINT_NEW(GC_FREE_BLOCKS()));
            PTUPLE_SET_ITEM(pt, 5, PSMALLINT_NEW(GC_PERIOD()));
            PTUPLE_SET_ITEM(pt, 6, PSMALLINT_NEW(GC_LAST_COLLECT()));
            *res = (PObject *)pt;
        }
        break;
        case GC_CMD_COLLECT: {
            gc_collect();
        }
        break;
        case GC_CMD_DISABLE: {
            return ERR_NOT_IMPLEMENTED_EXC;
        }
        break;
        case GC_CMD_ENABLE: {
            if (nargs == 2) {
                CHECK_ARG(args[1], PSMALLINT);
                code = PSMALLINT_VALUE(args[1]);
                if (code <= 0)
                    return ERR_VALUE_EXC;
            } else code = GC_PERIOD();
            GC_SET_PERIOD(code);
        }
        break;
    }
    return ERR_OK;
}

#define VM_CMD_VERSION  0
#define VM_CMD_INFO     1
#define VM_CMD_BOARD    2

NATIVE_FN(__vm) {
    NATIVE_UNWARN();
    CHECK_ARG(args[0], PSMALLINT);
    int32_t code = PSMALLINT_VALUE(args[0]);
    PObject *oo;

    switch (code) {
        case VM_CMD_VERSION: {
            *res = (PObject *)pstring_new(VM_VERSIONL, (uint8_t *)VM_VERSION);
        }
        break;
        case VM_CMD_INFO: {
            oo = (PObject *)ptuple_new(4, NULL);
            *res = oo;
            return ERR_NOT_IMPLEMENTED_EXC;
            /*
                        PTUPLE_SET_ITEM(oo, 0, P_NONE);
                        PTUPLE_SET_ITEM(oo, 1, P_NONE);
                        PTUPLE_SET_ITEM(oo, 2, P_NONE);
                        PTUPLE_SET_ITEM(oo, 3, P_NONE);
                        PString *sto = pstring_new(strlen(VM_STDOUT), VM_STDOUT);
                        PTUPLE_SET_ITEM(oo, 1, sto);
                        PString *ste = pstring_new(strlen(VM_STDERR), VM_STDERR);
                        PTUPLE_SET_ITEM(oo, 2, ste);*/
            /*
            int i;
            PTuple *drvs = ptuple_new(_ndrivers, NULL);
            for (i = 0; i < _ndrivers; i++) {
                PTUPLE_SET_ITEM(drvs, i, PNONE);
            }
            for (i = 0; i < _ndrivers; i++) {
                PTUPLE_SET_ITEM(drvs, i, pstring_new(4, _drvinfo[i].name));
            }
            PTUPLE_SET_ITEM(oo, 3, drvs);
            */
            break;
        }
        case VM_CMD_BOARD: {
            *res = (PObject *)pstring_new(VM_BOARD_LEN, (uint8_t *)VM_BOARD);
            break;
        }

        default:
            return ERR_UNSUPPORTED_EXC;
    }
    return ERR_OK;
}

NATIVE_FN(net_address) {
    NATIVE_UNWARN();

    NetAddress addr;
    if (nargs <= 2) {
        int tt = PTYPE(args[0]);
        if (nargs == 2) {
            CHECK_ARG(args[1], PSMALLINT);
            OAL_SET_NETPORT(addr.port, PSMALLINT_VALUE(args[1]));
        } else addr.port = 0;
        if (tt == PTUPLE) {
            if (PSEQUENCE_ELEMENTS(args[0]) < 4 || PSEQUENCE_ELEMENTS(args[0]) > 5)
                return ERR_TYPE_EXC;
            for (tt = 0; tt < 4; tt++) {
                PObject *pt = PTUPLE_ITEM(args[0], tt);
                CHECK_ARG(pt, PSMALLINT);
                int32_t pv = PSMALLINT_VALUE(pt);
                if (pv < 0 || pv > 255) return ERR_VALUE_EXC;
                OAL_IP_SET(addr.ip, tt, pv);
            }
        } else if (tt == PSTRING) {
            PString *url = (PString *)args[0];
            int32_t cnt = 0, ph = 3, cnz = 0, oct = 0;
            uint8_t *seq = (uint8_t *)url->seq[cnt];
            while (cnt < url->elements) {
                if (seq[cnt] >= '0' && seq[cnt] <= '9') {
                    oct *= 10;
                    oct += seq[cnt] - '0';
                    cnz++;
                } else if (seq[cnt] == '.') {
                    OAL_IP_SET(addr.ip, 3 - ph, oct);
                    ph--;
                    cnz = oct = 0;
                } else return ERR_VALUE_EXC;
                cnt++;
                if (cnz > 3 || ph < 0)
                    return ERR_VALUE_EXC;
            }
            if (ph != 0) return ERR_VALUE_EXC;
        } else return ERR_TYPE_EXC;
    } else if (nargs == 4) {
        addr.port = 0;
        goto check_numerical_ip;
    } else if (nargs == 5) {
        CHECK_ARG(args[4], PSMALLINT);
        OAL_SET_NETPORT(addr.port, PSMALLINT_VALUE(args[4]));
check_numerical_ip:
        CHECK_ARG(args[0], PSMALLINT);
        CHECK_ARG(args[1], PSMALLINT);
        CHECK_ARG(args[2], PSMALLINT);
        CHECK_ARG(args[3], PSMALLINT);

        OAL_MAKE_IP(addr.ip, PSMALLINT_VALUE(args[0]), PSMALLINT_VALUE(args[1]), PSMALLINT_VALUE(args[2]),
                    PSMALLINT_VALUE(args[3]));

    } else return ERR_TYPE_EXC;

    *res = netaddress_to_object(&addr);
    return ERR_OK;
}


#define __PACK_SPACE    0
#define __PACK_ORDER    1
#define __PACK_COUNT    2
#define __PACK_FORMT    3


typedef union _packu {
    int64_t i64;
    uint64_t u64;
    int32_t i32;
    uint32_t u32;
    int16_t i16;
    uint16_t u16;
    int8_t i8;
    uint8_t u8;
    float  fl;
    double dl;
    uint8_t cc[8];
} PackU;


uint8_t *_pack_fmt_parse(uint8_t *fmt, uint8_t *efmt, int32_t *cnt, int32_t *taf) {

    while (ISSPACE(*fmt) && fmt < efmt) fmt++;

    if (fmt < efmt) {
        switch (*fmt++) {
            case '@':
            case '=':
                *taf = PY_ENDIANNESS;
                break;
            case '<':
                *taf = LITTLE_ENDIAN;
                break;
            case '>':
            case '!':
                *taf = BIG_ENDIAN;
                break;
            default:
                *taf = PY_ENDIANNESS;
                fmt--;
                break;
        }
        *cnt = 0;

        while (*fmt >= '0' && *fmt <= '9' && fmt < efmt) {
            *cnt *= 10;
            *cnt += *fmt - '0';
            fmt++;
        }
        if (fmt < efmt) return fmt;
    }
    return NULL;
}

//pack(fmt,buf,offf,offb,obj)

NATIVE_FN(__pack) {
    NATIVE_UNWARN();
    int tval = PTYPE(args[0]); //fmt
    if (tval != PSTRING && tval != PBYTES && tval != PBYTEARRAY)
        return ERR_TYPE_EXC;
    tval = PTYPE(args[1]);
    if (tval != PSTRING && tval != PBYTES && tval != PBYTEARRAY)
        return ERR_TYPE_EXC;
    CHECK_ARG(args[2], PSMALLINT); //offf
    CHECK_ARG(args[3], PSMALLINT); //offb
    tval = PTYPE(args[4]);

    uint8_t *fmt = PSEQUENCE_BYTES(args[0]);
    uint8_t *sfmt = fmt;
    uint8_t *efmt = fmt + PSEQUENCE_ELEMENTS(args[0]);
    uint8_t *dst = PSEQUENCE_BYTES(args[1]);
    int32_t dstl = PSEQUENCE_ELEMENTS(args[1]);
    uint8_t *edst = dst + dstl;
    fmt += PSMALLINT_VALUE(args[2]);
    dst += (PSMALLINT_VALUE(args[3]) + dstl) % dstl;
    int32_t taf, cnt;
    PackU packu;
    uint8_t *buf;
    int32_t lbuf;
    if (fmt < efmt) {
        fmt = _pack_fmt_parse(fmt, efmt, &cnt, &taf);
        if (!fmt) return ERR_VALUE_EXC;
        if (dst >= edst) return ERR_VALUE_EXC;
        if (tval == PSMALLINT || tval == PINTEGER) {
            packu.i64 = INTEGER_VALUE(args[4]);
            goto expect_int;
        } else if (tval == PFLOAT) {
            packu.dl = (double)FLOAT_VALUE(args[4]);
            goto expect_float;
        } else if (tval == PSTRING || tval == PBYTES || tval == PBYTEARRAY) {
            buf = PSEQUENCE_BYTES(args[4]);
            lbuf = PSEQUENCE_ELEMENTS(args[4]);
            goto expect_buf;
        } else return ERR_TYPE_EXC;
expect_int:
        //debug("__packing at %i with %c and %i %i\r\n", fmt, *fmt, taf, cnt);
        switch (*fmt++) {
            case 'b':
            case 'B':
            case '?':
                *dst = packu.u8;
                break;
            //dst++;
            case 'h':
            case 'H':
                if (taf != PY_ENDIANNESS)
                    packu.u16 = BLTSWAP16(packu.u16);
                if (edst - dst < 2) return ERR_VALUE_EXC;
                memcpy(dst, &packu.u16, 2);
                //dst += 2;
                break;
            case 'i':
            case 'I':
            case 'l':
            case 'L':
                if (taf != PY_ENDIANNESS)
                    packu.u32 = BLTSWAP32(packu.u32);
                if (edst - dst < 4) return ERR_VALUE_EXC;
                memcpy(dst, &packu.u32, 4);
                //dst += 4;
                break;
            case 'q':
            case 'Q':
                if (taf != PY_ENDIANNESS)
                    packu.u64 = BLTSWAP64(packu.u64);
                if (edst - dst < 8) return ERR_VALUE_EXC;
                memcpy(dst, &packu.u64, 8);
                //dst += 8;
                break;
            default:
                return ERR_UNSUPPORTED_EXC;
        }
        goto ret_ok;
expect_float:
        switch (*fmt++) {
            case 'f':
                packu.fl = (float)packu.dl;
                if (edst - dst < 4) return ERR_VALUE_EXC;
                memcpy(dst, &packu.fl, 4);
                //dst += 4;
                break;
            case 'd':
                if (edst - dst < 8) return ERR_VALUE_EXC;
                memcpy(dst, &packu.dl, 8);
                //dst += 8;
                break;
            default:
                return ERR_UNSUPPORTED_EXC;
        }
        goto ret_ok;
expect_buf:
        switch (*fmt++) {
            case 's':
            case 'p':
                if ( (int32_t)(edst - dst) < cnt || cnt > lbuf) return ERR_VALUE_EXC;
                memcpy(dst, buf, cnt);
                //dst += cnt;
                break;
            default:
                return ERR_UNSUPPORTED_EXC;
        }
        goto ret_ok;
    }
ret_ok:
    *res = PSMALLINT_NEW( (int32_t) (fmt - sfmt) );
    return ERR_OK;
}

// _unpack(fmt,buf,offf,offb,lst) --> lst[0] = fmt_p  lst[1]= buf_p
NATIVE_FN(__unpack) {
    NATIVE_UNWARN();
    int tval = PTYPE(args[0]);
    if (tval != PSTRING && tval != PBYTES && tval != PBYTEARRAY)
        return ERR_TYPE_EXC;
    tval = PTYPE(args[1]);
    if (tval != PSTRING && tval != PBYTES && tval != PBYTEARRAY)
        return ERR_TYPE_EXC;
    CHECK_ARG(args[2], PSMALLINT);
    CHECK_ARG(args[3], PSMALLINT);
    PObject *lst = NULL;
    if (nargs == 5) {
        lst = args[4];
        CHECK_ARG(lst, PLIST);
        if (PSEQUENCE_ELEMENTS(lst) != 2)
            return ERR_TYPE_EXC;
    }
    uint8_t *fmt = PSEQUENCE_BYTES(args[0]);
    uint8_t *sfmt = fmt;
    uint8_t *efmt = fmt + PSEQUENCE_ELEMENTS(args[0]);
    fmt += PSMALLINT_VALUE(args[2]);
    uint8_t *dst = PSEQUENCE_BYTES(args[1]);
    uint8_t *edst = dst + PSEQUENCE_ELEMENTS(args[1]);
    uint8_t *sdst = dst;
    dst += PSMALLINT_VALUE(args[3]);
    int32_t taf, cnt;
    PackU packu;
    if (fmt < efmt) {
        if (dst >= edst) return ERR_VALUE_EXC;
        fmt = _pack_fmt_parse(fmt, efmt, &cnt, &taf);
        if (!fmt) return ERR_VALUE_EXC;
        switch (*fmt) {
            case 'b':
            case 'B':
            case '?':
                cnt = sizeof(uint8_t);
                packu.u8 = *dst;
                *res = (PObject *)((*fmt == 'b') ? PSMALLINT_NEW(packu.i8) : PSMALLINT_NEW(packu.u8));
                break;
            case 'h':
            case 'H':
                cnt = sizeof(uint16_t);;
                if ((int32_t)(edst - dst) < cnt) return ERR_VALUE_EXC;
                memcpy(&packu.u16, dst, cnt);
                if (taf != PY_ENDIANNESS)
                    packu.u16 = BLTSWAP16(packu.u16);
                *res = (PObject *)((*fmt == 'h') ? PSMALLINT_NEW(packu.i16) : PSMALLINT_NEW(packu.u16));
                break;
            case 'i':
            case 'I':
            case 'l':
            case 'L':
                cnt = sizeof(uint32_t);;
                if ((int32_t)(edst - dst) < cnt) return ERR_VALUE_EXC;
                memcpy(&packu.u32, dst, cnt);
                if (taf != PY_ENDIANNESS)
                    packu.u32 = BLTSWAP32(packu.u32);
                *res = (PObject *)((*fmt == 'i' || *fmt == 'l') ? pinteger_new(packu.i32) : pinteger_new(packu.u32));
                break;
            case 'q':
            case 'Q':
                cnt = sizeof(uint64_t);;
                if ((int32_t)(edst - dst) < cnt) return ERR_VALUE_EXC;
                memcpy(&packu.u64, dst, cnt);
                if (taf != PY_ENDIANNESS)
                    packu.u64 = BLTSWAP64(packu.u64);
                *res = (PObject *)((*fmt == 'q') ? pinteger_new(packu.i64) : pinteger_new(packu.u64));
                break;
            case 'f':
                cnt = sizeof(float);
                if ((int32_t)(edst - dst) < cnt) return ERR_VALUE_EXC;
                memcpy(&packu.fl, dst, cnt);
                *res = (PObject *)pfloat_new(packu.fl);
                break;
            case 'd':
                cnt = sizeof(double);
                if ((int32_t)(edst - dst) < cnt) return ERR_VALUE_EXC;
                memcpy(&packu.dl, dst, cnt);
                *res = (PObject *)pfloat_new(packu.dl);
                break;
            case 's':
            case 'p':
                if ((int32_t)(edst - dst) < cnt) return ERR_VALUE_EXC;
                *res = (PObject *)((*fmt == 'p') ? pbytes_new(cnt, dst) : pstring_new(cnt, dst));
                break;
            default:
                return ERR_UNSUPPORTED_EXC;
        }
        fmt++;
        dst += cnt;
    }
    if (lst) {
        PLIST_SET_ITEM(lst, 0, PSMALLINT_NEW((int32_t) (fmt - sfmt)));
        PLIST_SET_ITEM(lst, 1, PSMALLINT_NEW((int32_t) (dst - sdst)));
    }
    return ERR_OK;
}

NATIVE_FN(__byte_get) {
    NATIVE_UNWARN();
    int tval = PTYPE(args[0]);
    if (tval != PSTRING && tval != PBYTES && tval != PBYTEARRAY)
        return ERR_TYPE_EXC;
    CHECK_ARG(args[1], PSMALLINT);
    int32_t pos = PSMALLINT_VALUE(args[1]);
    if (pos < 0 || pos >= PSEQUENCE_ELEMENTS(args[0])) return ERR_INDEX_EXC;
    uint8_t *buf = PSEQUENCE_BYTES(args[0]);
    //debug("__byte_get @ %i of %i = %i--%i\r\n", pos, buf, buf[pos], PSMALLINT_NEW(buf[pos]));
    *res = PSMALLINT_NEW(buf[pos]);
    return ERR_OK;
}

NATIVE_FN(__byte_set) {
    NATIVE_UNWARN();
    int tval = PTYPE(args[0]);
    if (tval != PSTRING && tval != PBYTES && tval != PBYTEARRAY)
        return ERR_TYPE_EXC;
    CHECK_ARG(args[1], PSMALLINT);
    CHECK_ARG(args[2], PSMALLINT);
    int32_t pos = PSMALLINT_VALUE(args[1]);
    int32_t bt = PSMALLINT_VALUE(args[2]);
    if (pos < 0 || pos >= PSEQUENCE_ELEMENTS(args[0])) return ERR_INDEX_EXC;
    if (bt < 0 || bt > 255) return ERR_VALUE_EXC;
    uint8_t *buf = PSEQUENCE_BYTES(args[0]);
    buf[pos] = bt;
    *res = P_NONE;
    return ERR_OK;
}

NATIVE_FN(__elements_set) {
    NATIVE_UNWARN();
    if (!IS_PSEQUENCE(args[0]))
        return ERR_TYPE_EXC;
    CHECK_ARG(args[1], PSMALLINT);
    int32_t el = PSMALLINT_VALUE(args[1]);
    if (el < 0 || el > (int32_t)PSEQUENCE_SIZE(args[0])) return ERR_TYPE_EXC;
    PSEQUENCE_ELEMENTS(args[0]) = el;
    *res = P_NONE;
    return ERR_OK;
}



NATIVE_FN(__new_dict) {
    NATIVE_UNWARN();
    CHECK_ARG(args[0], PSMALLINT);
    int32_t n = PSMALLINT_VALUE(args[0]);
    if (n < 0)
        return ERR_TYPE_EXC;
    *res = (PObject *) pdict_new(n);
    return ERR_OK;
}

NATIVE_FN(__new_set) {
    NATIVE_UNWARN();
    CHECK_ARG(args[0], PSMALLINT);
    CHECK_ARG(args[1], PSMALLINT);
    int32_t n = PSMALLINT_VALUE(args[0]);
    int32_t type = PSMALLINT_VALUE(args[1]);
    if (n < 0)
        return ERR_TYPE_EXC;
    if (type != PSET && type != PFSET)
        return ERR_VALUE_EXC;
    *res = (PObject *)pset_new(type, n);
    return ERR_OK;
}


NATIVE_FN(__change_type) {
    NATIVE_UNWARN();
    CHECK_ARG(args[1], PSMALLINT);
    int32_t type = PSMALLINT_VALUE(args[1]);
    args[0]->header.type = type;
    *res = args[0];
    return ERR_OK;
}




/*========================================================================== */
/*====== NATIVE METHODS ==================================================== */
/*========================================================================== */



/* ========================================================================
    SEQUENCE METHODS
   ======================================================================== */


int _buf_index(uint8_t *buf, uint8_t *sub, int32_t lbuf, int32_t lsub) {
    debug( "_buf_index %i %i %i %i\r\n", buf, sub, lbuf, lsub);
    if (lsub > 0) {
        int i;
        for (i = 0; i < (lbuf - lsub + 1); i++) {
            debug( "_buf_index: check %c vs %c\r\n", buf[i], *sub);
            if (buf[i] == *sub) {
                //match 1st
                if (lsub == 1)
                    return i;
                else if (memcmp(buf + i, sub, lsub) == 0) {
                    return i;
                }
            }
        }
    }
    return -1;
}

int _buf_rindex(uint8_t *buf, uint8_t *sub, int32_t lbuf, int32_t lsub) {
    if (lsub > 0) {
        int i;
        for (i = (lbuf - lsub - 1); i > 0; i--) {
            if (buf[i] == *sub) {
                //match 1st
                if (lsub == 1)
                    return i;
                else if (memcmp(buf + i, sub, lsub) == 0) {
                    return i;
                }
            }
        }
    }
    return -1;
}

int _buf_count(uint8_t *buf, uint8_t *sub, int32_t lbuf, int32_t lsub) {
    int i = 0, rr = 0, cp = 0;
    while (i >= 0) {
        i = _buf_index(buf + cp, sub, lbuf - cp, lsub);
        debug( "_buf_count: found at %i of %i next %i sz %i %i %i\r\n", i, rr, cp + i + lsub, lbuf - cp, lsub, cp);
        if (i >= 0) {
            rr++;
            cp += i + lsub;
        }
    }
    return rr;
}

NATIVE_FN(__seq1_count) {
    NATIVE_UNWARN();

    PSequence *seq = (PSequence *)self;
    int tt = PTYPE(args[0]);
    if (!(tt == PSTRING || tt == PBYTES || tt == PBYTEARRAY))
        return ERR_TYPE_EXC;
    PSequence *sub = (PSequence *)args[0];
    int32_t end = PSEQUENCE_ELEMENTS(seq);
    uint8_t *buf1 = PSEQUENCE_BYTES(seq);
    uint8_t *subb = PSEQUENCE_BYTES(sub);
    int32_t subl = PSEQUENCE_ELEMENTS(sub);
    debug( "__seq1_count: searching %i bytes in %i bytes\r\n", subl, end);
    *res = (PObject *) pinteger_new(_buf_count(buf1, subb, end, subl));
    return ERR_OK;
}



NATIVE_FN(__seq1_find) {
    NATIVE_UNWARN();

    PSequence *seq = (PSequence *)self;
    int tt = PTYPE(args[0]);
    uint8_t *subb;
    int32_t subl, stmp;
    if ((tt == PSTRING || tt == PBYTES || tt == PBYTEARRAY)) {
        subb = PSEQUENCE_BYTES(args[0]);
        subl = PSEQUENCE_ELEMENTS(args[0]);
    } else if (tt == PSMALLINT) {
        stmp = PSMALLINT_VALUE(args[0]);
        if (stmp < 0 || stmp > 255)
            return ERR_VALUE_EXC;
        subb = (uint8_t *)(&stmp);
        subl = 1;
    } else
        return ERR_TYPE_EXC;
    int32_t s_start = 0;
    int32_t s_end = PSEQUENCE_ELEMENTS(seq);
    if (nargs >= 2) {
        CHECK_ARG(args[1], PSMALLINT);
        s_start = PSMALLINT_VALUE(args[1]);
    }
    if (nargs >= 3) {
        CHECK_ARG(args[2], PSMALLINT);
        s_end = PSMALLINT_VALUE(args[2]);
    }
    /*TODO: normalize s_start and s_end for slice notation */
    /*TODO: include s_end in the calculation */
    int32_t end = s_end - s_start;
    uint8_t *buf1 = PSEQUENCE_BYTES(seq) + s_start;
    //uint8_t *subb = PSEQUENCE_BYTES(sub);
    //int32_t subl = PSEQUENCE_ELEMENTS(sub);
    debug( "__seq1_find: searching %i bytes in %i bytes starting at %i\r\n", subl, end, s_start);

    int i = _buf_index(buf1, subb, end, subl);
    *res = (i < 0) ? P_M_ONE : PSMALLINT_NEW((i + s_start));
    return ERR_OK;
}


NATIVE_FN(__seq1_index) {
    NATIVE_UNWARN();

    *res = P_ZERO;
    err_t err = ntv____seq1_find(nargs, self, args, res);
    if (err != ERR_OK)
        return err;
    if (*res == P_M_ONE)
        return ERR_VALUE_EXC;
    return ERR_OK;
}


NATIVE_FN(__seq1_strip) {
    NATIVE_UNWARN();

    PSequence *seq = (PSequence *)self;
    PSequence *sub = (PSequence *)P_SPACE_S;
    if (nargs >= 1) {
        int tt = PTYPE(args[0]);
        if (!(tt == PSTRING || tt == PBYTES || tt == PBYTEARRAY))
            return ERR_TYPE_EXC;
        sub = (PSequence *)args[0];
    }
    int32_t dir = 0;
    if (nargs >= 2) {
        CHECK_ARG(args[1], PSMALLINT);
        dir = PSMALLINT_VALUE(args[1]);
    }
    int lstart = 0;
    int rstart = 0;
    int end = PSEQUENCE_ELEMENTS(seq);
    uint8_t *buf1 = PSEQUENCE_BYTES(seq);
    uint8_t *subb = PSEQUENCE_BYTES(sub);
    int32_t subl = PSEQUENCE_ELEMENTS(sub);

    debug( "__seq1_strip: searching %i bytes in %i bytes\r\n", subl, end);
    if (dir >= 0) {
        //lstrip
        for (; lstart < end; lstart++) {
            if (_buf_index(subb, buf1 + lstart, subl, 1) < 0)
                break;
        }
    }

    if (dir <= 0) {
        //rstrip
        for (; rstart < end; rstart++) {
            if (_buf_index(subb, buf1 + end - 1 - rstart, subl, 1) < 0)
                break;
        }
    }

    if (lstart == 0 && rstart == 0)
        *res = (PObject *)seq;
    else {
        dir = end - lstart - rstart;
        PSequence *r = psequence_new(PTYPE(seq), dir);
        r->elements = dir;
        uint8_t *bt = PSEQUENCE_BYTES(r);
        if (bt)
            memcpy(bt, buf1 + lstart, dir);
        *res = (PObject *)r;
    }
    return ERR_OK;
}


NATIVE_FN(__seq1_split) {
    NATIVE_UNWARN();

    PSequence *seq = (PSequence *)self;
    PSequence *sep = NULL;
    int32_t maxsplit = -1;

    if (nargs >= 1) {
        int tt = PTYPE(args[0]);
        if (!(tt == PSTRING || tt == PBYTES || tt == PBYTEARRAY))
            return ERR_TYPE_EXC;
        sep = (PSequence *)args[0];
    }
    if (nargs >= 2) {
        CHECK_ARG(args[1], PSMALLINT);
        maxsplit = PSMALLINT_VALUE(args[1]);
    }

    int end = PSEQUENCE_ELEMENTS(seq);
    uint8_t *buf1 = PSEQUENCE_BYTES(seq);
    uint8_t *subb = PSEQUENCE_BYTES(sep);
    int32_t subl = PSEQUENCE_ELEMENTS(sep);
    debug( "__seq1_split: searching %i bytes in %i bytes\r\n", subl, end);

    if (sep == NULL) {
        //faster alg
        int rs = (buf1[0] != ' ') ? 1 : 0;
        int bsp = 1;
        int i, cp = 0, pos = 0;
        for (i = 0; i < end; i++) {
            if (bsp) {
                //running in not space
                if (buf1[i] == ' ') {
                    bsp = 0;
                }
            } else {
                //running in spaces
                if (buf1[i] != ' ') {
                    bsp = 1;
                    rs++;
                }
            }
        }
        PList *l = (PList *)psequence_new(PLIST, rs);
        l->elements = rs;
        bsp = (buf1[0] == ' ') ? 0 : 1;
        rs = 0;
        for (i = 0; i < end; i++) {
            if (bsp) {
                //running in not space
                if (buf1[i] == ' ') {
                    bsp = 0;
                    cp = i;
                    PString *ss = pstring_new(cp - pos, buf1 + pos);
                    PLIST_SET_ITEM(l, rs, ss);
                    rs++;
                }
            } else {
                //running in space
                if (buf1[i] != ' ') {
                    bsp = 1;
                    pos = i;
                }
            }
        }

        *res = (PObject *) l;
    } else {
        int rs = _buf_count(buf1, subb, end, subl) + 1;
        if (maxsplit < 0) maxsplit = rs;
        debug( "__seq1_split: maxsplit %i rs %i bytes\r\n", maxsplit, rs);
        rs = (maxsplit < rs) ? maxsplit : rs;


        PList *l = (PList *)psequence_new(PLIST, rs);
        int i, pos = 0, cp = 0;
        for (i = 0; i < rs; i++) {
            pos = _buf_index(buf1 + cp, subb, end - cp, subl);
            if (pos < 0)
                pos = end - cp;
            debug( "__seq1_split: substring from %i to %i of %i bytes\r\n", cp, cp + pos, pos);
            PString *ss = pstring_new(pos, buf1 + cp);
            cp += pos + subl;

            PLIST_SET_ITEM(l, i, (PObject *)ss);
        }
        l->elements = rs;
        *res = (PObject *)l;
    }

    return ERR_OK;
}


NATIVE_FN(__seq1_startswith) {
    NATIVE_UNWARN();

    PSequence *seq = (PSequence *)self;
    int tt = PTYPE(args[0]);
    if (!(tt == PSTRING || tt == PBYTES || tt == PBYTEARRAY))
        return ERR_TYPE_EXC;
    PSequence *sub = (PSequence *)args[0];
    int32_t subl = PSEQUENCE_ELEMENTS(sub);
    if (subl > 0) {
        int32_t end = PSEQUENCE_ELEMENTS(seq);
        uint8_t *buf1 = PSEQUENCE_BYTES(seq);
        uint8_t *subb = PSEQUENCE_BYTES(sub);
        debug( "__seq1_startswith: searching %i bytes in %i bytes\r\n", subl, end);
        *res = (_buf_index(buf1, subb, end, subl) == 0) ? P_TRUE : P_FALSE;
    } else *res = P_TRUE;
    return ERR_OK;
}

NATIVE_FN(__seq1_endswith) {
    NATIVE_UNWARN();

    PSequence *seq = (PSequence *)self;
    int tt = PTYPE(args[0]);
    if (!(tt == PSTRING || tt == PBYTES || tt == PBYTEARRAY))
        return ERR_TYPE_EXC;
    PSequence *sub = (PSequence *)args[0];
    int32_t subl = PSEQUENCE_ELEMENTS(sub);
    if (subl > 0) {
        int32_t end = PSEQUENCE_ELEMENTS(seq);
        uint8_t *buf1 = PSEQUENCE_BYTES(seq);
        uint8_t *subb = PSEQUENCE_BYTES(sub);
        debug( "__seq1_endswith: searching %i bytes in %i bytes\r\n", subl, end);
        *res = (_buf_rindex(buf1, subb, end, subl) == (end - subl - 1)) ? P_TRUE : P_FALSE;
    } else *res = P_TRUE;
    return ERR_OK;
}

NATIVE_FN(__seq1_join) {
    NATIVE_UNWARN();

    PSequence *sep = (PSequence *)self;
    int tt = PTYPE(args[0]);
    PIterator *it = piterator_new(args[0]);
    if (!it)
        return ERR_TYPE_EXC;
    int tl = 0, cp = 0;
    int subl = PSEQUENCE_ELEMENTS(sep);
    do {
        PObject *oo = piterator_next(it);
        if (!oo) break;
        tt = PTYPE(oo);
        if (!(tt == PSTRING || tt == PBYTES || tt == PBYTEARRAY))
            return ERR_TYPE_EXC;
        tl += PSEQUENCE_ELEMENTS(oo);
        if (it->current > 1)
            tl += subl;
    } while (1);

    PString *jr = (PString *) psequence_new(PSTRING, tl);
    it = piterator_new(args[0]);
    do {
        PSequence *oo = (PSequence *)piterator_next(it);
        uint8_t *bts = NULL;
        if (!oo) break;
        if (it->current > 1) {
            bts = PSEQUENCE_BYTES(sep);
            if (bts)
                memcpy(jr->seq + cp, bts, subl);
            cp += subl;
        }
        bts = PSEQUENCE_BYTES(oo);
        if (bts)
            memcpy(jr->seq + cp, bts, PSEQUENCE_ELEMENTS(oo));
        cp += PSEQUENCE_ELEMENTS(oo);
    } while (1);
    *res = (PObject *)jr;
    return ERR_OK;
}

NATIVE_FN(__seq1_replace) {
    NATIVE_UNWARN();

    PSequence *seq = (PSequence *)self;
    int tt = PTYPE(args[0]);
    if (!(tt == PSTRING || tt == PBYTES || tt == PBYTEARRAY))
        return ERR_TYPE_EXC;
    PSequence *olds = (PSequence *)args[0];
    tt = PTYPE(args[1]);
    if (!(tt == PSTRING || tt == PBYTES || tt == PBYTEARRAY))
        return ERR_TYPE_EXC;
    PSequence *news = (PSequence *)args[1];

    int32_t oldl = PSEQUENCE_ELEMENTS(olds);
    //int32_t newl = PSEQUENCE_ELEMENTS(olds);
    if (oldl > 0) {
        PList *l;
        ntv____seq1_split(1, (PObject *)self, (PObject **)&olds, (PObject **) &l);
        if (l->elements > 1) {
            PString *rr;
            ntv____seq1_join(1, (PObject *)news, (PObject **)&l, (PObject **) &rr);
            *res = (PObject *) rr;
        } else *res = (PObject *)seq;
    } else *res = (PObject *)seq;
    return ERR_OK;
}

NATIVE_FN(__seq_append) {
    NATIVE_UNWARN();

    PSequence *seq = (PSequence *)self;
    PSequence *xx = NULL;
    debug( "__seq_append: called on %i\r\n", PHEADERTYPE(seq));
    if (PHEADERTYPE(seq) == PBYTEARRAY) {
        CHECK_ARG(args[0], PSMALLINT);
        int32_t x = PSMALLINT_VALUE(args[0]);
        if (x < 0 || x > 255)
            return ERR_TYPE_EXC;
        xx = (PSequence *) pbytes_new(1, (uint8_t *)&x);
    } else if (PHEADERTYPE(seq) == PSHORTARRAY) {
        CHECK_ARG(args[0], PSMALLINT);
        int32_t x = PSMALLINT_VALUE(args[0]);
        if (x < 0 || x > 0xffff)
            return ERR_TYPE_EXC;
        xx = (PSequence *) pshorts_new(1, (uint16_t *)&x);
    } else {
        xx = (PSequence *)ptuple_new(1, args);
    }

    PSlice slice;
    slice.header.type = PSLICE;
    slice.start = PSMALLINT_NEW(PSEQUENCE_ELEMENTS(seq));
    slice.stop = PSMALLINT_NEW(PSEQUENCE_ELEMENTS(seq));
    slice.step = P_ONE;
    *res = P_NONE;
    debug( "__seq_append: calling setslice with\r\n");
    int rr = psequence_setslice(seq, &slice, (PObject *) xx);
    return rr;
}

NATIVE_FN(__seq_insert) {
    NATIVE_UNWARN();

    PSequence *seq = (PSequence *)self;
    PSequence *xx = NULL;
    CHECK_ARG(args[0], PSMALLINT);
    if (PHEADERTYPE(seq) == PBYTEARRAY) {
        CHECK_ARG(args[1], PSMALLINT);
        int32_t x = PSMALLINT_VALUE(args[1]);
        if (x < 0 || x > 255)
            return ERR_TYPE_EXC;
        xx = (PSequence *) pbytes_new(1, (uint8_t *)&x);
    } else if (PHEADERTYPE(seq) == PSHORTARRAY) {
        CHECK_ARG(args[1], PSMALLINT);
        int32_t x = PSMALLINT_VALUE(args[1]);
        if (x < 0 || x > 0xffff)
            return ERR_TYPE_EXC;
        xx = (PSequence *) pshorts_new(1, (uint16_t *)&x);
    } else {
        xx = (PSequence *)ptuple_new(1, &args[1]);
    }
    PSlice slice;
    slice.header.type = PSLICE;
    slice.start = args[0];
    slice.stop = args[0];
    slice.step = P_ONE;
    *res = P_NONE;
    int rr = psequence_setslice(seq, &slice, (PObject *) xx);
    return rr;
}




NATIVE_FN(__seq_extend) {
    NATIVE_UNWARN();

    PSequence *seq = (PSequence *)self;
    PSlice slice;
    slice.header.type = PSLICE;
    slice.start = PSMALLINT_NEW(PSEQUENCE_ELEMENTS(seq));
    slice.stop = PSMALLINT_NEW(PSEQUENCE_ELEMENTS(seq));
    slice.step = P_ONE;
    *res = P_NONE;
    return psequence_setslice(seq, &slice, args[0]);

}

NATIVE_FN(__seq_clear) {
    NATIVE_UNWARN();

    PMutableSequence *seq = (PMutableSequence *)self;
    seq->elements = 0;
    return ERR_OK;
}




PObject *__seq1_upper_lower_helper(PSequence *seq, int lower) {
    uint8_t *buf;
    uint8_t *ebuf;
    if (IS_SEQ_MUTABLE(seq)) {
set_bufs:
        buf = PSEQUENCE_BYTES(seq);
        ebuf = buf + PSEQUENCE_ELEMENTS(seq);
    } else {
        seq = (PSequence *)((PHEADERTYPE(seq) == PSTRING) ? \
                            pstring_new( ((PString *)seq)->elements, (uint8_t *)((PString *)seq)->seq) : \
                            pbytes_new( ((PBytes *)seq)->elements, (uint8_t *)((PBytes *)seq)->seq));
        goto set_bufs;
    }
    while (buf < ebuf) {
        if (lower) {
            if (*buf >= 'A' && *buf <= 'Z')
                *buf = (*buf) + ('a' - 'A');
        } else {
            if (*buf >= 'a' && *buf <= 'z')
                *buf = (*buf) - ('a' - 'A');
        }
        buf++;
    }
    return (PObject *)seq;
}

/*TODO: optimize. Same code! */
NATIVE_FN(__seq1_upper) {
    NATIVE_UNWARN();
    PSequence *seq = (PSequence *)self;
    *res = __seq1_upper_lower_helper(seq, 0);
    return ERR_OK;

}
NATIVE_FN(__seq1_lower) {
    NATIVE_UNWARN();
    PSequence *seq = (PSequence *)self;
    *res = __seq1_upper_lower_helper(seq, 1);
    return ERR_OK;
}
NATIVE_FN(__seq1_format) {
    NATIVE_UNWARN();

    return ERR_NOT_IMPLEMENTED_EXC;
}
NATIVE_FN(__seq_pop) {
    NATIVE_UNWARN();
    return ERR_NOT_IMPLEMENTED_EXC;
}
NATIVE_FN(__seq_remove) {
    NATIVE_UNWARN();
    return ERR_NOT_IMPLEMENTED_EXC;
}
NATIVE_FN(__seq_reverse) {
    NATIVE_UNWARN();
    return ERR_NOT_IMPLEMENTED_EXC;
}
NATIVE_FN(__seq_copy) {
    NATIVE_UNWARN();
    return ERR_NOT_IMPLEMENTED_EXC;
}
NATIVE_FN(__seq_count) {
    NATIVE_UNWARN();
    return ERR_NOT_IMPLEMENTED_EXC;
}
NATIVE_FN(__seq_index) {
    NATIVE_UNWARN();
    return ERR_NOT_IMPLEMENTED_EXC;
}




/* ========================================================================
    THREAD METHODS
   ======================================================================== */


NATIVE_FN(__thread_start) {
    NATIVE_UNWARN();
    PThread *pth = (PThread *)self;
    vosThResume(pth->th);
    return ERR_OK;
}


NATIVE_FN(__thread_join) {
    NATIVE_UNWARN();
    //PThread *pth = (PThread *)self;
    //chThdWait(pth->th);
    return ERR_NOT_IMPLEMENTED_EXC;
}

NATIVE_FN(__thread_status) {
    NATIVE_UNWARN();
    *res = PSMALLINT_NEW(0);
    return ERR_NOT_IMPLEMENTED_EXC;
}


NATIVE_FN(__thread_getprio) {
    NATIVE_UNWARN();
    *res = PSMALLINT_NEW(vosThGetPriority());
    return ERR_OK;
}

NATIVE_FN(__thread_setprio) {
    NATIVE_UNWARN();
    return ERR_NOT_IMPLEMENTED_EXC;
}




/*========================================================================== */
/*====== END OF NATIVES ==================================================== */
/*========================================================================== */



err_t pnone_binary_op(uint32_t op, PObject *a, PObject *b, PObject **res) {
    switch (op) {
        case _BIN_OP(EQ): *res = (a == b) ? (P_TRUE) : (P_FALSE); return ERR_OK;
        case _BIN_OP(NOT_EQ): *res = (a != b) ? (P_TRUE) : (P_FALSE); return ERR_OK;
        case _BIN_OP(L_AND): *res = P_NONE; return ERR_OK;
        case _BIN_OP(L_OR): *res = (b==P_NONE) ? a:b; return ERR_OK;
    }
    return ERR_TYPE_EXC;
}


err_t pgeneric_binary_op(uint32_t op, PObject *a, PObject *b, PObject **res) {

    switch (op) {
        case _BIN_OP(EQ): *res = (a == b) ? (P_TRUE) : (P_FALSE); return ERR_OK;
        case _BIN_OP(NOT_EQ): *res = (a != b) ? (P_TRUE) : (P_FALSE); return ERR_OK;
        case _BIN_OP(L_AND): *res = (pobj_is_true(a) && pobj_is_true(b)) ? P_TRUE:P_FALSE; return ERR_OK;
        case _BIN_OP(L_OR): *res = (pobj_is_true(a)) ? a:b; return ERR_OK;
    }
    return ERR_TYPE_EXC;
}



PObject *netaddress_to_object(NetAddress *addr) {

    PTuple *oip = ptuple_new((addr->port) ? 5 : 4, NULL);
    PTUPLE_SET_ITEM(oip, 0, PSMALLINT_NEW(OAL_IP_AT(addr->ip, 0)));
    PTUPLE_SET_ITEM(oip, 1, PSMALLINT_NEW(OAL_IP_AT(addr->ip, 1)));
    PTUPLE_SET_ITEM(oip, 2, PSMALLINT_NEW(OAL_IP_AT(addr->ip, 2)));
    PTUPLE_SET_ITEM(oip, 3, PSMALLINT_NEW(OAL_IP_AT(addr->ip, 3)));
    if (addr->port)
        PTUPLE_SET_ITEM(oip, 4, PSMALLINT_NEW(OAL_GET_NETPORT(addr->port)));
    return (PObject *)oip;
}

err_t exception_matches(uint16_t name, PObject *ex) {
    uint32_t epos = PEXCEPTION_ERR(ex);
    PException *e = _vm.etable;
    debug( "match exception %i vs %i starting at %i\r\n", name, ex, epos);
    do {
        debug( "checking exception %i vs %i at %i with parent %i\r\n", name, e[epos].name, epos, e[epos].parent);
        if (e[epos].name == name) return 1;
        epos = e[epos].parent;
    } while (epos);
    return (e[epos].name == name) ? 1 : 0;
}


//TODO: optimize
int exception_search(uint16_t name) {
    PException *e = _vm.etable;
    int i;
    for (i = 0; i < _vm.program->nexcp; i++) {
        debug( "searching %i vs %i/%i at %i\r\n", name, e[i].name, e[i].parent, i);
        if (e[i].name == name)
            return i;
    }
    return 0;
}

PSysObject *psysobj_new(uint32_t type) {
    PSysObject *oo;
    oo = ALLOC_OBJ(PSysObject, PSYSOBJ, PFLAG_HASHABLE, 0);
    oo->type = type;
    return oo;
}


err_t unsupported_binary_op_fn(uint32_t op, PObject *a, PObject *b, PObject **r) {
    (void)op;
    (void)a;
    (void)b;
    (void)r;
    return ERR_UNSUPPORTED_EXC;
}

err_t unsupported_unary_op_fn(uint32_t op, PObject *a, PObject **r) {
    (void)op;
    (void)a;
    (void)r;
    return ERR_UNSUPPORTED_EXC;
}

err_t unsupported_subscr_fn(PObject *a, PObject *i, PObject *b, PObject **r) {
    (void)a;
    (void)b;
    (void)i;
    (void)r;
    return ERR_UNSUPPORTED_EXC;
}

err_t unsupported_attr_fn(PObject *a, uint16_t i, PObject *b, PObject **r) {
    (void)a;
    (void)b;
    (void)i;
    (void)r;
    return ERR_UNSUPPORTED_EXC;
}

err_t pdriver_attr_fn(PObject *a, uint16_t i, PObject *b, PObject **r) {
    (void)a;
    (void)b;
    (void)i;
    (void)r;
    if (i == NAME___ctl__) {
        *r = a;
        return ERR_OK;
    }
    return ERR_TYPE_EXC;
}

/*========================================================================== */
/*====== NATIVE METHODS HANDLERS =========================================== */
/*========================================================================== */



/*TODO: optimize search...*/
int find_native_mth(const NativeMth *table, uint16_t name, int size) {
    int i;
    for (i = 0; i < size; i++) {
        if (table[i].name == name)
            return i;
    }
    return -1;
}

PObject *get_native_mth(PObject *self, uint16_t name) {
    int type = PTYPE(self);
    int pos = find_native_mth(_types[type].mths, name, _types[type].nmth);
    if (pos < 0)
        return NULL;
    void *fn = _types[type].mths[pos].fn;
    if (fn) {
        PMethod *mth = pmethod_new(NULL, self);
        mth->fn = (PCallable *)PNATIVE_METHOD_MAKE(pos, type);
        /*
                PCallable *fn = (PCallable *)PNATIVE_METHOD_MAKE(pos, type);
                gc_wait();
                PMethod *mth = pthread_get_method_from_cache(PTHREAD_CURRENT(),self,fn);
                gc_signal();*/
        return (PObject *) mth;
    } else {
        PObject *res;
        err_t err = pmodule_attr_fn((PObject *) VM_BUILTINS(), _types[type].mths[pos].name, P_MARKER, &res);
        if (err == ERR_OK)
            return res;
    }
    return NULL;
}

err_t pmth_attr_fn(PObject *o, uint16_t name, PObject *b, PObject **res) {
    (void)b;
    if (b != P_MARKER)
        return ERR_UNSUPPORTED_EXC;
    *res = get_native_mth(o, name);
    if (!*res)
        return ERR_ATTRIBUTE_EXC;
    /*PMethod *mth = pmethod_new(NULL, o);
    int pos = find_native_mth(_types[tt].mths, name, _types[tt].nmth);
    if (pos < 0) {
        return ERR_ATTRIBUTE_EXC;
    }
    //TODO: if pos>=0 and fn in table==NULL, get name from __builtins__.py and return
    mth->fn = (PCallable *)PNATIVE_METHOD_MAKE(pos, tt);
    *res = (PObject *)mth;
    */
    return ERR_OK;
}


const NativeFn const native_fns[] STORED = {
#include "natives.def"
};


const NativeMth const pstring_mths[] STORED = {
    BUILD_NATIVE_MTH(__seq1_count      , 1,   1,   0, NAME_count),
    BUILD_NATIVE_MTH(__seq1_split      , 2,   0,   0, NAME_split),
    BUILD_NATIVE_MTH(__seq1_strip      , 2,   0,   0, NAME_strip),
    BUILD_NATIVE_MTH(__seq1_index      , 1,   1,   0, NAME_index),
    BUILD_NATIVE_MTH(__seq1_find       , 3,   1,   0, NAME_find),
    BUILD_NATIVE_MTH(__seq1_startswith , 1,   1,   0, NAME_startswith),
    BUILD_NATIVE_MTH(__seq1_endswith   , 1,   1,   0, NAME_endswith),
    BUILD_NATIVE_MTH(__seq1_replace    , 2,   2,   0, NAME_replace),
    BUILD_NATIVE_MTH(__seq1_join       , 1,   1,   0, NAME_join),
    BUILD_NATIVE_MTH(__len             , 0,   0,   0, NAME___len__),
    BUILD_NATIVE_MTH(__seq1_upper      , 0,   0,   0, NAME_upper),
    BUILD_NATIVE_MTH(__seq1_lower      , 0,   0,   0, NAME_lower),
    BUILD_NATIVE_MTH(__seq1_format     , 255,  1,   0, NAME_format),
};

const NativeMth const pbytes_mths [] STORED = {
    BUILD_NATIVE_MTH(__seq1_count      , 1,   1,   0, NAME_count),
    BUILD_NATIVE_MTH(__seq1_split      , 2,   0,   0, NAME_split),
    BUILD_NATIVE_MTH(__seq1_strip      , 2,   0,   0, NAME_strip),
    BUILD_NATIVE_MTH(__seq1_index      , 1,   1,   0, NAME_index),
    BUILD_NATIVE_MTH(__seq1_find       , 3,   1,   0, NAME_find),
    BUILD_NATIVE_MTH(__seq1_startswith , 1,   1,   0, NAME_startswith),
    BUILD_NATIVE_MTH(__seq1_endswith   , 1,   1,   0, NAME_endswith),
    BUILD_NATIVE_MTH(__seq1_replace    , 2,   2,   0, NAME_replace),
    BUILD_NATIVE_MTH(__seq1_join       , 1,   1,   0, NAME_join),
    BUILD_NATIVE_MTH(__len             , 0,   0,   0, NAME___len__),
    BUILD_NATIVE_MTH(__seq1_upper      , 0,   0,   0, NAME_upper),
    BUILD_NATIVE_MTH(__seq1_lower      , 0,   0,   0, NAME_lower),
    BUILD_NATIVE_MTH(__seq1_format     , 255,  1,   0, NAME_format),
};


const NativeMth const pbytearray_mths [] STORED = {
    BUILD_NATIVE_MTH(__seq1_count      , 1,   1,   0, NAME_count),
    BUILD_NATIVE_MTH(__seq1_split      , 2,   0,   0, NAME_split),
    BUILD_NATIVE_MTH(__seq1_strip      , 2,   0,   0, NAME_strip),
    BUILD_NATIVE_MTH(__seq1_index      , 1,   1,   0, NAME_index),
    BUILD_NATIVE_MTH(__seq1_find       , 3,   1,   0, NAME_find),
    BUILD_NATIVE_MTH(__seq1_startswith , 1,   1,   0, NAME_startswith),
    BUILD_NATIVE_MTH(__seq1_endswith   , 1,   1,   0, NAME_endswith),
    BUILD_NATIVE_MTH(__seq1_replace    , 2,   2,   0, NAME_replace),
    BUILD_NATIVE_MTH(__seq1_join       , 1,   1,   0, NAME_join),
    BUILD_NATIVE_MTH(__seq_append      , 1,   1,   0, NAME_append),
    BUILD_NATIVE_MTH(__seq_insert      , 2,   2,   0, NAME_insert),
    BUILD_NATIVE_MTH(__seq_extend      , 1,   1,   0, NAME_extend),
    BUILD_NATIVE_MTH(__seq_pop         , 1,   0,   0, NAME_pop),
    BUILD_NATIVE_MTH(__seq_remove      , 1,   1,   0, NAME_remove),
    BUILD_NATIVE_MTH(__seq_reverse     , 0,   0,   0, NAME_reverse),
    BUILD_NATIVE_MTH(__seq_clear       , 0,   0,   0, NAME_clear),
    BUILD_NATIVE_MTH(__seq_copy        , 0,   0,   0, NAME_copy),
    BUILD_NATIVE_MTH(__len             , 0,   0,   0, NAME___len__),
    BUILD_NATIVE_MTH(__seq1_upper      , 0,   0,   0, NAME_upper),
    BUILD_NATIVE_MTH(__seq1_lower      , 0,   0,   0, NAME_lower),
    BUILD_NATIVE_MTH(__seq1_format     , 255,  1,   0, NAME_format),
};


const NativeMth const pshorts_mths [] STORED = {
    BUILD_NATIVE_MTH(__seq_count , 1,   1,   0, NAME_count),
    BUILD_NATIVE_MTH(__seq_index , 1,   1,   0, NAME_index),
    BUILD_NATIVE_MTH(__len       , 0,   0,   0, NAME___len__),
};


const NativeMth const pshortarray_mths [] STORED = {
    BUILD_NATIVE_MTH(__seq_count   , 1,   1,   0, NAME_count),
    BUILD_NATIVE_MTH(__seq_index   , 1,   1,   0, NAME_index),
    BUILD_NATIVE_MTH(__seq_append  , 1,   1,   0, NAME_append),
    BUILD_NATIVE_MTH(__seq_insert  , 2,   2,   0, NAME_insert),
    BUILD_NATIVE_MTH(__seq_extend  , 1,   1,   0, NAME_extend),
    BUILD_NATIVE_MTH(__seq_pop     , 1,   0,   0, NAME_pop),
    BUILD_NATIVE_MTH(__seq_remove  , 1,   1,   0, NAME_remove),
    BUILD_NATIVE_MTH(__seq_reverse , 0,   0,   0, NAME_reverse),
    BUILD_NATIVE_MTH(__seq_clear   , 0,   0,   0, NAME_clear),
    BUILD_NATIVE_MTH(__seq_copy    , 0,   0,   0, NAME_copy),
    BUILD_NATIVE_MTH(__len         , 0,   0,   0, NAME___len__),
};


const NativeMth const plist_mths [] STORED = {
    BUILD_NATIVE_MTH(__seq_count   , 1,   1,   0, NAME_count),
    BUILD_NATIVE_MTH(__seq_index   , 1,   1,   0, NAME_index),
    BUILD_NATIVE_MTH(__seq_append  , 1,   1,   0, NAME_append),
    BUILD_NATIVE_MTH(__seq_insert  , 2,   2,   0, NAME_insert),
    BUILD_NATIVE_MTH(__seq_extend  , 1,   1,   0, NAME_extend),
    BUILD_NATIVE_MTH(__seq_pop     , 1,   0,   0, NAME_pop),
    BUILD_NATIVE_MTH(__seq_remove  , 1,   1,   0, NAME_remove),
    BUILD_NATIVE_MTH(__seq_reverse , 0,   0,   0, NAME_reverse),
    BUILD_NATIVE_MTH(__seq_clear   , 0,   0,   0, NAME_clear),
    BUILD_NATIVE_MTH(__seq_copy    , 0,   0,   0, NAME_copy),
    BUILD_NATIVE_MTH(__len         , 0,   0,   0, NAME___len__),
};

const NativeMth const ptuple_mths [] STORED = {
    BUILD_NATIVE_MTH(__seq_count , 1,   1,   0, NAME_count),
    BUILD_NATIVE_MTH(__seq_index , 1,   1,   0, NAME_index),
    BUILD_NATIVE_MTH(__len       , 0,   0,   0, NAME___len__),
};


const NativeMth const prange_mths [] STORED = {
    BUILD_NATIVE_MTH(__seq_count , 1,   1,   0, NAME_count),
    BUILD_NATIVE_MTH(__seq_index , 1,   1,   0, NAME_index),
    BUILD_NATIVE_MTH(__len       , 0,   0,   0, NAME___len__),
};


const NativeMth const pfset_mths [] STORED = {
    BUILD_NATIVE_MTH(__hash_copy                , 0,   0,   0, NAME_copy),
    BUILD_NATIVE_MTH(__len                      , 0,   0,   0, NAME___len__),
    BUILD_NATIVE_MTH(__set_difference           , 255,   1,   0, NAME_difference),
    BUILD_NATIVE_MTH(__set_intersection         , 255,   1,   0, NAME_intersection),
    BUILD_NATIVE_MTH(__set_isdisjoint           , 1,   1,   0, NAME_isdisjoint),
    BUILD_NATIVE_MTH(__set_issubset             , 1,   1,   0, NAME_issubset),
    BUILD_NATIVE_MTH(__set_issuperset           , 1,   1,   0, NAME_issuperset),
    BUILD_NATIVE_MTH(__set_symmetric_difference , 1,   1,   0, NAME_symmetric_difference),
    BUILD_NATIVE_MTH(__set_union                , 255,   1,   0, NAME_union),
};

const NativeMth const pset_mths [] STORED = {
    BUILD_NATIVE_MTH(__set_pop                         , 0,   0,   0, NAME_pop),
    BUILD_NATIVE_MTH(__set_remove                      , 1,   1,   0, NAME_remove),
    BUILD_NATIVE_MTH(__hash_clear                      , 0,   0,   0, NAME_clear),
    BUILD_NATIVE_MTH(__hash_copy                       , 0,   0,   0, NAME_copy),
    BUILD_NATIVE_MTH(__len                             , 0,   0,   0, NAME___len__),
    BUILD_NATIVE_MTH(__set_update                      , 255,   1,   0, NAME_update),
    BUILD_NATIVE_MTH(__set_difference                  , 255,   1,   0, NAME_difference),
    BUILD_NATIVE_MTH(__set_intersection                , 255,   1,   0, NAME_intersection),
    BUILD_NATIVE_MTH(__set_isdisjoint                  , 1,   1,   0, NAME_isdisjoint),
    BUILD_NATIVE_MTH(__set_issubset                    , 1,   1,   0, NAME_issubset),
    BUILD_NATIVE_MTH(__set_issuperset                  , 1,   1,   0, NAME_issuperset),
    BUILD_NATIVE_MTH(__set_symmetric_difference        , 1,   1,   0, NAME_symmetric_difference),
    BUILD_NATIVE_MTH(__set_union                       , 255,   1,   0, NAME_union),
    BUILD_NATIVE_MTH(__set_intersection_update         , 255,   1,   0, NAME_intersection_update),
    BUILD_NATIVE_MTH(__set_difference_update           , 255,   1,   0, NAME_difference_update),
    BUILD_NATIVE_MTH(__set_symmetric_difference_update , 1,   1,   0, NAME_symmetric_difference_update),
    BUILD_NATIVE_MTH(__set_add                         , 1,   1,   0, NAME_add),
    BUILD_NATIVE_MTH(__set_discard                     , 1,   1,   0, NAME_discard),
};



const NativeMth const pdict_mths [] STORED = {
    BUILD_NATIVE_MTH(__dict_pop                        , 2,   1,   0, NAME_pop),
    BUILD_NATIVE_MTH(__hash_clear                      , 0,   0,   0, NAME_clear),
    BUILD_NATIVE_MTH(__hash_copy                       , 0,   0,   0, NAME_copy),
    BUILD_NATIVE_MTH(__len                             , 0,   0,   0, NAME___len__),
    BUILD_NATIVE_MTH(__dict_get                        , 2,   1,   0, NAME_get),
    BUILD_NATIVE_MTH(__dict_keys                       , 0,   0,   0, NAME_keys),
    BUILD_NATIVE_MTH(__dict_items                      , 0,   0,   0, NAME_items),
    BUILD_NATIVE_MTH(__dict_values                     , 0,   0,   0, NAME_values),
    BUILD_NATIVE_MTH(__dict_update                     , 1,   1,   0, NAME_update),
    BUILD_NATIVE_MTH(__dict_popitem                    , 1,   1,   0, NAME_popitem),
};


const NativeMth const pthread_mths [] STORED = {
    BUILD_NATIVE_MTH(__thread_join                     , 0,   0,   0, NAME_join),
    BUILD_NATIVE_MTH(__thread_start                    , 0,   0,   0, NAME_start),
    BUILD_NATIVE_MTH(__thread_setprio                  , 1,   1,   0, NAME_set_priority),
    BUILD_NATIVE_MTH(__thread_getprio                  , 0,   0,   0, NAME_get_priority),
    BUILD_NATIVE_MTH(__thread_status                   , 0,   0,   0, NAME_get_status),
};




const Type const _types[] STORED = {
    /* PSMALLINT  */ {pnumber_binary_op        , unsupported_attr_fn     , unsupported_subscr_fn  , 0                  , NULL}       ,
    /* PINTEGER   */ {pnumber_binary_op        , unsupported_attr_fn , unsupported_subscr_fn  , 0                  , NULL}       ,
    /* PFLOAT     */ {pnumber_binary_op        , unsupported_attr_fn , unsupported_subscr_fn  , 0                  , NULL}       ,
    /* PBOOL      */ {pnumber_binary_op        , unsupported_attr_fn , unsupported_subscr_fn  , 0                  , NULL}       ,
    /* PSTRING    */ {psequence_binary_op      , pmth_attr_fn        , psequence_subscr       , _NMTH(pstring)}    ,
    /* PBYTES     */ {psequence_binary_op      , pmth_attr_fn        , psequence_subscr       , _NMTH(pbytes)}     ,
    /* PBYTEARRAY */ {psequence_binary_op      , pmth_attr_fn        , psequence_subscr       , _NMTH(pbytearray)} ,
    /* PSHORTS    */ {psequence_binary_op      , pmth_attr_fn        , psequence_subscr       , _NMTH(pshorts)}    ,
    /* PSHORTARRAY*/ {psequence_binary_op      , pmth_attr_fn        , psequence_subscr       , _NMTH(pshortarray)},
    /* PLIST      */ {psequence_binary_op      , pmth_attr_fn        , psequence_subscr       , _NMTH(plist)}      ,
    /* PTUPLE     */ {psequence_binary_op      , pmth_attr_fn        , psequence_subscr       , _NMTH(ptuple)}     ,
    /* PRANGE     */ {psequence_binary_op      , pmth_attr_fn        , psequence_subscr       , _NMTH(prange)}     ,
    /* PFSET      */ {pset_binary_op           , pmth_attr_fn        , unsupported_subscr_fn  , _NMTH(pfset)}      ,
    /* PSET       */ {pset_binary_op           , pmth_attr_fn        , unsupported_subscr_fn  , _NMTH(pset)}       ,
    /* PDICT      */ {pdict_binary_op          , pmth_attr_fn        , pdict_subscr_fn        , _NMTH(pdict)}      ,
    /* PFUNCTION  */ {unsupported_binary_op_fn , pmth_attr_fn        , unsupported_subscr_fn  , 0                  , NULL}       ,
    /* PMETHOD    */ {unsupported_binary_op_fn , unsupported_attr_fn , unsupported_subscr_fn  , 0                  , NULL}       ,
    /* PCLASS     */ {unsupported_binary_op_fn , pclass_attr_fn      , unsupported_subscr_fn  , 0                  , NULL}       ,
    /* PINSTANCE  */ {unsupported_binary_op_fn , pinstance_attr_fn   , unsupported_subscr_fn  , 0                  , NULL}       ,
    /* PMODULE    */ {unsupported_binary_op_fn , pmodule_attr_fn     , unsupported_subscr_fn  , 0                  , NULL}       ,
    /* PBUFFER    */ {unsupported_binary_op_fn , unsupported_attr_fn , unsupported_subscr_fn  , 0                  , NULL}       ,
    /* PSLICE     */ {unsupported_binary_op_fn , unsupported_attr_fn , unsupported_subscr_fn  , 0                  , NULL}       ,
    /* PITERATOR  */ {unsupported_binary_op_fn , unsupported_attr_fn , unsupported_subscr_fn  , 0                  , NULL}       ,
    /* PFRAME     */ {unsupported_binary_op_fn , unsupported_attr_fn , unsupported_subscr_fn  , 0                  , NULL}       ,
    /* PBLOCK     */ {unsupported_binary_op_fn , unsupported_attr_fn , unsupported_subscr_fn  , 0                  , NULL}       ,
    /* PNONE      */ {pnone_binary_op          , unsupported_attr_fn , unsupported_subscr_fn  , 0                  , NULL}       ,
    /* PEXCEPTION */ {unsupported_binary_op_fn , unsupported_attr_fn , unsupported_subscr_fn  , 0                  , NULL}       ,
    /* PNATIVE    */ {unsupported_binary_op_fn , unsupported_attr_fn , unsupported_subscr_fn  , 0                  , NULL}       ,
    /* PSYSOBJ    */ {unsupported_binary_op_fn , unsupported_attr_fn , unsupported_subscr_fn  , 0                  , NULL}       ,
    /* PDRIVER    */ {unsupported_binary_op_fn , pdriver_attr_fn     , unsupported_subscr_fn  , 0                  , NULL}       ,
    /* PTHREAD    */ {unsupported_binary_op_fn , pmth_attr_fn        , unsupported_subscr_fn  , _NMTH(pthread)}    ,
    /* NATIVES    */ {unsupported_binary_op_fn , unsupported_attr_fn , unsupported_subscr_fn  , 0                  , native_fns}
};

