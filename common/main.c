#include "vbl.h"
#include "port.h"
#include "lang.h"


//static VM
VM vm;


#ifdef VM_UPLOADED_BYTECODE

uint8_t *codemem;// = codearea;

#else

#include "../../../tests/testcode.h"
uint8_t *codemem = codearea;

#endif



#ifdef VM_DEBUG

volatile uint32_t debug_active = 0;
int32_t debug_level = VM_DEBUG_LEVEL;

void viper_debug(int lvl, const char *fmt, ...) {
    if (!debug_active) return;
    if(lvl<debug_level) return;
    va_list vl;
    va_start(vl, fmt);
    vosSemWait(_dbglock);
    vbl_printf(vhalSerialWrite, VM_STDERR, (uint8_t *)fmt, &vl);
    //vosThSleep(TIME_U(10,MILLIS));
    vosSemSignal(_dbglock);
    va_end(vl);
}

#endif


void blink(int, int);

int main(void) {

    //Initialize RTOS
    vosInit(NULL);
    //Initialize VHAL
    vhalInit(NULL);
    //Set additional fields
    vosThSetData(vosThCurrent(), NULL);

#ifdef VM_UPLOADED_BYTECODE
    //Initialize codemem if in bytecode upload mode
    codemem = vbl_init();
#endif
#ifdef VM_DEBUG
    vhalSerialInit(VM_STDERR, 115200, SERIAL_PARITY_NONE, SERIAL_STOP_ONE, SERIAL_BITS_8, VM_RXPIN(VM_STDERR),VM_TXPIN(VM_STDERR));
    debug_active = 1;
    debug("\r\n\n\n\nHello Viper! codemem@ %x\r\n\n\n", codemem);
#endif

    //configure board leds
    PORT_LED_CONFIG();
#ifdef VM_UPLOADED_BYTECODE
    //Let's check for incoming bytecode
    vm_upload();
#endif

    if (vm_init(&vm)) {
        debug("starting vm_run with %x\n", _vm.thlist);
        vm_run(_vm.thlist);
    }



#ifdef VM_DEBUG
    //ooops, main thread is gone! blink! blink!
    PORT_LED_CONFIG();
    while (1) {
        vosThSleep(TIME_U(500, MILLIS));
        PORT_LED_ON();
        vosThSleep(TIME_U(500, MILLIS));
        PORT_LED_OFF();
    }
#endif

    vosThSetPriority(VOS_PRIO_IDLE);
    while(1);

    return 0;
}
