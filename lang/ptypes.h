#ifndef __PTYPES__
#define __PTYPES__

#include "opcodes.h"

#define PSMALLINT   0
#define PINTEGER    1
#define PFLOAT      2
#define PNUMBER     2
#define PBOOL       3
#define PSTRING     4
#define PBYTES      5
#define PBYTEARRAY  6
#define PSHORTS     7
#define PSHORTARRAY 8
#define PLIST       9
#define PTUPLE      10
#define PRANGE      11
#define PSEQUENCE   11
#define PFSET       12
#define PSET        13
#define PDICT       14
#define PFUNCTION   15
#define PMETHOD     16
#define PCLASS      17
#define PINSTANCE   18
#define PMODULE     19
#define PBUFFER     20
#define PSLICE      21
#define PITERATOR   22
#define PFRAME      23
#define PCELL       24
#define PNONE       25
#define PEXCEPTION  26
#define PNATIVE     27
#define PSYSOBJ     28
#define PDRIVER     29
#define PTHREAD     30

#define NTYPES  31


extern const char *const typestrings[];



#define ERR_OK                  0
#define ERR_TYPE_EXC            NAME_TypeError
#define ERR_ZERODIV_EXC         NAME_ZeroDivisionError
#define ERR_ATTRIBUTE_EXC       NAME_AttributeError
#define ERR_RUNTIME_EXC         NAME_RuntimeError
#define ERR_VALUE_EXC           NAME_ValueError
#define ERR_INDEX_EXC           NAME_IndexError
#define ERR_KEY_EXC             NAME_KeyError
#define ERR_NOT_IMPLEMENTED_EXC NAME_NotImplementedError
#define ERR_UNSUPPORTED_EXC     NAME_UnsupportedError
#define ERR_OVERFLOW_EXC        NAME_OverflowError
#define ERR_STOP_ITERATION      NAME_StopIteration
#define ERR_NAME_EXC            NAME_NameError 
#define ERR_IOERROR_EXC         NAME_IOError 
#define ERR_CONNECTION_REF_EXC  NAME_ConnectionRefusedError
#define ERR_CONNECTION_RES_EXC  NAME_ConnectionResetError
#define ERR_CONNECTION_ABR_EXC  NAME_ConnectionAbortedError
#define ERR_TIMEOUT_EXC         NAME_TimeoutError

#define ERR_PERIPHERAL_ERROR_EXC NAME_PeripheralError
#define ERR_PERIPHERAL_INVALID_PIN_EXC NAME_InvalidPinError
#define ERR_PERIPHERAL_INVALID_HARDWARE_STATUS_EXC NAME_InvalidHardwareStatusError

#endif