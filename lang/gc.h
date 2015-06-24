#ifndef __GARBAGE_COLLECTOR__
#define __GARBAGE_COLLECTOR__


#include "pobj.h"

#define GC_ALIGNMENT 4
#define GC_TRIGGER_MIN  4096
#define GC_TRIGGER_CRIT 1024


#define GC_LOCK() SYSLOCK(); hlocked++; SYSUNLOCK()
#define GC_UNLOCK() SYSLOCK(); hlocked--; SYSUNLOCK()
#define GC_IS_LOCKED() hlocked


#define GC_FRAGMENTATION() (((uint32_t)(hend-hedge)*100)/hfreemem)
#define GC_OCCUPATION() (((uint32_t)(hend-hbase)*100)/hfreemem)
#define GC_MORE_FRAGMENTED_THAN(x) (GC_FRAGMENTATION()<(100-(x)))
#define GC_TOTAL_MEMORY() (int32_t)(hend-hbase)
#define GC_FREE_MEMORY() (int32_t)(hfreemem)
#define GC_FREE_BLOCKS() (int32_t)(hfblocks)
#define GC_USED_BLOCKS() (int32_t)(hblocks)
#define GC_PERIOD() (int32_t)(gc_period)
#define GC_SET_PERIOD(x) gc_period=(x)
#define GC_LAST_COLLECT() (_systime_millis-last_collect)
#define GC_CHECKTIME 1500
#define GC_HEDGE_START() (((uint32_t)hedge)+sizeof(PObjectHeader))


extern uint8_t *hbase;
extern uint8_t *hend;
extern uint8_t *hedge;
extern uint32_t hfreemem;
extern uint32_t gc_period;
extern uint32_t hblocks;
extern uint32_t hfblocks;
extern uint32_t last_collect;


void gc_init(void);
PObject *gc_alloc(uint8_t type, uint8_t flags, uint16_t size, uint8_t onheap);

void gc_mark(void);
void gc_sweep(void);
void gc_compact(void);
void gc_trace(void);
void gc_collect(void);
void gc_start(void);
void gc_keep(PObject **objs, uint32_t nobjs);
#define gc_wait() vosSemWait(_memlock)
#define gc_signal() vosSemSignal(_memlock)

#define GCM_KEEP(o) do { gc_wait(); gc_keep(&(o),1); gc_signal(); } while(0)
#define GCM_KEEP_NOTNULL(o)  do { if(o) {gc_wait(); gc_keep(&(o),1); gc_signal();} } while(0)
#define GCM_KEEP_MANY(o,n) do { gc_wait(); gc_keep(o,n); gc_signal(); } while(0)


#define GC_USED 0x1
#define GC_MARKED 0x2

#define GC_FREE 0x0
#define GC_USED_MARKED 0x3
#define GC_USED_UNMARKED 0x1
#define GC_STAGED 0x2

#define GC_MASK 0x3
#define GC_IMASK 0xfffffffc


#define GCH_OBJ(b) (((PObject*)(b))->header)
#define GCH_SET(b,flags,offs) (((PObject*)(b))->header.gch.ref = (PObject*)(((uint32_t)(offs))|(flags)))
#define GCH_FLAG(b) (((PObject*)(b))->header.gch.flags&GC_MASK)
#define GCH_FLAG_SET(b,flgs) (((PObject*)(b))->header.gch.flags=(((PObject*)(b))->header.gch.flags&GC_IMASK)|(flgs))
#define GCH_NEXT(b) ((PObject*)( ((uint32_t)((PObject*)(b))->header.gch.next)&GC_IMASK))
#define GCH_NEXT_SET(b,nxt) (((PObject*)(b))->header.gch.next=(PObject*)(nxt))
#define GCH_NEXT_SET2(b,nxt) (((PObject*)(b))->header.gch.next= (PObject*)(((uint32_t)(nxt))|GCH_FLAG(b)) )
#define GCH_SIZE(b) (((PObject*)(b))->header.size)
#define GCH_SIZE_SET(b,sz) (((PObject*)(b))->header.size=(uint16_t)(sz))

#define GCH_HEAP_NEXT(b) (PObject*)(((int32_t)b)+ ((int32_t)((GCH_OBJ(b).gch.ext.hnext))&(GC_IMASK)) )
#define GCH_HEAP_PREV(b) (PObject*)(((int32_t)b)+ ((int32_t)((GCH_OBJ(b).gch.ext.hprev))&(GC_IMASK)) )
#define GCH_HEAP_NEXT_SET(b,nxt) (GCH_OBJ(b).gch.ext.hnext=(int16_t)( (int32_t)(nxt)-(int32_t)(b))|GC_STAGED)
#define GCH_HEAP_PREV_SET(b,prv) (GCH_OBJ(b).gch.ext.hprev=(int16_t)( (int32_t)(prv)-(int32_t)(b))|GC_STAGED)



#define GCH_REF(b) ((PObject*)( ((uint32_t)((PObject*)(b))->header.gch.ref)&GC_IMASK))
#define GC_UPDATE_REF(a,type) (a)=(type*)GCH_REF(a)


#define GC_START_STAGING() gc_wait()
#define GC_STAGE(o) GCH_FLAG_SET(o,GC_STAGED)
#define GC_UNSTAGE(o) GCH_FLAG_SET(o,GC_USED)
#define GC_STOP_STAGING() gc_signal()


//#define GC_ALIGN_SIZE(s) (((s)&(GC_ALIGNMENT-1)) ? (((s)+GC_ALIGNMENT)&~(GC_ALIGNMENT-1)):(s))
#define GC_ALIGN_SIZE(s)  (((s)&(GC_ALIGNMENT-1)) ? ((s)+(GC_ALIGNMENT - ((s)&(GC_ALIGNMENT-1)) )):(s))
#define GC_ALIGN_PREV_SIZE(s) ((s)&~(GC_ALIGNMENT-1))


#define ALLOC_OBJ(type,htype,hflags, extrasize) (type *) gc_alloc(htype,hflags,sizeof(type)+(extrasize),0)
#define ALLOC_OBJ_HEAP(type,htype,hflags, extrasize) (type *) gc_alloc(htype,hflags,sizeof(type)+(extrasize),1)



//void gc_dump(Stream *ps, int thid);


#endif