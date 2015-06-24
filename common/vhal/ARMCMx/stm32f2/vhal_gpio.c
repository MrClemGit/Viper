#include "vhal.h"
#include "hal.h"
#include "vhal_gpio.h"



/* GPIO */

const PinPort _vhalpinports[] STORED = {IOPORTA, IOPORTB, IOPORTC, IOPORTD};


int vhalInitGPIO(void *data) {
    (void)data;
    //Nothing to be done, we still depend on ChibiOS PAL driver
    return 0;
}

const uint8_t _pinmodes[] STORED = {PAL_MODE_INPUT, PAL_MODE_INPUT, PAL_MODE_INPUT_PULLUP, PAL_MODE_INPUT_PULLDOWN, PAL_MODE_OUTPUT_PUSHPULL, PAL_MODE_OUTPUT_OPENDRAIN, PAL_MODE_OUTPUT_OPENDRAIN, PAL_MODE_INPUT_ANALOG};


int vhalPinSetToPeripheral(int vpin, int prph, uint32_t prms){
    PIN_SET_TO_PRPH(vpin,prph);
    palSetPadMode(CPIN_PORT(vpin), PIN_PAD(vpin), prms);
    return 0;
}


int vhalPinSetMode(int vpin, int mode) {
    PIN_SET_TO_PRPH(vpin,0xff);
    palSetPadMode(CPIN_PORT(vpin), PIN_PAD(vpin), _pinmodes[mode & 0x0f]);
    return 0;
}


int vhalPinRead(int vpin) {
    return palReadPad(CPIN_PORT(vpin), PIN_PAD(vpin));
}

int vhalPinWrite(int vpin, int value) {
    if (value)
        palSetPad(CPIN_PORT(vpin), PIN_PAD(vpin));
    else
        palClearPad(CPIN_PORT(vpin), PIN_PAD(vpin));
    return 0;
}

int vhalPinToggle(int vpin) {
    palTogglePad(CPIN_PORT(vpin), PIN_PAD(vpin));
    return 0;
}


void vhalPinFastSet(void *port, int pad) {
    palSetPad((GPIO_PORT)port, pad);
}

void vhalPinFastClear(void *port, int pad) {
    palClearPad((GPIO_PORT)port, pad);
}
int vhalPinFastRead(void *port, int pad) {
    return palReadPad((GPIO_PORT)port, pad);
}
void *vhalPinGetPort(int vpin) {
    return PIN_PORT(vpin);
}
int vhalPinGetPad(int vpin) {
    return PIN_PAD(vpin);
}
