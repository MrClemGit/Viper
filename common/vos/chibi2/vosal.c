/*
{{ project }}
Copyright (C) {{ year }}  {{ organization }}

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "vosal.h"
#include "ch.h"
#include "hal.h"

/* TYPES */
typedef struct _vsem {
    uint32_t svalue;
    Semaphore sem;
} _vSem;

typedef struct _vtimer {
    uint32_t start_ms;
    uint32_t start_us;
    uint32_t delay;
    vsystimer_fn fn;
    void *arg;
    VirtualTimer tm;
} _vTimer;


/* STATIC */

VSemaphore _memlock;
VSemaphore _dbglock;
VSemaphore _gillock;

_vSem _memsem;
_vSem _gilsem;
#ifdef VM_DEBUG
_vSem _dbgsem;
#endif

volatile uint32_t _systime_seconds = 0;
volatile uint32_t _systime_millis = 0;
volatile uint32_t _system_frequency = 0;

static uint32_t __thid = 1;
static const uint8_t _vos_prio[] STORED = {
    IDLEPRIO,
    NORMALPRIO - 6,
    NORMALPRIO - 4,
    NORMALPRIO - 2,
    NORMALPRIO,
    NORMALPRIO + 2,
    NORMALPRIO + 4,
    NORMALPRIO + 6
};

/* SYSTEM */
void vosInit(void *data) {
    (void)data;
    //Start Chibi
    halInit();

    
    chSysInit();
    
    _system_frequency = halGetCounterFrequency();
    //Initialize System Semaphores
    chSemInit(&_memsem.sem, 1);
    chSemInit(&_gilsem.sem, 1);
#ifdef VM_DEBUG
    chSemInit(&_dbgsem.sem, 1);
#endif

    _memlock = (void *) &_memsem;
    _gillock = (void *) &_gilsem;
#ifdef VM_DEBUG
    _dbglock = (void *) &_dbgsem;
#else
    _dbglock = NULL;
#endif

    
    //Initialize GC
    gc_init();
}

void vosSysLock(void) {
    chSysLock();
}
void vosSysUnlock(void) {
    chSysUnlock();
}
void vosSysLockIsr(void) {
    chSysLockFromIsr();
}
void vosSysUnlockIsr(void) {
    chSysUnlockFromIsr();
}

uint32_t* vosTicks() {
    //DWT_CYCCNT: check in nvic.h
    return (uint32_t*)0xE0001004U;
}



/* Threads */
VThread vosThCreate(uint32_t size, int32_t prio, void *fn, void *arg, void *data) {
    int thsize = THD_WA_SIZE(size);
    void *thwks = gc_malloc(thsize);
    Thread *res = NULL;
    if (thwks) {
        chSysLock();
        res = chThdCreateI(thwks, thsize, _vos_prio[prio], fn, arg);
        res->vdata = data;
        res->thid = __thid++;
        chSysUnlock();
    }
    return (VThread) res;
}

void vosThDestroy(VThread th) {
    // while (vosThGetStatus(th) != VTHREAD_INACTIVE) {
    //     vosThSleep(TIME_U(100, MILLIS));
    // }
    gc_free(th);
}

void vosThSetData(VThread th, void *data) {
    Thread *chth = (Thread *)th;
    chth->vdata = data;
}

void *vosThGetData(VThread th) {
    Thread *chth = (Thread *)th;
    return chth->vdata;
}

uint32_t vosThGetId(VThread th) {
    Thread *chth = (Thread *)th;
    return chth->thid;
}


void vosThSetPriority(int32_t prio) {
    chThdSetPriority(_vos_prio[prio]);
}

int32_t vosThGetPriority(void) {
    int32_t sprio = chThdGetPriority();
    int i;
    for (i = 0; i < (int) sizeof(_vos_prio); i++) {
        if (sprio == _vos_prio[i])
            return i;
    }
    return -1;
}

VThread vosThCurrent(void) {
    return (VThread)chThdSelf();
}

uint32_t time_to_ticks(uint32_t time) {
    uint32_t ctime = GET_TIME_VALUE(time);
    switch (GET_TIME_UNIT(time)) {
        case MICROS:
            return US2ST(ctime);
            break;
        case MILLIS:
            return MS2ST(ctime);
            break;
        case SECONDS:
            return S2ST(ctime);
            break;
    }
    return 0;
}

uint32_t vosMillis(void){
    return chTimeNow();
}

void vosThSleep(uint32_t time) {
    chThdSleep(time_to_ticks(time));
}

void vosThYield(void) {
    chThdYield();
}

void vosThSuspend(void) {
    chSchGoSleepS(THD_STATE_SUSPENDED);
}

VThread vosThResume(VThread th) {
    return chThdResume((Thread *)th);
}

VThread vosThResumeIsr(VThread th) {
    chSchReadyI(th);
    return th;
}


uint32_t vosThGetStatus(VThread th) {
    uint32_t st;
    chSysLock();
    Thread *chth = (Thread *)th;
    switch (chth->p_state) {
        case THD_STATE_READY:
            st = VTHREAD_READY;
            break;
        case THD_STATE_CURRENT:
            st = VTHREAD_RUNNING;
            break;
        case THD_STATE_FINAL:
            st = VTHREAD_INACTIVE;
            break;
        default:
            st = VTHREAD_WAITING;
            break;
    }
    chSysUnlock();
    return st;
}


/* Semaphores */
VSemaphore vosSemCreate(uint32_t n) {
    _vSem *sem = (_vSem *)gc_malloc(sizeof(_vSem));
    chSemInit(&(sem->sem), n);
    sem->svalue = n;
    return (VSemaphore)sem;
}

void vosSemDestroy(VSemaphore sem) {
    vosSemReset(sem);
    gc_free(sem);
}


void vosSemReset(VSemaphore sem) {
    _vSem *vsem = (_vSem *)sem;
    chSemReset(&(vsem->sem), vsem->svalue);
}

int32_t vosSemGetValue(VSemaphore sem) {
    _vSem *vsem = (_vSem *)sem;
    return chSemGetCounterI(&(vsem->sem));
}

void vosSemSignal(VSemaphore sem) {
    _vSem *vsem = (_vSem *)sem;
    chSemSignal(&(vsem->sem));
}

void vosSemSignalIsr(VSemaphore sem) {
    _vSem *vsem = (_vSem *)sem;
    chSemSignalI(&(vsem->sem));
}

int32_t vosSemWaitTimeout(VSemaphore sem, uint32_t timeout) {
    _vSem *vsem = (_vSem *)sem;
    return chSemWaitTimeout(&(vsem->sem), (timeout == VTIME_INFINITE) ? TIME_INFINITE : time_to_ticks(timeout));
}


/* Mutexes */
VMutex vosMtxCreate(void) {
    Mutex *mtx = gc_malloc(sizeof(Mutex));
    chMtxInit(mtx);
    return (VMutex)mtx;
}

void vosMtxLock(VMutex mtx) {
    Mutex *_mtx = (Mutex *)mtx;
    chMtxLock(_mtx);
}

int32_t vosMtxTryLock(VMutex mtx) {
    Mutex *_mtx = (Mutex *)mtx;
    return chMtxTryLock(_mtx);
}

VMutex vosMtxUnlock(VMutex mtx) {
    (void)mtx;
    return (VMutex) chMtxUnlock();
}

void vosMtxDestroy(VMutex mtx) {
    gc_free(mtx);
}


/* Timers */
void _sys_timer_irq(void *tm) {
    _vTimer *vtm = (_vTimer *)tm;
    if (vtm->delay) {
        chSysLockFromIsr();
        chVTSetI(&(vtm->tm), vtm->delay, &_sys_timer_irq, tm);
        chSysUnlockFromIsr();
    }
    if (vtm->fn) {
        vtm->fn(vtm->arg);
    }
}

VSysTimer vosTimerCreate(void) {
    _vTimer *vt = (_vTimer *)gc_malloc(sizeof(_vTimer));
    vt->start_ms = _systime_millis;
    vt->start_us = DWT_CYCCNT;
    return (VSysTimer)vt;
}
uint32_t vosTimerReadMillis(VSysTimer tm) {
    _vTimer *vtm = (_vTimer *)tm;
    return (_systime_millis - vtm->start_ms);
}
uint32_t vosTimerReadMicros(VSysTimer tm) {
    _vTimer *vtm = (_vTimer *)tm;
    return  (DWT_CYCCNT - vtm->start_us);
}


void vosTimerOneShot(VSysTimer tm, uint32_t time, vsystimer_fn fn, void *arg) {
    //chSysLock();
    _vTimer *vtm = (_vTimer *)tm;
    if (chVTIsArmedI(&(vtm->tm))) {
        chVTResetI(&(vtm->tm));
    }
    vtm->fn = fn;
    vtm->arg = arg;
    vtm->delay = 0;
    chVTSetI(&(vtm->tm), time_to_ticks(time), &_sys_timer_irq, tm);
    //chSysUnlock();
}



void vosTimerRecurrent(VSysTimer tm, uint32_t time, vsystimer_fn fn, void *arg) {
    //chSysLock();
    _vTimer *vtm = (_vTimer *)tm;
    if (chVTIsArmedI(&(vtm->tm))) {
        chVTResetI(&(vtm->tm));
    }
    vtm->fn = fn;
    vtm->arg = arg;
    vtm->delay = time_to_ticks(time);
    chVTSetI(&(vtm->tm), vtm->delay, &_sys_timer_irq, tm);
    //chSysUnlock();
}

void vosTimerReset(VSysTimer tm) {
    //chSysLock();
    _vTimer *vtm = (_vTimer *)tm;
    chVTResetI(&(vtm->tm));
    vtm->start_ms = _systime_millis;
    vtm->start_us = DWT_CYCCNT;
    //chSysUnlock();
}

void vosTimerDestroy(VSysTimer tm) {
    _vTimer *vtm = (_vTimer *)tm;
    gc_free(vtm);
}


/* Queues */


typedef struct _vqueue {
    uint8_t *buf;
    uint32_t length;
    uint32_t head;
    uint32_t tail;
    VSemaphore notfull;
    VSemaphore notempty;
} _vq;


VQueue vosQueueCreate(uint32_t size) {
    _vq *vq = (_vq *)gc_malloc(sizeof(_vq));
    if (vq) {
        vq->length = size;
        vq->buf = (uint8_t *)gc_malloc(vq->length);
        vq->head = 0;
        vq->tail = 0;
        vq->notfull = vosSemCreate(1);
        vq->notempty = vosSemCreate(0);
    }
    return (VQueue)vq;
}


int vosQueueRead(VQueue q, uint8_t *buf, uint32_t len) {
    _vq *vq = (_vq *)q;
    chSysLock();
    while (len) {
        if (vq->head == vq->tail) {
            //empty
            chSysUnlock();
            vosSemWait(vq->notempty);
            chSysLock();
        } else {
            *buf++ = vq->buf[vq->tail];
            vq->tail = (vq->tail + 1) % vq->length;
        }
    }
    chSysUnlock();
    return len;
}

int vosQueueWrite(VQueue q, uint8_t *buf, uint32_t len) {
    _vq *vq = (_vq *)q;
    uint32_t next;
    chSysLock();
    while (len) {
        next = (vq->head + 1) % (vq->length);
        if (next != vq->tail) {
            //not full
            vq->buf[vq->head] = *buf++;
            len--;
            vq->head = next;
        } else {
            //full! wait
            chSysUnlock();
            vosSemWait(vq->notfull);
            chSysLock();
        }
    }
    chSysUnlock();
    return len;
}
