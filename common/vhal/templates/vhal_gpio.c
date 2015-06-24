#include "vhal.h"
#include "vhal_gpio.h"



/* GPIO */

const PinPort _vhalpinports[] STORED = {NULL,NULL,NULL,NULL};


int vhalInitGPIO(void *data) {
    (void)data;
    return 0;
}

//const uint8_t _pinmodes[] STORED = {PAL_MODE_INPUT, PAL_MODE_INPUT, PAL_MODE_INPUT_PULLUP, PAL_MODE_INPUT_PULLDOWN, PAL_MODE_OUTPUT_PUSHPULL, PAL_MODE_OUTPUT_OPENDRAIN, PAL_MODE_OUTPUT_OPENDRAIN, PAL_MODE_INPUT_ANALOG};


int vhalPinSetToPeripheral(int vpin, int prph, uint32_t prms){
    PIN_SET_TO_PRPH(vpin,prph);
    return 0;
}


int vhalPinSetMode(int vpin, int mode) {
    PIN_SET_TO_PRPH(vpin,0xff);
    return 0;
}


int vhalPinRead(int vpin) {
    return 0;
}

int vhalPinWrite(int vpin, int value) {
    return 0;
}

int vhalPinToggle(int vpin) {
    return 0;
}


void vhalPinFastSet(void *port, int pad) {
}

void vhalPinFastClear(void *port, int pad) {
}

int vhalPinFastRead(void *port, int pad) {
    return 0;
}

void *vhalPinGetPort(int vpin) {
    return NULL;
}
int vhalPinGetPad(int vpin) {
    return PIN_PAD(vpin);
}
