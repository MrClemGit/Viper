#include "vhal.h"
#include "hal.h"
#include "vhal_gpio.h"


uint8_t serstatus[6];
VSemaphore sersems[6];
SerialDriver *serdrv[6];


int vhalInitSER(void *data) {
    (void)data;

#if STM32_SERIAL_USE_USART1
    serdrv[PERIPHERAL_ID(1)] = &SD1;
#endif
#if STM32_SERIAL_USE_USART2
    serdrv[PERIPHERAL_ID(2)] = &SD2;
#endif
#if STM32_SERIAL_USE_USART6
    serdrv[PERIPHERAL_ID(6)] = &SD6;
#endif
    return 0;
}


const uint32_t const _ser_opts_p[] STORED = {
    0, //NO PARITY
    USART_CR1_PCE, // EVEN PARITY
    USART_CR1_PCE | USART_CR1_PS //ODD PARITY
};

const uint32_t const _ser_opts_s[] STORED = {
    USART_CR2_STOP1_BITS | USART_CR2_LINEN,
    USART_CR2_STOP1P5_BITS | USART_CR2_LINEN,
    USART_CR2_STOP2_BITS | USART_CR2_LINEN
};

const uint32_t const _ser_opts_b[] STORED = {
    0, //8 bits
    0, //8 bits instead of 7
};

void blink(int, int);

int vhalSerialInit(uint32_t ser, uint32_t baud, uint32_t parity, uint32_t stop, uint32_t bits, uint32_t rxpin, uint32_t txpin) {
    if (ser >= PERIPHERAL_NUM(serial))
        return -1;
    int serid = GET_PERIPHERAL_ID(serial, ser);
    if (!serdrv[serid])
        return -2;

    int ser_status = serstatus[serid];

    vhalPinSetToPeripheral(rxpin, PRPH_SER, ALTERNATE_FN(PIN_CLASS_DATA2(rxpin)) | STM32_PUDR_NOPULL | STM32_OSPEED_HIGHEST);
    vhalPinSetToPeripheral(txpin, PRPH_SER, ALTERNATE_FN(PIN_CLASS_DATA2(txpin)) | STM32_PUDR_NOPULL | STM32_OSPEED_HIGHEST);

    if (!ser_status) {
        SerialConfig cfg;
        cfg.speed = baud;
        cfg.cr1 = _ser_opts_p[parity];
        cfg.cr2 = _ser_opts_s[stop];
        cfg.cr3 = _ser_opts_b[bits];
        sdStart(serdrv[serid], &cfg);
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
    chSysLock();
    uint32_t v = chIQGetFullI(&serdrv[serid]->iqueue);
    chSysUnlock();
    return v;
}

int vhalSerialDone(uint32_t ser) {
    int serid = GET_PERIPHERAL_ID(serial, ser);
    sdStop(serdrv[serid]);
    serstatus[serid] = 0;
    return 0;
}