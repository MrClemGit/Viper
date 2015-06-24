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

#ifndef ____VOS____
#define ____VOS____

#include "vosal.h"

#define CH_FREQUENCY                    1000
#define CH_TIME_QUANTUM                 20
#define CH_NO_IDLE_THREAD               FALSE
#define CH_OPTIMIZE_SPEED               TRUE
#define CH_USE_REGISTRY                 FALSE

//Chibi Synch
#define CH_USE_WAITEXIT                 TRUE
#define CH_USE_SEMAPHORES               TRUE
#define CH_USE_SEMAPHORES_PRIORITY      FALSE
#define CH_USE_SEMSW                    FALSE
#define CH_USE_MUTEXES                  TRUE
#define CH_USE_CONDVARS                 FALSE
#define CH_USE_CONDVARS_TIMEOUT         FALSE
#define CH_USE_EVENTS                   TRUE
#define CH_USE_EVENTS_TIMEOUT           FALSE
#define CH_USE_MESSAGES                 FALSE
#define CH_USE_MESSAGES_PRIORITY        FALSE
#define CH_USE_MAILBOXES                FALSE
#define CH_USE_QUEUES                   TRUE

//Chibi Memory: DISABLED
#define CH_MEMCORE_SIZE                 0
#define CH_USE_MEMCORE                  FALSE
#define CH_USE_HEAP                     FALSE
#define CH_USE_MALLOC_HEAP              FALSE
#define CH_USE_MEMPOOLS                 FALSE
#define CH_USE_DYNAMIC                  FALSE


//Chibi Debug: DISABLED
#define CH_DBG_SYSTEM_STATE_CHECK       FALSE
#define CH_DBG_ENABLE_CHECKS            FALSE
#define CH_DBG_ENABLE_ASSERTS           FALSE
#define CH_DBG_ENABLE_TRACE             FALSE
#define CH_DBG_ENABLE_STACK_CHECK       FALSE
#define CH_DBG_FILL_THREADS             FALSE
#define CH_DBG_THREADS_PROFILING        FALSE


//Chibi System Hooks
#define THREAD_EXT_FIELDS                                                   \
    /* Add threads custom fields here.*/                                    \
    /* pointer to data used in vosal */                                       \
    uint32_t thid;                      \
    void *vdata;

#define THREAD_EXT_INIT_HOOK(tp) {                                          \
        /* Add threads initialization code here.*/                                \
    }

#define THREAD_EXT_EXIT_HOOK(tp) {                                          \
        /* Add threads finalization code here.*/                                  \
    }

#define THREAD_CONTEXT_SWITCH_HOOK(ntp, otp) {                              \
        /* System halt code here.*/                                               \
    }

#define IDLE_LOOP_HOOK() {                                                  \
        /* Idle loop code here.*/                                                 \
    }

#define SYSTEM_TICK_EVENT_HOOK() {                                          \
        /* System tick event code here.*/                                         \
        _systime_millis++;  \
        if(_systime_millis%1000==0) \
            _systime_seconds++;     \
    }

#define SYSTEM_HALT_HOOK() {                                                \
        /* System halt code here.*/                                               \
    }

#include "vboard.h"

#endif