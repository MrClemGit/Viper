#include "vbl.h"
#include "port.h"
#include "lang.h"


/* 23 arduino like + 19*2 morpho L is left (cn7), R is right (cn10)*/
#define NUM_PINS   12

#define PA PORT_A
#define PB PORT_B
#define PC PORT_C
#define PD PORT_D

PinStatus _vhalpinstatus[NUM_PINS];

const PinInfo const _vhalpinmap[] STORED = {
    /* D0   */ MAKE_PIN(PB, 0,  HAS_EXT),
    /* D1   */ MAKE_PIN(PB, 1,  HAS_EXT),
    /* TX0  */ MAKE_PIN(PA, 9,  HAS_EXT | HAS_SER), //usart1
    /* RX0  */ MAKE_PIN(PA, 10,  HAS_EXT | HAS_SER),
    /* A0   */ MAKE_PIN(PA, 1,  HAS_EXT | HAS_ADC),
    /* A1   */ MAKE_PIN(PA, 2,  HAS_EXT | HAS_ADC),
    /* A3   */ MAKE_PIN(PA, 3,  HAS_EXT | HAS_ADC),
    /* MOSI */ MAKE_PIN(PA, 7,  HAS_EXT | HAS_SPI),
    /* MISO */ MAKE_PIN(PA, 6,  HAS_EXT | HAS_SPI),
    /* SCK */  MAKE_PIN(PA, 5,  HAS_EXT | HAS_SPI),
    /* NSS */  MAKE_PIN(PA, 4,  HAS_EXT),
    /* D2   */ MAKE_PIN(PB, 6,  HAS_EXT), //usart1, i2c,tim4
    /* D3   */ MAKE_PIN(PB, 7,  HAS_EXT),

};

const PinClass const _digitalclass[] STORED = {
    /* D0 */ MAKE_PIN_CLASS(0, 0, 0, 0),
    /* D1 */ MAKE_PIN_CLASS(1, 1, 1, 1),
    /* D2 */ MAKE_PIN_CLASS(11, 0, 0, 0),
    /* D3 */ MAKE_PIN_CLASS(12, 1, 1, 1),
};

const PinClass const _analogclass[] STORED = {
    /* A0 */ MAKE_PIN_CLASS(4, 1, 0, 0),
    /* A1 */ MAKE_PIN_CLASS(5, 2, 0, 0),
    /* A2 */ MAKE_PIN_CLASS(6, 3, 0, 0),
    /* A3 */ MAKE_PIN_CLASS(0, 8, 0, 0),
    /* A4 */ MAKE_PIN_CLASS(1, 9, 0, 0),
};

const PinClass const _spiclass[] STORED = {
    /* MOSI0 */ MAKE_PIN_CLASS(7, 1, 0, 5),
    /* MISO0 */ MAKE_PIN_CLASS(8, 1, 0, 5),
    /* SCK0 */ MAKE_PIN_CLASS(9, 1, 0, 5),

};

const PinClass const _i2cclass[] STORED = {
};

const PinClass const _pwmclass[] STORED = {
    /* PWM0 */ MAKE_PIN_CLASS(0, 3, 3, 0),
    /* PWM1 */ MAKE_PIN_CLASS(1, 3, 4, 0),
    /* PWM2 */ MAKE_PIN_CLASS(11, 4, 1, 0),
    /* PWM3 */ MAKE_PIN_CLASS(12, 4, 2, 0),

};

const PinClass const _htmclass[] STORED = {
};

const PinClass const _icuclass[] STORED = {
    /* PWM0 */ MAKE_PIN_CLASS(0, 3, 3, 0),
    /* PWM1 */ MAKE_PIN_CLASS(1, 3, 4, 0),
};

const PinClass const _canclass[] STORED = {};

const PinClass const _serclass[] STORED = {
    /* RX0 */ MAKE_PIN_CLASS(3, 0, 0, 7),
    /* TX0 */ MAKE_PIN_CLASS(2, 0, 0, 7),
    /* RX1 */ MAKE_PIN_CLASS(11, 0, 0, 7),
    /* TX1 */ MAKE_PIN_CLASS(12, 0, 0, 7),
};

const PinClass const _dacclass[] STORED = {};

const PinClass const _ledclass[] STORED = {
};

const PinClass const _btnclass[] STORED = {
};



VHAL_PORT_DECLARATIONS();


/* PERIPHERAL MAPS */

BEGIN_PERIPHERAL_MAP(serial) \
END_PERIPHERAL_MAP(serial);


const SerialPins const _vm_serial_pins[] STORED = {
    {RX0,TX0}
};

BEGIN_PERIPHERAL_MAP(spi) \
PERIPHERAL_ID(1), \
END_PERIPHERAL_MAP(spi);

BEGIN_PERIPHERAL_MAP(adc) \
PERIPHERAL_ID(1), \
END_PERIPHERAL_MAP(adc);


BEGIN_PERIPHERAL_MAP(pwm) \
PERIPHERAL_ID(3), \
PERIPHERAL_ID(4), \
END_PERIPHERAL_MAP(pwm);


BEGIN_PERIPHERAL_MAP(icu) \
PERIPHERAL_ID(3), \
PERIPHERAL_ID(4), \
END_PERIPHERAL_MAP(icu);


BEGIN_PERIPHERAL_MAP(htm) \
PERIPHERAL_ID(1), \
PERIPHERAL_ID(2), \
PERIPHERAL_ID(5), \
PERIPHERAL_ID(6), \
PERIPHERAL_ID(7), \
PERIPHERAL_ID(8), \
PERIPHERAL_ID(9), \
PERIPHERAL_ID(10), \
PERIPHERAL_ID(11), \
END_PERIPHERAL_MAP(htm);


/* vbl layer */

void *begin_bytecode_storage(int size) {
    uint8_t *cm = codemem;
    vhalFlashErase(cm, size);
    return cm;
}

void *bytecode_store(void *where, uint8_t *buf, uint16_t len) {
    uint32_t bb = len - len % 4;

    if (where < (void *)0x8000000)
        return NULL;

    int ret = vhalFlashWrite(where, buf, bb);

    debug("bstored %i of %i\r\n", ret, bb);
    if ((uint32_t)ret != bb)
        return NULL;

    if (bb != len) {
        buf += bb;
        uint8_t bbuf[4];
        int i;
        for (i = 0; i < len % 4; i++) {
            bbuf[i] = buf[i];
        }
        ret = vhalFlashWrite( ((uint8_t *)where) + bb, bbuf, 4);
        if (ret != 4)
            return NULL;
        bb += 4;
    }

    return ((uint8_t *)where) + bb;
}

void *end_bytecode_storage() {
    return 0;
}

void *vbl_get_adjusted_codemem(void *codemem) {
    return vhalFlashAlignToSector(codemem);
}