#include "port.h"
#include "vbl.h"
#include "lang.h"



#define NUM_PINS   50
const uint16_t _vhalpinnums = NUM_PINS;

#define PA PORT_A
#define PB PORT_B
#define PC PORT_C
#define PD PORT_D
#define PH PORT_H

PinStatus _vhalpinstatus[NUM_PINS];

const PinInfo const _vhalpinmap[] STORED = {
    /* D0   */ MAKE_PIN(PA, 3,  HAS_EXT | HAS_ATM | HAS_SER | HAS_ADC),
    /* D1   */ MAKE_PIN(PA, 2,  HAS_EXT | HAS_ATM | HAS_SER | HAS_ADC),
    /* D2   */ MAKE_PIN(PA, 10, HAS_EXT | HAS_ATM | HAS_SER),
    /* D3   */ MAKE_PIN(PB, 3, HAS_EXT | HAS_ATM | HAS_I2C | HAS_SPI),
    /* D4   */ MAKE_PIN(PB, 5, HAS_EXT | HAS_ATM | HAS_SPI),
    /* D5   */ MAKE_PIN(PB, 4, HAS_EXT | HAS_ATM | HAS_I2C | HAS_SPI),
    /* D6   */ MAKE_PIN(PB, 10, HAS_EXT | HAS_ATM | HAS_I2C),
    /* D7   */ MAKE_PIN(PA, 8, HAS_EXT | HAS_ATM | HAS_I2C),
    /* D8   */ MAKE_PIN(PA, 9, HAS_EXT | HAS_ATM | HAS_SER),
    /* D9   */ MAKE_PIN(PC, 7, HAS_EXT | HAS_ATM | HAS_SER),
    /* D10  */ MAKE_PIN(PB, 6, HAS_EXT | HAS_ATM | HAS_SER | HAS_I2C),
    /* D11  */ MAKE_PIN(PA, 7, HAS_EXT | HAS_PWM | HAS_ADC | HAS_SPI),
    /* D12  */ MAKE_PIN(PA, 6, HAS_EXT | HAS_ATM | HAS_ADC | HAS_SPI),
    /* D13  */ MAKE_PIN(PA, 5, HAS_EXT | HAS_ATM | HAS_ADC | HAS_SPI),
    /* D14  */ MAKE_PIN(PB, 9, HAS_EXT | HAS_ATM | HAS_I2C),
    /* D15  */ MAKE_PIN(PB, 8, HAS_EXT | HAS_ATM | HAS_I2C),
    /* D16  */ MAKE_PIN(PC, 13, HAS_EXT), //BTN

    //RIGHT MORPHO
    /* D17   */ MAKE_PIN(PC, 4, HAS_EXT | HAS_ADC),
    /* D18   */ MAKE_PIN(PB, 13, HAS_EXT |HAS_SPI|HAS_PWM),
    /* D19   */ MAKE_PIN(PB, 14, HAS_EXT |HAS_SPI|HAS_PWM),
    /* D20   */ MAKE_PIN(PB, 15, HAS_EXT |HAS_SPI|HAS_PWM),
    /* D21   */ MAKE_PIN(PB, 1, HAS_EXT |HAS_ADC|HAS_PWM),
    /* D22   */ MAKE_PIN(PB, 2, HAS_EXT),
    /* D23   */ MAKE_PIN(PB, 12, HAS_EXT),
    /* D24   */ MAKE_PIN(PA, 11, HAS_EXT|HAS_ATM|HAS_SER),
    /* D25   */ MAKE_PIN(PA, 12, HAS_EXT|HAS_SER),
    /* D26   */ MAKE_PIN(PC, 5, HAS_EXT|HAS_ADC),
    /* D27   */ MAKE_PIN(PC, 6, HAS_EXT|HAS_SER|HAS_ATM),
    /* D28   */ MAKE_PIN(PC, 8, HAS_EXT|HAS_ATM),
    /* D29   */ MAKE_PIN(PC, 9, HAS_EXT|HAS_ATM|HAS_I2C), //inner
    
    //LEFT MORPHO
    /* D30   */ MAKE_PIN(PC, 3, HAS_EXT|HAS_ADC|HAS_SPI),
    /* D31   */ MAKE_PIN(PC, 2, HAS_EXT|HAS_ADC|HAS_SPI),
    /* D32   */ MAKE_PIN(PH, 1, HAS_EXT),
    /* D33   */ MAKE_PIN(PH, 0, HAS_EXT),
    /* D34   */ MAKE_PIN(PC, 15, HAS_EXT),
    /* D35   */ MAKE_PIN(PC, 14, HAS_EXT),
    /* D36   */ MAKE_PIN(PB, 7, HAS_EXT|HAS_ATM|HAS_I2C),
    /* D37   */ MAKE_PIN(PA, 15, HAS_EXT|HAS_ATM),
    /* D38   */ MAKE_PIN(PA, 14, HAS_EXT),
    /* D39   */ MAKE_PIN(PA, 13, HAS_EXT),
    /* D40   */ MAKE_PIN(PC, 12, HAS_EXT|HAS_SPI),
    /* D41   */ MAKE_PIN(PC, 10, HAS_EXT|HAS_SPI),
    /* D42   */ MAKE_PIN(PC, 11, HAS_EXT|HAS_SPI),
    /* D43   */ MAKE_PIN(PD, 2, HAS_EXT),

    /* A0   */ MAKE_PIN(PA, 0, HAS_EXT | HAS_ATM | HAS_ADC),
    /* A1   */ MAKE_PIN(PA, 1, HAS_EXT | HAS_ATM | HAS_ADC),
    /* A2   */ MAKE_PIN(PA, 4, HAS_EXT | HAS_ADC),
    /* A3   */ MAKE_PIN(PB, 0, HAS_EXT | HAS_PWM | HAS_ADC),
    /* A4   */ MAKE_PIN(PC, 1, HAS_EXT | HAS_ADC),
    /* A5   */ MAKE_PIN(PC, 0, HAS_EXT | HAS_ADC),


};

const PinClass const _analogclass[] STORED = {
    /* A0 */ MAKE_PIN_CLASS(44, 0, 0, 0),
    /* A1 */ MAKE_PIN_CLASS(45, 1, 0, 0),
    /* A2 */ MAKE_PIN_CLASS(46, 4, 0, 0),
    /* A3 */ MAKE_PIN_CLASS(47, 8, 0, 0),
    /* A4 */ MAKE_PIN_CLASS(48, 11, 0, 0),
    /* A5 */ MAKE_PIN_CLASS(49, 10, 0, 0),
    /* A6 */ MAKE_PIN_CLASS(0, 3, 0, 0),
    /* A7 */ MAKE_PIN_CLASS(1, 2, 0, 0),
    /* A8 */ MAKE_PIN_CLASS(11, 7, 0, 0),
    /* A9 */ MAKE_PIN_CLASS(12, 6, 0, 0),
    /* A10 */ MAKE_PIN_CLASS(13, 5, 0, 0),
    /* A11 */ MAKE_PIN_CLASS(17, 14, 0, 0),
    /* A12 */ MAKE_PIN_CLASS(21, 9, 0, 0),
    /* A13 */ MAKE_PIN_CLASS(26, 15, 0, 0),
    /* A14 */ MAKE_PIN_CLASS(30, 13, 0, 0),
    /* A15 */ MAKE_PIN_CLASS(31, 12, 0, 0),
};

const PinClass const _spiclass[] STORED = {
    /* MOSI0 */ MAKE_PIN_CLASS(11, 1, 0, 0),
    /* MISO0 */ MAKE_PIN_CLASS(12, 1, 0, 0),
    /* SCK0 */ MAKE_PIN_CLASS(13, 1, 0, 0),
    /* MOSI1 */ MAKE_PIN_CLASS(4, 1, 0, 0),
    /* MISO1 */ MAKE_PIN_CLASS(5, 1, 0, 0),
    /* SCK1 */ MAKE_PIN_CLASS(3, 1, 0, 0),
    /* MOSI2 */ MAKE_PIN_CLASS(20, 2, 0, 0),
    /* MISO2 */ MAKE_PIN_CLASS(19, 2, 0, 0),
    /* SCK2 */ MAKE_PIN_CLASS(18, 2, 0, 0),
    /* MOSI3 */ MAKE_PIN_CLASS(30, 2, 0, 0),
    /* MISO3 */ MAKE_PIN_CLASS(31, 2, 0, 0),
    /* SCK3 */ MAKE_PIN_CLASS(18, 2, 0, 0),
    /* MOSI4 */ MAKE_PIN_CLASS(40, 3, 0, 0),
    /* MISO4 */ MAKE_PIN_CLASS(42, 3, 0, 0),
    /* SCK4 */ MAKE_PIN_CLASS(41, 3, 0, 0),

};

const PinClass const _i2cclass[] STORED = {
    /* SDA0 */ MAKE_PIN_CLASS(14, 1, 0, 0),
    /* SCL0 */ MAKE_PIN_CLASS(15, 1, 0, 0),
    /* SDA1 */ MAKE_PIN_CLASS(3, 2, 0, 0),
    /* SCL1 */ MAKE_PIN_CLASS(6, 2, 0, 0),
    /* SDA2 */ MAKE_PIN_CLASS(5, 3, 0, 0),
    /* SCL2 */ MAKE_PIN_CLASS(7, 3, 0, 0),
    /* SDA3 */ MAKE_PIN_CLASS(29, 3, 0, 0),
    /* SCL3 */ MAKE_PIN_CLASS(7, 3, 0, 0),
};


//phys, timer, channel, afio
const PinClass const _pwmclass[] STORED = {
    /* PWM0 */ MAKE_PIN_CLASS(0, 2, 4, 1),
    /* PWM1 */ MAKE_PIN_CLASS(1, 2, 3, 1),
    /* PWM2 */ MAKE_PIN_CLASS(2, 1, 3, 1),
    /* PWM3 */ MAKE_PIN_CLASS(3, 2, 2, 1),
    /* PWM4 */ MAKE_PIN_CLASS(4, 3, 2, 2),
    /* PWM5 */ MAKE_PIN_CLASS(5, 3, 1, 2),
    /* PWM6 */ MAKE_PIN_CLASS(6, 2, 3, 1),
    /* PWM7 */ MAKE_PIN_CLASS(7, 1, 1, 1),
    /* PWM8 */ MAKE_PIN_CLASS(8, 1, 2, 1),
    /* PWM9 */ MAKE_PIN_CLASS(9, 3, 2, 2),
    /* PWM10 */ MAKE_PIN_CLASS(10, 4, 1, 2),
    /* PWM11 */ MAKE_PIN_CLASS(11, 1, 1, 1),
    /* PWM12 */ MAKE_PIN_CLASS(12, 3, 1, 2),
    /* PWM13 */ MAKE_PIN_CLASS(13, 2, 1, 1),
    /* PWM14 */ MAKE_PIN_CLASS(14, 4, 4, 2),
    /* PWM15 */ MAKE_PIN_CLASS(15, 4, 3, 2),
    //right morpho
    /* PWM16 */ MAKE_PIN_CLASS(18, 1, 1, 1),
    /* PWM17 */ MAKE_PIN_CLASS(19, 1, 2, 1),
    /* PWM18 */ MAKE_PIN_CLASS(20, 1, 3, 1),
    /* PWM19 */ MAKE_PIN_CLASS(21, 1, 3, 1),
    /* PWM20 */ MAKE_PIN_CLASS(24, 1, 4, 1),
    /* PWM21 */ MAKE_PIN_CLASS(27, 3, 1, 2),
    /* PWM22 */ MAKE_PIN_CLASS(28, 3, 3, 2),
    /* PWM23 */ MAKE_PIN_CLASS(29, 3, 4, 2),
    
    //A
    /* PWM24 */ MAKE_PIN_CLASS(44, 2, 1, 1),
    /* PWM25 */ MAKE_PIN_CLASS(45, 2, 2, 1),
    /* PWM26 */ MAKE_PIN_CLASS(47, 2, 2, 1),

    //left morpho
    /* PWM27 */ MAKE_PIN_CLASS(36, 4, 2, 2),
    /* PWM28 */ MAKE_PIN_CLASS(37, 2, 1, 1),

};


const PinClass const _icuclass[] STORED = {
    /* PWM0 */ MAKE_PIN_CLASS(0, 2, 4, 1),
    /* PWM1 */ MAKE_PIN_CLASS(1, 2, 3, 1),
    /* PWM2 */ MAKE_PIN_CLASS(2, 1, 3, 1),
    /* PWM3 */ MAKE_PIN_CLASS(3, 2, 2, 1),
    /* PWM4 */ MAKE_PIN_CLASS(4, 3, 2, 2),
    /* PWM5 */ MAKE_PIN_CLASS(5, 3, 1, 2),
    /* PWM6 */ MAKE_PIN_CLASS(6, 2, 3, 1),
    /* PWM7 */ MAKE_PIN_CLASS(7, 1, 1, 1),
    /* PWM8 */ MAKE_PIN_CLASS(8, 1, 2, 1),
    /* PWM9 */ MAKE_PIN_CLASS(9, 3, 2, 2),
    /* PWM10 */ MAKE_PIN_CLASS(10, 4, 1, 2),
    /* PWM12 */ MAKE_PIN_CLASS(12, 3, 1, 2),
    /* PWM13 */ MAKE_PIN_CLASS(13, 2, 1, 1),
    /* PWM14 */ MAKE_PIN_CLASS(14, 4, 4, 2),
    /* PWM15 */ MAKE_PIN_CLASS(15, 4, 3, 2),
    //right morpho
    /* PWM20 */ MAKE_PIN_CLASS(24, 1, 4, 1),
    /* PWM21 */ MAKE_PIN_CLASS(27, 3, 1, 2),
    /* PWM22 */ MAKE_PIN_CLASS(28, 3, 3, 2),
    /* PWM23 */ MAKE_PIN_CLASS(29, 3, 4, 2),
    
    //A
    /* PWM24 */ MAKE_PIN_CLASS(44, 2, 1, 1),
    /* PWM25 */ MAKE_PIN_CLASS(45, 2, 2, 1),

    //left morpho
    /* PWM27 */ MAKE_PIN_CLASS(36, 4, 2, 2),
    /* PWM28 */ MAKE_PIN_CLASS(37, 2, 1, 1),
};

const PinClass const _canclass[] STORED = {

};

const PinClass const _serclass[] STORED = {
    /* RX0 */ MAKE_PIN_CLASS(0, 2, 0, 7),
    /* TX0 */ MAKE_PIN_CLASS(1, 2, 0, 7),
    /* RX1 */ MAKE_PIN_CLASS(2, 1, 0, 8),
    /* TX1 */ MAKE_PIN_CLASS(8, 1, 0, 8),
    /* RX2 */ MAKE_PIN_CLASS(25, 6, 0, 8),
    /* TX2 */ MAKE_PIN_CLASS(24, 6, 0, 8),
    /* RX3 */ MAKE_PIN_CLASS(2, 1, 0, 8),
    /* TX3 */ MAKE_PIN_CLASS(10, 1, 0, 8),
};

const PinClass const _dacclass[] STORED = {};

const PinClass const _ledclass[] STORED = {
    /* LED0 */ MAKE_PIN_CLASS(13, 0, 0, 0)
};

const PinClass const _btnclass[] STORED = {
    /* BTN0 */ MAKE_PIN_CLASS(16, 0, 0, 0)
};



VHAL_PORT_DECLARATIONS();


/* PERIPHERAL MAPS */

BEGIN_PERIPHERAL_MAP(serial) \
PERIPHERAL_ID(2), \
PERIPHERAL_ID(6), \
PERIPHERAL_ID(1), \
END_PERIPHERAL_MAP(serial);


BEGIN_PERIPHERAL_MAP(spi) \
PERIPHERAL_ID(1), \
PERIPHERAL_ID(2), \
PERIPHERAL_ID(3), \
END_PERIPHERAL_MAP(spi);

BEGIN_PERIPHERAL_MAP(adc) \
PERIPHERAL_ID(1), \
END_PERIPHERAL_MAP(adc);


BEGIN_PERIPHERAL_MAP(pwm) \
PERIPHERAL_ID(1), \
PERIPHERAL_ID(2), \
PERIPHERAL_ID(3), \
PERIPHERAL_ID(4), \
END_PERIPHERAL_MAP(pwm);


BEGIN_PERIPHERAL_MAP(icu) \
PERIPHERAL_ID(1), \
PERIPHERAL_ID(2), \
PERIPHERAL_ID(3), \
PERIPHERAL_ID(4), \
END_PERIPHERAL_MAP(icu);


BEGIN_PERIPHERAL_MAP(htm) \
PERIPHERAL_ID(5), \
PERIPHERAL_ID(8), \
PERIPHERAL_ID(9), \
PERIPHERAL_ID(10), \
PERIPHERAL_ID(11), \
END_PERIPHERAL_MAP(htm);



/* vbl layer */

const SerialPins const _vm_serial_pins[] STORED = {
    {RX0, TX0},
    {RX2, TX2},
    {RX1, TX2},
};


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



