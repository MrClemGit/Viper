#include "hal.h"
#include "vbl.h"
#include "port.h"
#include "oal_conf.h"
#include "lang.h"


/*
    fmt:
        'i' : PSMALLINT
        's' : PSTRING
        'b' : PBYTEARRAY
        'n' : NetAddress
        'f' : float

*/


int parse_py_args(const char *fmt, int nargs, PObject **args, ...) {
    uint32_t conv = 0;
    int32_t cur;
    int32_t *ival;
    int32_t idef;
    uint8_t **sval;
    uint8_t *sdef;
    double fdef;
    double *fval;
    NetAddress *addr;
    int tt;
    va_list vl;
    va_start(vl, args);
    for (cur = 0; cur < nargs; cur++, fmt++) {
        tt = PTYPE(args[cur]);
        switch (*fmt) {
            case 'I':
                idef = va_arg(vl, int32_t);
            case 'i':
                if (tt != PSMALLINT) goto ppy_err;
                ival = va_arg(vl, int32_t *);
                *ival = PSMALLINT_VALUE(args[cur]);
                break;
            case 'S':
                sdef = va_arg(vl, uint8_t *);
            case 's':
                if (!IS_BYTE_PSEQUENCE_TYPE(tt)) goto ppy_err;
                sval = va_arg(vl, uint8_t **);
                *sval = PSEQUENCE_BYTES(args[cur]);
                ival = va_arg(vl, int32_t *);
                *ival = PSEQUENCE_ELEMENTS(args[cur]);
                break;
            case 'B':
                sdef = va_arg(vl, uint8_t *);
            case 'b':
                if (!IS_BYTE_PSEQUENCE_TYPE(tt)) goto ppy_err;
                sval = va_arg(vl, uint8_t **);
                *sval = PSEQUENCE_BYTES(args[cur]);
                ival = va_arg(vl, int32_t *);
                *ival = PSEQUENCE_SIZE(args[cur]);
                break;
            case 'N':
                ival = va_arg(vl, int32_t *);
                *ival = 1; //signal that the optional netaddres was given (if errors 'n' will raise exc)
            case 'n':
                if (tt != PTUPLE || PSEQUENCE_ELEMENTS(args[cur]) < 4 || PSEQUENCE_ELEMENTS(args[cur]) > 5) goto ppy_err;
                addr = va_arg(vl, NetAddress *);
                for (idef = 3; idef >= 0; idef--) {
                    PObject *ti = PTUPLE_ITEM(args[cur], idef);
                    if (PTYPE(ti) != PSMALLINT || PSMALLINT_VALUE(ti) < 0 || PSMALLINT_VALUE(ti) > 255)
                        goto ppy_err;
                    OAL_IP_SET(addr->ip, idef, PSMALLINT_VALUE(ti));
                }
                if (PSEQUENCE_ELEMENTS(args[cur]) == 5) {
                    idef = 4;
                    PObject *ti = PTUPLE_ITEM(args[cur], idef);
                    if (PTYPE(ti) != PSMALLINT || PSMALLINT_VALUE(ti) < 0)
                        goto ppy_err;
                    OAL_SET_NETPORT(addr->port, PSMALLINT_VALUE(ti));
                }
                break;
            case 'F':
                fdef = va_arg(vl, double);
            case 'f':
                if (tt != PFLOAT) goto ppy_err;
                fval = va_arg(vl, double *);
                *((FLOAT_TYPE *)fval) = FLOAT_VALUE(args[cur]);
                break;

            default:
                goto ppy_err;
        }
        conv++;
    }
    while (*fmt) {
        switch (*fmt) {
            case 'I':
                idef = va_arg(vl, int32_t);
                ival = va_arg(vl, int32_t *);
                *ival = idef;
                break;
            case 'S' :
                sdef = va_arg(vl, uint8_t *);
                sval = va_arg(vl, uint8_t **);
                *sval = sdef;
                ival = va_arg(vl, int32_t *);
                *ival = strlen((char *)sdef);
                break;
            case 'N': //optional address defaults are not possible. Instead we signal that addres was not given
                ival = va_arg(vl, int32_t *);
                *ival = 0;
                va_arg(vl, NetAddress *);
                break;
            case 'F':
                fdef = va_arg(vl, double);
                fval = va_arg(vl, double *);
                *((FLOAT_TYPE *)fval) = (FLOAT_TYPE)fdef;
                break;
            default:
                goto ppy_err;
        }
        fmt++;
        conv++;
    }
    va_end(vl);
    return conv;
ppy_err:
    va_end(vl);
    return -1;
}


int parse_py_arg(PObject **var, int ptype, int *nargs, PObject *** args, PObject *defval) {
    if ((*nargs) > 0) {
        PObject *arg = *(*args);
        if (PTYPE(arg) != (uint32_t)ptype) return ERR_TYPE_EXC;
        *var = arg;
        (*nargs)--; (*args)++;
        return ERR_OK;
    }
    if (defval) {
        *var = defval;
        return ERR_OK;
    }
    return ERR_TYPE_EXC;
}


/*====== TIME ===================================================== */

/*
uint32_t millis() {
    uint32_t ticks = chTimeNow();
    return ((ticks - 1) * 1000 + 1) / ((uint32_t)CH_FREQUENCY);
}

uint32_t micros() {
    uint32_t clocks = halGetCounterValue() / (halGetCounterFrequency() / 1000000);
    return clocks;
}
*/


/*====== DRIVER MAP ================================================== */


err_t oal_unsupported_prph(int nargs, PObject *self, PObject **args, PObject **res) {
    (void)nargs;
    (void)self;
    (void)args;
    (void)res;
    return ERR_NOT_IMPLEMENTED_EXC;
}

extern err_t _spi_ctl(int, PObject *, PObject **, PObject **) ALIASED(oal_unsupported_prph);
extern err_t _i2c_ctl(int, PObject *, PObject **, PObject **) ALIASED(oal_unsupported_prph);
extern err_t _can_ctl(int, PObject *, PObject **, PObject **) ALIASED(oal_unsupported_prph);
extern err_t _dac_ctl(int, PObject *, PObject **, PObject **) ALIASED(oal_unsupported_prph);
extern err_t _sdc_ctl(int, PObject *, PObject **, PObject **) ALIASED(oal_unsupported_prph);
extern err_t _htm_ctl(int, PObject *, PObject **, PObject **) ALIASED(oal_unsupported_prph);
extern err_t _rtc_ctl(int, PObject *, PObject **, PObject **) ALIASED(oal_unsupported_prph);
extern err_t _cryp_ctl(int, PObject *, PObject **, PObject **) ALIASED(oal_unsupported_prph);
extern err_t _nfo_ctl(int, PObject *, PObject **, PObject **) ALIASED(oal_unsupported_prph);
extern err_t _rng_ctl(int, PObject *, PObject **, PObject **) ALIASED(oal_unsupported_prph);

err_t _ser_ctl(int nargs, PObject *self, PObject **args, PObject **res) {
    (void)self;
    int32_t code;
    int32_t drvid;
    *res = P_NONE;
    PARSE_PY_ARGCODE(code);
    PARSE_PY_INT(drvid);

    switch (code) {
        case DRV_CMD_INIT: {
            uint32_t baud;
            uint32_t parity;
            uint32_t stop;
            uint32_t bits;

            if (parse_py_args("IIII", nargs, args
                              , 115200, &baud
                              , SERIAL_PARITY_NONE, &parity
                              , SERIAL_STOP_ONE, &stop
                              , SERIAL_BITS_8, &bits
                             ) != 4) goto ret_err_type;

            if (vhalSerialInit(drvid, baud, parity, stop, bits, VM_RXPIN(drvid), VM_TXPIN(drvid))) {
                return ERR_UNSUPPORTED_EXC;
            }
        }
        break;

        case DRV_CMD_WRITE:{
            uint8_t *buf;
            uint32_t len;
            int32_t timeout;
            if ((parse_py_args("sI", nargs, args, &buf, &len, VTIME_INFINITE,
                               &timeout)) != 2) goto ret_err_type;
            if (len <= 0) {
                *res = P_ZERO;
                goto ret_ok;
            }
            RELEASE_GIL();
            len = vhalSerialWrite(drvid, buf, len);
            ACQUIRE_GIL();
            *res = PSMALLINT_NEW(len);
            goto ret_ok;

        }
        break;
        case DRV_CMD_READ: {
            uint8_t *buf;
            int32_t blen;
            int32_t len;
            int32_t ofs;
            int32_t timeout;
            if ((parse_py_args("siII", nargs, args, &buf, &blen, &len, 0, &ofs, VTIME_INFINITE,
                               &timeout)) != 4) goto ret_err_type;
            blen-=ofs;
            if (len <= 0 || blen<=0) {
                *res = P_ZERO;
                goto ret_ok;
            }
            RELEASE_GIL();
            len = vhalSerialRead(drvid, buf+ofs, MIN(blen, len));
            ACQUIRE_GIL();
            *res = PSMALLINT_NEW(len);
            goto ret_ok;
        }
        break;
        case DRV_CMD_AVAILABLE: {
            *res = PSMALLINT_NEW(vhalSerialAvailable(drvid));
        }
        break;
        case DRV_CMD_DONE: {
            if (vhalSerialDone(drvid)) {
                return ERR_UNSUPPORTED_EXC;
            }
        }
        break;
    }
ret_ok:
    return ERR_OK;
ret_err_type:
    return ERR_TYPE_EXC;
}


err_t _adc_ctl(int nargs, PObject *self, PObject **args, PObject **res) {
    (void)self;
    int32_t code;
    int32_t drvid;
    *res = P_NONE;
    uint16_t *pins = NULL;
    PARSE_PY_ARGCODE(code);
    PARSE_PY_INT(drvid);

    switch (code) {
        case DRV_CMD_INIT: {
            vhalAdcConf conf;
            debug( "adc init: %i %i %i\n", drvid, conf.samples_per_second, nargs);
            if (parse_py_args("i", nargs, args
                              , &conf.samples_per_second) != 1) goto ret_err_type;

            if (vhalAdcInit(drvid, &conf)) {
                return ERR_UNSUPPORTED_EXC;
            }
        }
        break;

        case DRV_CMD_READ: {
            vhalAdcCaptureInfo nfo;
            uint16_t pin;
            nfo.capture_mode =  ADC_CAPTURE_SINGLE;
            int tt = PTYPE(args[0]);
            if (tt == PSMALLINT) {
                pin = PSMALLINT_VALUE(args[0]);
                nfo.pins = &pin;
                nfo.npins = 1;
            } else if (tt == PLIST || tt == PTUPLE) {

                nfo.npins = PSEQUENCE_ELEMENTS(args[0]);
                pins = gc_malloc(sizeof(uint16_t) * nfo.npins);
                PObject **vpins = PSEQUENCE_OBJECTS(args[0]);
                debug( "multi adc %i\n", nfo.npins);
                for (pin = 0; pin < nfo.npins; pin++) {
                    if (!(IS_PSMALLINT(vpins[pin]))) {
                        goto ret_err_type;
                    }
                    pins[pin] = PSMALLINT_VALUE(vpins[pin]);
                    debug( "pin %i\n", pins[pin]);
                }
                nfo.pins = pins;
            }
            if (!IS_PSMALLINT(args[1])) {
                goto ret_err_type;
            } else {
                nfo.samples = PSMALLINT_VALUE(args[1]);
            }
            code = vhalAdcPrepareCapture(drvid, &nfo);
            debug( "after prepare capture %i %i %i\n", code, nfo.samples, nfo.npins);
            if (code < 0)
                goto ret_unsup;
            if (code <= 2) {
                nfo.buffer = &pin;
            } else {
                nfo.buffer = gc_malloc(code);
            }
            RELEASE_GIL();
            vhalAdcRead(drvid, &nfo);
            ACQUIRE_GIL();
            if (nfo.samples == 1 && nfo.npins == 1) {
                *res = PSMALLINT_NEW(pin);
            } else {
                if (nfo.npins == 1) {
                    PTuple *tpl = pshorts_new(nfo.samples, (uint16_t *)nfo.buffer);
                    *res = (PObject *)tpl;
                    gc_free(nfo.buffer);
                } else {
                    PObject *gbl;
                    if (nfo.samples == 1) {
                        gbl = (PObject *)pshorts_new(nfo.npins, (uint16_t *)nfo.buffer);
                    } else {
                        gbl = (PObject *) ptuple_new(nfo.npins, NULL);
                        for (pin = 0; pin < nfo.npins; pin++) {
                            PObject *tpl = (PObject *)pshorts_new(nfo.samples, NULL);
                            for (drvid = 0; drvid < (int32_t)nfo.samples; drvid++) {
                                code = *(((uint16_t *)nfo.buffer) + (nfo.npins * drvid) + pin);
                                PSHORTS_SET_ITEM(tpl, drvid, code);
                            }
                            PTUPLE_SET_ITEM(gbl, pin, tpl);
                        }
                    }
                    *res = (PObject *)gbl;
                    gc_free(nfo.buffer);
                }
            }
            goto ret_ok;
        }
        break;
        default:
            goto ret_unsup;
    }
ret_ok:
    if (pins) gc_free(pins);
    return ERR_OK;
ret_err_type:
    if (pins) gc_free(pins);
    return ERR_TYPE_EXC;
ret_unsup:
    if (pins) gc_free(pins);
    return ERR_UNSUPPORTED_EXC;

}


err_t _pwm_ctl(int nargs, PObject *self, PObject **args, PObject **res) {
    (void)self;
    int32_t code;
    *res = P_NONE;
    PARSE_PY_ARGCODE(code);

    switch (code) {
        case DRV_CMD_INIT: {
            // Nothing to do
        }
        break;

        case DRV_CMD_WRITE: {
            uint32_t pin = 0;
            uint32_t period = 0;
            uint32_t pulse = 0;
            uint32_t time_unit = 0;
            if (parse_py_args("iiii", nargs, args, &pin, &period, &pulse, &time_unit) != 4) goto ret_err_type;
            period = TIME_U(period, time_unit);
            pulse = TIME_U(pulse, time_unit);
            code = vhalPwmStart(pin, period, pulse);
            if (code < 0)
                return -code;
        }
        break;
        default:
            goto ret_unsup;
    }
    return ERR_OK;
ret_err_type:
    return ERR_TYPE_EXC;
ret_unsup:
    return ERR_UNSUPPORTED_EXC;

}


void _icu_endcbk(void *vconf) {
    vhalIcuConf *conf = (vhalIcuConf *)vconf;
    PTuple *tpl = ptuple_new(conf->bufsize, NULL);
    PObject *callback = (PObject *)conf->args;
    int i;
    for (i = 0; i < conf->bufsize; i++) {
        PTUPLE_SET_ITEM(tpl, i, PSMALLINT_NEW(conf->buffer[i]));
    }
    //free and call
    gc_free(conf->buffer);
    gc_free(conf);
    vm_add_irq(callback, (PObject *)tpl);
    GC_START_STAGING();
    GC_UNSTAGE(callback);
    GC_STOP_STAGING();
}

err_t _icu_ctl(int nargs, PObject *self, PObject **args, PObject **res) {
    (void)self;
    int32_t code;
    *res = P_NONE;
    PARSE_PY_ARGCODE(code);

    switch (code) {
        case DRV_CMD_INIT: {
            // Nothing to do
        }
        break;

        case DRV_CMD_READ: {
            PObject *callback = NULL;
            int32_t time_window = 0;
            int32_t time_unit = 0;
            int32_t trigger = 0;
            int32_t samples = 0;
            int32_t pin;
            int32_t pull;

            if (parse_py_args("iiiiii", nargs - 1, args, &pin, &trigger, &samples, &time_window, &time_unit,&pull) != 6) goto ret_err_type;
            if (samples < 0 || samples > 0xffff || time_unit < MICROS || time_unit > SECONDS || time_window < 0)
                goto ret_err_type;
            vhalIcuConf *conf = (vhalIcuConf *) gc_malloc(sizeof(vhalIcuConf));
            conf->time_window = TIME_U(time_window, time_unit);
            conf->cfg = ICU_CFG(ICU_MODE_BOTH, (trigger) ? ICU_TRIGGER_HIGH : ICU_TRIGGER_LOW, 0,(pull) ? ICU_INPUT_PULLUP:ICU_INPUT_PULLDOWN);
            conf->bufsize = samples;
            conf->callback = NULL;
            conf->buffer = (uint32_t *) gc_malloc(sizeof(uint32_t) * conf->bufsize);

            callback = args[nargs - 1];
            //printf("%x\n", callback);
            if (callback == P_NONE) {
                //blocking
                conf->endcbk = NULL;
                //printf("Starting ICU...\n");
                RELEASE_GIL();
                code = vhalIcuStart(pin, conf);
                ACQUIRE_GIL();
                //printf("Finished ICU...\n");
                if (code < 0) {
                    code = -code;
                    gc_free(conf->buffer);
                    gc_free(conf);
                    goto ret_err;
                } else {
                    PTuple *tpl = ptuple_new(conf->bufsize, NULL);
                    for (trigger = 0; trigger < conf->bufsize; trigger++) {
                        PTUPLE_SET_ITEM(tpl, trigger, PSMALLINT_NEW(conf->buffer[trigger]));
                    }
                    *res = (PObject *)tpl;
                    gc_free(conf->buffer);
                    gc_free(conf);
                    goto ret_ok;
                }
            } else {
                //non-blocking
                conf->endcbk = _icu_endcbk;
                GC_START_STAGING();
                GC_STAGE(callback);
                GC_STOP_STAGING();
                conf->args = (void *)callback;
                code = vhalIcuStart(pin, conf);
                if (code < 0) {
                    code = -code;
                    gc_free(conf->buffer);
                    gc_free(conf);
                    goto ret_err;
                }
            }
        }
        break;
        default:
            goto ret_unsup;
    }
ret_ok:
    return ERR_OK;
ret_err_type:
    return ERR_TYPE_EXC;
ret_unsup:
    return ERR_UNSUPPORTED_EXC;
ret_err:
    return code;

}



const Driver _drivers[] STORED = {
    {PRPH_ADC, _adc_ctl},
    {PRPH_SPI, _spi_ctl},
    {PRPH_I2C, _i2c_ctl},
    {PRPH_SER, _ser_ctl},
    {PRPH_PWM, _pwm_ctl},
    {PRPH_ICU, _icu_ctl},
    {PRPH_CAN, _can_ctl},
    {PRPH_DAC, _dac_ctl},
    {PRPH_SDC, _sdc_ctl},
    {PRPH_HTM, _htm_ctl},
    {PRPH_RTC, _rtc_ctl},
    {PRPH_CRYP, _cryp_ctl},
    {PRPH_NFO, _nfo_ctl},
    {PRPH_RNG, _rng_ctl},
};

const uint8_t _ndrivers = sizeof(_drivers) / sizeof(Driver);

int get_driver(uint32_t id) {
    int i;
    int res = -1;
    int prph_n = id & 0xff;
    id = id >> 8;
    for (i = 0; i < _ndrivers; i++)  {
        if (_drivers[i].id == id) {
            switch (id) {
                case PRPH_ADC:
                    if (PERIPHERAL_NUM(adc) <= prph_n) return -1;
                    break;
                case PRPH_SPI:
                    if (PERIPHERAL_NUM(spi) <= prph_n) return -1;
                    break;
                case PRPH_SER:
                    if (PERIPHERAL_NUM(serial) <= prph_n) return -1;
                    break;
                case PRPH_PWM:
                    if (PERIPHERAL_NUM(pwm) <= prph_n) return -1;
                    break;
                case PRPH_ICU:
                    if (PERIPHERAL_NUM(icu) <= prph_n) return -1;
                    break;
                default:
                    return - 1;
            }
            res = i;
            break;
        }
    }
    return res;
}

void init_drivers() {
}


void uid_to_str(uint8_t *uid, uint8_t *uidstr, int uidlen) {
    int i;

    for (i = 0; i < uidlen; i++) {
        uint8_t n1 = uid[i] & 0x0f;
        uint8_t n2 = (uid[i] & 0xf0) > 4;
        uidstr[2 * i] = (n2 < 10) ? ('0' + n2) : ('a' - 10 + n2);
        uidstr[2 * i + 1] = (n1 < 10) ? ('0' + n1) : ('a' - 10 + n1);
    }
    uidstr[uidlen * 2] = 0;
}

#define MOD_ADLER_32 65521
uint32_t adler32(uint8_t *buf, uint32_t len) {
    uint32_t a = 1, b = 0, i;

    for (i = 0; i < len; i++) {
        a = (a + buf[i]) % MOD_ADLER_32;
        b = (b + a) % MOD_ADLER_32;
    }

    return (b << 16) | a;

}

/*

#if !PORT_PROVIDE_RANDOM

// adapted from: http://burtleburtle.net/bob/rand/smallprng.html

typedef uint32_t  u4;
typedef struct ranctx {
    u4 a;
    u4 b;
    u4 c;
    u4 d;
} ranctx;

#define rot(x,k) (((x)<<(k))|((x)>>(32-(k))))

static ranctx randstr;

u4 oal_random() {

    ranctx *x = &randstr;
    SYSLOCK();
    u4 e = x->a - rot(x->b, 27);
    x->a = x->b ^ rot(x->c, 17);
    x->b = x->c + x->d;
    x->c = x->d + e;
    x->d = e + x->a;
    SYSUNLOCK();
    return x->d;
}

void oal_rand_seed(uint32_t seed) {
    u4 i;
    ranctx *x = &randstr;
    x->a = 0xf1ea5eed, x->b = x->c = x->d = seed;
    for (i = 0; i < 20; ++i) {
        (void)oal_random();
    }
}


#endif
*/

uint8_t *vbl_init(void) {
    uint8_t *pcodemem = (uint8_t *) &__codemem__;

    //find flash sector for codemem


    pcodemem = vbl_get_adjusted_codemem(pcodemem);
    //pcodemem = vhalFlashAlignToSector(pcodemem);

    //set uid
    //uint8_t *uid = get_uid();
    //uid_to_str(uid, _uidstr, UID_BYTES);
    //oal_rand_seed((uint32_t) uid);
    //oal_rand_seed((uint32_t) 2);

    return pcodemem;
}


#define PRINTFBUFSIZE 16

static uint8_t *itos(uint8_t *p, uint32_t num, int radix) {
    int i;
    uint8_t *q;
    uint32_t l, ll;

    l = num;
    ll = num;

    q = p + PRINTFBUFSIZE;
    do {
        i = (int)(l % radix);
        i += '0';
        if (i > '9')
            i += 'A' - '0' - 10;
        *--q = i;
        l /= radix;
    } while ((ll /= radix) != 0);

    i = (int)(p + PRINTFBUFSIZE - q);
    do
        *p++ = *q++;
    while (--i);
    *p = 0;

    return p;
}

void vbl_printf_stdout(uint8_t *fmt, ...) {
    va_list vl;
    va_start(vl, fmt);
    vbl_printf(vhalSerialWrite, VM_STDOUT, (uint8_t *)fmt, &vl);
    va_end(vl);
}

void vbl_printf(void *fn, uint32_t drvid, uint8_t *fmt, va_list *vl) {
    //va_list vl;
    //va_start(vl, fmt);
    uint8_t tmp[PRINTFBUFSIZE] = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'};
    uint8_t c;
    int i;
    int (*prn)(uint32_t, uint8_t *, uint32_t) = (int (*)(uint32_t, uint8_t *, uint32_t))fn;
    int32_t inum;
    uint32_t unum;
    uint8_t *str;

    while ( (c = *fmt++)) {
        if (c != '%') {
            prn(drvid, &c, 1);
            continue;
        }
        c = *fmt++;
        switch (c) {
            case 'i':
            case 'I':
            case 'd':
            case 'D':
                inum = va_arg(*vl, int32_t);
                if (inum < 0) {
                    prn(drvid, (uint8_t *)"-", 1);
                    unum = (~((uint32_t)inum)) + 1;
                } else unum = (uint32_t)inum;
                inum = 10;
                goto print_num;
            case 'u':
            case 'U':
                unum = va_arg(*vl, uint32_t);
                inum = 10;
                goto print_num;
            case 'x':
            case 'X':
            case 'p':
                unum = va_arg(*vl, uint32_t);
                inum = 16;
print_num:
                itos(&(tmp[0]), unum, inum);
                i = 0;
                while (tmp[i]) {
                    prn(drvid, &tmp[i], 1);
                    i++;
                }
                break;
            case 's':
                str = (uint8_t *)va_arg(*vl, uint8_t *);
                i = 0;
                while (str[i]) {
                    prn(drvid, &str[i], 1);
                    i++;
                }
                break;
            case '%':
                prn(drvid, &c, 1);
                break;
            case 'c': {
                c = (uint8_t) va_arg(*vl, int);
                prn(drvid, &c, 1);
            }
            break;
            default:
                break;
        }
    }

    //va_end(vl);
}
