#ifndef __VMTHREAD__
#define __VMTHREAD__

#include "vbl.h"
#include "pobj.h"
#include "pframe.h"
#include "builtins.h"




#define VMT_INACTIVE  0
#define VMT_SUSPENDED 1
#define VMT_RUNNING   2
#define VMT_RETURNING 3
#define VMT_FINISHED  4

#define VMT_EVENT  EVT_MAKE(1)
#define VMT_SIGNAL(th) EVT_SIGNAL((th)->native,VMT_EVENT)
#define VMT_WAIT() EVT_WAIT_ANY(VMT_EVENT)

#define VM_WAIT() EVT_WAIT_ANY(VMT_EVENT)
#define VM_SIGNAL() EVT_SIGNAL(_vm.main,VMT_EVENT)
#define VM_SIGNAL_I() EVT_SIGNAL_I(_vm.main,VMT_EVENT)


typedef struct _co_cache {
	uint32_t addr;
	PObject *obj;
} ConstCache;

typedef struct _pth_status {
	PTuple *freevars;
	PTuple *cellvars;
	PBlock *block;
	PCallable *callable;
	PCode *fcode;
	PObject *res;
	err_t err;
	uint16_t tword;
	uint8_t opcode;
	uint8_t is_delegation;
	uint8_t fl1;
	uint8_t fl2;
	uint8_t fl3;
	uint8_t fl4;
}  PThStatus;

typedef struct _pthread {
	PObjectHeader header;
	struct _pthread *next;
	struct _pthread *prev;
	PObject *stage;
	PFrame *frame;
	VThread th;
	uint16_t exc_code;
	uint16_t exc_offs;
	uint16_t exc_idx;
	uint16_t cachesize;
	PThStatus status;
	ConstCache cocache[];
} PThread;




#define PTHREAD_CURRENT() (PThread *) vosThGetData(vosThCurrent())
#define CONST_CACHE_POS(addr,sz) (((uint32_t)(addr)))%(sz)
PObject *code_getSConst(PThread *th, PViperCode *code, uint16_t pos);
PObject *code_getNConst(PViperCode *code, uint16_t pos, uint8_t opcode);

PThread *pthread_new(uint32_t size, uint8_t prio, void *fn, int cachesize);
PThread *pthread_get_by_id(uint32_t id);
PMethod *pthread_get_method_from_cache(PThread *th, PObject *self, PCallable *fn);

#define PTHREAD_SET_TERMINATED(th) (th)->header.flags|=1
#define PTHREAD_IS_TERMINATED(th) (th)->header.flags&1

#define PTHREAD_SET_IRQ(th) (th)->header.flags|=2
#define PTHREAD_IS_IRQ(th) ((th)->header.flags&2)


#define PTHREAD_SET_MAIN(th) (th)->header.flags|=4
#define PTHREAD_IS_MAIN(th) ((th)->header.flags&4)


#endif