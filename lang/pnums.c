#include "lang.h"

#include <math.h>


FLOAT_TYPE real_part(PObject *o, uint8_t ta) {
    switch (ta) {
        case PSMALLINT: return (FLOAT_TYPE)PSMALLINT_VALUE(o);
        case PINTEGER: return (FLOAT_TYPE)(((PInteger *)o)->val);
        case PFLOAT: return (FLOAT_TYPE)(((PFloat *)o)->val);
        default: return (FLOAT_TYPE)(((PComplex *)o)->re);
    }
}

/*
FLOAT_TYPE img_part(PObject *o, uint8_t ta) {
    if (ta != PCOMPLEX) return FLOAT_ZERO;
    else return (FLOAT_TYPE)(((PComplex *)o)->im);
}
*/

INT_TYPE int_part(PObject *o, uint8_t ta) {
    if (ta == PSMALLINT) return PSMALLINT_VALUE(o);
    return ((PInteger *)o)->val;
}

PFloat *pfloat_new(FLOAT_TYPE n) {
    PFloat *res = ALLOC_OBJ(PFloat, PFLOAT, PFLAG_HASHABLE, 0);
    res->val = n;
    return res;
}


PInteger *pinteger_new(INT_TYPE n) {
    if (PSMALLINT_OVERFLOWED(n)) {
        PInteger *res = ALLOC_OBJ(PInteger, PINTEGER, PFLAG_HASHABLE, 0);
        res->val = n;
        return res;
    }
    return (PInteger *) PSMALLINT_NEW(n);
}

/*
PComplex *pcomplex_new(FLOAT_TYPE re, FLOAT_TYPE im) {
    PComplex *res = ALLOC_OBJ(PComplex, PCOMPLEX, PFLAG_HASHABLE, 0);
    res->re = re;
    res->im = im;
    return res;

}
*/

err_t pnumber_unary_op(uint32_t op, PObject *a, PObject **res) {
    register uint8_t ta = PTYPE(a);
    switch (ta) {
        case PSMALLINT:
            switch (op) {
                case UNEG: *res =(PObject*) PSMALLINT_NEW(-PSMALLINT_VALUE(a)); break;
                case UPOS: *res = a; break;
                case INVERT: *res = (PObject*)PSMALLINT_NEW(~PSMALLINT_VALUE(a)); break;
            }
            break;
        case PINTEGER:
            switch (op) {
                case UNEG: *res =(PObject*) pinteger_new(-((PInteger *)a)->val); break;
                case UPOS: *res = a; break;
                case INVERT: *res = (PObject*)pinteger_new(~((PInteger *)a)->val); break;
            }
            break;
        case PFLOAT:
            switch (op) {
                case UNEG: *res = (PObject*)pfloat_new(-((PFloat *)a)->val); break;
                case UPOS: *res = a; break;
                case INVERT: return ERR_TYPE_EXC;
            }
            break;
        default:
            return ERR_TYPE_EXC;
    }
    return ERR_OK;
}

err_t pnumber_binary_op(uint32_t op, PObject *a, PObject *b, PObject **res) {
    register uint8_t ta;
    register uint8_t tb;

    ta = PTYPE(a);
    tb = PTYPE(b);

    if (!IS_NUMBER_TYPE(tb)) {
        if (IS_PSEQUENCE_TYPE(tb))
            return psequence_binary_op(op, b, a, res);
        if (IS_BOOL_TYPE(tb)) {
            tb = PSMALLINT;
            b = PSMALLINT_NEW(PBOOL_VALUE(b));
            goto num_calc;
        }
        return pgeneric_binary_op(op,a,b,res);
    }
num_calc:
    if (ta == PSMALLINT && tb == PSMALLINT) {
        int32_t x, y, r = 0;
        x = PSMALLINT_VALUE(a);
        y = PSMALLINT_VALUE(b);
        switch (op) {
            case _BIN_OP(ADD):
            case _BIN_OP(IADD): r = x + y; break;
            case _BIN_OP(SUB):
            case _BIN_OP(ISUB): r = x - y; break;
            case _BIN_OP(MUL):
            case _BIN_OP(IMUL): r = x * y; break;
            case _BIN_OP(DIV):
            case _BIN_OP(IDIV):
            case _BIN_OP(FDIV):
            case _BIN_OP(IFDIV):
            case _BIN_OP(MOD):
            case _BIN_OP(IMOD):
                if (y == 0) return ERR_ZERODIV_EXC;
                if (op >= _BIN_OP(MOD))  {
                    r = x % y;
                    break;
                }
                if (op >= _BIN_OP(FDIV)) {
                    r = x / y;
                    break;
                }
                *res = (PObject *)pfloat_new( (FLOAT_TYPE)x / (FLOAT_TYPE)y);
                return ERR_OK;
            case _BIN_OP(POW):
            case _BIN_OP(IPOW): *res = (PObject *)pinteger_new((INT_TYPE) FLOAT_POW(x, y)); return ERR_OK;
            case _BIN_OP(LSHIFT):
            case _BIN_OP(ILSHIFT): r = (x << y); break;
            case _BIN_OP(RSHIFT):
            case _BIN_OP(IRSHIFT): r = (x >> y); break;
            case _BIN_OP(BIT_OR):
            case _BIN_OP(IBIT_OR): r = (x | y); break;
            case _BIN_OP(BIT_XOR):
            case _BIN_OP(IBIT_XOR): r = (x ^ y); break;
            case _BIN_OP(BIT_AND):
            case _BIN_OP(IBIT_AND): r = (x & y); break;
            case _BIN_OP(EQ): *res = (x == y) ? (P_TRUE) : (P_FALSE); return ERR_OK;
            case _BIN_OP(NOT_EQ): *res = (x != y) ? (P_TRUE) : (P_FALSE); return ERR_OK;
            case _BIN_OP(LT): *res = (x < y) ? (P_TRUE) : (P_FALSE); return ERR_OK;
            case _BIN_OP(LTE): *res = (x <= y) ? (P_TRUE) : (P_FALSE); return ERR_OK;
            case _BIN_OP(GT): *res = (x > y) ? (P_TRUE) : (P_FALSE); return ERR_OK;
            case _BIN_OP(GTE): *res = (x >= y) ? (P_TRUE) : (P_FALSE); return ERR_OK;
            case _BIN_OP(L_AND): *res = (x && y ) ? (P_TRUE) : (P_FALSE); return ERR_OK;
            case _BIN_OP(L_OR): *res = (x || y) ? (P_TRUE) : (P_FALSE); return ERR_OK;
            default:
                return ERR_TYPE_EXC;
        }
        *res = (PObject *) pinteger_new((INT_TYPE)r);
    }/* else if (ta == PCOMPLEX || tb == PCOMPLEX) {
        FLOAT_TYPE x, u, y, v, rer, imr;
        x = real_part(a, ta);
        u = real_part(b, tb);
        y = img_part(a, ta);
        v = img_part(b, tb);
        switch (op) {
            case _BIN_OP(ADD):
            case _BIN_OP(IADD): rer = x + u; imr = y + v; break;
            case _BIN_OP(SUB):
            case _BIN_OP(ISUB): rer = x - u; imr = y - v; break;
            case _BIN_OP(MUL):
            case _BIN_OP(IMUL): rer = x * u - y * v; imr = x * v + y * u; break;
            case _BIN_OP(DIV):
            case _BIN_OP(IDIV):
                imr = u * u + v * v;
                if (imr == FLOAT_ZERO) return ERR_ZERODIV_EXC;
                rer = (x * u + y * v) / imr;
                imr = (y * u - x * v) / imr;
                break;
            case _BIN_OP(FDIV):
            case _BIN_OP(IFDIV):
            case _BIN_OP(MOD):
            case _BIN_OP(IMOD):
            case _BIN_OP(POW):
            case _BIN_OP(IPOW):
            case _BIN_OP(LSHIFT):
            case _BIN_OP(ILSHIFT):
            case _BIN_OP(RSHIFT):
            case _BIN_OP(IRSHIFT):
            case _BIN_OP(BIT_OR):
            case _BIN_OP(IBIT_OR):
            case _BIN_OP(BIT_XOR):
            case _BIN_OP(IBIT_XOR):
            case _BIN_OP(BIT_AND):
            case _BIN_OP(IBIT_AND):
                return ERR_TYPE_EXC;
            case _BIN_OP(EQ): *res = (x == u && y == v) ? (P_TRUE) : (P_FALSE); return ERR_OK;
            case _BIN_OP(NOT_EQ): *res = (x != u || y != v) ? (P_TRUE) : (P_FALSE); return ERR_OK;
            default:
                return ERR_TYPE_EXC;
        }
        *res = (PObject *) pcomplex_new(rer, imr);
    } else*/ if (ta == PFLOAT || tb == PFLOAT) {
        FLOAT_TYPE x, y, r;
        x = real_part(a, ta);
        y = real_part(b, tb);
        switch (op) {
            case _BIN_OP(ADD):
            case _BIN_OP(IADD): r = x + y; break;
            case _BIN_OP(SUB):
            case _BIN_OP(ISUB): r = x - y; break;
            case _BIN_OP(MUL):
            case _BIN_OP(IMUL): r = x * y; break;
            case _BIN_OP(DIV):
            case _BIN_OP(IDIV):
            case _BIN_OP(FDIV):
            case _BIN_OP(IFDIV):
            case _BIN_OP(MOD):
            case _BIN_OP(IMOD):
                if (y == FLOAT_ZERO) return ERR_ZERODIV_EXC;
                r = x / y;
                if (op >= _BIN_OP(FDIV)) r = FLOAT_FLOOR(r);
                if (op >= _BIN_OP(MOD)) r = x - r * y;
                break;
            case _BIN_OP(POW):
            case _BIN_OP(IPOW): r = FLOAT_POW(x, y); break;
            case _BIN_OP(LSHIFT):
            case _BIN_OP(ILSHIFT):
            case _BIN_OP(RSHIFT):
            case _BIN_OP(IRSHIFT):
            case _BIN_OP(BIT_OR):
            case _BIN_OP(IBIT_OR):
            case _BIN_OP(BIT_XOR):
            case _BIN_OP(IBIT_XOR):
            case _BIN_OP(BIT_AND):
            case _BIN_OP(IBIT_AND):
                return ERR_TYPE_EXC;
            case _BIN_OP(EQ): *res = (x == y) ? (P_TRUE) : (P_FALSE); return ERR_OK;
            case _BIN_OP(NOT_EQ): *res = (x != y) ? (P_TRUE) : (P_FALSE); return ERR_OK;
            case _BIN_OP(LT): *res = (x < y) ? (P_TRUE) : (P_FALSE); return ERR_OK;
            case _BIN_OP(LTE): *res = (x <= y) ? (P_TRUE) : (P_FALSE); return ERR_OK;
            case _BIN_OP(GT): *res = (x > y) ? (P_TRUE) : (P_FALSE); return ERR_OK;
            case _BIN_OP(GTE): *res = (x >= y) ? (P_TRUE) : (P_FALSE); return ERR_OK;
            case _BIN_OP(L_AND): *res = (x != FLOAT_ZERO && y != FLOAT_ZERO) ? (P_TRUE) : (P_FALSE); return ERR_OK;
            case _BIN_OP(L_OR): *res = (x != FLOAT_ZERO || y != FLOAT_ZERO) ? (P_TRUE) : (P_FALSE); return ERR_OK;
            default:
                return ERR_TYPE_EXC;
        }
        *res = (PObject *)pfloat_new(r);
    } else if (ta == PINTEGER || tb == PINTEGER) {
        INT_TYPE x, y, r = 0;
        x = int_part(a, ta);
        y = int_part(b, tb);
        switch (op) {
            case _BIN_OP(ADD):
            case _BIN_OP(IADD): r = x + y; break;
            case _BIN_OP(SUB):
            case _BIN_OP(ISUB): r = x - y; break;
            case _BIN_OP(MUL):
            case _BIN_OP(IMUL): r = x * y; break;
            case _BIN_OP(DIV):
            case _BIN_OP(IDIV):
            case _BIN_OP(FDIV):
            case _BIN_OP(IFDIV):
            case _BIN_OP(MOD):
            case _BIN_OP(IMOD):
                if (y == 0) return ERR_ZERODIV_EXC;
                if (op >= _BIN_OP(MOD)) {
                    r = x % y;
                    break;
                }
                if (op >= _BIN_OP(FDIV)) {
                    r = x / y;
                    break;
                }
                *res = (PObject *)pfloat_new( (FLOAT_TYPE)x / (FLOAT_TYPE)y);
                return ERR_OK;
            case _BIN_OP(POW):
            case _BIN_OP(IPOW): r = (INT_TYPE) FLOAT_POW(x, y); break;
            case _BIN_OP(LSHIFT):
            case _BIN_OP(ILSHIFT): r = (x << y); break;
            case _BIN_OP(RSHIFT):
            case _BIN_OP(IRSHIFT): r = (x >> y); break;
            case _BIN_OP(BIT_OR):
            case _BIN_OP(IBIT_OR): r = (x | y); break;
            case _BIN_OP(BIT_XOR):
            case _BIN_OP(IBIT_XOR): r = (x ^ y); break;
            case _BIN_OP(BIT_AND):
            case _BIN_OP(IBIT_AND): r = (x & y); break;
            case _BIN_OP(EQ): *res = (x == y) ? (P_TRUE) : (P_FALSE); return ERR_OK;
            case _BIN_OP(NOT_EQ): *res = (x != y) ? (P_TRUE) : (P_FALSE); return ERR_OK;
            case _BIN_OP(LT): *res = (x < y) ? (P_TRUE) : (P_FALSE); return ERR_OK;
            case _BIN_OP(LTE): *res = (x <= y) ? (P_TRUE) : (P_FALSE); return ERR_OK;
            case _BIN_OP(GT): *res = (x > y) ? (P_TRUE) : (P_FALSE); return ERR_OK;
            case _BIN_OP(GTE): *res = (x >= y) ? (P_TRUE) : (P_FALSE); return ERR_OK;
            case _BIN_OP(L_AND): *res = (x && y ) ? (P_TRUE) : (P_FALSE); return ERR_OK;
            case _BIN_OP(L_OR): *res = (x || y) ? (P_TRUE) : (P_FALSE); return ERR_OK;
            default:
                return ERR_TYPE_EXC;
        }
        *res = (PObject *)pinteger_new(r);
    } else if (ta == PBOOL) {
        ta = PSMALLINT;
        a = PSMALLINT_NEW(PBOOL_VALUE(a));
        goto num_calc;
    }
    return ERR_OK;
}


