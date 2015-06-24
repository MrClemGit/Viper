#include "vhal.h"
#include "vhal_gpio.h"


int vhalInitEXT(void *data) {
    (void)data;
    return 0;
}

int vhalPinAttachInterrupt(int vpin, int mode, extCbkFn fn) {
    return 0;
}