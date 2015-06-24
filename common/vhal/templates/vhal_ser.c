#include "vhal.h"
#include "vhal_gpio.h"

int vhalInitSER(void *data) {
    (void)data;

    return 0;
}

int vhalSerialInit(uint32_t ser, uint32_t baud, uint32_t parity, uint32_t stop, uint32_t bits, uint32_t rxpin, uint32_t txpin) {
    return 0;
}

int vhalSerialRead(uint32_t ser, uint8_t *buf, uint32_t len) {
    return 0;
}
int vhalSerialWrite(uint32_t ser, uint8_t *buf, uint32_t len) {
    return 0;
}

int vhalSerialAvailable(uint32_t ser) {
    return 0;
}

int vhalSerialDone(uint32_t ser) {
    return 0;
}