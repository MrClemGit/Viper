#ifndef __VIPER_TEST__
#define __VIPER_TEST__

#include "port.h"
#include "lang/lang.h"



typedef char *(*testfn)(void);

typedef struct _test {
    char *desc;
    testfn fn;
} TestInfo;

extern TestInfo _testlist[];
extern void testdebug(const char *fmt,...);

#define printf(...) testdebug(__VA_ARGS__)

#define TEST(name) static char* name(void)
#define ADD_TEST(fn,desc) {(char*)desc,fn}
#define BEGIN_TEST(desc) TestInfo _testlist[] STORED={ {(char*)desc,0}
#define END_TEST() {0,0}}

#define TEST_OK() (char*)0
#define TEST_KO(msg) (char*)msg
#define TEST_ASSERT(message, test) do { if (!(test)) return message; } while (0)
#define TEST_EQUAL(message,test,eq) do { if ((test)!=(eq)) return message; } while (0)


#endif