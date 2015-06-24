#ifndef __PCODE__
#define __PCODE__


#include "pobj.h"
#include "builtins.h"

#define CODETYPE_VIPER      0
#define CODESUBTYPE_VIPER_FUN       0
#define CODESUBTYPE_VIPER_MODULE    1
#define CODESUBTYPE_VIPER_CLASS     2

#define CODETYPE_CNATIVE    1


typedef struct _code {
    uint8_t codetype;
    uint8_t args;
    uint8_t kwargs;
    uint8_t vararg;

    uint8_t nlocals;
    uint8_t nfree;
    uint8_t stacksize;
    uint8_t bstacksize;

    uint16_t name;
} PCode;


typedef struct _vipercode {
    uint8_t codetype;
    uint8_t args;
    uint8_t kwargs;
    uint8_t vararg;

    uint8_t nlocals;
    uint8_t nfree;
    uint8_t stacksize;
    uint8_t bstacksize;

    uint16_t name;
    uint16_t nconsts;

    uint16_t nmstart;
    uint16_t bcstart;
    uint8_t bytecode[];
} PViperCode;


typedef struct _cnativecode {
    uint8_t codetype;
    uint8_t args;
    uint8_t kwargs;
    uint8_t vararg;

    uint8_t nlocals;
    uint8_t nfree;
    uint8_t stacksize;
    uint8_t bstacksize;

    uint16_t name;

    uint16_t tableidx;
    uint8_t minitable[];
} PCNativeCode;

typedef struct _code_header {
    const uint32_t magic;
    const uint8_t flags;
    const uint8_t nmodules;
    const uint16_t nobjs;
    const uint16_t nexcp;
    const uint16_t ncnatives;
    const uint32_t ram_start;
    const uint32_t data_start;
    const uint32_t data_end;
    const uint32_t data_bss;
    const uint32_t const codeobjs[];
} CodeHeader;


#define PCODE_MAKE(n) ((PCode*)(((uint8_t*)_vm.program)+_vm.program->codeobjs[(n)]))
#define ViperCode(code) ((PViperCode*)code)
#define CNativeCode(code) ((PCNativeCode*)code)

#define PCODE_CELLVARS(code) ((code)->nfree&0x0f)
#define PCODE_FREEVARS(code) ((code)->nfree>>4)
#define PCODE_HASVARS(code) (code)->nfree

#define PCODE_GET_BYTECODE(code) ViperCode(code)->bytecode + ViperCode(code)->bcstart
#endif

