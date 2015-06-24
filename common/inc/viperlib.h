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

#ifndef __VIPERLIB__
#define __VIPERLIB__


#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>


#define MAX_INT INT32_MAX
#define MIN_INT INT32_MIN

#define MIN(a,b) ((a)<(b) ? (a):(b))
#define MAX(a,b) ((a)>(b) ? (a):(b))


/* ========================================================================
    MEMORY
   ======================================================================== */

void gc_init(void);
void *gc_malloc(uint32_t sz);
void *gc_realloc(void *pnt, uint32_t sz);
void gc_free(void *pnt);


/* ========================================================================
    TIME
   ======================================================================== */

#define NANOS  0x00
#define MICROS  0x01
#define MILLIS  0x02
#define SECONDS 0x03

#define TIME_U(n,u) (((n)<<2)|(u))
#define GET_TIME_UNIT(n) ((n)&0x03)
#define GET_TIME_VALUE(n) ((n)>>2)
//NANOS not supported yet
#define GET_TIME_MICROS(n) (GET_TIME_VALUE(n)*((GET_TIME_UNIT(n)==MILLIS) ? 1000:((GET_TIME_UNIT(n)==SECONDS) ? 1000000:1)))

/* ========================================================================
    MISC
   ======================================================================== */

#define STORED __attribute__ ((section (".storedata")))
#define ALIASED(x) __attribute__((weak, alias(#x)))


#endif