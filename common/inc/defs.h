#ifndef __PLATFORM_DEFS__
#define __PLATFORM_DEFS__


#error you shall not include this!

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>


#include "oal.h"
//#include "magicmacro.h"

#define MAX_INT	INT32_MAX
#define MIN_INT INT32_MIN


//#define MAKE_QWORD(a,b,c,d,e,f,g,h) (((uint8_t)(a))<<56)|(((uint8_t)(b))<<48)|(((uint8_t)(c))<<40)|(((uint8_t)(d))<<32)|(((uint8_t)(e))<<24)|(((uint8_t)(f))<<16)|(((uint8_t)(g))<<8)|(((uint8_t)(h)))
//#define MAKE_DWORD(a,b,c,d) (((uint8_t)(a))<<24)|(((uint8_t)(b))<<16)|(((uint8_t)(c))<<8)|(((uint8_t)(d)))
//#define MAKE_WORD(a,b) (((uint8_t)(a))<<8)|(((uint8_t)(b)))

#define MIN(a,b) ((a)<(b) ? (a):(b))
#define MAX(a,b) ((a)>(b) ? (a):(b))

//#define ALLOC(n, what, name) what* name = (what*)calloc(n,sizeof(what)); __allocated_mem+=(n)*sizeof(what)
//#define ALLOC_TO(n,what,name) name = (what*)calloc(n,sizeof(what)); __allocated_mem+=(n)*sizeof(what)
//#define REALLOC_TO(olds, news, what, name) name = (what*)realloc(name,news*sizeof(what)); __allocated_mem+=(news-olds)*sizeof(what)
//#define REALLOC_TO_O(olds, news, what, name,src)  what* name = (what*)realloc(src,news*sizeof(what)); __allocated_mem+=(news-olds)*sizeof(what)

//#define FREE(x,what) free(x); __allocated_mem-=sizeof(what)
//#define FREE_BLOCK(n,what,x) free(x); __allocated_mem-=(n)*sizeof(what)


#define ALIAS(type, dst, src) type dst; memcpy(&dst,src,sizeof(type))
#define ALIAS_TO(type,dst,src) memcpy(&(dst),src,sizeof(type))


#define FETCH_WORD(code,i) (code[2*i]|(code[2*i+1]<<8))
#define FETCH_WORD_FAST(code) code[0]|(code[1]<<8); code+=sizeof(WORD)
#define FETCH_DWORD_FAST(code) code[0]|(code[1]<<8)|(code[2]<<16)|(code[3]<<24); code+=sizeof(DWORD)
#define FETCH_BYTE_FAST(code) *code++
#define FETCH_OFFSET_FAST(code) code[0]|(code[1]<<8); code+=sizeof(WORD)


#define RETVAL_OK 0

#endif