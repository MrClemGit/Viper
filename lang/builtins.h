#ifndef __BUILTINS__
#define __BUILTINS__

#include "pobj.h"
#include "psequence.h"

#include "natives.h"

//#define EX_TAGGED_MASK 0xfe




/* ========================================================================
     SYSOBJS
   ======================================================================== */


typedef struct _irq_fn {
    PObject *fn;
    PTuple *args;
} IrqFn;


typedef struct _psysobj {
    PObjectHeader header;
    uint32_t type;
    union {
        VSemaphore sem;
        struct {
            struct _psysobj *next;
            struct _psysobj *prev;
            VSysTimer vtm;
            IrqFn fn;
        } timer;
    } sys;
} PSysObject;


#define PSYS_SEMAPHORE   0
#define PSYS_BSEMAPHORE  1
#define PSYS_TIMER       2

PSysObject *psysobj_new(uint32_t);
PObject *netaddress_to_object(NetAddress *addr);



/* ========================================================================
     TAGGED TYPES
   ======================================================================== */

extern const uint8_t const _typetags[];

#define P_MARKER ((PObject*)(0x2))
#define DRIVER_TAG ((1<<2)|0x2)
#define NONE_TAG ((2<<2)|0x2)
#define EXCEPTION_TAG ((3<<2)|0x2)
#define BOOL_TAG ((4<<2)|0x2)
#define CELL_TAG ((6<<2)|0x2)
#define TYPE_FROM_TAGGED(o) _typetags[(((uint32_t)(o))&0x0000001f)>>2]

//native_tag is in natives.h


/* ========================================================================
     DRIVER
   ======================================================================== */

//#define DRIVER_TAG  ((PDRIVER<<2)|0x2)
#define PDRIVER_MAKE(drv) (((drv)<<8)|DRIVER_TAG)
#define PDRIVER_ID(pdrv) (((uint32_t)(pdrv))>>8)
#define PDRIVER_TO_DRIVER(pdrv) (&_drivers[PDRIVER_ID(pdrv)])
#define PDRIVER_TO_CTL(pdrv) ((native_fn)(&_drivers[PDRIVER_ID(pdrv)])->ctl)


/* ========================================================================
     NONE
   ======================================================================== */


#define MAKE_NONE() ((PObject*)(NONE_TAG))
err_t pnone_binary_op(uint32_t op, PObject *a, PObject *b, PObject **res);
err_t pgeneric_binary_op(uint32_t op, PObject *a, PObject *b, PObject **res);


/* ========================================================================
     EXCEPTIONS
   ======================================================================== */


typedef struct _pexception {
    uint16_t name;
    uint16_t parent;
    uint32_t msg;
} PException;


//#define EXCEPTION_TAG ((PEXCEPTION<<2)|0x2)
#define EXCEPTION_NO_TH 0xff
#define PEXCEPTION_MAKE(idx) ((PObject*)((uint32_t)(((idx)<<16)|((EXCEPTION_NO_TH)<<8)|(EXCEPTION_TAG))))
#define PEXCEPTION_MAKE_TH(idx,th) ((PObject*)((uint32_t)(((idx)<<16)|((th)<<8)|(EXCEPTION_TAG))))
#define PEXCEPTION_ERR(e) ((((uint32_t)(e))&0xffff0000)>>16)
#define PEXCEPTION_TH(e) ((((uint32_t)(e))&0x0000ff00)>>8)
#define IS_EXCEPTION(e) ( (((uint32_t)(e))&0xff) == EXCEPTION_TAG)

err_t exception_matches(uint16_t, PObject *);
int exception_search(uint16_t);



/* ========================================================================
     BOOL
   ======================================================================== */

//#define BOOL_TAG ((PBOOL<<2)|0x2)
#define IS_BOOL_TYPE(e) ((e)==PBOOL)
#define IS_BOOL(e) ( (((uint32_t)(e))&0xff) == BOOL_TAG)
#define PBOOL_TRUE() (PObject*)((1<<8)|BOOL_TAG)
#define PBOOL_FALSE() (PObject*)(0|BOOL_TAG)
#define PBOOL_VALUE(b) (((uint32_t)b) >> 8)



/* ========================================================================
     CELL
   ======================================================================== */

// Cells store a pointer to a tuple and an idx in the range [0..15]
// offs from start of ram 23 bits, 4 bits of idx, 5 bits of tag

extern uint8_t *__ram_start__;
#define RAMSTART (uint32_t)(&__ram_start__)

#define PCELL_MAKE(idx,arg) ((PObject*) ((((uint32_t)(arg)-RAMSTART)<<9)|((idx)<<5)|CELL_TAG))
#define PCELL_IDX(cell) ((((uint32_t)(cell))>>5)&0xf)
#define PCELL_ARG(cell) ((PObject*)((((uint32_t)(cell))>>9)+RAMSTART))
#define PCELL_GET(cell) PTUPLE_ITEM(PCELL_ARG(cell),PCELL_IDX(cell))
#define PCELL_SET(cell,arg) PTUPLE_SET_ITEM(PCELL_ARG(cell),PCELL_IDX(cell),arg)


//#define PCELL_MAKE(idx,arg) ((PObject*)((((uint32_t)(arg))>>2)| (((uint32_t)(idx))<<28)))
//#define PCELL_IDX(cell) (((uint32_t)(cell))>>28)
//#define PCELL_ARG(cell) ((PObject*)((((uint32_t)(cell))&0x0fffffff)<<2))
//#define PCELL_GET(cell) PTUPLE_ITEM(PCELL_ARG(cell),PCELL_IDX(cell))
//#define PCELL_SET(cell,arg) PTUPLE_SET_ITEM(PCELL_ARG(cell),PCELL_IDX(cell),arg)


#endif