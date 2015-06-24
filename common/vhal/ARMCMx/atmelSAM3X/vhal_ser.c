#include "vhal.h"
#include "hal.h"
#include "vhal_gpio.h"


uint8_t serstatus[5];
SerialDriver *serdrv[5];

#define UARTIDX 4

int vhalInitSER(void *data) {
    (void)data;

#if SAM3X_SERIAL_USE_SD1
    serdrv[PERIPHERAL_ID(1)] = &SD1;
#endif
#if SAM3X_SERIAL_USE_SD2
    serdrv[PERIPHERAL_ID(2)] = &SD2;
#endif
#if SAM3X_SERIAL_USE_SD3
    serdrv[PERIPHERAL_ID(3)] = &SD3;
#endif
#if SAM3X_SERIAL_USE_SD4
    serdrv[PERIPHERAL_ID(4)] = &SD4;
#endif
#if SAM3X_SERIAL_USE_SD5
    serdrv[PERIPHERAL_ID(5)] = &SD5;
#endif

    return 0;
}



const uint16_t const _ser_opts_p[] STORED = {
    SAM3X_SERIAL_PARITY_NO, //NO PARITY
    SAM3X_SERIAL_PARITY_EVEN, // EVEN PARITY
    SAM3X_SERIAL_PARITY_ODD //ODD PARITY
};

/*
//this is not necessary: uart flags are equal to serial flags
const uint16_t const _uart_opts_p[] STORED = {
    SAM3X_UART_PARITY_NO, //NO PARITY
    SAM3X_UART_PARITY_EVEN, // EVEN PARITY
    SAM3X_UART_PARITY_ODD //ODD PARITY
};
*/

const uint16_t const _ser_opts_s[] STORED = {
    SAM3X_SERIAL_STOP_1,
    SAM3X_SERIAL_STOP_1_5,
    SAM3X_SERIAL_STOP_2
};

const uint16_t const _ser_opts_b[] STORED = {
    SAM3X_SERIAL_CHAR_8_BIT, //8 bits
    SAM3X_SERIAL_CHAR_7_BIT, //7 bits
};

void blink(int, int);

extern void testdebug(const char *fmt, ...);

#define printf(...) testdebug(__VA_ARGS__)

int vhalSerialInit(uint32_t ser, uint32_t baud, uint32_t parity, uint32_t stop, uint32_t bits, uint32_t rxpin, uint32_t txpin) {
    if (ser >= PERIPHERAL_NUM(serial))
        return -1;
    int serid = GET_PERIPHERAL_ID(serial, ser);
    if (!serdrv[serid])
        return -2;

    int ser_status = serstatus[serid];

    vhalPinSetToPeripheral(txpin, PRPH_SER, SAM3X_PIN_PR(txpin) | PAL_MODE_OUTPUT_PUSHPULL);
    vhalPinSetToPeripheral(rxpin, PRPH_SER, SAM3X_PIN_PR(rxpin) | PAL_MODE_INPUT_PULLUP);
    if (!ser_status) {
        SerialConfig cfg;
        cfg.speed = baud;
        if (serid < UARTIDX) {
            cfg.flags = _ser_opts_p[parity] | _ser_opts_s[stop] | _ser_opts_b[bits];
        } else {
            cfg.flags = _ser_opts_p[parity];
        }
        sdStart((SerialDriver *)serdrv[serid], &cfg);
        serstatus[serid] = 1;
    }
    return 0;
}

int vhalSerialRead(uint32_t ser, uint8_t *buf, uint32_t len) {
    int serid = GET_PERIPHERAL_ID(serial, ser);
    return chnReadTimeout((BaseChannel *)serdrv[serid], buf, len, VTIME_INFINITE);
}


int vhalSerialWrite(uint32_t ser, uint8_t *buf, uint32_t len) {
    int serid = GET_PERIPHERAL_ID(serial, ser);
    return chnWriteTimeout((BaseChannel *)serdrv[serid], buf, len, VTIME_INFINITE);
}

int vhalSerialAvailable(uint32_t ser) {
    int serid = GET_PERIPHERAL_ID(serial, ser);
    uint32_t v;
    chSysLock();
    v = chIQGetFullI(&serdrv[serid]->iqueue);
    chSysUnlock();
    return v;
}

int vhalSerialDone(uint32_t ser) {
    int serid = GET_PERIPHERAL_ID(serial, ser);
    sdStop(serdrv[serid]);
    serstatus[serid] = 0;
    return 0;
}