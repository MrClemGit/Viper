#include "vhal.h"
#include "hal.h"
#include "vhal_gpio.h"


uint8_t serstatus[4];
SerialDriver *serdrv[4];

#if VHAL_CDC
extern SerialUSBDriver SDU1;
extern const SerialUSBConfig vhal_serusbcfg;
extern const USBConfig vhal_usbcfg;
#endif

int vhalInitSER(void *data) {
    (void)data;

#if STM32_SERIAL_USE_USART1
    serdrv[PERIPHERAL_ID(1)] = &SD1;
#endif
#if STM32_SERIAL_USE_USART2
    serdrv[PERIPHERAL_ID(2)] = &SD2;
#endif
#if STM32_SERIAL_USE_USART3
    serdrv[PERIPHERAL_ID(3)] = &SD3;
#endif
#if VHAL_CDC
    sduObjectInit(&SDU1);
    serdrv[PERIPHERAL_ID(4)] = (SerialDriver *)&SDU1;
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

extern void blink(int, int);
extern void blink_off(int);

int vhalSerialInit(uint32_t ser, uint32_t baud, uint32_t parity, uint32_t stop, uint32_t bits, uint32_t rxpin, uint32_t txpin) {
    if (ser >= PERIPHERAL_NUM(serial))
        return -1;
    int serid = GET_PERIPHERAL_ID(serial, ser);
    if (!serdrv[serid])
        return -2;

    int ser_status = serstatus[serid];

    if (serid != 3) {
        vhalPinSetToPeripheral(rxpin, PRPH_SER, PIN_PARAMS(PINPRM_MODE_INPUT, PINPRM_CNF_FLOATING, PIN_CLASS_DATA2(rxpin), PIN_CLASS_DATA0(rxpin), PIN_CLASS_DATA1(rxpin)));
        vhalPinSetToPeripheral(txpin, PRPH_SER, PIN_PARAMS(PINPRM_MODE_50MHZ, PINPRM_CNF_ALTERNATE_PP, PIN_CLASS_DATA2(txpin), PIN_CLASS_DATA0(txpin), PIN_CLASS_DATA1(txpin)));
    } else {
        //TODO:
        //remember to set usbp and usbm as free input (passed as rx and tx)
        //usb disc is set to output pushpull by board.h (gpio init)
    }

    if (!ser_status) {
#if VHAL_CDC
        if (serid == 3) {
            //CDC
            if (SDU1.state == SDU_STOP) {
                sduStart(&SDU1, &vhal_serusbcfg);
                usbDisconnectBus(vhal_serusbcfg.usbp);
                vosThSleep(TIME_U(1500, MILLIS));
                usbStart(vhal_serusbcfg.usbp, &vhal_usbcfg);
                usbConnectBus(vhal_serusbcfg.usbp);
            }
        } else
#endif
        {
            SerialConfig cfg;
            cfg.speed = baud;
            cfg.cr1 = _ser_opts_p[parity];
            cfg.cr2 = _ser_opts_s[stop];
            cfg.cr3 = _ser_opts_b[bits];
            sdStart(serdrv[serid], &cfg);
        }
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
#if VHAL_CDC
    if (serid == 3) {
        usbDisconnectBus(vhal_serusbcfg.usbp);
        sduStop(serdrv[serid]);
    } else
#endif
    {
        sdStop(serdrv[serid]);
    }
    serstatus[serid] = 0;
    return 0;
}
