#include "viper_test.h"

//test framework
//inspired (actually copied & pasted) by http://www.jera.com/techinfo/jtns/jtn002.html


//needed declarations for the compiler to work :P
//VM vm;
uint8_t *codemem;


//test specific declarations
int serial = VM_STDOUT;


void testdebug(const char *fmt, ...) {
    va_list vl;
    va_start(vl, fmt);
    vbl_printf(vhalSerialWrite, serial, (uint8_t *)fmt, &vl);
    va_end(vl);
}



int all_tests(void);

int all_tests() {
    int i = 0;

    while (1) {
        if (_testlist[i].desc) {
            if (_testlist[i].fn) {
                printf("\n----------------------------------------------TEST %i\n >> %s\n", i, _testlist[i].desc);
                char *result = _testlist[i].fn();
                if (result) {
                    printf(" :: %s\n :: Test Not Passed! :(\n", result);
                    return i;
                } else {
                    printf(" :: Test Passed! :)\n");
                }
            } else printf("TEST SUITE: %s\n", _testlist[i].desc);
            i++;
        } else return -i;
    }
    return 0;
}

//must be here if VM_DEBUG is enabled
//however tests should be done for low level parts, no VM needed


#ifdef VM_DEBUG

volatile uint32_t debug_active = 0;
int32_t debug_level = VM_DEBUG_LEVEL;

void viper_debug(int lvl, const char *fmt, ...) {
    if (!debug_active) return;
    if (lvl < debug_level) return;
    va_list vl;
    va_start(vl, fmt);
    vosSemWait(_dbglock);
    vbl_printf(vhalSerialWrite, VM_STDERR, (uint8_t *)fmt, &vl);
    vosSemSignal(_dbglock);
    va_end(vl);
}

#endif

extern void blink(p,n);

int main(void) {


    //Initialize RTOS
    vosInit(NULL);
    //Initialize HAL
    vhalInit(NULL);
    //Set additional fields
    vosThSetData(vosThCurrent(), NULL);

#ifdef VM_DEBUG
    vhalSerialInit(VM_STDERR, 115200, SERIAL_PARITY_NONE, SERIAL_STOP_ONE, SERIAL_BITS_8, VM_RXPIN(VM_STDERR),VM_TXPIN(VM_STDERR));
    debug_active = 1;
#endif


    vhalSerialInit(VM_STDOUT, 115200, SERIAL_PARITY_NONE, SERIAL_STOP_ONE, SERIAL_BITS_8, VM_RXPIN(VM_STDOUT),VM_TXPIN(VM_STDOUT));

    printf("====================\n");
    printf("VIPER test framework\n");
    printf("====================\n\n");

    int result = all_tests();
    printf("\n====================\n");
    if (result < 0) {
        printf("All tests passed (%i)\n", -result - 1);
    } else if (result > 0) {
        printf("Stopped at test (%i)\n", result);
    } else {
        printf("Something really wrong!\n");
    }

    printf("====================\n\n");

    while (1);
}
