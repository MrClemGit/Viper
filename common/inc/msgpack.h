#ifndef __MSGPACK__
#define __MSGPACK__

#include "oal.h"


#define MSGPCK_NIL		0xc0
#define MSGPCK_TRUE		0xc2
#define MSGPCK_FALSE	0xc3


#define MSGPCK_UINT8	0xcc
#define MSGPCK_UINT16	0xcd
#define MSGPCK_UINT32	0xce
#define MSGPCK_UINT64	0xcf

#define MSGPCK_INT8		0xd0
#define MSGPCK_INT16	0xd1
#define MSGPCK_INT32	0xd2
#define MSGPCK_INT64	0xd3

#define MSGPCK_FLOAT32	0xca
#define MSGPCK_FLOAT64	0xcb

#define MSGPCK_STRING	0xda
#define MSGPCK_BINARY	0xc5

#define MSGPCK_ARRAY	0xdc
#define MSGPCK_MAP		0xde



//void msgpack_write(Stream *ps,uint8_t type, ...);


#endif