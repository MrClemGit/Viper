#ifndef __OPCODES__
#define __OPCODES__


extern const char * const opstrings[];



//OPCODES
#define NOP				0x00
#define STOP			0x01

//constants
#define CONSTI 			0x02
#define CONSTF 			0x03
#define CONSTS 			0x04

#define CONST_NONE 		0x05

#define CONSTI_0 		0x06
#define CONSTI_1 		0x07
#define CONSTI_2 		0x08
#define CONSTI_M1 		0x09

#define CONSTF_0 		0x0A
#define CONSTF_1 		0x0B
#define CONSTF_M1 		0x0C

#define CONSTS_0 		0x0D
#define CONSTS_S 		0x0E
#define CONSTS_N 		0x0F


//VARS & NAMES
#define LOAD_FAST  		0x10
#define LOAD_GLOBAL 	0x11
#define LOAD_DEREF  	0x12
#define LOAD_SUBSCR		0x13
#define LOAD_ATTR		0x14
#define LOOKUP_BUILTIN	0x15


#define STORE_FAST  	0x16
#define STORE_GLOBAL  	0x17
#define STORE_DEREF  	0x18
#define STORE_SUBSCR	0x19
#define STORE_ATTR		0x1A

#define DELETE_FAST  	0x1B
#define DELETE_GLOBAL  	0x1C
#define DELETE_DEREF  	0x1D
#define DELETE_SUBSCR	0x1E
#define DELETE_ATTR		0x1F


#define UPOS			0x20
#define UNEG			0x21
#define NOT				0x22
#define INVERT			0x23

//binary ops
#define ADD             0x24
#define IADD            0x25
#define SUB             0x26
#define ISUB            0x27
#define MUL             0x28
#define IMUL            0x29
#define DIV             0x2A
#define IDIV            0x2B
#define FDIV            0x2C
#define IFDIV           0x2D
#define MOD             0x2E
#define IMOD            0x2F
#define POW             0x30
#define IPOW            0x31
#define LSHIFT          0x32
#define ILSHIFT         0x33
#define RSHIFT          0x34
#define IRSHIFT         0x35
#define BIT_OR          0x36
#define IBIT_OR         0x37
#define BIT_XOR         0x38
#define IBIT_XOR        0x39
#define BIT_AND         0x3A
#define IBIT_AND        0x3B
#define EQ              0x3C
#define NOT_EQ          0x3D
#define LT              0x3E
#define LTE             0x3F
#define GT              0x40
#define GTE             0x41
#define L_AND           0x42
#define L_OR            0x43
#define IS              0x44
#define IS_NOT          0x45
#define IN              0x46
#define IN_NOT          0x47


#define BUILD_LIST		0x48
#define BUILD_TUPLE		0x49
#define BUILD_SLICE		0x4A
#define BUILD_DICT		0x4B
#define BUILD_SET		0x4C
#define BUILD_CLASS		0x4D
#define END_CLASS		0x4E
#define UNPACK			0x4F



//branch && LOOPS
#define JUMP_IF_TRUE	0x50
#define JUMP_IF_FALSE	0x51
#define IF_TRUE			0x52
#define IF_FALSE		0x53
#define JUMP 			0x54
#define SETUP_LOOP		0x55
#define POP_BLOCK 		0x56
#define BREAK 			0x57
#define CONTINUE 		0x58
#define GET_ITER 		0x59
#define FOR_ITER		0x5A

//MISC
#define MAKE_FUNCTION	0x5B
#define MAKE_CLOSURE	0x5C
#define CALL 			0x5D
#define CALL_VAR 		0x5E
#define RET 			0x5F

#define POP				0x60
#define DUP				0x61
#define DUP_TWO			0x62
#define ROT_TWO			0x63
#define ROT_THREE		0x64

#define LOOKUP_CODE		0x65
#define LOOKUP_NAME		0x66
#define LOOKUP_NATIVE    0x67


//EXCEPTIONS
#define RAISE	 		0x68
#define SETUP_FINALLY 	0x69
#define SETUP_EXCEPT	0x6A
#define POP_EXCEPT 		0x6B
#define END_FINALLY 	0x6C
#define BUILD_EXCEPTION 0x6D
#define CHECK_EXCEPTION 0x6E


#define IMPORT_NAME		0x6F
#define IMPORT_BUILTINS	0x70

#define MAP_STORE       0x71
#define LIST_STORE      0x72
#define CONST_TRUE      0x73
#define CONST_FALSE     0x74
#define MAKE_CELL       0x75
#define YIELD           0x76



#define _BIN_OP(a) (a-ADD)
#define _OPSTR(a) opstrings[a]

#define __RES_NAME___BUILTINS__ 256
#define __RES_NAME___MODULE__   257

#define NAME___init__      256
#define NAME___str__       257
#define NAME_count         258
#define NAME_split         259
#define NAME_strip         260
#define NAME_index         261
#define NAME_find          262
#define NAME_startswith    263
#define NAME_endswith      264
#define NAME_replace       265
#define NAME_join          266
#define NAME_append        267
#define NAME_insert        268
#define NAME_extend        269
#define NAME_pop           270
#define NAME_remove        271
#define NAME_reverse       272
#define NAME_clear         273
#define NAME_copy          274
//Exceptions
#define NAME_Exception              275
#define NAME_ArithmeticError        276
#define NAME_ZeroDivisionError      277
#define NAME_FloatingPointError     278
#define NAME_OverflowError          279
#define NAME_LookupError            280
#define NAME_IndexError             281
#define NAME_KeyError               282
#define NAME_NameError              283
#define NAME_RuntimeError           284
#define NAME_NotImplementedError    285
#define NAME_TypeError              286
#define NAME_ValueError             287
#define NAME_AttributeError         288
#define NAME_UnsupportedError       289
#define NAME_StopIteration          290
//Iterator
#define NAME___iter__               291
#define NAME___next__               292
//Containers
#define NAME___getitem__            293
#define NAME___setitem__            294
#define NAME___delitem__            295
#define NAME___len__                296
#define NAME___contains__           297
#define NAME___add__                298
#define NAME___iadd__               299
#define NAME___mul__                300
#define NAME___imul__               301
//dicts
#define NAME_get                    302
#define NAME_keys                   303
#define NAME_items                  304
#define NAME_values                 305
#define NAME_update                 306
#define NAME_popitem                307
//string
#define NAME_upper                  308
#define NAME_lower                  309
#define NAME_format                 310
//sets
#define NAME_difference             311
#define NAME_intersection           312
#define NAME_isdisjoint             313
#define NAME_issubset               314
#define NAME_issuperset             315
#define NAME_symmetric_difference   316
#define NAME_union                  317
#define NAME_intersection_update    318
#define NAME_difference_update      319
#define NAME_symmetric_difference_update    320
#define NAME_add                    321
#define NAME_discard                322
//drivers
#define NAME___ctl__                323
//threads
#define NAME_start                  324
#define NAME_get_status             325
#define NAME_get_priority           326
#define NAME_set_priority           327

//IO Exceptions
#define NAME_IOError                328
#define NAME_ConnectionError        329
#define NAME_ConnectionAbortedError 330
#define NAME_ConnectionResetError   331
#define NAME_ConnectionRefusedError 332
#define NAME_TimeoutError           333

//Peripheral Exceptions
#define NAME_PeripheralError	    334
#define NAME_InvalidPinError	    335
#define NAME_InvalidHardwareStatusError 336
#endif